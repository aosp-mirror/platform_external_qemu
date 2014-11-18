/* Copyright (C) 2007-2008 The Android Open Source Project
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
#include "android/skin/keyset.h"

#include "android/skin/keycode.h"
#include "android/utils/debug.h"
#include "android/utils/bufprint.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define  DEBUG  1

#if 1
#  define  D_ACTIVE  VERBOSE_CHECK(keys)
#else
#  define  D_ACTIVE  DEBUG
#endif

#if DEBUG
#  define  D(...)   VERBOSE_PRINT(keys,__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif

#define _SKIN_KEY_COMMAND(x,y)    #x ,
static const char* const command_strings[ SKIN_KEY_COMMAND_MAX ] = {
    SKIN_KEY_COMMAND_LIST
};
#undef _SKIN_KEY_COMMAND

const char*
skin_key_command_to_str( SkinKeyCommand  cmd )
{
    if (cmd > SKIN_KEY_COMMAND_NONE && cmd < SKIN_KEY_COMMAND_MAX)
        return  command_strings[cmd];

    return NULL;
}

SkinKeyCommand
skin_key_command_from_str( const char*  str, int  len )
{
    int  nn;
    if (len < 0)
        len = strlen(str);
    for (nn = 0; nn < SKIN_KEY_COMMAND_MAX; nn++) {
        const char*  cmd = command_strings[nn];

        if ( !memcmp( cmd, str, len ) && cmd[len] == 0 )
            return (SkinKeyCommand) nn;
    }
    return SKIN_KEY_COMMAND_NONE;
}


#define _SKIN_KEY_COMMAND(x,y)    y ,
static const char* const command_descriptions[ SKIN_KEY_COMMAND_MAX ] = {
    SKIN_KEY_COMMAND_LIST
};
#undef _SKIN_KEY_COMMAND

const char*
skin_key_command_description( SkinKeyCommand  cmd )
{
    if (cmd > SKIN_KEY_COMMAND_NONE && cmd < SKIN_KEY_COMMAND_MAX)
        return command_descriptions[cmd];

    return NULL;
}

typedef struct {
    int             sym;
    int             mod;
    SkinKeyCommand  command;
} SkinKeyItem;


struct SkinKeyset {
    int           num_items;
    int           max_items;
    SkinKeyItem*  items;
};


static int
skin_keyset_add( SkinKeyset*  kset, int  sym, int  mod, SkinKeyCommand  command )
{
    SkinKeyItem*  item = kset->items;
    SkinKeyItem*  end  = item + kset->num_items;
    SkinKeyItem*  first = NULL;
    int           count = 0;

    D("adding binding %s to %s",
      skin_key_command_to_str(command),
      skin_key_pair_to_string(sym,mod));

    for ( ; item < end; item++) {
        if (item->command == command) {
            if (!first)
                first = item;
            if (++count == SKIN_KEY_COMMAND_MAX_BINDINGS) {
                /* replace the first (oldest) one in the list */
                first->sym = sym;
                first->mod = mod;
                return 0;
            }
            continue;
        }
        if (item->sym == sym && item->mod == mod) {
            /* replace a (sym,mod) binding */
            item->command = command;
            return 0;
        }
    }
    if (kset->num_items >= kset->max_items) {
        int           old_size  = kset->max_items;
        int           new_size  = old_size + (old_size >> 1) + 4;
        SkinKeyItem*  new_items = realloc( kset->items, new_size*sizeof(SkinKeyItem) );
        if (new_items == NULL) {
            return -1;
        }
        kset->items     = new_items;
        kset->max_items = new_size;
    }
    item = kset->items + kset->num_items++;
    item->command = command;
    item->sym     = sym;
    item->mod     = mod;
    return 1;
}


SkinKeyset*
skin_keyset_new ( AConfig*  root )
{
    SkinKeyset*  kset = calloc(1, sizeof(*kset));
    AConfig*     node = root->first_child;;

    if (kset == NULL)
        return NULL;

    for ( ; node != NULL; node = node->next )
    {
        SkinKeyCommand  command;
        uint32_t        sym, mod;
        char*           p;

        command = skin_key_command_from_str( node->name, -1 );
        if (command == SKIN_KEY_COMMAND_NONE) {
            D( "ignoring unknown keyset command '%s'", node->name );
            continue;
        }
        p = (char*)node->value;
        while (*p) {
            char*  q = strpbrk( p, " \t,:" );
            if (q == NULL)
                q = p + strlen(p);

            if (q > p) {
                int   len = q - p;
                char  keys[24];
                if (len+1 >= (int)sizeof(keys)) {
                    D("key binding too long: '%s'", p);
                }
                else {
                    memcpy( keys, p, len );
                    keys[len] = 0;
                    if ( skin_key_pair_from_string( keys, &sym, &mod ) < 0 ) {
                        D( "ignoring unknown keys '%s' for command '%s'",
                                keys, node->name );
                    } else {
                        skin_keyset_add( kset, sym, mod, command );
                    }
                }
            } else if (*q)
                q += 1;

            p = q;
        }
    }
    return  kset;
}


