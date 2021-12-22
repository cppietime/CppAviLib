/*
avi.cpp
*/

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>
#include "aviutil.hpp"

namespace Avi {
    
    constexpr const static char *IDX1_ID = "idx1";
    
    void IndexEntry::match(const Riff::RiffData& rd)
    {
        const char *cc = rd.getFourCC();
        std::copy(cc, cc + Riff::FOURCC_SIZE, fourCC);
    }
    
    std::vector<std::uint8_t>& operator<<(
                std::vector<std::uint8_t>& vector, const IndexEntry& ie)
    {
        toVectorBytes(vector, reinterpret_cast<const std::uint8_t*>(ie.fourCC), Riff::FOURCC_SIZE);
        toVectorLE(vector, ie.flags, sizeof(std::uint32_t));
        toVectorLE(vector, ie.offset, sizeof(std::uint32_t));
        toVectorLE(vector, ie.size, sizeof(std::uint32_t));
        return vector;
    }
    
    void AviMainHeader::writeTo(std::ostream& stream)
    {
        std::streampos store = -1;
        if (offset != -1) {
            store = stream.tellp();
            stream.flush();
            stream.seekp(offset);
        }
        std::cout << "Writing AVIH at " << stream.tellp() << std::endl;
        std::vector<std::uint8_t> vec;
        std::uint32_t usecPerFrame = 1000000 / fps;
        toVectorLE(vec, usecPerFrame, sizeof(std::uint32_t));
        toVectorLE(vec, 500000, sizeof(std::uint32_t));
        toVectorLE(vec, 0, sizeof(std::uint32_t));
        toVectorLE(
            vec, 0 | 0 | AVIF_ISINTERLEAVED, sizeof(std::uint32_t));
        toVectorLE(vec, numFrames, sizeof(std::uint32_t));
        toVectorLE(vec, 0, sizeof(std::uint32_t));
        toVectorLE(vec, numStreams, sizeof(std::uint32_t));
        toVectorLE(vec, 0x100000, sizeof(std::uint32_t));
        toVectorLE(vec, width, sizeof(std::uint32_t));
        toVectorLE(vec, height, sizeof(std::uint32_t));
        toVectorLE(vec, 0, sizeof(std::uint32_t));
        toVectorLE(vec, 0, sizeof(std::uint32_t));
        toVectorLE(vec, 0, sizeof(std::uint32_t));
        toVectorLE(vec, 0, sizeof(std::uint32_t));
        dataSize = vec.size();
        Riff::RiffChunk::writeTo(stream);
        stream.write(reinterpret_cast<const char*>(vec.data()), vec.size());
        // if (store != -1) {
            // stream.seekp(store);
        // }
    }
    
    // void AviStrl::writeTo(std::ostream& stream)
    // {
        // Riff::RiffList::writeTo(stream);
        // for (auto it = streams.begin(); it != streams.end(); it++) {
            // Riff::RiffData strh = (*it)->getStrhChunk();
            // strh.writeTo(stream);
            // Riff::RiffData strf = (*it)->getStrfChunk();
            // strf.writeTo(stream);
        // }
        // markSize(stream);
        // rewriteLength(stream);
    // }
    
    void AviHdrl::writeTo(std::ostream& stream)
    {
        // std::streampos store = -1;
        // if (offset != -1) {
            // store = stream.tellp();
            // stream.seekp(offset);
        // }
        std::cout << "Writing HDRL at " << stream.tellp() << std::endl;
        Riff::RiffList::writeTo(stream);
        avih.writeTo(stream);
        // strl.writeTo(stream);
        for (auto it = streams.begin(); it != streams.end(); it++) {
            (*it)->writeTo(stream);
        }
        markSize(stream);
        rewriteLength(stream);
        // if (store != -1) {
            // stream.seekp(store);
        // }
    }
    
    void Avi::writeFrame(
        std::ostream& stream,
        size_t streamNo, float seconds, std::uint32_t flags, const std::uint8_t *data, size_t size)
    {
        Riff::RiffData rd = operator[](streamNo).dataToChunk(data, size, streamNo);
        IndexEntry ie(
            seconds, moviOffset + Riff::FOURCC_SIZE, rd.getSize(), flags);
        ie.match(rd);
        indexEntries.push_back(ie);
        rd.writeTo(stream);
        operator[](streamNo).updateChunkSize(rd.getSize());
        moviOffset += rd.getSize() + Riff::FOURCC_SIZE + Riff::LENGTH_SIZE;
        moviOffset += moviOffset & 1;
        if (operator[](streamNo).type == VIDEO) {
            headerList.avih.numFrames++;
        }
    }
    
    void Avi::writeBeforeFrames(std::ostream& stream)
    {
        writeTo(stream);
        headerList.writeTo(stream);
        moviList.writeTo(stream);
    }
    
    void Avi::writeAfterFrames(std::ostream& stream)
    {
        // headerList.avih.writeTo(stream);
        moviList.expand(moviOffset);
        moviList.rewriteLength(stream);
        std::streampos store = stream.tellp();
        stream.flush();
        stream.seekp(headerList.getOffset());
        headerList.writeTo(stream);
        stream.flush();
        stream.seekp(store);
        std::vector<std::uint8_t> indexData;
        std::sort(indexEntries.begin(), indexEntries.end());
        for (auto it = indexEntries.begin(); it != indexEntries.end(); it++) {
            indexData << *it;
        }
        Riff::RiffData index(IDX1_ID, indexData);
        index.writeTo(stream);
        finalize(stream);
    }
    
}