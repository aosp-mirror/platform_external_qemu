package com.android.emulator.utils

/**
 * Interface for something that can be canceled.
 */
interface Cancelable {
    /**
     * Cancels this Cancelable if it hasn't been canceled or finished already.
     */
    fun cancel()
}