SkinKeyset*
skin_keyset_new_from_text( const char*  text )
{
    AConfig*     root = aconfig_node("","");
    char*        str = strdup(text);
    SkinKeyset*  result;

    D("kset new from:\n%s", text);
    aconfig_load( root, str );
    result = skin_keyset_new( root );
    free(str);
    D("kset done result=%p", result);
    return result;
}


void
skin_keyset_free( SkinKeyset*  kset )
{
    if (kset) {
        free(kset->items);
        kset->items     = NULL;
        kset->num_items = 0;
        kset->max_items = 0;
        free(kset);
    }
}


extern int
skin_keyset_get_bindings( SkinKeyset*      kset,
                          SkinKeyCommand   command,
                          SkinKeyBinding*  bindings )
{
    if (kset) {
        int     count = 0;
        SkinKeyItem*  item = kset->items;
        SkinKeyItem*  end  = item + kset->num_items;

        for ( ; item < end; item++ ) {
            if (item->command == command) {
                bindings->sym = item->sym;
                bindings->mod = item->mod;
                bindings ++;
                if ( ++count >= SKIN_KEY_COMMAND_MAX_BINDINGS ) {
                    /* shouldn't happen, but be safe */
                    break;
                }
            }
        }
        return count;
    }
    return -1;
}


/* retrieve the command corresponding to a given (sym,mod) pair. returns SKIN_KEY_COMMAND_NONE if not found */
SkinKeyCommand
skin_keyset_get_command( SkinKeyset*  kset, int  sym, int mod )
{
    if (kset) {
        SkinKeyItem*  item = kset->items;
        SkinKeyItem*  end  = item + kset->num_items;

        for ( ; item < end; item++ ) {
            if (item->sym == sym && item->mod == mod) {
                return item->command;
            }
        }
    }
    return SKIN_KEY_COMMAND_NONE;
}


const char*
skin_keyset_get_default_text( void )
{
    return
    "BUTTON_CALL         F3\n"
    "BUTTON_HANGUP       F4\n"
    "BUTTON_HOME         Home\n"
    "BUTTON_BACK         Escape\n"
    "BUTTON_MENU         F2, PageUp\n"
    "BUTTON_STAR         Shift-F2, PageDown\n"
    "BUTTON_POWER        F7\n"
    "BUTTON_SEARCH       F5\n"
    "BUTTON_CAMERA       Ctrl-Keypad_5, Ctrl-F3\n"
    "BUTTON_VOLUME_UP    Keypad_Plus, Ctrl-F5\n"
    "BUTTON_VOLUME_DOWN  Keypad_Minus, Ctrl-F6\n"

    "TOGGLE_NETWORK      F8\n"
    "TOGGLE_TRACING      F9\n"
    "TOGGLE_FULLSCREEN   Alt-Enter\n"

    "BUTTON_DPAD_CENTER  Keypad_5\n"
    "BUTTON_DPAD_UP      Keypad_8\n"
    "BUTTON_DPAD_LEFT    Keypad_4\n"
    "BUTTON_DPAD_RIGHT   Keypad_6\n"
    "BUTTON_DPAD_DOWN    Keypad_2\n"

    "TOGGLE_TRACKBALL    F6\n"
    "SHOW_TRACKBALL      Delete\n"

    "CHANGE_LAYOUT_PREV  Keypad_7, Ctrl-F11\n"
    "CHANGE_LAYOUT_NEXT  Keypad_9, Ctrl-F12\n"
    "ONION_ALPHA_UP      Keypad_Multiply\n"
    "ONION_ALPHA_DOWN    Keypad_Divide\n"
    ;
}

static SkinKeyset* s_default = NULL;

SkinKeyset* skin_keyset_get_default(void) {
    return s_default;
}

void skin_keyset_set_default(SkinKeyset* set) {
    s_default = set;
}
