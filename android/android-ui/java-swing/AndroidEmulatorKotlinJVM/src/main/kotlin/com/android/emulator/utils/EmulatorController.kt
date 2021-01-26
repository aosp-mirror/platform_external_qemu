/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.emulator.utils;

import com.android.emulator.control.ClipData
import com.android.emulator.control.EmulatorControllerGrpc
import com.android.emulator.control.EmulatorStatus
import com.android.emulator.control.Image
import com.android.emulator.control.ImageFormat
import com.android.emulator.control.KeyboardEvent
import com.android.emulator.control.MouseEvent
import com.android.emulator.control.PhysicalModelValue
import com.android.emulator.control.VmRunState
import io.grpc.CallCredentials
import io.grpc.CompressorRegistry
import io.grpc.ConnectivityState
import io.grpc.DecompressorRegistry
import io.grpc.ManagedChannel
import io.grpc.ManagedChannelBuilder
import io.grpc.Metadata
import io.grpc.MethodDescriptor
import io.grpc.Status
import io.grpc.StatusRuntimeException
import io.grpc.stub.ClientCallStreamObserver
import io.grpc.stub.ClientCalls
import io.grpc.stub.ClientResponseObserver
import io.grpc.stub.StreamObserver
import java.util.concurrent.Executor
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference

import com.android.emulator.control.Image as ImageMessage

/**
 * Controls a running Emulator.
 */
class EmulatorController() {
    private var channel: ManagedChannel? = null
    @Volatile private var emulatorControllerStub: EmulatorControllerGrpc.EmulatorControllerStub? = null
    private var emulatorController: EmulatorControllerGrpc.EmulatorControllerStub
        get() {
            return emulatorControllerStub!!
        }
        private inline set(stub) {
            emulatorControllerStub = stub
        }

    init {

    }

    fun connect() {
        val maxInboundMessageSize: Int
        val IMG_WIDTH = 1080 / 3
        val IMG_HEIGHT = 1920 / 3

        // TODO: Change 4 to 3 after b/174277113 is fixed.
        maxInboundMessageSize = IMG_WIDTH * IMG_HEIGHT * 4 + 100 // Four bytes per pixel.

        connectGrpc(maxInboundMessageSize)
    }

    /**
     * Establishes a gRPC connection to the Emulator.
     *
     * @param maxInboundMessageSize: size of maximum inbound gRPC message, default to 4 MiB.
     */
    fun connectGrpc(maxInboundMessageSize: Int = 4 * 1024 * 1024) {
        val grpcPort = 12345
        val channel = ManagedChannelBuilder
            .forAddress("localhost", grpcPort)
            .usePlaintext() // TODO: Add support for TLS encryption.
            .maxInboundMessageSize(maxInboundMessageSize)
            .compressorRegistry(CompressorRegistry.newEmptyInstance()) // Disable data compression.
            .decompressorRegistry(DecompressorRegistry.emptyInstance())
            .build()
        this.channel = channel

        emulatorController = EmulatorControllerGrpc.newStub(channel)
    }

    /**
     * Streams a series of screenshots.
     */
    fun streamScreenshot(imageFormat: ImageFormat, streamObserver: StreamObserver<Image>): Cancelable {
        System.out.println("Starting streamScreenshot")
        val method = EmulatorControllerGrpc.getStreamScreenshotMethod()
        val call = emulatorController.channel.newCall(method, emulatorController.callOptions)
        ClientCalls.asyncServerStreamingCall(call, imageFormat, DelegatingStreamObserver(streamObserver, method))
        return object : Cancelable {
            override fun cancel() {
                call.cancel("Canceled by consumer", null)
            }
        }
    }

    fun shutdown() {
        channel?.shutdown()
    }

    /**
     * The state of the [EmulatorController].
     */
    enum class ConnectionState {
        NOT_INITIALIZED,
        CONNECTING,
        CONNECTED,
        DISCONNECTED
    }

    enum class EmulatorState {
        RUNNING,
        SHUTDOWN_REQUESTED,
        SHUTDOWN_SENT
    }

    private open inner class DelegatingStreamObserver<RequestT, ResponseT>(
        open val delegate: StreamObserver<in ResponseT>?,
        val method: MethodDescriptor<in RequestT, in ResponseT>
    ) : StreamObserver<ResponseT> {

        override fun onNext(response: ResponseT) {
            delegate?.onNext(response)
        }

        override fun onError(t: Throwable) {
            if (!(t is StatusRuntimeException && t.status.code == Status.Code.CANCELLED) && channel?.isShutdown == false) {
                System.out.println("${method.fullMethodName} call failed - ${t.message}")
            }

            delegate?.onError(t)

            if (t is StatusRuntimeException && t.status.code == Status.Code.UNAVAILABLE) {
                System.out.println("channel disconnected")
            }
        }

        override fun onCompleted() {
            delegate?.onCompleted()
        }
    }

    private open inner class DelegatingClientResponseObserver<RequestT, ResponseT>(
        override val delegate: ClientResponseObserver<RequestT, ResponseT>?,
        method: MethodDescriptor<in RequestT, in ResponseT>
    ) : DelegatingStreamObserver<RequestT, ResponseT>(delegate, method), ClientResponseObserver<RequestT, ResponseT> {

        override fun beforeStart(requestStream: ClientCallStreamObserver<RequestT>?) {
            delegate?.beforeStart(requestStream)
        }
    }
}