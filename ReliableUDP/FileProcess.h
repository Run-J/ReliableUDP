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

    //  Inform if received all data
    int FinishedReceivedAllData();

    // Parsing received data
    int ProcessReceivedPacket(const unsigned char* packet, size_t packetSize);

    // LoadFile: Loads the file, computes MD5 and slices file into blocks
    int LoadFile(const char* filename);



    // SaveFile:
    //   Reconstructs the file from the loaded blocks and saves it to disk.
    // Parameters:
    //   const char* filename (optional) - The output file name.
    // Return:
    //   int - Returns the number of bytes written on success, or -1 on error.
    // int SaveFile(const char* filename = nullptr) const;

};

#endif // _FILEPROCESS_H_
