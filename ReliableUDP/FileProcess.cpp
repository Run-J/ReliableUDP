// File Name: FileProcess.cpp
// Date: 2025-02
// File Description:
//   This file implements file handling operations for a reliable UDP-based file transfer system. 
//   It provides functionalities to load a file, split it into blocks for transmission, verify its integrity using MD5, 
//   and reassemble received blocks back into a complete file.

#include "FileProcess.h"



// Function Name: SaveFile
// Parameters: None
// Return Value: int - Returns 0 on success, -1 if an error occurs (e.g., file cannot be opened or written).
// Description:
//      -- Saves the received file data to disk using the filename stored in `metaPacket`.
int FileBlock::SaveFile() const
{
    // Open output file using the filename from metaPacket in binary mode.
    ofstream outFile(metaPacket.filename, ios::binary);
    if (!outFile)
    {
        fprintf(stderr, "Error: Cannot open file for writing: %s\n", metaPacket.filename);
        return -1;
    }

    // Write the entire fileData vector to the file.
    outFile.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
    if (!outFile)
    {
        fprintf(stderr, "Error: Failed to write all data to file: %s\n", metaPacket.filename);
        return -1;
    }
    outFile.close();

    // Debug: print success message and number of bytes written.
    printf("File saved successfully: %s, %zu bytes written.\n", metaPacket.filename, fileData.size());

    return 0;
}



// Function Name: VerifyFileContent
// Parameters: None
// Return Value: bool - Returns true if the computed MD5 matches the expected MD5, false otherwise.
// Function Description:
//      -- Computes the MD5 checksum of the received file and compares it with the expected MD5 stored in `metaPacket`.
bool FileBlock::VerifyFileContent()
{
    // Compute MD5 of fileData
    uint8_t computedMD5[MD5_HASH_LENGTH] = { 0 };
    MD5Context ctx;
    md5Init(&ctx);

    md5Update(&ctx, fileData.data(), fileData.size());
    md5Finalize(&ctx);
    memcpy(computedMD5, ctx.digest, MD5_HASH_LENGTH);

    // Compare the computed MD5 with the metaPacket's MD5.
    if (memcmp(metaPacket.md5, computedMD5, MD5_HASH_LENGTH) == 0)
    {
        printf("Checksum verification successful!\n");
        return true;
    }
    else
    {
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("Checksum verification failed.\n");
        printf("Computed MD5: ");
        for (int i = 0; i < MD5_HASH_LENGTH; i++)
        {
            printf("%02x", computedMD5[i]);
        }
        printf("\nExpected MD5: ");
        for (int i = 0; i < MD5_HASH_LENGTH; i++)
        {
            printf("%02x", metaPacket.md5[i]);
        }
        printf("\n");
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        return false;
    }
}


// Function Name: FinishedReceivedAllData
// Parameters: None
// Return Value: int - Returns 0 if all data has been received, otherwise returns -1.
// Function Description:
//      --  Checks whether all expected data packets have been received.
int FileBlock::FinishedReceivedAllData()
{
    return allDone;
}



// Function Name: ProcessReceivedPacket
// Parameters:
//   - const unsigned char* packet: Pointer to the received packet.
//   - size_t packetSize: Size of the received packet.
// Return Value: int - Returns 0 on success, -1 if the packet is invalid or unrecognized.
// Function Description:
//      -- Processes an incoming packet, either updating metadata (`MetaPacket`) or storing file data (`BlockPacket`).
int FileBlock::ProcessReceivedPacket(const unsigned char* packet, size_t packetSize)
{
    // Check that the packet is valid and has at least one full packet's size.
    if (packet == nullptr || packetSize < PACKET_SIZE)
    {
        fprintf(stderr, "Invalid packet received.\n");
        return -1;
    }

    // Get the packet type from the first byte.
    uint8_t packetType = packet[0];

    // If the packet type is Meta, update the metaPacket and allocate fileData.
    if (packetType == TYPE_META)
    {
        // Interpret packet as MetaPacket
        const MetaPacket* meta = reinterpret_cast<const MetaPacket*>(packet);

        // Update the internal metaPacket member (copy all fields)
        metaPacket = *meta;

        // print debug message
        printf("Received Meta Packet: filename = %s, fileSize = %llu, totalBlocks = %llu\n",
            metaPacket.filename,
            (unsigned long long)metaPacket.fileSize,
            (unsigned long long)metaPacket.totalBlocks);

        // Allocate fileData space according to metaPacket.fileSize to hold the entire file contents.
        fileData.resize(static_cast<size_t>(metaPacket.fileSize));

        return 0;
    }
    // If the packet type is Data, store the payload into the correct position in fileData.
    else if (packetType == TYPE_DATA)
    {
        // Interpret packet as BlockPacket
        const BlockPacket* block = reinterpret_cast<const BlockPacket*>(packet);

        // offset = localSequence * PAYLOAD_SIZE
        uint64_t seq = block->localSequence;
        size_t offset = static_cast<size_t>(seq * PAYLOAD_SIZE);

        // Determine the number of bytes of data to be copied
        // For the last block, the actual data may be less than PAYLOAD_SIZE
        size_t copySize = PAYLOAD_SIZE;
        if (offset + copySize > fileData.size())
            copySize = fileData.size() - offset;

        // Copy the payLoad data from the current block to the correct location in fileData.
        memcpy(fileData.data() + offset, block->payLoad, copySize);

        // print debug message
        printf("Received Data Packet: localSequence = %llu, copied %zu bytes\n",
            (unsigned long long)seq, copySize);


        // Check if Received all of data
        if (block->localSequence == metaPacket.totalBlocks - 1)
        {
            allDone = 0;
        }

        return 0;
    }
    else
    {
        printf("Received unknown packet type: %d\n", packetType);
        return -1;
    }
}


