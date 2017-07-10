/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <keymaster/android_keymaster.h>

#include <assert.h>
#include <string.h>

#include <cstddef>

#include <openssl/rand.h>
#include <openssl/x509.h>

#include <UniquePtr.h>

#include <keymaster/android_keymaster_utils.h>
#include <keymaster/key_factory.h>
#include <keymaster/keymaster_context.h>

#include "ae.h"
#include "key.h"
#include "openssl_err.h"
#include "operation.h"
#include "operation_table.h"

namespace keymaster {

namespace {

const uint8_t MAJOR_VER = 1;
const uint8_t MINOR_VER = 1;
const uint8_t SUBMINOR_VER = 0;

keymaster_error_t CheckVersionInfo(const AuthorizationSet& tee_enforced,
                                   const AuthorizationSet& sw_enforced,
                                   const KeymasterContext& context) {
    uint32_t os_version;
    uint32_t os_patchlevel;
    context.GetSystemVersion(&os_version, &os_patchlevel);

    uint32_t key_os_patchlevel;
    if (tee_enforced.GetTagValue(TAG_OS_PATCHLEVEL, &key_os_patchlevel) ||
        sw_enforced.GetTagValue(TAG_OS_PATCHLEVEL, &key_os_patchlevel)) {
        if (key_os_patchlevel < os_patchlevel)
            return KM_ERROR_KEY_REQUIRES_UPGRADE;
        else if (key_os_patchlevel > os_patchlevel)
            return KM_ERROR_INVALID_KEY_BLOB;
    }

    return KM_ERROR_OK;
}

}  // anonymous namespace

AndroidKeymaster::AndroidKeymaster(KeymasterContext* context, size_t operation_table_size)
    : context_(context), operation_table_(new OperationTable(operation_table_size)) {}

AndroidKeymaster::~AndroidKeymaster() {}

struct AE_CTX_Delete {
    void operator()(ae_ctx* ctx) const { ae_free(ctx); }
};
typedef UniquePtr<ae_ctx, AE_CTX_Delete> Unique_ae_ctx;

// TODO(swillden): Unify support analysis.  Right now, we have per-keytype methods that determine if
// specific modes, padding, etc. are supported for that key type, and AndroidKeymaster also has
// methods that return the same information.  They'll get out of sync.  Best to put the knowledge in
// the keytypes and provide some mechanism for AndroidKeymaster to query the keytypes for the
// information.

template <typename T>
bool check_supported(const KeymasterContext& context, keymaster_algorithm_t algorithm,
                     SupportedResponse<T>* response) {
    if (context.GetKeyFactory(algorithm) == NULL) {
        response->error = KM_ERROR_UNSUPPORTED_ALGORITHM;
        return false;
    }
    return true;
}

void AndroidKeymaster::GetVersion(const GetVersionRequest&, GetVersionResponse* rsp) {
    if (rsp == NULL)
        return;

    rsp->major_ver = MAJOR_VER;
    rsp->minor_ver = MINOR_VER;
    rsp->subminor_ver = SUBMINOR_VER;
    rsp->error = KM_ERROR_OK;
}

void AndroidKeymaster::SupportedAlgorithms(const SupportedAlgorithmsRequest& /* request */,
                                           SupportedAlgorithmsResponse* response) {
    if (response == NULL)
        return;

    response->error = KM_ERROR_OK;

    size_t algorithm_count = 0;
    const keymaster_algorithm_t* algorithms = context_->GetSupportedAlgorithms(&algorithm_count);
    if (algorithm_count == 0)
        return;
    response->results_length = algorithm_count;
    response->results = dup_array(algorithms, algorithm_count);
    if (!response->results)
        response->error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
}

template <typename T>
void GetSupported(const KeymasterContext& context, keymaster_algorithm_t algorithm,
                  keymaster_purpose_t purpose,
                  const T* (OperationFactory::*get_supported_method)(size_t* count) const,
                  SupportedResponse<T>* response) {
    if (response == NULL || !check_supported(context, algorithm, response))
        return;

    const OperationFactory* factory = context.GetOperationFactory(algorithm, purpose);
    if (!factory) {
        response->error = KM_ERROR_UNSUPPORTED_PURPOSE;
        return;
    }

    size_t count;
    const T* supported = (factory->*get_supported_method)(&count);
    response->SetResults(supported, count);
}

void AndroidKeymaster::SupportedBlockModes(const SupportedBlockModesRequest& request,
                                           SupportedBlockModesResponse* response) {
    GetSupported(*context_, request.algorithm, request.purpose,
                 &OperationFactory::SupportedBlockModes, response);
}

void AndroidKeymaster::SupportedPaddingModes(const SupportedPaddingModesRequest& request,
                                             SupportedPaddingModesResponse* response) {
    GetSupported(*context_, request.algorithm, request.purpose,
                 &OperationFactory::SupportedPaddingModes, response);
}

void AndroidKeymaster::SupportedDigests(const SupportedDigestsRequest& request,
                                        SupportedDigestsResponse* response) {
    GetSupported(*context_, request.algorithm, request.purpose, &OperationFactory::SupportedDigests,
                 response);
}

void AndroidKeymaster::SupportedImportFormats(const SupportedImportFormatsRequest& request,
                                              SupportedImportFormatsResponse* response) {
    if (response == NULL || !check_supported(*context_, request.algorithm, response))
        return;

    size_t count;
    const keymaster_key_format_t* formats =
        context_->GetKeyFactory(request.algorithm)->SupportedImportFormats(&count);
    response->SetResults(formats, count);
}

void AndroidKeymaster::SupportedExportFormats(const SupportedExportFormatsRequest& request,
                                              SupportedExportFormatsResponse* response) {
    if (response == NULL || !check_supported(*context_, request.algorithm, response))
        return;

    size_t count;
    const keymaster_key_format_t* formats =
        context_->GetKeyFactory(request.algorithm)->SupportedExportFormats(&count);
    response->SetResults(formats, count);
}

void AndroidKeymaster::AddRngEntropy(const AddEntropyRequest& request,
                                     AddEntropyResponse* response) {
    response->error = context_->AddRngEntropy(request.random_data.peek_read(),
                                              request.random_data.available_read());
}

void AndroidKeymaster::GenerateKey(const GenerateKeyRequest& request,
                                   GenerateKeyResponse* response) {
    if (response == NULL)
        return;

    keymaster_algorithm_t algorithm;
    KeyFactory* factory = 0;
    UniquePtr<Key> key;
    if (!request.key_description.GetTagValue(TAG_ALGORITHM, &algorithm) ||
        !(factory = context_->GetKeyFactory(algorithm)))
        response->error = KM_ERROR_UNSUPPORTED_ALGORITHM;
    else {
        KeymasterKeyBlob key_blob;
        response->enforced.Clear();
        response->unenforced.Clear();
        response->error = factory->GenerateKey(request.key_description, &key_blob,
                                               &response->enforced, &response->unenforced);
        if (response->error == KM_ERROR_OK)
            response->key_blob = key_blob.release();
    }
}

void AndroidKeymaster::GetKeyCharacteristics(const GetKeyCharacteristicsRequest& request,
                                             GetKeyCharacteristicsResponse* response) {
    if (response == NULL)
        return;

    KeymasterKeyBlob key_material;
    response->error =
        context_->ParseKeyBlob(KeymasterKeyBlob(request.key_blob), request.additional_params,
                               &key_material, &response->enforced, &response->unenforced);
    if (response->error != KM_ERROR_OK)
        return;

    response->error = CheckVersionInfo(response->enforced, response->unenforced, *context_);
}

static KeyFactory* GetKeyFactory(const KeymasterContext& context,
                                 const AuthorizationSet& hw_enforced,
                                 const AuthorizationSet& sw_enforced,
                                 keymaster_algorithm_t* algorithm, keymaster_error_t* error) {
    *error = KM_ERROR_UNSUPPORTED_ALGORITHM;
    if (!hw_enforced.GetTagValue(TAG_ALGORITHM, algorithm) &&
        !sw_enforced.GetTagValue(TAG_ALGORITHM, algorithm))
        return nullptr;
    KeyFactory* factory = context.GetKeyFactory(*algorithm);
    if (factory)
        *error = KM_ERROR_OK;
    return factory;
}

void AndroidKeymaster::BeginOperation(const BeginOperationRequest& request,
                                      BeginOperationResponse* response) {
    if (response == NULL)
        return;
    response->op_handle = 0;

    AuthorizationSet hw_enforced;
    AuthorizationSet sw_enforced;
    const KeyFactory* key_factory;
    UniquePtr<Key> key;
    response->error = LoadKey(request.key_blob, request.additional_params, &hw_enforced,
                              &sw_enforced, &key_factory, &key);
    if (response->error != KM_ERROR_OK)
        return;

    response->error = KM_ERROR_UNKNOWN_ERROR;
    keymaster_algorithm_t key_algorithm;
    if (!key->authorizations().GetTagValue(TAG_ALGORITHM, &key_algorithm))
        return;

    response->error = KM_ERROR_UNSUPPORTED_PURPOSE;
    OperationFactory* factory = key_factory->GetOperationFactory(request.purpose);
    if (!factory)
        return;

    UniquePtr<Operation> operation(
        factory->CreateOperation(*key, request.additional_params, &response->error));
    if (operation.get() == NULL)
        return;

    if (context_->enforcement_policy()) {
        km_id_t key_id;
        response->error = KM_ERROR_UNKNOWN_ERROR;
        if (!context_->enforcement_policy()->CreateKeyId(request.key_blob, &key_id))
            return;
        operation->set_key_id(key_id);
        response->error = context_->enforcement_policy()->AuthorizeOperation(
            request.purpose, key_id, key->authorizations(), request.additional_params,
            0 /* op_handle */, true /* is_begin_operation */);
        if (response->error != KM_ERROR_OK)
            return;
    }

    response->output_params.Clear();
    response->error = operation->Begin(request.additional_params, &response->output_params);
    if (response->error != KM_ERROR_OK)
        return;

    operation->SetAuthorizations(key->authorizations());
    response->error = operation_table_->Add(operation.release(), &response->op_handle);
}

void AndroidKeymaster::UpdateOperation(const UpdateOperationRequest& request,
                                       UpdateOperationResponse* response) {
    if (response == NULL)
        return;

    response->error = KM_ERROR_INVALID_OPERATION_HANDLE;
    Operation* operation = operation_table_->Find(request.op_handle);
    if (operation == NULL)
        return;

    if (context_->enforcement_policy()) {
        response->error = context_->enforcement_policy()->AuthorizeOperation(
            operation->purpose(), operation->key_id(), operation->authorizations(),
            request.additional_params, request.op_handle, false /* is_begin_operation */);
        if (response->error != KM_ERROR_OK) {
            operation_table_->Delete(request.op_handle);
            return;
        }
    }

    response->error =
        operation->Update(request.additional_params, request.input, &response->output_params,
                          &response->output, &response->input_consumed);
    if (response->error != KM_ERROR_OK) {
        // Any error invalidates the operation.
        operation_table_->Delete(request.op_handle);
    }
}

void AndroidKeymaster::FinishOperation(const FinishOperationRequest& request,
                                       FinishOperationResponse* response) {
    if (response == NULL)
        return;

    response->error = KM_ERROR_INVALID_OPERATION_HANDLE;
    Operation* operation = operation_table_->Find(request.op_handle);
    if (operation == NULL)
        return;

    if (context_->enforcement_policy()) {
        response->error = context_->enforcement_policy()->AuthorizeOperation(
            operation->purpose(), operation->key_id(), operation->authorizations(),
            request.additional_params, request.op_handle, false /* is_begin_operation */);
        if (response->error != KM_ERROR_OK) {
            operation_table_->Delete(request.op_handle);
            return;
        }
    }

    response->error = operation->Finish(request.additional_params, request.input, request.signature,
                                        &response->output_params, &response->output);
    operation_table_->Delete(request.op_handle);
}

void AndroidKeymaster::AbortOperation(const AbortOperationRequest& request,
                                      AbortOperationResponse* response) {
    if (!response)
        return;

    Operation* operation = operation_table_->Find(request.op_handle);
    if (!operation) {
        response->error = KM_ERROR_INVALID_OPERATION_HANDLE;
        return;
    }

    response->error = operation->Abort();
    operation_table_->Delete(request.op_handle);
}

void AndroidKeymaster::ExportKey(const ExportKeyRequest& request, ExportKeyResponse* response) {
    if (response == NULL)
        return;

    AuthorizationSet hw_enforced;
    AuthorizationSet sw_enforced;
    KeymasterKeyBlob key_material;
    response->error =
        context_->ParseKeyBlob(KeymasterKeyBlob(request.key_blob), request.additional_params,
                               &key_material, &hw_enforced, &sw_enforced);
    if (response->error != KM_ERROR_OK)
        return;

    keymaster_algorithm_t algorithm;
    KeyFactory* key_factory =
        GetKeyFactory(*context_, hw_enforced, sw_enforced, &algorithm, &response->error);
    if (!key_factory)
        return;

    UniquePtr<Key> key;
    response->error = key_factory->LoadKey(key_material, request.additional_params, hw_enforced,
                                           sw_enforced, &key);
    if (response->error != KM_ERROR_OK)
        return;

    UniquePtr<uint8_t[]> out_key;
    size_t size;
    response->error = key->formatted_key_material(request.key_format, &out_key, &size);
    if (response->error == KM_ERROR_OK) {
        response->key_data = out_key.release();
        response->key_data_length = size;
    }
}

void AndroidKeymaster::AttestKey(const AttestKeyRequest& request, AttestKeyResponse* response) {
    if (!response)
        return;

    AuthorizationSet tee_enforced;
    AuthorizationSet sw_enforced;
    const KeyFactory* key_factory;
    UniquePtr<Key> key;
    response->error = LoadKey(request.key_blob, request.attest_params, &tee_enforced, &sw_enforced,
                              &key_factory, &key);
    if (response->error != KM_ERROR_OK)
        return;

    response->error = key->GenerateAttestation(*context_, request.attest_params, tee_enforced,
                                               sw_enforced, &response->certificate_chain);
}

void AndroidKeymaster::UpgradeKey(const UpgradeKeyRequest& request, UpgradeKeyResponse* response) {
    if (!response)
        return;

    KeymasterKeyBlob upgraded_key;
    response->error = context_->UpgradeKeyBlob(KeymasterKeyBlob(request.key_blob),
                                               request.upgrade_params, &upgraded_key);
    if (response->error != KM_ERROR_OK)
        return;
    response->upgraded_key = upgraded_key.release();
}

void AndroidKeymaster::ImportKey(const ImportKeyRequest& request, ImportKeyResponse* response) {
    if (response == NULL)
        return;

    keymaster_algorithm_t algorithm;
    KeyFactory* factory = 0;
    UniquePtr<Key> key;
    if (!request.key_description.GetTagValue(TAG_ALGORITHM, &algorithm) ||
        !(factory = context_->GetKeyFactory(algorithm)))
        response->error = KM_ERROR_UNSUPPORTED_ALGORITHM;
    else {
        keymaster_key_blob_t key_material = {request.key_data, request.key_data_length};
        KeymasterKeyBlob key_blob;
        response->error = factory->ImportKey(request.key_description, request.key_format,
                                             KeymasterKeyBlob(key_material), &key_blob,
                                             &response->enforced, &response->unenforced);
        if (response->error == KM_ERROR_OK)
            response->key_blob = key_blob.release();
    }
}

void AndroidKeymaster::DeleteKey(const DeleteKeyRequest& request, DeleteKeyResponse* response) {
    if (!response)
        return;
    response->error = context_->DeleteKey(KeymasterKeyBlob(request.key_blob));
}

void AndroidKeymaster::DeleteAllKeys(const DeleteAllKeysRequest&, DeleteAllKeysResponse* response) {
    if (!response)
        return;
    response->error = context_->DeleteAllKeys();
}

void AndroidKeymaster::Configure(const ConfigureRequest& request, ConfigureResponse* response) {
    if (!response)
        return;
    response->error = context_->SetSystemVersion(request.os_version, request.os_patchlevel);
}

bool AndroidKeymaster::has_operation(keymaster_operation_handle_t op_handle) const {
    return operation_table_->Find(op_handle) != nullptr;
}

keymaster_error_t AndroidKeymaster::LoadKey(const keymaster_key_blob_t& key_blob,
                                            const AuthorizationSet& additional_params,
                                            AuthorizationSet* hw_enforced,
                                            AuthorizationSet* sw_enforced,
                                            const KeyFactory** factory, UniquePtr<Key>* key) {
    KeymasterKeyBlob key_material;
    keymaster_error_t error = context_->ParseKeyBlob(KeymasterKeyBlob(key_blob), additional_params,
                                                     &key_material, hw_enforced, sw_enforced);
    if (error != KM_ERROR_OK)
        return error;

    error = CheckVersionInfo(*hw_enforced, *sw_enforced, *context_);
    if (error != KM_ERROR_OK)
        return error;

    keymaster_algorithm_t algorithm;
    *factory = GetKeyFactory(*context_, *hw_enforced, *sw_enforced, &algorithm, &error);
    if (error != KM_ERROR_OK)
        return error;

    return (*factory)->LoadKey(key_material, additional_params, *hw_enforced, *sw_enforced, key);
}

}  // namespace keymaster
