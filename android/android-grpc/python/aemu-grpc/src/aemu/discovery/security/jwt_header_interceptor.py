from tink import jwt
from aemu.discovery.security.jwt_keygen import JwtKeygen

import collections
import datetime
import time
from datetime import timedelta, timezone

import aemu.discovery.generic_client_interceptor as generic_client_interceptor
import grpc


def jwt_header_interceptor(key_path, active_path):

    keygen = JwtKeygen(key_path, active_path)

    def intercept_call(
        client_call_details, request_iterator, request_streaming, response_streaming
    ):
        metadata = []
        if client_call_details.metadata is not None:
            metadata = list(client_call_details.metadata)

        raw_jwt = jwt.new_raw_jwt(
            audiences=[client_call_details.method],
            expiration=datetime.datetime.now(datetime.timezone.utc)
            + datetime.timedelta(minutes=15),
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
