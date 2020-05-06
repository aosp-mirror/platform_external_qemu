# Lint as: python3
#  Copyright (C) 2019 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""A very simplistic script to extract the crc32 values that are collected
   From a grpc service definition. This can used to create queries against
   the recorded metrics.

   Calls that are deprecated will be postfixed with _deprecated.

   You usually want to run this script when you have added new methods
   to any of the gRPC services for which we are tracking metrics.

   If you are removing methods from the service definitions consider commenting
   them out, so the simple parser will still pick them up and you don't end up
   with missing data in charts.

   For example:

   // The following methods have been deleted on the given date
   // rpc thisWasDeletedAWhileBack (03/12/11)
"""
import argparse
import datetime
import re
import sys
import zlib
from itertools import chain


class ServiceParser(object):
    """Parses a grpc service file, translating every method into the corresponding
       crc32 hash that is used by the gRPC engine when recording metrics.
    """

    RPC_RE = re.compile(r".*rpc (\w+)\(.*\) returns")
    RPC_RE_DEL = re.compile(r".*rpc (\w+) \((\d+\/\d+\/\d+)\)")
    SERVICE_RE = re.compile(r"service (\w+)")
    PACKAGE_RE = re.compile(r"package (.*);")

    def __init__(self, grpc_proto_file, expired=360):
        # We interpret deleted metrics for 180 days.
        keep_after_date = datetime.datetime.now() - datetime.timedelta(days=expired)
        self.package, self.service, discovered, deleted = self._parse_grpc_file(
            grpc_proto_file, keep_after_date
        )

        if self.service is None or self.package is None:
            raise Exception("No service or package definition found.")

        self.methods = {}  # crc32 -> method name
        for name in discovered:
            method = self.url(name)
            crc = str(zlib.crc32(bytearray(method, "utf-8")))
            self.methods[crc] = name

        self.deprecated = {}
        for name in deleted:
            method = self.url(name)
            crc = str(zlib.crc32(bytearray(method, "utf-8")))
            self.deprecated[crc] = name

    def _parse_grpc_file(self, fname, keep_after_date):
        """Parses the service file, returning the package, service and discovered method names."""
        package = ""
        service = ""
        discovered = []
        deprecated = []
        with open(fname, "r") as pfile:
            for line in pfile.readlines():
                m = ServiceParser.PACKAGE_RE.match(line)
                if m:
                    if package:
                        raise Exception(
                            "Already found a package defintion: {}".format(self.package)
                        )
                    package = m.group(1)

                m = ServiceParser.SERVICE_RE.match(line)
                if m:
                    if service:
                        raise Exception(
                            "Already found a service defintion: {}".format(self.service)
                        )
                    service = m.group(1)
                m = ServiceParser.RPC_RE.match(line)
                if m:
                    discovered.append(m.group(1))

                m = ServiceParser.RPC_RE_DEL.match(line)
                if m:
                    date = datetime.datetime.strptime(m.group(2), "%m/%d/%y")
                    if date > keep_after_date:
                        deprecated.append(m.group(1))

        return package, service, discovered, deprecated

    def url(self, method):
        """The actual url for the given method, used to derive crc32."""
        return "/{}.{}/{}".format(self.package, self.service, method)

    def url_from_crc32(self, crc32):
        if crc32 in self.methods:
            return self.url(self.methods[crc32])
        return self.url(self.deprecated.get(crc32, "__unknown__"))

    def sql_call_id_to_name(self):
        """Constructs a sql function that maps crc32 -> strings."""
        case = "CREATE TEMP FUNCTION callIdToName(call_id INT64) AS (\n"
        case += " CASE call_id \n"
        for crc in self.methods.keys():
            method = self.methods[crc]
            case += '  WHEN {} THEN "{}"\n'.format(crc, method)
        for crc in self.deprecated.keys():
            method = self.deprecated[crc]
            case += '  WHEN {} THEN "{}_deprecated"\n'.format(crc, method)
        case += ' ELSE CONCAT(CAST(call_id as STRING), ", missing translation (deprecated?)") \n'
        case += "END);"
        return case

    def sql_in_snippet(self):
        """Constructs a sql snippet that can be placed in the where clause."""
        keys = chain(self.methods.keys(), self.deprecated.keys())
        return "android_studio_event.emulator_details.grpc.call_id IN (\n  {})".format(
            ",\n  ".join(keys)
        )

    def __str__(self):
        return "\n".join(
            ["{} : {}".format(k, self.url_from_crc32(k)) for k in self.methods.keys()] +
            ["{} : {}_deprecated".format(k, self.url_from_crc32(k)) for k in self.deprecated.keys()]
        )


def main(argv):
    parser = argparse.ArgumentParser(
        description="Extract the CRC32 values used by the metrics system from the gRPC service definition.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("grpc", help="gPRC service file to parse")
    parser.add_argument("-s", "--sql", action="store_true", help="Create SQL snippets.")
    parser.add_argument(
        "-e",
        "--expire",
        default=360,
        type=int,
        help="Keep metrics that were deleted less than EXPIRE days ago today.",
    )
    args = parser.parse_args()

    s = ServiceParser(args.grpc, args.expire)
    if args.sql:
        print("# AUTOGENERATED TRANSLATION FUNCTION FROM")
        print("# {}".format(" ".join(argv)))
        print(s.sql_call_id_to_name())
        print("# SQL FROM CLAUSE")
        print(s.sql_in_snippet())
    else:
        print(s)


if __name__ == "__main__":
    main(sys.argv)
