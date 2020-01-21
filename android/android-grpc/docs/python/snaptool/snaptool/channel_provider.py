"""
A Channel Provider can provide a gRPC channel to an emulator running on a given port, using the proper
certs when needed.
"""
import os

import grpc


def getEmulatorChannel(dest):
    """Opens up a grpc an (in)secure channel to the emulator on the given dest.
    dest should be in the form <host>:<port>. For example:

    localhost:8556

    If a cert is available in ~/.android then
    this expects that the emulator is using a self signed cert:
    made as follows:

    openssl req -x509 -out localhost.crt -keyout localhost.key \
        -newkey rsa:2048 -nodes -sha256 \
        -subj '/CN=localhost' -extensions EXT -config <( \
       printf "[dn]\nCN=localhost\n[req]\ndistinguished_name = dn\n[EXT]\nsubjectAltName=DNS:localhost\nkeyUsage=digitalSignature\nextendedKeyUsage=serverAuth")
    """

    cert = os.path.join(os.path.expanduser("~"), ".android", "emulator-grpc.cer")
    if os.path.exists(cert):
        with open(cert, "rb") as f:
            creds = grpc.ssl_channel_credentials(f.read())
            channel = grpc.secure_channel(dest, creds)
    else:
        # Let's go insecure..
        channel = grpc.insecure_channel(dest)

    return channel
