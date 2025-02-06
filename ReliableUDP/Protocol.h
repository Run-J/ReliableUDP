#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <cstdint>

#define PACKET_SIZE 256  // whole packet
#define MAX_FILENAME_LENGTH 100
#define MD5_HASH_LENGTH 16
#define PADDING_SIZE (PACKET_SIZE - sizeof(uint8_t) - MAX_FILENAME_LENGTH - sizeof(uint64_t) * 2 - MD5_HASH_LENGTH)

#define PAYLOAD_SIZE   (PACKET_SIZE - sizeof(uint8_t) - sizeof(uint64_t)) // PACKET_SIZE - packetType - localSequence


struct MetaPacket
{
    uint8_t   packetType; // 1 Byte
    char      filename[MAX_FILENAME_LENGTH]; // 100 Bytes
    uint64_t  fileSize; // 8 Bytes
    uint64_t  totalBlocks; // 8 Bytes
    uint8_t   md5[MD5_HASH_LENGTH]; // 16 Bytes
    uint8_t   padding[PADDING_SIZE]; 
};

struct BlockPacket
{
    uint8_t   packetType; // 1 Byte
    uint64_t  localSequence; // 8 Bytes
    char      payLoad[PAYLOAD_SIZE];
};

#endif // !_PROTOCOL_H_

