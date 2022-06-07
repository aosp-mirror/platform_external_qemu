#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


import json
import os
import sys
import time
import logging
import argparse

if sys.version_info[0] == 2:
    from urllib import quote
else:
    from urllib.parse import quote
    import urllib.request, urllib.error


import requests
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry


class SymbolFileServer(object):
    """Class to verify and upload symbols to the crash symbol api v1."""

    # The api key should work for both staging and production.
    API_KEY_FILE = os.path.join(os.path.expanduser("~"), ".emulator_symbol_server_key")
    API_URL = {
        "prod": "https://prod-crashsymbolcollector-pa.googleapis.com/v1",
        "staging": "https://staging-crashsymbolcollector-pa.googleapis.com/v1",
    }
    STATUS_MSG = {
        "OK": "The symbol data was written to symbol storage.",
        "DUPLICATE_DATA": "The symbol data was identical to data already in storage, so no changes were made.",
    }

    CLASSIC_PRODUCT = "AndroidEmulator"

    # Timeout for individual web requests.  an exception is raised if the server has not issued a response for timeout
    # seconds (more precisely, if no bytes have been received on the underlying socket for timeout seconds).
    DEFAULT_TIMEOUT = 30

    # Number of times we will attempt to make the call, including waiting between calls.
    MAX_RETRY = 5

    def __init__(self, environment="prod", api_key=None):
        endpoint = SymbolFileServer.API_URL
        self.api_url = endpoint[environment.lower()]
        self.api_key = api_key

    def _extract_symbol_info(self, symbol_file):
        """Extracts os, archictecture, debug id and debug file.

        Raises an error if the first line of the file does not contain:
        MODULE {os} {arch} {id} {filename}
        """
        with open(symbol_file, "r") as f:
            info = f.readline().split()

        if len(info) != 5 or info[0] != "MODULE":
            raise IOError("Corrupt symbol file: %s, %s" % (symbol_file, info))

        _, os, arch, dbg_id, dbg_file = info
        return os, arch, dbg_id, dbg_file

    def _exec_request_with_retry(self, oper, url, **kwargs):
        """Makes a web request with default timeout, returning the response.

        The request will be tried multiple times, with exponential backoff:
        sleep 1 * (2 ^ ({number of total retries} - 1))
        """
        retries = Retry(
            total=SymbolFileServer.MAX_RETRY,
            backoff_factor=1,
            status_forcelist=[502, 503, 504],
        )
        with requests.Session() as s:
            s.mount("https://", HTTPAdapter(max_retries=retries))
            s.mount("http://", HTTPAdapter(max_retries=retries))

            return s.request(
                oper,
                url,
                params={"key": self.api_key},
                timeout=SymbolFileServer.DEFAULT_TIMEOUT,
                **kwargs
            )

    def _exec_request(self, oper, url, **kwargs):
        """Makes a web request with default timeout, returning the json result.

        The api key will be added to the url request as a parameter.

        This method will raise a requests.exceptions.HTTPError
        if the status code is not 4XX, 5XX

        Note: If you are using verbose logging it is entirely possible that the subsystem will
        write your api key to the logs!
        """
        resp = self._exec_request_with_retry(oper, url, **kwargs)
        if resp.status_code > 399:
            # Make sure we don't leak secret keys by accident.
            resp.url = resp.url.replace(quote(self.api_key), "XX-HIDDEN-XX")
            logging.error(
                'Url: %s, Status: %s, response: "%s", in: %s',
                resp.url,
                resp.status_code,
                resp.text,
                resp.elapsed,
            )
            resp.raise_for_status()

        if resp.content:
            return resp.json()
        return {}

    def is_available(self, symbol_file):
        """True if the symbol_file is available on the server."""
        _, _, dbg_id, dbg_file = self._extract_symbol_info(symbol_file)
        url = "{}/symbols/{}/{}:checkStatus".format(self.api_url, dbg_file, dbg_id)
        status = self._exec_request("get", url)
        return status["status"] == "FOUND"

    def upload(self, symbol_file):
        """Makes the symbol_file available on the server, returning the status result."""
        _, _, dbg_id, dbg_file = self._extract_symbol_info(symbol_file)
        upload = self._exec_request("post", "{}/uploads:create".format(self.api_url))
        self._exec_request(
            "put", upload["uploadUrl"], data=open(symbol_file, "rb"), stream=True
        )

        status = self._exec_request(
            "post",
            "{}/uploads/{}:complete".format(self.api_url, upload["uploadKey"]),
            data=json.dumps(
                {"symbol_id": {"debug_file": dbg_file, "debug_id": dbg_id}}
            ),
        )

        return status["result"]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--environment",
        default="prod",
        type=str.lower,
        choices=["prod", "staging"],
        help="Which symbol server endpoint to use, can be staging or prod",
    )

    parser.add_argument(
        "--api_key",
        default=None,
        help="Which api key to use. By default will load contents of ~/.emulator_symbol_server_key",
    )

    parser.add_argument(
        "--symbol_file",
        default=None,
        required=True,
        help="The symbol file you wish to process",
    )
    parser.add_argument(
        "--force",
        default=False,
        action="store_true",
        help="Upload symbol file, even though it is already available.",
    )
    return parser.parse_args()


def main(args):
    # The lower level enging will log individual requests!
    logging.debug("-----------------------------------------------------------")
    logging.debug("- WARNING!! You are likely going to leak your api key    --")
    logging.debug("-----------------------------------------------------------")
    logging.warning("DEPRECATED!! Please use sym_upload for uploading symbols.")
    logging.warning("DEPRECATED!! This file might be removed in the future.")

    api_key = args.api_key
    available = False
    if not api_key:
        try:
            with open(SymbolFileServer.API_KEY_FILE) as f:
                api_key = f.read()
        except IOError as e:
            logging.error(
                "Unable to read api key due to: %s.",
                e,
            )
            logging.error("Please provide a valid api key.")
            sys.exit(1)

    server = SymbolFileServer(args.environment, api_key)
    if args.force or not server.is_available(args.symbol_file):
        start = time.time()
        status = server.upload(args.symbol_file)
        # Let's assume that our symbols will now become available
        available = status == "OK"
        print(
            "{} after {} seconds".format(
                SymbolFileServer.STATUS_MSG.get(
                    status, "Undefined or unknown result {}".format(status)
                ),
                time.time() - start,
            )
        )
    else:
        print("Symbol {} already available, skipping upload.".format(args.symbol_file))
        available = True

    # api key -> symbol file should have been made available.
    sys.exit(0 if available else 1)


def launch():
    main(parse_args())


if __name__ == "__main__":
    launch()
