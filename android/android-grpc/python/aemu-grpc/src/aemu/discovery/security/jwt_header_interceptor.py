import collections
import datetime

import aemu.discovery.generic_async_client_interceptor as generic_async_client_interceptor
import aemu.discovery.generic_client_interceptor as generic_client_interceptor
import grpc
from aemu.discovery.security.jwt_keygen import JwtKeygen
from tink import jwt


class _ClientCallDetails(
    collections.namedtuple(
        "_ClientCallDetails",
        ("method", "timeout", "metadata", "credentials", "wait_for_ready"),
    ),
    grpc.ClientCallDetails,
):
    pass


def jwt_header_interceptor(key_path, active_path, use_async, issuer="PyModule"):
    keygen = JwtKeygen(key_path, active_path)

    def intercept_call(
        client_call_details, request_iterator, request_streaming, response_streaming
    ):
        metadata = []
        if client_call_details.metadata is not None:
            metadata = list(client_call_details.metadata)

        method = client_call_details.method
        if getattr(method, "decode", None):
            method = method.decode("utf-8")
        raw_jwt = jwt.new_raw_jwt(
            audiences=[method],
            expiration=datetime.datetime.now(datetime.timezone.utc)
            + datetime.timedelta(minutes=15),
            issuer=issuer,
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
            client_call_details.wait_for_ready,
        )
        return client_call_details, request_iterator, None

    if use_async:
        return generic_async_client_interceptor.create(intercept_call)

    return generic_client_interceptor.create(intercept_call)
