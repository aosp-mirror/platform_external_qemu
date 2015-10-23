/* Copyright (C) 2007-2015 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android/skin/charmap.h"

#include "android/utils/path.h"
#include "android/utils/misc.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Parses .kcm file producing key characters map.
 * .kcm file parsed by this module is expected to contain 4 types of
 * lines:
 * 1. An empty line (containing no characters, or only space or tab
 *    characters).
 * 2. A comment line (begins with '#')
 * 3. A type section line (begins with '[')
 * 4. Character map line, formatted as such:
 * Key code value, followed by one or more space or tab characters.
 * Display value, followed by one or more space or tab characters.
 * Number value, followed by one or more space or tab characters.
 * Base value, followed by one or more space or tab characters.
 * Caps value, followed by one or more space or tab characters.
 * Fn value, followed by one or more space or tab characters.
 * Caps_fn value, followed by one or more space or tab characters.
 * All values, except for the key code value must be either in character
 * form ('X', where X is the value), or in hexadecimal form (0xXXXX, where
 * XXXX is hexadecimal representation of the value). Note that if value is
 * in hexadecimal form, it must not exceed value that can be contained in
 * variable of 'unsigned short' type.
 * Bellow are a couple of examples of valid .kcm file lines:
 * # keycode       display number  base    caps    fn      caps_fn
 * A               'A'     '2'     'a'     'A'     '#'     0x00
 * PERIOD          '.'     '.'     '.'     ':'     ':'     0x2026
 * SPACE           0x20    0x20    0x20    0x20    0xEF01  0xEF01
*/

/* Maximum length of a line expected in .kcm file. */
#define KCM_MAX_LINE_LEN    1024

/* Maximum length of a token in a key map line. */
#define KCM_MAX_TOKEN_LEN   512

/* Maps symbol name from .kcm file to a keycode value. */
typedef struct AKeycodeMapEntry {
    /* Symbol name from .kcm file. */
    const char* key_name;

    /* Key code value for the symbol. */
    int         key_code;
} AKeycodeMapEntry;

/* Result of parsing a line in a .kcm file. */
typedef enum {
    /* Line format was bad. */
    BAD_FORMAT,

    /* Line had been skipped (an empty line, or a comment, etc.). */
    SKIP_LINE,

    /* Line represents an entry in the key map. */
    KEY_ENTRY,
} ParseStatus;

static const AKeycodeMapEntry keycode_map[] = {
    /*  Symbol           Key code */

      { "A",             kKeyCodeA },
      { "B",             kKeyCodeB },
      { "C",             kKeyCodeC },
      { "D",             kKeyCodeD },
      { "E",             kKeyCodeE },
      { "F",             kKeyCodeF },
      { "G",             kKeyCodeG },
      { "H",             kKeyCodeH },
      { "I",             kKeyCodeI },
      { "J",             kKeyCodeJ },
      { "K",             kKeyCodeK },
      { "L",             kKeyCodeL },
      { "M",             kKeyCodeM },
      { "N",             kKeyCodeN },
      { "O",             kKeyCodeO },
      { "P",             kKeyCodeP },
      { "Q",             kKeyCodeQ },
      { "R",             kKeyCodeR },
      { "S",             kKeyCodeS },
      { "T",             kKeyCodeT },
      { "U",             kKeyCodeU },
      { "V",             kKeyCodeV },
      { "W",             kKeyCodeW },
      { "X",             kKeyCodeX },
      { "Y",             kKeyCodeY },
      { "Z",             kKeyCodeZ },
      { "0",             kKeyCode0 },
      { "1",             kKeyCode1 },
      { "2",             kKeyCode2 },
      { "3",             kKeyCode3 },
      { "4",             kKeyCode4 },
      { "5",             kKeyCode5 },
      { "6",             kKeyCode6 },
      { "7",             kKeyCode7 },
      { "8",             kKeyCode8 },
      { "9",             kKeyCode9 },
      { "COMMA",         kKeyCodeComma },
      { "PERIOD",        kKeyCodePeriod },
      { "AT",            kKeyCodeAt },
      { "SLASH",         kKeyCodeSlash },
      { "SPACE",         kKeyCodeSpace },
      { "ENTER",         kKeyCodeNewline },
      { "TAB",           kKeyCodeTab },
      { "GRAVE",         kKeyCodeGrave },
      { "MINUS",         kKeyCodeMinus },
      { "EQUALS",        kKeyCodeEquals },
      { "LEFT_BRACKET",  kKeyCodeLeftBracket },
      { "RIGHT_BRACKET", kKeyCodeRightBracket },
      { "BACKSLASH",     kKeyCodeBackslash },
      { "SEMICOLON",     kKeyCodeSemicolon },
      { "APOSTROPHE",    kKeyCodeApostrophe },
      { "STAR",          kKeyCodeStar },
      { "POUND",         kKeyCodePound },
      { "PLUS",          kKeyCodePlus },
      { "DEL",           kKeyCodeDel },
};

