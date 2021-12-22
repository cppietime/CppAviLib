/*
riff.cpp
*/

#include <algorithm>
#include <cstdint>
#include <iostream>
#include "aviutil.hpp"

static void wordAlgin(std::ostream& stream)
{
    std::streampos cpos = stream.tellp();
    if (cpos >= 0 && (cpos & 1) != 0) {
        stream.put(0x00);
    }
}

Riff::RiffChunk::RiffChunk(const char *fourCC, size_t dataSize) :
    dataSize {dataSize},
    offset {-1}
{
        std::copy(fourCC, fourCC + FOURCC_SIZE, this->fourCC);
}

void Riff::RiffChunk::writeTo(std::ostream& stream)
{
    wordAlgin(stream);
    markOffset(stream);
    stream.write(fourCC, FOURCC_SIZE);
    stream.put((std::uint8_t)(dataSize >> 0));
    stream.put((std::uint8_t)(dataSize >> 8));
    stream.put((std::uint8_t)(dataSize >> 16));
    stream.put((std::uint8_t)(dataSize >> 24));
}

void Riff::RiffChunk::rewriteLength(std::ostream& stream)
{
    if (offset == -1) {
        return;
    }
    std::streampos store = stream.tellp();
    stream.flush();
    stream.seekp(offset);
    stream.seekp(SIZE_OFFSET, std::ios_base::cur);
    stream.put((std::uint8_t)(dataSize >> 0));
    stream.put((std::uint8_t)(dataSize >> 8));
    stream.put((std::uint8_t)(dataSize >> 16));
    stream.put((std::uint8_t)(dataSize >> 24));
    stream.flush();
    stream.seekp(store);
}

Riff::RiffContainer::RiffContainer(const char *fourCC, const char *subCC) :
    RiffChunk(fourCC)
{
    dataSize = FOURCC_SIZE;
    std::copy(subCC, subCC + FOURCC_SIZE, containerType);
}

void Riff::RiffChunk::markSize(std::ostream& stream)
{
    std::streampos cpos = stream.tellp();
    cpos -= offset;
    cpos -= FOURCC_SIZE + LENGTH_SIZE;
    dataSize = cpos;
}

void Riff::RiffContainer::writeTo(std::ostream& stream)
{
    RiffChunk::writeTo(stream);
    stream.write(containerType, FOURCC_SIZE);
}

void Riff::RiffFile::finalize(std::ostream& stream)
{
    dataSize = stream.tellp() - offset - FOURCC_SIZE - LENGTH_SIZE;
    wordAlgin(stream);
    rewriteLength(stream);
}

void Riff::RiffConstList::writeTo(std::ostream& stream)
{
    RiffList::writeTo(stream);
    for (auto it = subChunks.begin(); it != subChunks.end(); it++) {
        it->writeTo(stream);
    }
}

void Riff::RiffConstList::add(const RiffChunk& subChunk)
{
    subChunks.push_back(subChunk);
    dataSize += subChunk.getSize();
}

void Riff::RiffData::writeTo(std::ostream& stream)
{
    RiffChunk::writeTo(stream);
    stream.write(reinterpret_cast<const char*>(data.data()), data.size());
}
