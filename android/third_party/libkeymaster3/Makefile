#####
# Local unit test Makefile
#
# This makefile builds and runs the keymaster unit tests locally on the development
# machine, not on an Android device.  Android.mk builds the same tests into the
# "keymaster_tests" binary for execution on-device, but this Makefile runs them locally,
# for a very fast edit/build/test development cycle.
#
# To build and run these tests, one pre-requisite must be manually installed: BoringSSL.
# This Makefile expects to find BoringSSL in a directory adjacent to $ANDROID_BUILD_TOP.
# To get and build it, first install the Ninja build tool (e.g. apt-get install
# ninja-build), then do:
#
# cd $ANDROID_BUILD_TOP/..
# git clone https://boringssl.googlesource.com/boringssl
# cd boringssl
# mdkir build
# cd build
# cmake -GNinja ..
# ninja
#
# Then return to $ANDROID_BUILD_TOP/system/keymaster and run "make".
#####

BASE=../..
SUBS=system/core \
	hardware/libhardware \
	external/gtest \
	system/security/softkeymaster \
	system/security/keystore
GTEST=$(BASE)/external/googletest/googletest

INCLUDES=$(foreach dir,$(SUBS),-I $(BASE)/$(dir)/include) \
	-I $(BASE)/libnativehelper/include/nativehelper \
	-I $(GTEST)/include -isystem $(GTEST) -Iinclude -I$(BASE)/../boringssl/include

ifdef FORCE_32_BIT
ARCH_FLAGS = -m32
endif

ifdef USE_CLANG
CC=/usr/bin/clang
CXX=/usr/bin/clang
CXXFLAGS +=-std=c++11 -DKEYMASTER_CLANG_TEST_BUILD
CFLAGS += -DKEYMASTER_CLANG_TEST_BUILD
else
CXXFLAGS +=-std=c++0x -fprofile-arcs -ftest-coverage
CFLAGS += -fprofile-arcs -ftest-coverage
endif

LDFLAGS += $(ARCH_FLAGS)
CPPFLAGS = $(INCLUDES) -g -O0 -MD -MP $(ARCH_FLAGS) -DKEYMASTER_UNIT_TEST_BUILD -DHOST_BUILD
CXXFLAGS += -Wall -Werror -Wno-unused -Winit-self -Wpointer-arith -Wunused-parameter \
	-Werror=sign-compare -Werror=return-type -fno-permissive \
	-Wno-deprecated-declarations -fno-exceptions -DKEYMASTER_NAME_TAGS $(ARCH_FLAGS)
CFLAGS += $(ARCH_FLAGS) -DKEYMASTER_UNIT_TEST_BUILD -DHOST_BUILD

# Uncomment to enable debug logging.
# CXXFLAGS += -DDEBUG

LDLIBS=-L$(BASE)/../boringssl/build/crypto -lcrypto -lpthread -lstdc++ -lgcov

CPPSRCS=\
	aes_key.cpp \
	aes_operation.cpp \
	android_keymaster.cpp \
	android_keymaster_messages.cpp \
	android_keymaster_messages_test.cpp \
	android_keymaster_test.cpp \
	android_keymaster_test_utils.cpp \
	android_keymaster_utils.cpp \
	asymmetric_key.cpp \
	asymmetric_key_factory.cpp \
	attestation_record.cpp \
	attestation_record_test.cpp \
	auth_encrypted_key_blob.cpp \
	authorization_set.cpp \
	authorization_set_test.cpp \
	ec_key.cpp \
	ec_key_factory.cpp \
	ec_keymaster0_key.cpp \
	ec_keymaster1_key.cpp \
	ecdsa_keymaster1_operation.cpp \
	ecdsa_operation.cpp \
	ecies_kem.cpp \
	ecies_kem_test.cpp \
	gtest_main.cpp \
	hkdf.cpp \
	hkdf_test.cpp \
	hmac.cpp \
	hmac_key.cpp \
	hmac_operation.cpp \
	hmac_test.cpp \
	integrity_assured_key_blob.cpp \
	iso18033kdf.cpp \
	kdf.cpp \
	kdf1_test.cpp \
	kdf2_test.cpp \
	kdf_test.cpp \
	key.cpp \
	key_blob_test.cpp \
	keymaster0_engine.cpp \
	keymaster1_engine.cpp \
	keymaster_configuration.cpp \
	keymaster_configuration_test.cpp \
	keymaster_enforcement.cpp \
	keymaster_enforcement_test.cpp \
	keymaster_tags.cpp \
	logger.cpp \
	nist_curve_key_exchange.cpp \
	nist_curve_key_exchange_test.cpp \
	ocb_utils.cpp \
	openssl_err.cpp \
	openssl_utils.cpp \
	operation.cpp \
	operation_table.cpp \
	rsa_key.cpp \
	rsa_key_factory.cpp \
	rsa_keymaster0_key.cpp \
	rsa_keymaster1_key.cpp \
	rsa_keymaster1_operation.cpp \
	rsa_operation.cpp \
	serializable.cpp \
	soft_keymaster_context.cpp \
	soft_keymaster_device.cpp \
	symmetric_key.cpp

