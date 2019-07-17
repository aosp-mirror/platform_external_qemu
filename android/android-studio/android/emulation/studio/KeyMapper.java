package android.emulation.studio;

import java.awt.event.KeyEvent;
import java.util.logging.Logger;

/**
 * A KeyMapper can translate Java Key Codes into Linux Key codes. This translation can be used to
 * send generic events to the android emulator. This requires proper support in the android
 * system-image.
 *
 * @author Erwin Jansen (jansene@google.com)
 */
public class KeyMapper {

  private static final Logger LOGGER = Logger.getLogger(KeyMapper.class.getName());
  /*
   * Java key event --> Android linux input events.
   * These values come dropUntil input-event-codes.h
   * Note that this is not a perfect mapping
   * as not all java codes have a corresponding value. Codes for which no mapping
   * exists are commented out.
   *
   * Keys and buttons
   *
   * Most of the keys/buttons are modeled after USB HUT 1.12
   * (see http://www.usb.org/developers/hidpage).
   * Abbreviations in the comments:
   * AC - Application Control
   * AL - Application Launch Button
   * SC - System Control
   */
  private static int[][] java_linux_map = new int[][]
      {
          {KeyEvent.VK_ESCAPE, 1},
          {KeyEvent.VK_1, 2},
          {KeyEvent.VK_2, 3},
          {KeyEvent.VK_3, 4},
          {KeyEvent.VK_4, 5},
          {KeyEvent.VK_5, 6},
          {KeyEvent.VK_6, 7},
          {KeyEvent.VK_7, 8},
          {KeyEvent.VK_8, 9},
          {KeyEvent.VK_9, 10},
          {KeyEvent.VK_0, 11},
          {KeyEvent.VK_MINUS, 12},
          {KeyEvent.VK_EQUALS, 13},
          {KeyEvent.VK_BACK_SPACE, 14},
          {KeyEvent.VK_TAB, 15},
          {KeyEvent.VK_Q, 16},
          {KeyEvent.VK_W, 17},
          {KeyEvent.VK_E, 18},
          {KeyEvent.VK_R, 19},
          {KeyEvent.VK_T, 20},
          {KeyEvent.VK_Y, 21},
          {KeyEvent.VK_U, 22},
          {KeyEvent.VK_I, 23},
          {KeyEvent.VK_O, 24},
          {KeyEvent.VK_P, 25},
          {KeyEvent.VK_OPEN_BRACKET, 26},
          {KeyEvent.VK_CLOSE_BRACKET, 27},
          {KeyEvent.VK_ENTER, 28},
          {KeyEvent.VK_CONTROL, 29},
          {KeyEvent.VK_A, 30},
          {KeyEvent.VK_S, 31},
          {KeyEvent.VK_D, 32},
          {KeyEvent.VK_F, 33},
          {KeyEvent.VK_G, 34},
          {KeyEvent.VK_H, 35},
          {KeyEvent.VK_J, 36},
          {KeyEvent.VK_K, 37},
          {KeyEvent.VK_L, 38},
          {KeyEvent.VK_SEMICOLON, 39},
          {KeyEvent.VK_QUOTE, 40}, // '
          {KeyEvent.VK_BACK_QUOTE, 0x18f}, // KEY_GREEN (okay)
          {KeyEvent.VK_SHIFT, 42},
          {KeyEvent.VK_BACK_SLASH, 43},
          {KeyEvent.VK_Z, 44},
          {KeyEvent.VK_X, 45},
          {KeyEvent.VK_C, 46},
          {KeyEvent.VK_V, 47},
          {KeyEvent.VK_B, 48},
          {KeyEvent.VK_N, 49},
          {KeyEvent.VK_M, 50},
          {KeyEvent.VK_COMMA, 51},
          {KeyEvent.VK_PERIOD, 52},
          {KeyEvent.VK_SLASH, 53},
          {KeyEvent.VK_SHIFT, 54}, // Right shift == shift
          // { KeyEvent.VK_KPASTERISK	,	55},
          {KeyEvent.VK_ALT, 56}, // Left alt == alt
          {KeyEvent.VK_SPACE, 57},
          {KeyEvent.VK_CAPS_LOCK, 58},
          {KeyEvent.VK_F1, 59},
          {KeyEvent.VK_F2, 60},
          {KeyEvent.VK_F3, 61},
          {KeyEvent.VK_F4, 62},
          {KeyEvent.VK_F5, 63},
          {KeyEvent.VK_F6, 64},
          {KeyEvent.VK_F7, 65},
          {KeyEvent.VK_F8, 66},
          {KeyEvent.VK_F9, 67},
          {KeyEvent.VK_F10, 68},
          {KeyEvent.VK_NUM_LOCK, 69},
          {KeyEvent.VK_SCROLL_LOCK, 70},
          {KeyEvent.VK_NUMPAD7, 71},
          {KeyEvent.VK_NUMPAD8, 72},
          {KeyEvent.VK_NUMPAD9, 73},
          {KeyEvent.VK_MINUS, 74}, // Numpad =/-
          {KeyEvent.VK_NUMPAD4, 75},
          {KeyEvent.VK_NUMPAD5, 76},
          {KeyEvent.VK_NUMPAD6, 77},
          {KeyEvent.VK_PLUS, 78},
          {KeyEvent.VK_NUMPAD1, 79},
          {KeyEvent.VK_NUMPAD2, 80},
          {KeyEvent.VK_NUMPAD3, 81},
          {KeyEvent.VK_NUMPAD0, 82},
          {KeyEvent.VK_PERIOD, 83}, // Numpad dot
          // { KeyEvent.VK_ZENKAKUHANKAKU	,	85},
          // { KeyEvent.VK_102ND	,	86},
          {KeyEvent.VK_F11, 87},
          {KeyEvent.VK_F12, 88},
          // { KeyEvent.VK_RO	,	89},
          {KeyEvent.VK_KATAKANA, 90},
          {KeyEvent.VK_HIRAGANA, 91},
          // { KeyEvent.VK_HENKAN	,	92},
          // { KeyEvent.VK_KATAKANAHIRAGANA	,	93},
          // { KeyEvent.VK_MUHENKAN	,	94},
          // { KeyEvent.VK_KPJPCOMMA	,	95},
          // { KeyEvent.VK_KPENTER	,	96},
          {KeyEvent.VK_CONTROL, 97},
          {KeyEvent.VK_SLASH, 98},
          // { KeyEvent.VK_SYSRQ	,	99},
          {KeyEvent.VK_ALT, 100},
          // { KeyEvent.VK_LINEFEED	,	101},
          {KeyEvent.VK_HOME, 102},
          {KeyEvent.VK_UP, 103},
          {KeyEvent.VK_PAGE_UP, 104},
          {KeyEvent.VK_LEFT, 105},
          {KeyEvent.VK_RIGHT, 106},
          {KeyEvent.VK_END, 107},
          {KeyEvent.VK_DOWN, 108},
          {KeyEvent.VK_PAGE_DOWN, 109},
          {KeyEvent.VK_INSERT, 110},
          {KeyEvent.VK_DELETE, 111},
          {KeyEvent.VK_DEAD_MACRON, 112},
          //  { KeyEvent.VK_MUTE	,	113},
          //  { KeyEvent.VK_VOLUMEDOWN	,	114},
          //  { KeyEvent.VK_VOLUMEUP	,	115},
          //  { KeyEvent.VK_POWER	,	116	,	/* SC System Power Down */},
          {KeyEvent.VK_EQUALS, 117}, // Keypad
          {KeyEvent.VK_PLUS, 118}, // Plus/Minus
          {KeyEvent.VK_PAUSE, 119},
          //  { KeyEvent.VK_SCALE	,	120	,	/* AL Compiz Scale (Expose) */},
          {KeyEvent.VK_COMMA, 121},
          //  { KeyEvent.VK_HANGEUL	,	122},
          //  { KeyEvent.VK_HANGUEL	,	KeyEvent.VK_HANGEUL},
          //  { KeyEvent.VK_HANJA	,	123},
          //  { KeyEvent.VK_YEN	,	124},
          {KeyEvent.VK_META, 125},  // Left meta
          {KeyEvent.VK_META, 126}, // Right
          {KeyEvent.VK_COMPOSE, 127},
          //  { KeyEvent.VK_FN	,	0x1d0}
      };


  /**
   * Translates a JavaKey event into a string that can be send to the telnet console.
   *
   * Will return an empty string if no translation exists. This can be safely send to the emulator.
   *
   * @param keyEvent The java key event to be translated
   * @param down True if this is a down event, or false if it is an up event.
   * @return The string to be send to the emulator, empty string if no translation exists.
   */
  public static String translate(KeyEvent keyEvent, boolean down) {
    int androidKeyCode = -1;
    int lookup = keyEvent.getKeyCode();
    for (int i = 0; i < java_linux_map.length; i++) {
      if (java_linux_map[i][0] == lookup) {
        androidKeyCode = java_linux_map[i][1];
        break;
      }
    }
    // Unknown key code,
    if (androidKeyCode == -1) {
      LOGGER.info("No translation for " +keyEvent);
      return "";
    }

    return String.format("event send EV_KEY:%d:%d", androidKeyCode, (down ? 1 : 0));
  }
}
