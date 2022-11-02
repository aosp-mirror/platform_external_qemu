// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "aemu/base/memory/SharedMemory.h"

#include "studio-view-headers.h"

using android::base::SharedMemory;

/*
 * Class:     android_emulation_studio_SharedMemory
 * Method:    openMemoryHandle
 * Signature: (Ljava/lang/String;I)J
 */
JNIEXPORT jlong JNICALL
Java_android_emulation_studio_SharedMemory_openMemoryHandle(JNIEnv* env,
                                                            jobject jThis,
                                                            jstring jHandle,
                                                            jint jSize) {
    jboolean isCopy;
    const char* handleString = env->GetStringUTFChars(jHandle, &isCopy);
    auto memory = new SharedMemory(handleString, jSize);
    int err = memory->open(SharedMemory::AccessMode::READ_ONLY);
    env->ReleaseStringUTFChars(jHandle, handleString);

    if (err != 0) {
        printf("Unable to open memory mapped handle: [%s], due to: %d\n",
               handleString, err);
        return 0;
    };
    return (jlong)memory;
}

/*
 * Class:     android_emulation_studio_SharedMemory
 * Method:    readMemory
 * Signature: (J[BI)Z
 */
JNIEXPORT jboolean JNICALL
Java_android_emulation_studio_SharedMemory_readMemoryB(JNIEnv* env,
                                                       jobject jThis,
                                                       jlong jHandle,
                                                       jbyteArray array,
                                                       jint offset) {
    jbyte* bufferPtr = env->GetByteArrayElements(array, nullptr);
    SharedMemory* mem = reinterpret_cast<SharedMemory*>(jHandle);
    jsize size = std::min<jsize>(mem->size(), env->GetArrayLength(array));
    if (size > 0 && offset < size)
        memcpy(bufferPtr, ((uint8_t*)mem->get()) + offset, size - offset);

    env->ReleaseByteArrayElements(array, bufferPtr, 0);
    return true;
};

/*
 * Class:     android_emulation_studio_SharedMemory
 * Method:    readMemory
 * Signature: (J[BI)Z
 */
JNIEXPORT jboolean JNICALL
Java_android_emulation_studio_SharedMemory_readMemoryI(JNIEnv* env,
                                                       jobject jThis,
                                                       jlong jHandle,
                                                       jintArray array,
                                                       jint offset) {
    jint* bufferPtr = env->GetIntArrayElements(array, nullptr);
    SharedMemory* mem = reinterpret_cast<SharedMemory*>(jHandle);
    jsize size = std::min<jsize>(mem->size(), env->GetArrayLength(array));
    if (size > 0 && offset < size)
        memcpy(bufferPtr, ((uint8_t*)mem->get()) + offset, 4 * size - offset);

    env->ReleaseIntArrayElements(array, bufferPtr, 0);
    return true;
};

/*
 * Class:     android_emulation_studio_SharedMemory
 * Method:    close
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_android_emulation_studio_SharedMemory_close(JNIEnv* env,
                                                 jobject jThis,
                                                 jlong jHandle) {
    SharedMemory* mem = reinterpret_cast<SharedMemory*>(jHandle);
    delete mem;
}