/* the following is automatically generated by the 'gen-charmap.py' script
 * do not touch. the generation command was:
 *   gen-charmap.py qwerty2.kcm
 */

static const SkinKeyEntry  _qwerty2_keys[] =
{
   /* keycode                   base   caps    fn  caps+fn   number */

    { kKeyCodeA             ,   'a',   'A',   'a',    'A',   'a' },
    { kKeyCodeB             ,   'b',   'B',   'b',    'B',   'b' },
    { kKeyCodeC             ,   'c',   'C', 0x00e7, 0x00E7,   'c' },
    { kKeyCodeD             ,   'd',   'D',  '\'',   '\'',  '\'' },
    { kKeyCodeE             ,   'e',   'E',   '"', 0x0301,   '"' },
    { kKeyCodeF             ,   'f',   'F',   '[',    '[',   '[' },
    { kKeyCodeG             ,   'g',   'G',   ']',    ']',   ']' },
    { kKeyCodeH             ,   'h',   'H',   '<',    '<',   '<' },
    { kKeyCodeI             ,   'i',   'I',   '-', 0x0302,   '-' },
    { kKeyCodeJ             ,   'j',   'J',   '>',    '>',   '>' },
    { kKeyCodeK             ,   'k',   'K',   ';',    '~',   ';' },
    { kKeyCodeL             ,   'l',   'L',   ':',    '`',   ':' },
    { kKeyCodeM             ,   'm',   'M',   '%',   0x00,   '%' },
    { kKeyCodeN             ,   'n',   'N',  0x00, 0x0303,   'n' },
    { kKeyCodeO             ,   'o',   'O',   '+',    '+',   '+' },
    { kKeyCodeP             ,   'p',   'P',   '=', 0x00A5,   '=' },
    { kKeyCodeQ             ,   'q',   'Q',   '|', 0x0300,   '|' },
    { kKeyCodeR             ,   'r',   'R',   '`', 0x20AC,   '`' },
    { kKeyCodeS             ,   's',   'S',  '\\', 0x00DF,  '\\' },
    { kKeyCodeT             ,   't',   'T',   '{', 0x00A3,   '}' },
    { kKeyCodeU             ,   'u',   'U',   '_', 0x0308,   '_' },
    { kKeyCodeV             ,   'v',   'V',   'v',    'V',   'v' },
    { kKeyCodeW             ,   'w',   'W',   '~',    '~',   '~' },
    { kKeyCodeX             ,   'x',   'X',   'x',    'X',   'x' },
    { kKeyCodeY             ,   'y',   'Y',   '}', 0x00A1,   '}' },
    { kKeyCodeZ             ,   'z',   'Z',   'z',    'Z',   'z' },
    { kKeyCodeComma         ,   ',',   '<',   ',',    ',',   ',' },
    { kKeyCodePeriod        ,   '.',   '>',   '.', 0x2026,   '.' },
    { kKeyCodeAt            ,   '@',   '@',   '@', 0x2022,   '@' },
    { kKeyCodeSlash         ,   '/',   '?',   '?',    '?',   '/' },
    { kKeyCodeSpace         ,  0x20,  0x20,   0x9,    0x9,  0x20 },
    { kKeyCodeNewline       ,   0xa,   0xa,   0xa,    0xa,   0xa },
    { kKeyCode0             ,   '0',   ')',   ')',    ')',   '0' },
    { kKeyCode1             ,   '1',   '!',   '!',    '!',   '1' },
    { kKeyCode2             ,   '2',   '@',   '@',    '@',   '2' },
    { kKeyCode3             ,   '3',   '#',   '#',    '#',   '3' },
    { kKeyCode4             ,   '4',   '$',   '$',    '$',   '4' },
    { kKeyCode5             ,   '5',   '%',   '%',    '%',   '5' },
    { kKeyCode6             ,   '6',   '^',   '^',    '^',   '6' },
    { kKeyCode7             ,   '7',   '&',   '&',    '&',   '7' },
    { kKeyCode8             ,   '8',   '*',   '*',    '*',   '8' },
    { kKeyCode9             ,   '9',   '(',   '(',    '(',   '9' },
    { kKeyCodeTab           ,   0x9,   0x9,   0x9,    0x9,   0x9 },
    { kKeyCodeGrave         ,   '`',   '~',   '`',    '~',   '`' },
    { kKeyCodeMinus         ,   '-',   '_',   '-',    '_',   '-' },
    { kKeyCodeEquals        ,   '=',   '+',   '=',    '+',   '=' },
    { kKeyCodeLeftBracket   ,   '[',   '{',   '[',    '{',   '[' },
    { kKeyCodeRightBracket  ,   ']',   '}',   ']',    '}',   ']' },
    { kKeyCodeBackslash     ,  '\\',   '|',  '\\',    '|',  '\\' },
    { kKeyCodeSemicolon     ,   ';',   ':',   ';',    ':',   ';' },
    { kKeyCodeApostrophe    ,  '\'',   '"',  '\'',    '"',  '\'' },
};

static const SkinCharmap  _default_charmap =
{
    _qwerty2_keys,
    51,
    "qwerty2"
};

