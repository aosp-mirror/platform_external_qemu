// Copyright 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include <gtest/gtest.h>                  // for Asser...

#include "android/emulation/control/GrpcServices.h"

#include <grpcpp/grpcpp.h>                // for OK
#include <grpcpp/security/credentials.h>  // for Insec...

#include <fstream>      // for ifstream
#include <iterator>     // for istre...
#include <memory>       // for alloc...
#include <string>       // for char_...
#include <tuple>        // for tuple...
#include <type_traits>  // for add_c...

#include "android/base/StringView.h"           // for Strin...
#include "android/base/files/PathUtils.h"      // for pj
#include "android/base/system/System.h"        // for System
#include "android/base/testing/TestSystem.h"   // for TestS...
#include "android/base/testing/TestTempDir.h"  // for TestT...
#include "android/emulation/control/test/BasicTokenAuthenticator.h"
#include "android/emulation/control/test/CertificateFactory.h"  // for Certi...
#include "android/emulation/control/test/TestEchoService.h"     // for getTe...
#include "grpc/grpc_security_constants.h"
#include "test_echo_service.grpc.pb.h"  // for TestEcho
#include "test_echo_service.pb.h"       // for Msg

namespace android {
namespace emulation {
namespace control {

using android::base::PathUtils;
using android::base::System;
using android::base::TestSystem;

using android::base::pj;
using grpc::Service;

class GrpcServiceTest : public ::testing::Test {
protected:
    GrpcServiceTest() : mTestSystem("/", System::kProgramBitness) {
        auto testDir = mTestSystem.getTempRoot();
        EXPECT_TRUE(testDir->makeSubDir("home"));
        mTestSystem.setHomeDirectory(testDir->makeSubPath("home"));
    }

    void writeToken(std::string fname) {
        std::ofstream tokFile(fname);
        tokFile << "foobar";
    }

    void SetUp() {
        mTestSystem.host()->setEnvironmentVariable("GRPC_VERBOSITY", "DEBUG");
       // mTestSystem.host()->setEnvironmentVariable("GRPC_VERBOSITY", "DEBUG");
        mEchoService = new TestEchoServiceImpl();
        mHelloWorld.set_msg(HELLO);
    }

    bool construct() {
        mEmuController = mBuilder.build();
        return mEmuController.get() != nullptr;
    }

    std::string address() {
        return !mEmuController
                       ? ""
                       : "localhost:" + std::to_string(mEmuController->port());
    }

    std::string readFile(std::string fname) {
        std::ifstream fstream(fname);
        std::string contents((std::istreambuf_iterator<char>(fstream)),
                             std::istreambuf_iterator<char>());
        return contents;
    }

    std::tuple<Msg, grpc::Status> sayHello(
            const std::shared_ptr<grpc::ChannelCredentials>& creds,
            const bool wait_for_ready=true,
            const std::shared_ptr<grpc::CallCredentials>& call_creds =
                    nullptr) {
        // Now let's connect with a client.
        auto channel = grpc::CreateChannel(address(), creds);
        auto client = TestEcho::NewStub(channel);
        grpc::ClientContext ctx;
        // You might see unexpected failures with fail fast semantics
        // and transient connection issues.
        ctx.set_wait_for_ready(wait_for_ready);
        if (call_creds)
            ctx.set_credentials(call_creds);
        Msg response;
        auto status = client->echo(&ctx, mHelloWorld, &response);
        return std::make_tuple(response, status);
    }

    std::shared_ptr<grpc::CallCredentials> getTokenCredentials() {
        return grpc::MetadataCredentialsFromPlugin(
                std::make_unique<BasicTokenAuthenticator>(mToken));
    }

    TestEchoServiceImpl* mEchoService;
    TestSystem mTestSystem;
    Msg mHelloWorld;
    EmulatorControllerService::Builder mBuilder;
    std::unique_ptr<EmulatorControllerService> mEmuController;
    std::string mToken{"super_secret_token"};

