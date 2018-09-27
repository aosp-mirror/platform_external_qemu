/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * QAPI/QMP schema introspection
 *
 * Copyright (C) 2015-2018 Red Hat, Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "test-qapi-introspect.h"

const QLitObject test_qmp_schema_qlit = QLIT_QLIST(((QLitObject[]) {
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("user_def_cmd0") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("1") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("user_def_cmd") },
        { "ret-type", QLIT_QSTR("1") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("2") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("user_def_cmd1") },
        { "ret-type", QLIT_QSTR("1") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("3") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("user_def_cmd2") },
        { "ret-type", QLIT_QSTR("4") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("5") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("guest-get-time") },
        { "ret-type", QLIT_QSTR("int") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("6") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("guest-sync") },
        { "ret-type", QLIT_QSTR("any") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("7") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("boxed-struct") },
        { "ret-type", QLIT_QSTR("1") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("8") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("boxed-union") },
        { "ret-type", QLIT_QSTR("1") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(true) },
        { "arg-type", QLIT_QSTR("1") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("an-oob-command") },
        { "ret-type", QLIT_QSTR("1") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("1") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("EVENT_A") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("1") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("EVENT_B") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("9") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("EVENT_C") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("10") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("EVENT_D") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("7") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("EVENT_E") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("11") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("EVENT_F") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("12") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("__ORG.QEMU_X-EVENT") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("13") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("__org.qemu_x-command") },
        { "ret-type", QLIT_QSTR("14") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("1") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("ud1a") },
                { "type", QLIT_QSTR("15") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("2") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("ud1a") },
                { "type", QLIT_QSTR("15") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ud1b") },
                { "type", QLIT_QSTR("15") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("3") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string0") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("dict1") },
                { "type", QLIT_QSTR("16") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("4") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("a") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("b") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("5") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("int") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("int") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arg") },
                { "type", QLIT_QSTR("any") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("6") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("value") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("any") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("integer") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("7") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("17") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("8") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("integer") },
                { "type", QLIT_QSTR("18") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("s8") },
                { "type", QLIT_QSTR("19") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("s16") },
                { "type", QLIT_QSTR("20") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("s32") },
                { "type", QLIT_QSTR("21") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("s64") },
                { "type", QLIT_QSTR("22") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("u8") },
                { "type", QLIT_QSTR("23") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("u16") },
                { "type", QLIT_QSTR("24") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("u32") },
                { "type", QLIT_QSTR("25") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("u64") },
                { "type", QLIT_QSTR("26") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("number") },
                { "type", QLIT_QSTR("27") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("boolean") },
                { "type", QLIT_QSTR("28") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("string") },
                { "type", QLIT_QSTR("29") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("sizes") },
                { "type", QLIT_QSTR("30") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("any") },
                { "type", QLIT_QSTR("31") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("a") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("b") },
                { "type", QLIT_QSTR("15") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("c") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("9") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("a") },
                { "type", QLIT_QSTR("32") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("b") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("c") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("enum3") },
                { "type", QLIT_QSTR("33") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("10") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("34") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("33") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("null") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("alternate") },
        { "name", QLIT_QSTR("11") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("__org.qemu_x-member1") },
                { "type", QLIT_QSTR("35") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("__org.qemu_x-member2") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("wchar-t") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("12") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("a") },
                { "type", QLIT_QSTR("[35]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("b") },
                { "type", QLIT_QSTR("[12]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("c") },
                { "type", QLIT_QSTR("36") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("d") },
                { "type", QLIT_QSTR("37") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("13") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("38") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("14") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("__org.qemu_x-branch") },
                { "type", QLIT_QSTR("39") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("integer") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("enum1") },
                { "type", QLIT_QSTR("33") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("15") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("string") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("str") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string1") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("dict2") },
                { "type", QLIT_QSTR("40") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("dict3") },
                { "type", QLIT_QSTR("40") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("16") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("17") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("integer"),
            QLIT_QSTR("s8"),
            QLIT_QSTR("s16"),
            QLIT_QSTR("s32"),
            QLIT_QSTR("s64"),
            QLIT_QSTR("u8"),
            QLIT_QSTR("u16"),
            QLIT_QSTR("u32"),
            QLIT_QSTR("u64"),
            QLIT_QSTR("number"),
            QLIT_QSTR("boolean"),
            QLIT_QSTR("string"),
            QLIT_QSTR("sizes"),
            QLIT_QSTR("any"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("18") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("19") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("20") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("21") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("22") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("23") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("24") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("25") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("26") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[number]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("27") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[bool]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("28") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[str]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("29") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("30") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("[any]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("31") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("struct1") },
                { "type", QLIT_QSTR("15") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("enum2") },
                { "type", QLIT_QSTR("33") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("32") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("33") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("value1"),
            QLIT_QSTR("value2"),
            QLIT_QSTR("value3"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("integer") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("enum1") },
                { "type", QLIT_QSTR("33") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("34") },
        { "tag", QLIT_QSTR("enum1") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("value1") },
                { "type", QLIT_QSTR("41") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("value2") },
                { "type", QLIT_QSTR("42") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("value3") },
                { "type", QLIT_QSTR("42") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("null") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("null") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("35") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("__org.qemu_x-value"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("35") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[35]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("12") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[12]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("__org.qemu_x-member1") },
                { "type", QLIT_QSTR("35") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("36") },
        { "tag", QLIT_QSTR("__org.qemu_x-member1") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("__org.qemu_x-value") },
                { "type", QLIT_QSTR("43") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("44") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("alternate") },
        { "name", QLIT_QSTR("37") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("38") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("__org.qemu_x-branch"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("39") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("userdef") },
                { "type", QLIT_QSTR("15") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("40") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("int") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[int]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("number") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[number]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("number") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("number") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("bool") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[bool]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("boolean") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("bool") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("str") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[str]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("any") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[any]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("boolean") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("a_b") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("41") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("intb") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("a-b") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("42") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("array") },
                { "type", QLIT_QSTR("[14]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("43") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("__org.qemu_x-member1") },
                { "type", QLIT_QSTR("35") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("44") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("14") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[14]") },
        {}
    })),
    {}
}));
/* Dummy declaration to prevent empty .o file */
char dummy_test_qapi_introspect_c;
