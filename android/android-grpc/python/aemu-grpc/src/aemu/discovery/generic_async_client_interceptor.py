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
"""Base class for interceptors that operate on all async RPC types."""
import grpc


class _AsyncUnaryUnaryClientInterceptor(grpc.aio.UnaryUnaryClientInterceptor):
    def __init__(self, interceptor_function):
        self._fn = interceptor_function

    async def intercept_unary_unary(self, continuation, client_call_details, request):
        new_details, new_request_iterator, postprocess = self._fn(
            client_call_details, iter((request,)), False, False
        )
        response = await continuation(new_details, next(new_request_iterator))
        return postprocess(response) if postprocess else response


class _AsyncUnaryStreamClientInterceptor(grpc.aio.UnaryStreamClientInterceptor):
    def __init__(self, interceptor_function):
        self._fn = interceptor_function

    async def intercept_unary_stream(self, continuation, client_call_details, request):
        new_details, new_request_iterator, postprocess = self._fn(
            client_call_details, iter((request,)), False, True
        )
        response_it = await continuation(new_details, next(new_request_iterator))
        return postprocess(response_it) if postprocess else response_it


class _AsyncStreamUnaryClientInterceptor(grpc.aio.StreamUnaryClientInterceptor):
    def __init__(self, interceptor_function):
        self._fn = interceptor_function

    async def intercept_stream_unary(
        self, continuation, client_call_details, request_iterator
    ):
        new_details, new_request_iterator, postprocess = self._fn(
            client_call_details, request_iterator, True, False
        )
        response = await continuation(new_details, new_request_iterator)
        return postprocess(response) if postprocess else response


class _AsyncStreamStreamClientInterceptor(grpc.aio.StreamStreamClientInterceptor):
    def __init__(self, interceptor_function):
        self._fn = interceptor_function

    async def intercept_stream_stream(
        self, continuation, client_call_details, request_iterator
    ):
        new_details, new_request_iterator, postprocess = self._fn(
            client_call_details, request_iterator, True, True
        )
        response_it = await continuation(new_details, new_request_iterator)
        return postprocess(response_it) if postprocess else response_it


def create(intercept_call):
    return [
        _AsyncUnaryUnaryClientInterceptor(intercept_call),
        _AsyncUnaryStreamClientInterceptor(intercept_call),
        _AsyncStreamUnaryClientInterceptor(intercept_call),
        _AsyncStreamStreamClientInterceptor(intercept_call),
    ]