CCSRCS=$(GTEST)/src/gtest-all.cc
CSRCS=ocb.c

OBJS=$(CPPSRCS:.cpp=.o) $(CCSRCS:.cc=.o) $(CSRCS:.c=.o)
DEPS=$(CPPSRCS:.cpp=.d) $(CCSRCS:.cc=.d) $(CSRCS:.c=.d)

BINARIES = \
	android_keymaster_messages_test \
	android_keymaster_test \
	attestation_record_test \
	authorization_set_test \
	ecies_kem_test \
	hkdf_test \
	hmac_test \
	kdf1_test \
	kdf2_test \
	kdf_test \
	key_blob_test \
	keymaster_configuration_test \
	keymaster_enforcement_test \
	nist_curve_key_exchange_test

.PHONY: coverage memcheck massif clean run

%.run: %
	./$<
	touch $@

run: $(BINARIES:=.run)

coverage: coverage.info
	genhtml coverage.info --output-directory coverage

coverage.info: run
	lcov --capture --directory=. --output-file coverage.info

%.coverage : %
	$(MAKE) clean && $(MAKE) $<
	./$<
	lcov --capture --directory=. --output-file coverage.info
	genhtml coverage.info --output-directory coverage

#UNINIT_OPTS=--track-origins=yes
UNINIT_OPTS=--undef-value-errors=no

MEMCHECK_OPTS=--leak-check=full \
	--show-reachable=yes \
	--vgdb=full \
	$(UNINIT_OPTS) \
	--error-exitcode=1 \
	--suppressions=valgrind.supp \
	--gen-suppressions=all

MASSIF_OPTS=--tool=massif \
	--stacks=yes

%.memcheck : %
	valgrind $(MEMCHECK_OPTS) ./$< && \
	touch $@

%.massif : %
	valgrind $(MASSIF_OPTS) --massif-out-file=$@ ./$<

memcheck: $(BINARIES:=.memcheck)

massif: $(BINARIES:=.massif)

GTEST_OBJS = $(GTEST)/src/gtest-all.o gtest_main.o

keymaster_configuration_test: keymaster_configuration_test.o \
	authorization_set.o \
	serializable.o \
	logger.o \
	keymaster_configuration.o \
	$(GTEST_OBJS)

hmac_test: hmac_test.o \
	android_keymaster_test_utils.o \
	android_keymaster_utils.o \
	authorization_set.o \
	hmac.o \
	keymaster_tags.o \
	logger.o \
	serializable.o \
	$(GTEST_OBJS)

hkdf_test: hkdf_test.o \
	android_keymaster_test_utils.o \
	android_keymaster_utils.o \
	authorization_set.o \
	hkdf.o \
	hmac.o \
	kdf.o \
	keymaster_tags.o \
	logger.o \
	serializable.o \
	$(GTEST_OBJS)

kdf_test: kdf_test.o \
	android_keymaster_utils.o \
	kdf.o \
	logger.o \
	serializable.o \
	$(GTEST_OBJS)

kdf1_test: kdf1_test.o \
	android_keymaster_test_utils.o \
	android_keymaster_utils.o \
	authorization_set.o \
	iso18033kdf.o \
	kdf.o \
	keymaster_tags.o \
	logger.o \
	serializable.o \
	$(GTEST_OBJS)

kdf2_test: kdf2_test.o \
	android_keymaster_test_utils.o \
	android_keymaster_utils.o \
	authorization_set.o \
	iso18033kdf.o \
	kdf.o \
	keymaster_tags.o \
	logger.o \
	serializable.o \
	$(GTEST_OBJS)

