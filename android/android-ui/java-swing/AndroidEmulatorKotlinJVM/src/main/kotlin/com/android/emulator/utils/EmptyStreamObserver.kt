package com.android.emulator.utils

import io.grpc.stub.StreamObserver

open class EmptyStreamObserver<T> : StreamObserver<T> {
    override fun onNext(response: T) {
    }

    override fun onError(t: Throwable) {
    }

    override fun onCompleted() {
    }
}