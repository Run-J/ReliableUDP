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
    MetaPacket metaPacket;           // File metadata (fixed size 256 bytes)

    vector<BlockPacket> blocks;      // Vector of file blocks

    vector<uint8_t> fileData;        // Complete file data for saving/verification


public:

    // LoadFile:
    //   Loads the file from disk, splits it into blocks for transmission,
    //   and computes the MD5 checksum.
    // Parameters:
    //   const char* filename - The file name to load.
    // Return:
    //   int - Returns the number of blocks on success, or -1 on error.
    int LoadFile(const char* filename);



    // SaveFile:
    //   Reconstructs the file from the loaded blocks and saves it to disk.
    // Parameters:
    //   const char* filename (optional) - The output file name.
    // Return:
    //   int - Returns the number of bytes written on success, or -1 on error.
    // int SaveFile(const char* filename = nullptr) const;



    // VerifyFileContent:
    //   Recomputes the MD5 checksum of the loaded file and compares it with the stored value.
    // Return:
    //   bool - Returns true if the MD5 values match, false otherwise.
    // bool VerifyFileContent();

};

#endif // _FILEPROCESS_H_
