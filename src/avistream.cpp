/*
avistream.cpp
*/

#include <cstdint>
#include <endian.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "aviutil.hpp"

namespace Avi {
    
    constexpr static const char FOURCCS[][5] = {
        "auds",
        "mids",
        "txts",
        "vids"
    };
    constexpr static std::uint16_t FLAC_TAG = 61868;
    constexpr static size_t FLAC_STREAMINFO_OFFSET = 8;
    
    AviStream::AviStream(
        StreamType type, float fps, const char *handler, unsigned int scale,
        unsigned int width, unsigned int height) :
            Riff::RiffList(STRL_ID),
            type {type},
            scale {scale},
            rate {(unsigned int)(fps * scale)},
            width {width},
            height {height},
            length {0},
            biggestChunk {0},
            time {0}
    {
        if (handler != nullptr) {
            std::copy(handler, handler + Riff::FOURCC_SIZE, this->handler);
        }
        else {
            std::fill(this->handler, this->handler + Riff::FOURCC_SIZE, 0);
        }
    }
    
    void AviStream::writeTo(std::ostream& stream)
    {
        std::cout << "Writing strl at " << stream.tellp() << std::endl;
        Riff::RiffList::writeTo(stream);
        Riff::RiffData strh = getStrhChunk();
        strh.writeTo(stream);
        Riff::RiffData strf = getStrfChunk();
        std::cout << "Writing STRF at " << stream.tellp() << std::endl;
        strf.writeTo(stream);
        markSize(stream);
        rewriteLength(stream);
    }
    
    Riff::RiffData AviStream::getStrhChunk() const
    {
        std::vector<std::uint8_t> data;
        toVectorBytes(data, reinterpret_cast<const std::uint8_t*>(FOURCCS[type]), Riff::FOURCC_SIZE);
        toVectorBytes(data, reinterpret_cast<const std::uint8_t*>(handler), Riff::FOURCC_SIZE);
        toVectorLE(data, 0, sizeof(std::uint32_t));
        toVectorLE(data, 0, sizeof(std::uint16_t));
        toVectorLE(data, 0, sizeof(std::uint16_t));
        toVectorLE(data, 0, sizeof(std::uint32_t));
        toVectorLE(data, scale, sizeof(std::uint32_t));
        toVectorLE(data, rate, sizeof(std::uint32_t));
        toVectorLE(data, 0, sizeof(std::uint32_t));
        toVectorLE(data, length, sizeof(std::uint32_t));
        toVectorLE(data, biggestChunk, sizeof(std::uint32_t));
        toVectorLE(data, 0xFFFFFFFF, sizeof(std::uint32_t));
        toVectorLE(data, 0, sizeof(std::uint32_t));
        toVectorLE(data, 0, sizeof(std::uint16_t));
        toVectorLE(data, 0, sizeof(std::uint16_t));
        toVectorLE(data, width, sizeof(std::uint16_t));
        toVectorLE(data, height, sizeof(std::uint16_t));
        return Riff::RiffData(STRH_ID, data);
    }
    
    Riff::RiffData AviMjpegStream::getStrfChunk()
    {
        std::vector<std::uint8_t> data;
        toVectorLE(data, 40, sizeof(std::uint32_t));
        toVectorLE(data, width, sizeof(std::uint32_t));
        toVectorLE(data, height, sizeof(std::uint32_t));
        toVectorLE(data, 1, sizeof(std::uint16_t));
        toVectorLE(data, settings.components.size() * settings.bitDepth, sizeof(std::uint16_t));
        toVectorBytes(data, reinterpret_cast<const std::uint8_t*>(MJPEG_HANDLER), 4);
        toVectorLE(data, (width * height * settings.components.size() * settings.bitDepth) >> 3,
            sizeof(std::uint32_t));
        toVectorLE(data, 0, sizeof(std::uint32_t));
        toVectorLE(data, 0, sizeof(std::uint32_t));
        toVectorLE(data, 0, sizeof(std::uint32_t));
        toVectorLE(data, 0, sizeof(std::uint32_t));
        return Riff::RiffData(STRF_ID, data);
    }
    
    Riff::RiffData AviFlacStream::getStrfChunk()
    {
        std::vector<std::uint8_t> data;
        toVectorLE(data, FLAC_TAG, sizeof(std::uint16_t));
        toVectorLE(data, settings.numChannels, sizeof(std::uint16_t));
        toVectorLE(data, settings.sampleRate, sizeof(std::uint32_t));
        toVectorLE(data, 16000, sizeof(std::uint32_t));
        toVectorLE(
            data, (settings.bitsPerSample * settings.numChannels + 7) >> 3, sizeof(std::uint16_t));
        toVectorLE(data, settings.bitsPerSample, sizeof(std::uint16_t));
        std::stringstream sstr;
        flac.writeHeaderTo(sstr);
        std::string str = sstr.str();
        toVectorLE(data, str.size() - FLAC_STREAMINFO_OFFSET, sizeof(std::uint16_t));
        toVectorBytes(data,
            reinterpret_cast<const std::uint8_t*>(str.data() + FLAC_STREAMINFO_OFFSET),
            str.size() - FLAC_STREAMINFO_OFFSET);
        return Riff::RiffData(STRF_ID, data);
    }
    
    Riff::RiffData AviStream::dataToChunk(
        const std::uint8_t *data, size_t size, size_t streamNo) const
    {
        char subCC[4] = {(char)('0' + (streamNo / 10)), (char)('0' + (streamNo % 10)),
            idCode[0], idCode[1]};
        return Riff::RiffData(subCC, data, size);
    }
    
}