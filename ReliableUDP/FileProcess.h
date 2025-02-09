// File Name: FileProcess.h
// Date: 2025-02
// File Description: 
//      -- Including all of method prototypes of FileBlock class

#ifndef _FILEPROCESS_H_
#define _FILEPROCESS_H_

#include <cstddef>
#include <vector>
#include "Protocol.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <cstring>
#include "md5.h"

using namespace std;


// Define packet type
const uint8_t TYPE_META = 1; // 1 - Meta Packet
const uint8_t TYPE_DATA = 2; // 2 - Data Packet



// FileBlock class 
//      -- handles file loading, slicing, saving and verification.
class FileBlock
{

private:

    int allDone = -1;                // Flag of received all data from client done; -1 => no-all done, 0 = > all done

    MetaPacket metaPacket;           // File metadata (fixed size 256 bytes)

    vector<BlockPacket> blocks;      // Vector of file blocks

    vector<uint8_t> fileData;        // Complete file data for saving/verification


public:

    // Accessor fo blocks
    vector<BlockPacket> GetBlocks(void);

    // Accessor of fileName
    const MetaPacket& GetMetaPacket(void);

    // Checking document integrity
    bool VerifyFileContent();

    // Inform flag if received all data
    int FinishedReceivedAllData();

    // Parsing received data
    int ProcessReceivedPacket(const unsigned char* packet, size_t packetSize);

    // Reads a file from disk, computes its MD5 checksum, and splits it into blocks.
    int LoadFile(const char* filename);

    // Writes received file data to disk after successful transmission.
    int SaveFile() const;

};

#endif // _FILEPROCESS_H_
