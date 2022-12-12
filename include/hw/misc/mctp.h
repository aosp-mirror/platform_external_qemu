#ifndef QEMU_MCTP_H
#define QEMU_MCTP_H

#define MCTP_BASELINE_MTU 64

enum {
    MCTP_H_FLAGS_EOM = 1 << 6,
    MCTP_H_FLAGS_SOM = 1 << 7,
};

enum {
    MCTP_MESSAGE_TYPE_CONTROL   = 0x0,
    MCTP_MESSAGE_TYPE_NMI       = 0x4,

    MCTP_MESSAGE_IC             = 1 << 7,
};

typedef struct MCTPPacketHeader {
    uint8_t version;
    struct {
        uint8_t dest;
        uint8_t source;
    } eid;
    uint8_t flags;
} MCTPPacketHeader;

typedef struct MCTPPacket {
    MCTPPacketHeader hdr;
    uint8_t          payload[];
} MCTPPacket;

typedef struct MCTPControlMessage {
    uint8_t type;
    uint8_t flags;
    uint8_t command;
    uint8_t data[];
} MCTPControlMessage;

enum {
    MCTP_CONTROL_ERROR_UNSUPPORTED_CMD = 0x5,
};

#endif /* QEMU_MCTP_H */
