/* hand-written for now; consider .hx autogen */

static mon_cmd_t android_redir_cmds[] = {
    {
        .name = "list",
        .args_type = "",
        .params = "",
        .help = "list current redirections",
        .mhandler.cmd = android_console_redir_list,
    },
    {
        .name = "add",
        .args_type = "arg:s",
        .params = "[tcp|udp]:hostport:guestport",
        .help = "add new redirection",
        .mhandler.cmd = android_console_redir_add,
    },
    {
        .name = "del",
        .args_type = "arg:s",
        .params = "[tcp|udp]:hostport",
        .help = "remove existing redirection",
        .mhandler.cmd = android_console_redir_del,
    },
    { NULL, NULL, },
};

static mon_cmd_t android_power_cmds[] = {
    {
        .name = "display",
        .args_type = "",
        .params = "",
        .help = "display battery and charger state",
        .mhandler.cmd = android_console_power_display,
    },
    {
        .name = "ac",
        .args_type = "arg:s?",
        .params = "",
        .help = "set AC charging state",
        .mhandler.cmd = android_console_power_ac,
    },
    {
        .name = "status",
        .args_type = "arg:s?",
        .params = "",
        .help = "set battery status",
        .mhandler.cmd = android_console_power_status,
    },
    {
        .name = "present",
        .args_type = "arg:s?",
        .params = "",
        .help = "set battery present state",
        .mhandler.cmd = android_console_power_present,
    },
    {
        .name = "health",
        .args_type = "arg:s?",
        .params = "",
        .help = "set battery health state",
        .mhandler.cmd = android_console_power_health,
    },
    {
        .name = "capacity",
        .args_type = "arg:s?",
        .params = "",
        .help = "set battery capacity state",
        .mhandler.cmd = android_console_power_capacity,
    },
    { NULL, NULL, },
};

static mon_cmd_t android_event_cmds[] = {
    {
        .name = "types",
        .args_type = "arg:s?",
        .params = "",
        .help = "list all <type> aliases",
        .mhandler.cmd = android_console_event_types,
    },
    {
        .name = "codes",
        .args_type = "arg:s?",
        .params = "",
        .help = "list all <code> aliases for a given <type>",
        .mhandler.cmd = android_console_event_codes,
    },
    {
        .name = "send",
        .args_type  = "arg:s?",
        .params = "",
        .help = "send a series of events to the kernel",
        .mhandler.cmd = android_console_event_send,
    },
    {
        .name = "text",
        .args_type  = "arg:S?",
        .params = "",
        .help = "simulate keystrokes from a given text",
        .mhandler.cmd = android_console_event_text,
    },
    { NULL, NULL, },
};

static mon_cmd_t android_avd_snapshot_cmds[] = {
    {
        .name = "list",
        .args_type = "",
        .params = "",
        .help = "'avd snapshot list' will show a list of all state snapshots "
                "that can be loaded",
        .mhandler.cmd = android_console_avd_snapshot_list,
    },
    {
        .name = "save",
        .args_type = "arg:s?",
        .params = "",
        .help = "'avd snapshot save <name>' will save the current (run-time) "
                "state to a snapshot with the given name",
        .mhandler.cmd = android_console_avd_snapshot_save,
    },
    {
        .name = "load",
        .args_type = "arg:s?",
        .params = "",
        .help = "'avd snapshot load <name>' will load the state snapshot of "
                "the given name",
        .mhandler.cmd = android_console_avd_snapshot_load,
    },
    {
        .name = "del",
        .args_type = "arg:s?",
        .params = "",
        .help = "'avd snapshot del <name>' will delete the state snapshot with "
                "the given name",
        .mhandler.cmd = android_console_avd_snapshot_del,
    },
    { NULL, NULL, },
};

static mon_cmd_t android_avd_cmds[] = {
    {
        .name = "stop",
        .args_type = "",
        .params = "",
        .help = "stop the virtual device",
        .mhandler.cmd = android_console_avd_stop,
    },
    {
        .name = "start",
        .args_type = "",
        .params = "",
        .help = "start/restart the virtual device",
        .mhandler.cmd = android_console_avd_start,
    },
    {
        .name = "status",
        .args_type = "",
        .params = "",
        .help = "query virtual device status",
        .mhandler.cmd = android_console_avd_status,
    },
    {
        .name = "name",
        .args_type = "",
        .params = "",
        .help = "query virtual device name",
        .mhandler.cmd = android_console_avd_name,
    },
    {
        .name = "snapshot",
        .args_type = "item:s",
        .params = "",
        .help = "state snapshot commands",
        .mhandler.cmd = android_console_avd_snapshot,
        .sub_cmds.static_table = android_avd_snapshot_cmds,
    },
    { NULL, NULL, },
};

