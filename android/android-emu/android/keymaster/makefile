INCLUDES=-Iinclude/ -I. -I../../../../../boringssl/include/
CPPFLAGS = $(INCLUDES) -std=c++11
libkeymasterx.a:aes_key.o aes_operation.o android_keymaster.o android_keymaster_messages.o android_keymaster_utils.o asymmetric_key.o asymmetric_key_factory.o ocb.o attestation_record.o auth_encrypted_key_blob.o authorization_set.o ec_key.o ec_key_factory.o ecdsa_operation.o hmac_key.o hmac_operation.o key.o keymaster_enforcement.o logger.o ocb_utils.o openssl_err.o openssl_utils.o operation.o operation_table.o rsa_key.o rsa_key_factory.o rsa_operation.o serializable.o symmetric_key.o keymaster_tags.o
	ar cr $@ $^
aes_operation.o:aes_operation.cpp
aes_key.o:aes_key.cpp
key.o:key.cpp
clean:
	rm -f *.o libkeymasterx.a
#	g++  -c aes_key.cpp $(CPPFLAGS) -o aes_key.o
