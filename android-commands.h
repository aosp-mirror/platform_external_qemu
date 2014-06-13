/* hand-written for now; consider .hx autogen */

static mon_cmd_t android_cmds[] = {
    {
        .name = "help|h|?",
        .args_type = "name:S?",
        .params = "",
        .help = "print a list of commands",
        .mhandler.cmd = do_help_cmd,
    },
    {
        .name = "kill",
        .args_type = "",
        .params = "",
        .help = "kill the emulator instance",
        .mhandler.cmd = android_console_kill,
    },
    { NULL, NULL, },
};
