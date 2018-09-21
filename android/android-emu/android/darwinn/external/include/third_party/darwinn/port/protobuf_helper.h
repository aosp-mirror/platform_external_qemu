#ifndef THIRD_PARTY_DARWINN_PORT_PROTOBUF_HELPER_H_
#define THIRD_PARTY_DARWINN_PORT_PROTOBUF_HELPER_H_

#ifdef PROTOBUF_INTERNAL_IMPL

#include "net/proto2/io/public/coded_stream.h"
#include "net/proto2/io/public/zero_copy_stream_impl.h"
#include "net/proto2/public/repeated_field.h"
#include "net/proto2/public/text_format.h"
#include "net/proto2/util/public/message_differencer.h"

using ::proto2::RepeatedField;             // NOLINT
using ::proto2::RepeatedPtrField;          // NOLINT
using ::proto2::TextFormat;                // NOLINT
using ::proto2::io::CodedOutputStream;     // NOLINT
using ::proto2::io::FileOutputStream;      // NOLINT
using ::proto2::util::MessageDifferencer;  // NOLINT

#else

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/message_differencer.h>

using ::google::protobuf::RepeatedField;             // NOLINT
using ::google::protobuf::RepeatedPtrField;          // NOLINT
using ::google::protobuf::TextFormat;                // NOLINT
using ::google::protobuf::io::CodedOutputStream;     // NOLINT
using ::google::protobuf::io::FileOutputStream;      // NOLINT
using ::google::protobuf::util::MessageDifferencer;  // NOLINT

#endif

#endif  // THIRD_PARTY_DARWINN_PORT_PROTOBUF_HELPER_H_