/* Custom character map created with -charmap option. */
static SkinCharmap android_custom_charmap = { 0 };

static const SkinCharmap* android_charmap = &_default_charmap;

/* Checks if a character represents a path separator.
 * Returns a non-zero value if ch is a path separator.
 * Returns zero value if ch is not a path separator.
*/
static int
kcm_is_path_separator(char ch) {
#ifdef _WIN32
    return '/' == ch || '\\' == ch;
#else
    return '/' == ch;
#endif  // _WIN32
}

void
kcm_extract_charmap_name(const char* kcm_file_path,
                         char* charmap_name,
                         int max_len) {
    const char* ext_separator;
    size_t to_copy;

    // Initialize charmap name with name of .kcm file.
    // First, get file name from the full path to .kcm file.
    const char* file_name = kcm_file_path + strlen(kcm_file_path);
    while (!kcm_is_path_separator(*file_name) &&
           (file_name != kcm_file_path)) {
        file_name--;
    }
    if (kcm_is_path_separator(*file_name)) {
        file_name++;
    }

    // Cut off file name extension.
    ext_separator = strrchr(file_name, '.');
    if (NULL == ext_separator) {
      // "filename" is legal name.
      ext_separator = file_name + strlen(file_name);
    } else if (ext_separator == file_name) {
      // ".filename" is legal name too. In this case we will use
      // "filename" as our custom charmap name.
      file_name++;
      ext_separator = file_name + strlen(file_name);
    }

    // Copy file name to charmap name.
    to_copy = ext_separator - file_name;
    if (to_copy > (max_len - 1)) {
        to_copy = max_len - 1;
    }
    memcpy(charmap_name, file_name, to_copy);
    charmap_name[to_copy] = '\0';
}

void
skin_charmap_setup(const char* kcm_file_path) {

    /* Return if we already loaded a charmap */
    if (android_charmap != &_default_charmap || kcm_file_path == NULL)
        return;

    strncpy(android_custom_charmap.name, kcm_file_path, sizeof(android_custom_charmap.name));
    android_charmap = &android_custom_charmap;
    return;
}

void
skin_charmap_done(void) {
    if (android_charmap != &_default_charmap)
        AFREE((void*)android_charmap->entries);
}

const SkinCharmap*
skin_charmap_get_by_name(const char* name) {
    if (name != NULL) {
        if (!strcmp(android_charmap->name, name))
            return android_charmap;
        if (!strcmp(_default_charmap.name, name))
            return &_default_charmap;
    }
    return NULL;
}

int
skin_charmap_reverse_map_unicode(const SkinCharmap* cmap,
                                    unsigned int unicode,
                                    int  down,
                                    SkinKeycodeBuffer* keycodes)
{
    int                 n;

    if (cmap == NULL)
        return 0;

    if (unicode == 0)
        return 0;

    /* check base keys */
    for (n = 0; n < cmap->num_entries; n++) {
        if (cmap->entries[n].base == unicode) {
            skin_keycode_buffer_add(keycodes, cmap->entries[n].code, down);
            return 1;
        }
    }

    /* check caps + keys */
    for (n = 0; n < cmap->num_entries; n++) {
        if (cmap->entries[n].caps == unicode) {
            if (down) {
                skin_keycode_buffer_add(keycodes, kKeyCodeCapLeft, down);
            }
            skin_keycode_buffer_add(keycodes, cmap->entries[n].code, down);
            if (!down) {
                skin_keycode_buffer_add(keycodes, kKeyCodeCapLeft, down);
            }
            return 2;
        }
    }

    /* check fn + keys */
    for (n = 0; n < cmap->num_entries; n++) {
        if (cmap->entries[n].fn == unicode) {
            if (down) {
                skin_keycode_buffer_add(keycodes, kKeyCodeAltLeft, down);
            }
            skin_keycode_buffer_add(keycodes, cmap->entries[n].code, down);
            if (!down) {
                skin_keycode_buffer_add(keycodes, kKeyCodeAltLeft, down);
            }
            return 2;
        }
    }

    /* check caps + fn + keys */
    for (n = 0; n < cmap->num_entries; n++) {
        if (cmap->entries[n].caps_fn == unicode) {
            if (down) {
                skin_keycode_buffer_add(keycodes, kKeyCodeAltLeft, down);
                skin_keycode_buffer_add(keycodes, kKeyCodeCapLeft, down);
            }
            skin_keycode_buffer_add(keycodes, cmap->entries[n].code, down);
            if (!down) {
                skin_keycode_buffer_add(keycodes, kKeyCodeCapLeft, down);
                skin_keycode_buffer_add(keycodes, kKeyCodeAltLeft, down);
            }
            return 3;
        }
    }

    /* no match */
    return 0;
}

const SkinCharmap* android_get_default_charmap(void)
{
    return &_default_charmap;
}

const SkinCharmap* skin_charmap_get(void)
{
    return android_charmap;
}

const char* skin_charmap_get_name(void)
{
    return skin_charmap_get()->name;
}