static mon_cmd_t android_finger_cmds[] = {
    {
        .name = "touch",
        .args_type = "arg:s?",
        .params = "",
        .help = "touch fingerprint sensor with <fingerid>",
        .mhandler.cmd = android_console_finger_touch,
    },
    {
        .name = "remove",
        .args_type = "",
        .params = "",
        .help = "remove finger from the fingerprint sensor",
        .mhandler.cmd = android_console_finger_remove,
    },
    { NULL, NULL, },
};

static mon_cmd_t android_geo_cmds[] = {
    {
        .name = "nmea",
        .args_type = "arg:s?",
        .params = "",
        .help = "send a GPS NMEA sentence\n"
            "'geo nema <sentence>' sends an NMEA 0183 sentence to the emulated device, as\n"
            "if it came from an emulated GPS modem. <sentence> must begin with '$GP'. Only\n"
            "'$GPGGA' and '$GPRCM' sentences are supported at the moment.\n",
        .mhandler.cmd = android_console_geo_nmea,
    },
    {
        .name = "fix",
        .args_type = "arg:S?",
        .params = "",
        .help = "send a simple GPS fix\n"
            "'geo fix <longitude> <latitude> [<altitude> [<satellites>]]'\n"
            " allows you to send a simple GPS fix to the emulated system.\n"
            " The parameters are:\n\n"
            "  <longitude>   longitude, in decimal degrees\n"
            "  <latitude>    latitude, in decimal degrees\n"
            "  <altitude>    optional altitude in meters\n"
            "  <satellites>  number of satellites being tracked (1-12)\n"
            "\n",
        .mhandler.cmd = android_console_geo_fix,
    },
    { NULL, NULL, },
};

static mon_cmd_t android_sms_cmds[] = {
    {
        .name = "send",
        .args_type = "arg:S?",
        .params = "",
        .help = "send inbound SMS text message\n"
            "'sms send <phonenumber> <message>' allows you to simulate a new inbound sms message\n",
        .mhandler.cmd = android_console_sms_send,
    },
    {
        .name = "pdu",
        .args_type = "arg:s?",
        .params = "",
        .help = "send inbound SMS PDU\n"
            "'sms pdu <hexstring>' allows you to simulate a new inbound sms PDU\n"
             "(used internally when one emulator sends SMS messages to another instance).\n"
             "you probably don't want to play with this at all\n"
             "\n",
        .mhandler.cmd = android_console_sms_pdu,
    },
    { NULL, NULL, },
};

static mon_cmd_t android_cmds[] = {
    {
        .name = "help|h|?",
        .args_type = "helptext:S?",
        .params = "",
        .help = "print a list of commands",
        .mhandler.cmd = android_console_help,
    },
    {
        .name = "kill",
        .args_type = "",
        .params = "",
        .help = "kill the emulator instance",
        .mhandler.cmd = android_console_kill,
    },
    {
        .name = "quit|exit",
        .args_type = "",
        .params = "",
        .help = "quit control session",
        .mhandler.cmd = android_console_quit,
    },
    {
        .name = "redir",
        .args_type = "item:s?",
        .params = "",
        .help = "manage port redirections",
        .mhandler.cmd = android_console_redir,
        .sub_cmds.static_table = android_redir_cmds,
    },
    {   .name = "power",
        .args_type = "item:s?",
        .params = "",
        .help = "power related commands",
        .mhandler.cmd = android_console_power,
        .sub_cmds.static_table = android_power_cmds,
    },
    {   .name = "event",
        .args_type = "item:s?",
        .params = "",
        .help = "simulate hardware events",
        .mhandler.cmd = android_console_event,
        .sub_cmds.static_table = android_event_cmds,
    },
    {   .name = "avd",
        .args_type = "item:s?",
        .params = "",
        .help = "control virtual device execution",
        .mhandler.cmd = android_console_avd,
        .sub_cmds.static_table = android_avd_cmds,
    },
    {   .name = "finger",
        .args_type = "item:s?",
        .params = "",
        .help = "manage emulator fingerprint",
        .mhandler.cmd = android_console_finger,
        .sub_cmds.static_table = android_finger_cmds,
    },
    {   .name = "geo",
        .args_type = "item:s?",
        .params = "",
        .help = "Geo-location commands",
        .mhandler.cmd = android_console_geo,
        .sub_cmds.static_table = android_geo_cmds,
    },
    {   .name = "sms",
        .args_type = "item:s?",
        .params = "",
        .help = "SMS related commands",
        .mhandler.cmd = android_console_sms,
        .sub_cmds.static_table = android_sms_cmds,
    },
    { NULL, NULL, },
};
