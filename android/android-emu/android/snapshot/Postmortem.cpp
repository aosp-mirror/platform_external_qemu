// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/snapshot/Postmortem.h"

#include <memory>
#include <stdio.h>

#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/threads/Async.h"
#include "android/emulation/AdbMessageSniffer.h"
#include "android/Jdwp/Jdwp.h"
#include <atomic>

#define DEBUG 2

#if DEBUG >= 1
#define D(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D(...) (void)0
#endif

#if DEBUG >= 2
#define DD(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define DD(...) (void)0
#endif

#define ADB_SYNC 0x434e5953
#define ADB_CNXN 0x4e584e43
#define ADB_OPEN 0x4e45504f
#define ADB_OKAY 0x59414b4f
#define ADB_CLSE 0x45534c43
#define ADB_WRTE 0x45545257
#define ADB_AUTH 0x48545541

#define ADB_AUTH_TOKEN         1
#define ADB_AUTH_SIGNATURE     2
#define ADB_AUTH_RSAPUBLICKEY  3
#define A_VERSION 0x01000000

using amessage = android::emulation::AdbMessageSniffer::amessage;
using apacket = android::emulation::AdbMessageSniffer::apacket;
using namespace android::jdwp;

static int s_adb_port = -1;
static std::atomic_uint32_t s_id = {6000};

static size_t packetSize(const apacket& packet) {
    return sizeof(packet.mesg) + packet.mesg.data_length;
}

static bool recvPacket(int s, apacket* packet) {
    using android::base::socketRecvAll;
    if (!socketRecvAll(s, &packet->mesg, sizeof(packet->mesg))) {
        return false;
    }
    if (packet->mesg.data_length && !socketRecvAll(s, packet->data, packet->mesg.data_length)) {
        return false;
    }
    //printf("package length %d\n", packet->mesg.data_length);
    return true;
}

static bool recvOkey(int s) {
    apacket connect_ok;
    if (!recvPacket(s, &connect_ok)) {
        return false;
    }
    if (connect_ok.mesg.command != ADB_OKAY) {
        return false;
    }
    return true;
}

