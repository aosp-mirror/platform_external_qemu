/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI/QMP commands
 *
 * Copyright IBM, Corp. 2011
 * Copyright (C) 2014-2018 Red Hat, Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#ifndef QAPI_COMMANDS_H
#define QAPI_COMMANDS_H

#include "qapi-commands-common.h"
#include "qapi-commands-sockets.h"
#include "qapi-commands-run-state.h"
#include "qapi-commands-crypto.h"
#include "qapi-commands-block.h"
#include "qapi-commands-char.h"
#include "qapi-commands-net.h"
#include "qapi-commands-rocker.h"
#include "qapi-commands-tpm.h"
#include "qapi-commands-ui.h"
#include "qapi-commands-migration.h"
#include "qapi-commands-transaction.h"
#include "qapi-commands-trace.h"
#include "qapi-commands-introspect.h"
#include "qapi-commands-misc.h"
#include "qapi-types.h"
#include "qapi/qmp/dispatch.h"

void qmp_init_marshal(QmpCommandList *cmds);

#endif /* QAPI_COMMANDS_H */
