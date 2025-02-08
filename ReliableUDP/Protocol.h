#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <cstdint>

#define PACKET_SIZE 256  // whole packet
#define MAX_FILENAME_LENGTH 100
#define MD5_HASH_LENGTH 16
#define PADDING_SIZE (PACKET_SIZE - sizeof(uint8_t) - MAX_FILENAME_LENGTH - sizeof(uint64_t) * 2 - MD5_HASH_LENGTH)

#define PAYLOAD_SIZE   (PACKET_SIZE - sizeof(uint8_t) - sizeof(uint64_t)) // PACKET_SIZE - packetType - localSequence


typedef struct MetaPacket // 256 Bytes fixed
{
    uint8_t   packetType; // 1 Byte
    char      filename[MAX_FILENAME_LENGTH]; // 100 Bytes
    uint64_t  fileSize; // 8 Bytes
    uint64_t  totalBlocks; // 8 Bytes
    uint8_t   md5[MD5_HASH_LENGTH]; // 16 Bytes (128 bits)
    uint8_t   padding[PADDING_SIZE]; // 133 Bytes
}MetaPacket;


#pragma pack(push, 1) // Sets the byte alignment of the structure to 1 byte, 
                      // otherwiese the packetType will be auto fill seven zero after the packetType(uint_8) => 1000 0000 
                      // and the payLoad will be lost 7 bytes data
struct BlockPacket // 256 Bytes fixed
{
    uint8_t   packetType; // 1 Byte
    uint64_t  localSequence; // 8 Bytes
    char      payLoad[PAYLOAD_SIZE]; // Maximum 247 Bytes
};
#pragma pack(pop)

#endif // !_PROTOCOL_H_

