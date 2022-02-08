# Copyright 2017 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Interceptor that adds headers to outgoing requests."""

import collections
import datetime
import time
from datetime import timedelta, timezone

import aemu.discovery.generic_client_interceptor as generic_client_interceptor
import grpc
from tink import jwt
from aemu.discovery.jwt_keygen import JwtKeygen

def currenttz():
    if time.daylight:
        return timezone(timedelta(seconds=-time.altzone), time.tzname[1])
    else:
        return timezone(timedelta(seconds=-time.timezone), time.tzname[0])


class _ClientCallDetails(
    collections.namedtuple(
        "_ClientCallDetails", ("method", "timeout", "metadata", "credentials")
    ),
    grpc.ClientCallDetails,
):
    pass


def header_adder_interceptor(header, value):
    def intercept_call(
        client_call_details, request_iterator, request_streaming, response_streaming
    ):
        metadata = []
        if client_call_details.metadata is not None:
            metadata = list(client_call_details.metadata)
        metadata.append(
            (
                header,
                value,
            )
        )
        client_call_details = _ClientCallDetails(
            client_call_details.method,
            client_call_details.timeout,
            metadata,
            client_call_details.credentials,
        )
        return client_call_details, request_iterator, None

    return generic_client_interceptor.create(intercept_call)


def jwt_header_inceptor(key_path, active_path):

    keygen = JwtKeygen(key_path, active_path)

    def intercept_call(
        client_call_details, request_iterator, request_streaming, response_streaming
    ):
        metadata = []
        if client_call_details.metadata is not None:
            metadata = list(client_call_details.metadata)

        raw_jwt = jwt.new_raw_jwt(
            audiences=[client_call_details.method],
            expiration=datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(minutes=15),
            issuer="PyModule",
            issued_at=datetime.datetime.now(datetime.timezone.utc),
        )
        token = keygen.sign_and_encode(raw_jwt)
        bearer = "Bearer {}".format(token)
        metadata.append(
            (
                "authorization",
                bearer,
            )
        )
        client_call_details = _ClientCallDetails(
            client_call_details.method,
            client_call_details.timeout,
            metadata,
            client_call_details.credentials,
        )
        return client_call_details, request_iterator, None

    return generic_client_interceptor.create(intercept_call)
