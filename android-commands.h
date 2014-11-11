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
    { NULL, NULL, },
};