namespace android {
namespace postmortem {

void setAdbPort(int adb_port) {
    s_adb_port = adb_port;
}

bool track_async(int pid, const char *snapshot_name) {
    return base::async([pid, snapshot_name] {
        bool result = track(pid, snapshot_name);
        D("track result %d\n", result);
    });
}
bool track(int pid, const char *snapshot_name) {
    if (s_adb_port == -1) {
        fprintf(stderr, "adb port uninitialized\n");
        return false;
    }

    int s = base::socketTcp4LoopbackClient(s_adb_port);
    if (s < 0) {
        s = base::socketTcp6LoopbackClient(s_adb_port);
    }
    if (s < 0) {
        return false;
    }

    uint32_t local_id = s_id.fetch_add(1);
    uint32_t remote_id = 0;
    // It is ok for some non-thread safe to happen during the following loop,
    // as long as you don't have 2^32 threads simultaneously.
    if (s_id == 0) {
        s_id ++;
    }
    base::ScopedSocket ss(s);
    base::socketSetBlocking(s);
    base::socketSetNoDelay(s);
    D("Setup socket");

#define _SEND_PACKET(packet) { if (!base::socketSendAll(s, (const void*)&packet, packetSize(packet))) return false; }
#define _SEND_PACKET_OK(packet) { _SEND_PACKET(packet); if (!recvOkey(s)) return false; }
#define _RECV_PACKET(packet) { if (!recvPacket(s, &packet)) return false; }
    {
        apacket to_guest;
        to_guest.mesg.command = ADB_CNXN;
        to_guest.mesg.arg0 = 0;
        to_guest.mesg.arg1 = 256*1024;
        to_guest.mesg.data_length = 0;
        to_guest.mesg.data_check = 0;
        to_guest.mesg.magic = to_guest.mesg.command ^ 0xffffffff;
        DD("now write connection command...\n");
        _SEND_PACKET(to_guest);

        DD("now read ...\n");
        apacket pack_recv;
        amessage &mesg = pack_recv.mesg;
        _RECV_PACKET(pack_recv);

        apacket pack_send;
        memset(&pack_send, 0, sizeof(pack_send));

        // TODO: authenticate ADB for non-playstore image
        /*if (mesg.command == ADB_AUTH) {
            int siglen = 20;
            sign_auth_token((const char*)pack_recv.data, mesg.data_length, (char*)pack_send.data, siglen);

            pack_send.mesg.command = ADB_AUTH;
            pack_send.mesg.arg0 = ADB_AUTH_SIGNATURE;
            pack_send.mesg.data_length = siglen;

            DD("send auth packt\n");
            SendPacket(sfd, &pack_send);
            print_mesg(pack_send.msg);q

            DD("read for connection\n");
            if(!ReadFully(sfd, &mesg, sizeof(mesg))) return 0;
            print_mesg(mesg);
            ReadFully(sfd, pack_recv.data, mesg.data_length);
            DD("data check sum coompute %d\n", GetCheckSum(&pack_recv));
            for (int i=0; i < mesg.data_length; ++i) {
                fprintf(stderr, "%c", pack_recv.data[i]);
            }
            DD("\n");
        }*/
    }

    {
        apacket jdwp_connect;
        jdwp_connect.mesg.command = ADB_OPEN;
        jdwp_connect.mesg.arg0 = local_id;
        jdwp_connect.mesg.arg1 = 0;
        jdwp_connect.mesg.data_check = 0;
        jdwp_connect.mesg.magic = jdwp_connect.mesg.command ^ 0xffffffff;
        jdwp_connect.mesg.data_length = sprintf((char*)jdwp_connect.data,
                                                "jdwp:%d", pid) + 1;
        _SEND_PACKET(jdwp_connect);
    }
    {
        apacket connect_ok;
        _RECV_PACKET(connect_ok);
        if (connect_ok.mesg.command != ADB_OKAY) {
            return false;
        }
        remote_id = connect_ok.mesg.arg0;
    }
    D("Open jdwp");
    apacket ok_out;
    ok_out.mesg.command = ADB_OKAY;
    ok_out.mesg.arg0 = local_id;
    ok_out.mesg.arg1 = remote_id;
    ok_out.mesg.data_check = 0;
    ok_out.mesg.magic = ok_out.mesg.command ^ 0xffffffff;
#define _RECV_PACKET_OK(packet) { _RECV_PACKET(packet); _SEND_PACKET(ok_out); }

    apacket packet_out;
    packet_out.mesg.command = ADB_WRTE;
    packet_out.mesg.arg0 = local_id;
    packet_out.mesg.arg1 = remote_id;
    packet_out.mesg.data_check = 0;
    packet_out.mesg.magic = packet_out.mesg.command ^ 0xffffffff;

    // Send handshake
    packet_out.mesg.data_length = sprintf((char*)packet_out.data,
                                            "JDWP-Handshake");
    _SEND_PACKET_OK(packet_out);
    {
        apacket handshake_recv;
        _RECV_PACKET_OK(handshake_recv);
        // Don't use strcmp. There is no trailing 0.
        if (memcmp(handshake_recv.data, "JDWP-Handshake", 14) != 0) {
            return false;
        }
    }
    D("Handshake OK");
    int jdwp_id = 1;
    JdwpIdSize id_size;
    {
        // ID size
        JdwpCommandHeader jdwp_query;
        apacket reply;
        jdwp_query.length = 11;
        jdwp_query.id = jdwp_id ++;
        jdwp_query.flags = 0;
        jdwp_query.command_set = 0x1;
        jdwp_query.command = 0x7;
        packet_out.mesg.data_length = jdwp_query.length;
        jdwp_query.writeToBuffer(packet_out.data);
        _SEND_PACKET_OK(packet_out);
        D("ID size query OK");
        _RECV_PACKET_OK(reply);
        id_size.parseFrom(reply.data + 11);

        // Version
        jdwp_query.command = 0x1;
        packet_out.mesg.data_length = jdwp_query.length;
        jdwp_query.writeToBuffer(packet_out.data);
        _SEND_PACKET_OK(packet_out);
        _RECV_PACKET_OK(reply);

        // Capatibility
        jdwp_query.command = 0xc;
        packet_out.mesg.data_length = jdwp_query.length;
        jdwp_query.writeToBuffer(packet_out.data);
        _SEND_PACKET_OK(packet_out);
        _RECV_PACKET_OK(reply);

        // All classes
        jdwp_query.command = 0x3;
        packet_out.mesg.data_length = jdwp_query.length;
        jdwp_query.writeToBuffer(packet_out.data);
        _SEND_PACKET_OK(packet_out);
        _RECV_PACKET_OK(reply);
        JdwpCommandHeader class_header(reply.data);
        std::vector<uint8_t> class_buffer(class_header.length);
        int total_length = 0;
        do {
            int header_bytes = total_length == 0 ? 11 : 0;
            memcpy(class_buffer.data() + total_length, reply.data + header_bytes, reply.mesg.data_length - header_bytes);
            total_length += reply.mesg.data_length - header_bytes;
            if (total_length < class_header.length - 11) {
                _RECV_PACKET_OK(reply);
            } else {
                break;
            }
        } while (true);

        JdwpAllClasses classes;
        classes.parseFrom(class_buffer.data(), id_size);
        printf("%d classes\n", (int)classes.classes.size());
        for (const auto& clazz: classes.classes) {
            printf("class %s\n", clazz.signature.c_str());
        }
    }

    {
        apacket reply;
        JdwpEventRequestSet set_request;
        set_request.event_kind = 0x8; // CLASS_PREPARE
        set_request.suspend_policy = 0;
        JdwpCommandHeader jdwp_event_set;
        jdwp_event_set.length = 11 + set_request.writeToBuffer(packet_out.data + 11);
        jdwp_event_set.id = jdwp_id ++;
        jdwp_event_set.flags = 0;
        jdwp_event_set.command_set = 0xf;
        jdwp_event_set.command = 0x1;
        packet_out.mesg.data_length = jdwp_event_set.length;
        jdwp_event_set.writeToBuffer(packet_out.data);
        _SEND_PACKET_OK(packet_out);
        _RECV_PACKET_OK(reply);

        set_request.event_kind = 0x9; // CLASS_UNLOAD
        set_request.writeToBuffer(packet_out.data + 11);
        jdwp_event_set.id = jdwp_id ++;
        jdwp_event_set.writeToBuffer(packet_out.data);
        _SEND_PACKET_OK(packet_out);
        _RECV_PACKET_OK(reply);

        set_request.event_kind = 0x6; // THREAD_START
        set_request.writeToBuffer(packet_out.data + 11);
        jdwp_event_set.id = jdwp_id ++;
        jdwp_event_set.writeToBuffer(packet_out.data);
        _SEND_PACKET_OK(packet_out);
        _RECV_PACKET_OK(reply);

        set_request.event_kind = 0x7; // THREAD_DEATH
        set_request.writeToBuffer(packet_out.data + 11);
        jdwp_event_set.id = jdwp_id ++;
        jdwp_event_set.writeToBuffer(packet_out.data);
        _SEND_PACKET_OK(packet_out);
        _RECV_PACKET_OK(reply);

        set_request.event_kind = 0x4; // EXCEPTION
        set_request.suspend_policy = 2; // Suspend all
        jdwp_event_set.length = 11 + set_request.writeToBuffer(
            packet_out.data + 11, true, &id_size);
        jdwp_event_set.id = jdwp_id ++;
        jdwp_event_set.writeToBuffer(packet_out.data);
        packet_out.mesg.data_length = jdwp_event_set.length;
        _SEND_PACKET_OK(packet_out);
        _RECV_PACKET_OK(reply);
        /*jdwp_event_set.id = jdwp_id ++;
        set_request.class_matches.clear();
        set_request.class_matches.push_back({"sun.instrument.InstrumentationImpls"});
        jdwp_event_set.length = 11 + set_request.writeToBuffer(packet_out.data + 11);
        jdwp_event_set.writeToBuffer(packet_out.data);
        _SEND_PACKET_OK(packet_out);
        //_RECV_PACKET_OK(reply);

        jdwp_event_set.id = jdwp_id ++;
        set_request.class_matches.clear();
        set_request.class_matches.push_back({"sun.instrument.InstrumentationImpls"});
        jdwp_event_set.length = 11 + set_request.writeToBuffer(packet_out.data + 11);
        jdwp_event_set.writeToBuffer(packet_out.data);
        _SEND_PACKET_OK(packet_out);
        //_RECV_PACKET_OK(reply);*/
    }
    while (1) {
        apacket reply;
        _RECV_PACKET_OK(reply);
    }
    return true;
}
}
}