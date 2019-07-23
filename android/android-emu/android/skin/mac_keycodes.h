#pragma once
// Copied from
// Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/Headers/Event.h
// Add prefix kMacOS to avoid naming conflicts.
enum {
    /* modifiers */
    kMacOSActiveFlagBit = 0,  /* activate? (activateEvt and mouseDown)*/
    kMacOSBtnStateBit = 7,    /* state of button?*/
    kMacOSCmdKeyBit = 8,      /* command key down?*/
    kMacOSShiftKeyBit = 9,    /* shift key down?*/
    kMacOSAlphaLockBit = 10,  /* alpha lock down?*/
    kMacOSOptionKeyBit = 11,  /* option key down?*/
    kMacOSControlKeyBit = 12, /* control key down?*/
    kMacOSRightShiftKeyBit =
            13, /* right shift key down? Not supported on Mac OS X.*/
    kMacOSRightOptionKeyBit =
            14, /* right Option key down? Not supported on Mac OS X.*/
    kMacOSRightControlKeyBit =
            15 /* right Control key down? Not supported on Mac OS X.*/
};

enum {
    kMacOSActiveFlag = 1 << kMacOSActiveFlagBit,
    kMacOSBtnState = 1 << kMacOSBtnStateBit,
    kMacOSCmdKey = 1 << kMacOSCmdKeyBit,
    kMacOSShiftKey = 1 << kMacOSShiftKeyBit,
    kMacOSAlphaLock = 1 << kMacOSAlphaLockBit,
    kMacOSOptionKey = 1 << kMacOSOptionKeyBit,
    kMacOSControlKey = 1 << kMacOSControlKeyBit,
    kMacOSRightShiftKey =
            1 << kMacOSRightShiftKeyBit, /* Not supported on Mac OS X.*/
    kMacOSRightOptionKey =
            1 << kMacOSRightOptionKeyBit, /* Not supported on Mac OS X.*/
    kMacOSRightControlKey =
            1 << kMacOSRightControlKeyBit /* Not supported on Mac OS X.*/
};

/*
 *  Summary:
 *    Virtual keycodes
 *
 *  Discussion:
 *    These constants are the virtual keycodes defined originally in
 *    Inside Mac Volume V, pg. V-191. They identify physical keys on a
 *    keyboard. Those constants with "ANSI" in the name are labeled
 *    according to the key position on an ANSI-standard US keyboard.
 *    For example, kVK_ANSI_A indicates the virtual keycode for the key
 *    with the letter 'A' in the US keyboard layout. Other keyboard
 *    layouts may have the 'A' key label on a different physical key;
 *    in this case, pressing 'A' will generate a different virtual
 *    keycode.
 */

enum {
    kMacOS_VK_ANSI_A = 0x00,
    kMacOS_VK_ANSI_S = 0x01,
    kMacOS_VK_ANSI_D = 0x02,
    kMacOS_VK_ANSI_F = 0x03,
    kMacOS_VK_ANSI_H = 0x04,
    kMacOS_VK_ANSI_G = 0x05,
    kMacOS_VK_ANSI_Z = 0x06,
    kMacOS_VK_ANSI_X = 0x07,
    kMacOS_VK_ANSI_C = 0x08,
    kMacOS_VK_ANSI_V = 0x09,
    kMacOS_VK_ANSI_B = 0x0B,
    kMacOS_VK_ANSI_Q = 0x0C,
    kMacOS_VK_ANSI_W = 0x0D,
    kMacOS_VK_ANSI_E = 0x0E,
    kMacOS_VK_ANSI_R = 0x0F,
    kMacOS_VK_ANSI_Y = 0x10,
    kMacOS_VK_ANSI_T = 0x11,
    kMacOS_VK_ANSI_1 = 0x12,
    kMacOS_VK_ANSI_2 = 0x13,
    kMacOS_VK_ANSI_3 = 0x14,
    kMacOS_VK_ANSI_4 = 0x15,
    kMacOS_VK_ANSI_6 = 0x16,
    kMacOS_VK_ANSI_5 = 0x17,
    kMacOS_VK_ANSI_Equal = 0x18,
    kMacOS_VK_ANSI_9 = 0x19,
    kMacOS_VK_ANSI_7 = 0x1A,
    kMacOS_VK_ANSI_Minus = 0x1B,
    kMacOS_VK_ANSI_8 = 0x1C,
    kMacOS_VK_ANSI_0 = 0x1D,
    kMacOS_VK_ANSI_RightBracket = 0x1E,
    kMacOS_VK_ANSI_O = 0x1F,
    kMacOS_VK_ANSI_U = 0x20,
    kMacOS_VK_ANSI_LeftBracket = 0x21,
    kMacOS_VK_ANSI_I = 0x22,
    kMacOS_VK_ANSI_P = 0x23,
    kMacOS_VK_ANSI_L = 0x25,
    kMacOS_VK_ANSI_J = 0x26,
    kMacOS_VK_ANSI_Quote = 0x27,
    kMacOS_VK_ANSI_K = 0x28,
    kMacOS_VK_ANSI_Semicolon = 0x29,
    kMacOS_VK_ANSI_Backslash = 0x2A,
    kMacOS_VK_ANSI_Comma = 0x2B,
    kMacOS_VK_ANSI_Slash = 0x2C,
    kMacOS_VK_ANSI_N = 0x2D,
    kMacOS_VK_ANSI_M = 0x2E,
    kMacOS_VK_ANSI_Period = 0x2F,
    kMacOS_VK_ANSI_Grave = 0x32,
    kMacOS_VK_ANSI_KeypadDecimal = 0x41,
    kMacOS_VK_ANSI_KeypadMultiply = 0x43,
    kMacOS_VK_ANSI_KeypadPlus = 0x45,
    kMacOS_VK_ANSI_KeypadClear = 0x47,
    kMacOS_VK_ANSI_KeypadDivide = 0x4B,
    kMacOS_VK_ANSI_KeypadEnter = 0x4C,
    kMacOS_VK_ANSI_KeypadMinus = 0x4E,
    kMacOS_VK_ANSI_KeypadEquals = 0x51,
    kMacOS_VK_ANSI_Keypad0 = 0x52,
    kMacOS_VK_ANSI_Keypad1 = 0x53,
    kMacOS_VK_ANSI_Keypad2 = 0x54,
    kMacOS_VK_ANSI_Keypad3 = 0x55,
    kMacOS_VK_ANSI_Keypad4 = 0x56,
    kMacOS_VK_ANSI_Keypad5 = 0x57,
    kMacOS_VK_ANSI_Keypad6 = 0x58,
    kMacOS_VK_ANSI_Keypad7 = 0x59,
    kMacOS_VK_ANSI_Keypad8 = 0x5B,
    kMacOS_VK_ANSI_Keypad9 = 0x5C
};