// Accessor of blocks
//
vector<BlockPacket> FileBlock::GetBlocks(void)
{
    return blocks;
}


// Accessor of metaPacket
// Return Value: const MetaPacket& - Returns a reference to the stored metadata packet.
const MetaPacket& FileBlock::GetMetaPacket(void)
{
    return metaPacket;
}


// Function Name: LoadFile
// Parameters:
//   - const char* filename: The name of the file to load.
// Return Value: 
//      -- int - Returns the number of blocks on success, or -1 if an error occurs.
// Function Description:
//      -- Loads a file from disk, computes its MD5 checksum, and splits it into smaller blocks for transmission.
int FileBlock::LoadFile(const char* filename)
{
    // Check that filename is valid
    assert(filename != nullptr);


    // Open the file in binary mode and move to the end to get its size
    ifstream inFile(filename, ios::binary | ios::ate);
    if (!inFile)
    {
        fprintf(stderr, "Cannot open file for reading: %s\n", filename);
        return -1;
    }
    metaPacket.fileSize = static_cast<uint64_t>(inFile.tellg()); // get file size
    inFile.seekg(0, ios::beg); // move back to start of file


    // Set the meta packet type and copy the file name.
    metaPacket.packetType = TYPE_META;
    strcpy_s(metaPacket.filename, MAX_FILENAME_LENGTH, filename);


    // Calculate MD5 checksum
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "rb");

    if (!fp)
    {
        fprintf(stderr, "Failed to open file for MD5 calculation:  %s\n", filename);
        inFile.close();
        return -1;
    }
    md5File(fp, metaPacket.md5);
    fclose(fp);


    // Determine the total number of blocks needed, result round up
    uint64_t totalBlocks = (metaPacket.fileSize + PAYLOAD_SIZE - 1) / PAYLOAD_SIZE;
    metaPacket.totalBlocks = totalBlocks;


    // Allocate space for blocks and complete file data
    blocks.resize(static_cast<size_t>(totalBlocks));
    fileData.resize(static_cast<size_t>(metaPacket.fileSize));


    // Read entire file into fileData
    inFile.seekg(0, ios::beg);
    inFile.read(reinterpret_cast<char*>(fileData.data()), metaPacket.fileSize);
    inFile.close();


    // Split the file data into blocks
    for (uint64_t i = 0; i < totalBlocks; i++)
    {
        blocks[static_cast<size_t>(i)].packetType = TYPE_DATA;
        blocks[static_cast<size_t>(i)].localSequence = i;

        size_t offset = static_cast<size_t>(i * PAYLOAD_SIZE); // Starting position of the current block in fileData vector (buffer)

        // Check if last block of payload
        size_t blockSize = PAYLOAD_SIZE;
        if (offset + blockSize > fileData.size())
            blockSize = fileData.size() - offset;

        memcpy(blocks[static_cast<size_t>(i)].payLoad, fileData.data() + offset, blockSize);

        // Now, if the last piece of data is less than PAYLOAD_SIZE, the rest remains unused.
    }

    // return static_cast<int>(totalBlocks);
    return 0;
}