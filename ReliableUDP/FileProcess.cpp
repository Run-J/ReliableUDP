#include "FileProcess.h"

vector<BlockPacket> FileBlock::GetBlocks(void)
{
    return blocks;
}

const MetaPacket& FileBlock::GetMetaPacket(void)
{
    return metaPacket;
}

//---------------------------------------------------------------------
// LoadFile: Loads the file, computes MD5 and slices file into blocks
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