    const std::string HELLO{"Hello World"};
};

TEST_F(GrpcServiceTest, BasicRegistrationSucceeds) {
    mBuilder.withService(mEchoService).withPortRange(0, 1);
    EXPECT_TRUE(construct());

    auto [msg, status] = sayHello(::grpc::InsecureChannelCredentials());
    EXPECT_EQ(grpc::StatusCode::OK, status.error_code());
    EXPECT_EQ(HELLO, msg.msg());
}

TEST_F(GrpcServiceTest, BadSecretsRegistrationFails) {
    auto badfile = pj(mTestSystem.getHomeDirectory(), "does_not_exist");
    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withCertAndKey(badfile.c_str(), badfile.c_str(), badfile.c_str());
    EXPECT_FALSE(construct());
}

TEST_F(GrpcServiceTest, SecureRegistrationSucceeds) {
    const auto [priv, cert] = CertificateFactory::generateCertKeyPair(
            mTestSystem.getHomeDirectory(), "server_certs");

    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withCertAndKey(cert.c_str(), priv.c_str(), nullptr);
    EXPECT_TRUE(construct());
}

TEST_F(GrpcServiceTest, SecureWithCaRegistrationSucceeds) {
    const auto [priv, cert] = CertificateFactory::generateCertKeyPair(
            mTestSystem.getHomeDirectory(), "server_certs");

    const auto [client_priv, client_cert] =
            CertificateFactory::generateCertKeyPair(
                    mTestSystem.getHomeDirectory(), "client_certs");

    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withCertAndKey(cert.c_str(), priv.c_str(), client_cert.c_str());

    EXPECT_TRUE(construct());
}

TEST_F(GrpcServiceTest, SecureCanNotConnectWithInsecureClient) {
    // When tls is on, you must use tls!
    const auto [priv, cert] = CertificateFactory::generateCertKeyPair(
            mTestSystem.getHomeDirectory(), "server_certs");

    const auto [client_priv, client_cert] =
            CertificateFactory::generateCertKeyPair(
                    mTestSystem.getHomeDirectory(), "client_certs");

    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withCertAndKey(cert.c_str(), priv.c_str(), client_cert.c_str());

    EXPECT_TRUE(construct());
    auto [msg, status] = sayHello(::grpc::InsecureChannelCredentials());
    EXPECT_NE(HELLO, msg.msg());
    EXPECT_NE(grpc::StatusCode::OK, status.error_code());
}

TEST_F(GrpcServiceTest, SecureCanConnectWithSecure) {
    // A secure client can connect to a secure server.
    // This demonstrates mutual auth with self signed certs.
    const auto [priv, cert] = CertificateFactory::generateCertKeyPair(
            mTestSystem.getHomeDirectory(), "server_certs");

    const auto [client_priv, client_cert] =
            CertificateFactory::generateCertKeyPair(
                    mTestSystem.getHomeDirectory(), "client_certs");

    // Server will use the client cert as the certificate authority..
    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withCertAndKey(cert.c_str(), priv.c_str(), client_cert.c_str());

    EXPECT_TRUE(construct());

    // Client will use the server cert as the ca authority.
    grpc::SslCredentialsOptions sslOpts;
    sslOpts.pem_root_certs = readFile(cert);
    sslOpts.pem_private_key = readFile(client_priv);
    sslOpts.pem_cert_chain = readFile(client_cert);
    auto creds = grpc::SslCredentials(sslOpts);

    auto [msg, status] = sayHello(creds);
    EXPECT_EQ(HELLO, msg.msg());
    EXPECT_EQ(grpc::StatusCode::OK, status.error_code());
}

TEST_F(GrpcServiceTest, SecureRejectsWrongCerts) {
    // A secure client can connect to a secure server.
    // This demonstrates mutual auth with self signed certs.
    const auto [priv, cert] = CertificateFactory::generateCertKeyPair(
            mTestSystem.getHomeDirectory(), "server_certs");

    const auto [client_priv, client_cert] =
            CertificateFactory::generateCertKeyPair(
                    mTestSystem.getHomeDirectory(), "client_certs");

    const auto [client_wrong_priv, client_wrong_cert] =
            CertificateFactory::generateCertKeyPair(
                    mTestSystem.getHomeDirectory(), "wrong_certs");

    // Server will use the client cert as the certificate authority..
    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withCertAndKey(cert.c_str(), priv.c_str(), client_cert.c_str());

    EXPECT_TRUE(construct());

    // Client will use the server cert as the ca authority.
    grpc::SslCredentialsOptions sslOpts;
    sslOpts.pem_root_certs = readFile(cert);
    sslOpts.pem_private_key = readFile(client_wrong_priv);
    sslOpts.pem_cert_chain = readFile(client_wrong_cert);
    auto creds = grpc::SslCredentials(sslOpts);

    auto [msg, status] = sayHello(creds, false);
    EXPECT_NE(HELLO, msg.msg());
    EXPECT_NE(grpc::StatusCode::OK, status.error_code());
}

TEST_F(GrpcServiceTest, ServerSecureDoesNotDemandClientSecure) {
    // The server can identify itself, but does not require clients
    // to identify themselves.
    const auto [priv, cert] = CertificateFactory::generateCertKeyPair(
            mTestSystem.getHomeDirectory(), "server_certs");

    const auto [client_priv, client_cert] =
            CertificateFactory::generateCertKeyPair(
                    mTestSystem.getHomeDirectory(), "client_certs");

    // Server will use the client cert as the certificate authority,
    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withCertAndKey(cert.c_str(), priv.c_str(), nullptr);

    EXPECT_TRUE(construct());

    // Client will validate the server, but will not identify itself.
    grpc::SslCredentialsOptions sslOpts;
    sslOpts.pem_root_certs = readFile(cert);
    auto creds = grpc::SslCredentials(sslOpts);

    auto [msg, status] = sayHello(creds);
    EXPECT_EQ(HELLO, msg.msg());
    EXPECT_EQ(grpc::StatusCode::OK, status.error_code());
}

// ------------------------------------------------------------------------------------
// TOKEN BASED TESTS
// ------------------------------------------------------------------------------------
TEST_F(GrpcServiceTest, DoNotLaunchWithEmptyToken) {
    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withAuthToken("");
    EXPECT_FALSE(construct());
}

TEST_F(GrpcServiceTest, InsecureWithNoTokenRejects) {
    auto invocations = mEchoService->invocations();
    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withAuthToken(mToken.c_str());

    EXPECT_TRUE(construct());

    auto [msg, status] = sayHello(::grpc::InsecureChannelCredentials());
    EXPECT_NE(HELLO, msg.msg());
    EXPECT_EQ(grpc::StatusCode::UNAUTHENTICATED, status.error_code());

    // Underlying service method was not called
    EXPECT_EQ(invocations, mEchoService->invocations());
}

TEST_F(GrpcServiceTest, InsecureWithGoodTokenAccepts) {
    auto invocations = mEchoService->invocations();
    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withAuthToken(mToken.c_str());

    EXPECT_TRUE(construct());

    auto token = getTokenCredentials();
    // Note! ::grpc::InsecureChannel WILL NOT INJECT HEADERS!
    auto [msg, status] =
            sayHello(::grpc::experimental::LocalCredentials(LOCAL_TCP), true, token);
    EXPECT_EQ(HELLO, msg.msg());
    EXPECT_EQ(grpc::StatusCode::OK, status.error_code());

    // Underlying service method was called
    EXPECT_EQ(invocations + 1, mEchoService->invocations());
}

TEST_F(GrpcServiceTest, SecureWithBadTokenRejects) {
    auto invocations = mEchoService->invocations();
    const auto [priv, cert] = CertificateFactory::generateCertKeyPair(
            mTestSystem.getHomeDirectory(), "server_certs");

    const auto [client_priv, client_cert] =
            CertificateFactory::generateCertKeyPair(
                    mTestSystem.getHomeDirectory(), "client_certs");

    // Server will use the client cert as the certificate authority..
    // and we will require an auth token.
    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withCertAndKey(cert.c_str(), priv.c_str(), client_cert.c_str())
            .withAuthToken(mToken.c_str());

    EXPECT_TRUE(construct());

    // Client will use the server cert as the ca authority, but we will not
    // send a token along.
    grpc::SslCredentialsOptions sslOpts;
    sslOpts.pem_root_certs = readFile(cert);
    sslOpts.pem_private_key = readFile(client_priv);
    sslOpts.pem_cert_chain = readFile(client_cert);
    auto creds = grpc::SslCredentials(sslOpts);

    auto [msg, status] = sayHello(creds);
    EXPECT_NE(HELLO, msg.msg());
    EXPECT_EQ(grpc::StatusCode::UNAUTHENTICATED, status.error_code());

    // Underlying service method was not called
    EXPECT_EQ(invocations, mEchoService->invocations());
}

TEST_F(GrpcServiceTest, SecureWithGoodTokenAccepts) {
    auto invocations = mEchoService->invocations();
    const auto [priv, cert] = CertificateFactory::generateCertKeyPair(
            mTestSystem.getHomeDirectory(), "server_certs");

    const auto [client_priv, client_cert] =
            CertificateFactory::generateCertKeyPair(
                    mTestSystem.getHomeDirectory(), "client_certs");

    // Server will use the client cert as the certificate authority..
    // and we will require an auth token.
    mBuilder.withService(mEchoService)
            .withPortRange(0, 1)
            .withCertAndKey(cert.c_str(), priv.c_str(), client_cert.c_str())
            .withAuthToken(mToken.c_str());

    EXPECT_TRUE(construct());

    // Client will use the server cert as the ca authority.
    grpc::SslCredentialsOptions sslOpts;
    sslOpts.pem_root_certs = readFile(cert);
    sslOpts.pem_private_key = readFile(client_priv);
    sslOpts.pem_cert_chain = readFile(client_cert);
    auto creds = grpc::SslCredentials(sslOpts);

    auto token = getTokenCredentials();
    auto [msg, status] = sayHello(creds, true, token);
    EXPECT_EQ(HELLO, msg.msg());
    EXPECT_EQ(grpc::StatusCode::OK, status.error_code());

    // Underlying methods was called.
    EXPECT_EQ(invocations + 1, mEchoService->invocations());
}

}  // namespace control
}  // namespace emulation
}  // namespace android