nist_curve_key_exchange_test: nist_curve_key_exchange_test.o \
	android_keymaster_test_utils.o \
	authorization_set.o \
	keymaster_tags.o \
	logger.o \
	nist_curve_key_exchange.o \
	openssl_err.o \
	openssl_utils.o \
	serializable.o \
	$(GTEST_OBJS)

ecies_kem_test: ecies_kem_test.o \
	android_keymaster_utils.o \
	android_keymaster_test_utils.o \
	authorization_set.o \
	ecies_kem.o \
	hkdf.o \
	hmac.o \
	kdf.o \
	keymaster_tags.o \
	logger.o \
	nist_curve_key_exchange.o \
	openssl_err.o \
	openssl_utils.o \
	serializable.o \
	$(GTEST_OBJS)

authorization_set_test: authorization_set_test.o \
	android_keymaster_test_utils.o \
	authorization_set.o \
	keymaster_tags.o \
	logger.o \
	serializable.o \
	$(GTEST_OBJS)

key_blob_test: key_blob_test.o \
	android_keymaster_test_utils.o \
	android_keymaster_utils.o \
	auth_encrypted_key_blob.o \
	authorization_set.o \
	integrity_assured_key_blob.o \
	keymaster_tags.o \
	logger.o \
	ocb.o \
	ocb_utils.o \
	openssl_err.o \
	serializable.o \
	$(GTEST_OBJS)

android_keymaster_messages_test: android_keymaster_messages_test.o \
	android_keymaster_messages.o \
	android_keymaster_test_utils.o \
	android_keymaster_utils.o \
	authorization_set.o \
	keymaster_tags.o \
	logger.o \
	serializable.o \
	$(GTEST_OBJS)

android_keymaster_test: android_keymaster_test.o \
	aes_key.o \
	aes_operation.o \
	android_keymaster.o \
	android_keymaster_messages.o \
	android_keymaster_test_utils.o \
	android_keymaster_utils.o \
	asymmetric_key.o \
	asymmetric_key_factory.o \
	attestation_record.o \
	auth_encrypted_key_blob.o \
	authorization_set.o \
	ec_key.o \
	ec_key_factory.o \
	ec_keymaster0_key.o \
	ec_keymaster1_key.o \
	ecdsa_keymaster1_operation.o \
	ecdsa_operation.o \
	hmac_key.o \
	hmac_operation.o \
	integrity_assured_key_blob.o \
	key.o \
	keymaster0_engine.o \
	keymaster1_engine.o \
	keymaster_enforcement.o \
	keymaster_tags.o \
	logger.o \
	ocb.o \
	ocb_utils.o \
	openssl_err.o \
	openssl_utils.o \
	operation.o \
	operation_table.o \
	rsa_key.o \
	rsa_key_factory.o \
	rsa_keymaster0_key.o \
	rsa_keymaster1_key.o \
	rsa_keymaster1_operation.o \
	rsa_operation.o \
	serializable.o \
	soft_keymaster_context.o \
	soft_keymaster_device.o \
	symmetric_key.o \
	$(BASE)/system/security/softkeymaster/keymaster_openssl.o \
	$(BASE)/system/security/keystore/keyblob_utils.o \
	$(GTEST_OBJS)

keymaster_enforcement_test: keymaster_enforcement_test.o \
	android_keymaster_messages.o \
	android_keymaster_test_utils.o \
	android_keymaster_utils.o \
	authorization_set.o \
	keymaster_enforcement.o \
	keymaster_tags.o \
	logger.o \
	serializable.o \
	$(GTEST_OBJS)

attestation_record_test: attestation_record_test.o \
	android_keymaster_test_utils.o \
	android_keymaster_utils.o \
	attestation_record.o \
	authorization_set.o \
	keymaster_tags.o \
	logger.o \
	openssl_err.o \
	serializable.o \
	$(GTEST_OBJS)

$(GTEST)/src/gtest-all.o: CXXFLAGS:=$(subst -Wmissing-declarations,,$(CXXFLAGS))

clean:
	rm -f $(OBJS) $(DEPS) $(BINARIES) \
		$(BINARIES:=.run) $(BINARIES:=.memcheck) $(BINARIES:=.massif) \
		*gcov *gcno *gcda coverage.info
	rm -rf coverage

-include $(CPPSRCS:.cpp=.d)
-include $(CCSRCS:.cc=.d)
