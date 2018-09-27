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

#ifndef QAPI_COMMANDS_TPM_H
#define QAPI_COMMANDS_TPM_H

#include "qapi-types-tpm.h"
#include "qapi/qmp/dispatch.h"

TpmModelList *qmp_query_tpm_models(Error **errp);
void qmp_marshal_query_tpm_models(QDict *args, QObject **ret, Error **errp);
TpmTypeList *qmp_query_tpm_types(Error **errp);
void qmp_marshal_query_tpm_types(QDict *args, QObject **ret, Error **errp);
TPMInfoList *qmp_query_tpm(Error **errp);
void qmp_marshal_query_tpm(QDict *args, QObject **ret, Error **errp);

#endif /* QAPI_COMMANDS_TPM_H */