/* keycodes for keys that are independent of keyboard layout*/
enum {
    kMacOS_VK_Return = 0x24,
    kMacOS_VK_Tab = 0x30,
    kMacOS_VK_Space = 0x31,
    kMacOS_VK_Delete = 0x33,
    kMacOS_VK_Escape = 0x35,
    kMacOS_VK_Command = 0x37,
    kMacOS_VK_Shift = 0x38,
    kMacOS_VK_CapsLock = 0x39,
    kMacOS_VK_Option = 0x3A,
    kMacOS_VK_Control = 0x3B,
    kMacOS_VK_RightCommand = 0x36,
    kMacOS_VK_RightShift = 0x3C,
    kMacOS_VK_RightOption = 0x3D,
    kMacOS_VK_RightControl = 0x3E,
    kMacOS_VK_Function = 0x3F,
    kMacOS_VK_F17 = 0x40,
    kMacOS_VK_VolumeUp = 0x48,
    kMacOS_VK_VolumeDown = 0x49,
    kMacOS_VK_Mute = 0x4A,
    kMacOS_VK_F18 = 0x4F,
    kMacOS_VK_F19 = 0x50,
    kMacOS_VK_F20 = 0x5A,
    kMacOS_VK_F5 = 0x60,
    kMacOS_VK_F6 = 0x61,
    kMacOS_VK_F7 = 0x62,
    kMacOS_VK_F3 = 0x63,
    kMacOS_VK_F8 = 0x64,
    kMacOS_VK_F9 = 0x65,
    kMacOS_VK_F11 = 0x67,
    kMacOS_VK_F13 = 0x69,
    kMacOS_VK_F16 = 0x6A,
    kMacOS_VK_F14 = 0x6B,
    kMacOS_VK_F10 = 0x6D,
    kMacOS_VK_F12 = 0x6F,
    kMacOS_VK_F15 = 0x71,
    kMacOS_VK_Help = 0x72,
    kMacOS_VK_Home = 0x73,
    kMacOS_VK_PageUp = 0x74,
    kMacOS_VK_ForwardDelete = 0x75,
    kMacOS_VK_F4 = 0x76,
    kMacOS_VK_End = 0x77,
    kMacOS_VK_F2 = 0x78,
    kMacOS_VK_PageDown = 0x79,
    kMacOS_VK_F1 = 0x7A,
    kMacOS_VK_LeftArrow = 0x7B,
    kMacOS_VK_RightArrow = 0x7C,
    kMacOS_VK_DownArrow = 0x7D,
    kMacOS_VK_UpArrow = 0x7E
};

/* ISO keyboards only*/
enum { kMacOS_VK_ISO_Section = 0x0A };

/* JIS keyboards only*/
enum {
    kMacOS_VK_JIS_Yen = 0x5D,
    kMacOS_VK_JIS_Underscore = 0x5E,
    kMacOS_VK_JIS_KeypadComma = 0x5F,
    kMacOS_VK_JIS_Eisu = 0x66,
    kMacOS_VK_JIS_Kana = 0x68
};