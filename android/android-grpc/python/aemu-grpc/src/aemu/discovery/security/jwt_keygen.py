#  Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import base64
import io
import json
import logging
import os
import uuid
import time

import tink
from tink import jwt, tink_config
from tink.jwt import _jwt_format
from tink.proto import (
    jwt_ecdsa_pb2,
    jwt_rsa_ssa_pkcs1_pb2,
    jwt_rsa_ssa_pss_pb2,
    tink_pb2,
)

_ECDSA_PARAMS = {
    jwt_ecdsa_pb2.ES256: ("ES256", "P-256"),
    jwt_ecdsa_pb2.ES384: ("ES384", "P-384"),
    jwt_ecdsa_pb2.ES512: ("ES512", "P-521"),
}

_JWT_ECDSA_PUBLIC_KEY_TYPE = "type.googleapis.com/google.crypto.tink.JwtEcdsaPublicKey"


def _base64_encode(data):
    return base64.urlsafe_b64encode(data).rstrip(b"=").decode("utf-8")


def _convert_jwt_ecdsa_key(key):
    """Converts a JwtEcdsaPublicKey into a JWK."""
    ecdsa_public_key = jwt_ecdsa_pb2.JwtEcdsaPublicKey.FromString(key.key_data.value)
    if ecdsa_public_key.algorithm not in _ECDSA_PARAMS:
        raise tink.TinkError("unknown ecdsa algorithm")
    alg, crv = _ECDSA_PARAMS[ecdsa_public_key.algorithm]
    output = {
        "kty": "EC",
        "crv": crv,
        "x": _base64_encode(ecdsa_public_key.x),
        "y": _base64_encode(ecdsa_public_key.y),
        "use": "sig",
        "alg": alg,
        "key_ops": ["verify"],
    }
    kid = _jwt_format.get_kid(key.key_id, key.output_prefix_type)
    if kid:
        output["kid"] = kid
    elif ecdsa_public_key.HasField("custom_kid"):
        output["kid"] = ecdsa_public_key.custom_kid.value
    return output


def _kid_is_in_file(fname, kids):
    with open(fname, "r") as f:
        jwk = json.load(f)
        if "keys" in jwk:
            for entry in jwk["keys"]:
                if "kid" in entry and entry["kid"] in kids:
                    return True
    return False


class JwtKeygen(object):
    def __init__(self, key_path, active_path):
        try:
            tink_config.register()
            jwt.register_jwt_signature()
        except tink.TinkError as e:
            logging.exception("Error initialising Tink: %s", e)

        self.fname = os.path.join(key_path, "{}.jwk".format(uuid.uuid4()))
        self.sign = self._generate_signer_and_write(self.fname, active_path)


    def __del__(self):
        if os.path.exists(self.fname):
            os.remove(self.fname)

    def sign_and_encode(self, raw_jwt):
        return self.sign.sign_and_encode(raw_jwt)

    def _export_public_key(self, keyset_handle):
        output_stream = io.BytesIO()
        writer = tink.BinaryKeysetWriter(output_stream)
        keyset_handle.write_no_secret(writer)
        keyset = tink_pb2.Keyset.FromString(output_stream.getvalue())
        keys = []
        for key in keyset.key:
            if key.status != tink_pb2.ENABLED:
                continue
            if key.key_data.key_material_type != tink_pb2.KeyData.ASYMMETRIC_PUBLIC:
                raise tink.TinkError("wrong key material type")
            if key.output_prefix_type not in [tink_pb2.RAW, tink_pb2.TINK]:
                raise tink.TinkError("unsupported output prefix type")
            if key.key_data.type_url == _JWT_ECDSA_PUBLIC_KEY_TYPE:
                keys.append(_convert_jwt_ecdsa_key(key))
            else:
                raise tink.TinkError("unsupported key type: %s" % key.key_data.type_url)
        return json.dumps({"keys": keys}, separators=(",", ":"))

    def _generate_signer(self):
        """Generate a signer and corresponding json webkey snippet."""
        private_handle = tink.new_keyset_handle(jwt.jwt_es512_template())
        public_handle = private_handle.public_keyset_handle()
        output_jwk_set = self._export_public_key(public_handle)
        sign = private_handle.primitive(jwt.JwtPublicKeySign)
        return sign, output_jwk_set

    def _generate_signer_and_write(self, jwk_file, active_path):
        sign, output_jwk_set = self._generate_signer()

        with open(jwk_file, "w") as f:
            f.write(output_jwk_set)

        # Fetch our key ids.
        jwk_set = json.loads(output_jwk_set)
        our_kids = [x["kid"] for x in jwk_set["keys"]]

        # Now we need to wait until the emulator has consumed the jwks, this means
        # We will be able to read the jwk with all the public keys, which should contain
        # ours..

        # We will look for at most 1 second.
        for i in range(10):
            if _kid_is_in_file(active_path, our_kids):
                return sign
            time.sleep(0.1)

        raise "Our key was never written to the active loaded set."
