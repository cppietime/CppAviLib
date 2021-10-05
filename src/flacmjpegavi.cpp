/*
flacmjpegavi.cpp
*/

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "aviutil.hpp"

namespace Avi {
    
    const static Flac::flag_t encodingFlags[ENCODING_MODES] = {
        Flac::lpcMethodNone | Flac::riceMethodEstimate,
        Flac::lpcMethodFixed | Flac::riceMethodEstimate,
        Flac::lpcMethodEstimate | Flac::riceMethodEstimate,
        Flac::lpcMethodBruteForce | Flac::riceMethodExact
    };
    
    const static unsigned int maxK[ENCODING_MODES] = {
        1,
        14,
        30,
        30
    };
    
    const std::uint32_t jpegFlags[ENCODING_MODES] = {
        Jpeg::flagHuffmanDefault,
        Jpeg::flagHuffmanDefault,
        Jpeg::flagHuffmanOptimal,
        Jpeg::flagHuffmanOptimal
    };
    
    FlacMjpegAvi::FlacMjpegAvi(
            int width, int height, float fps,
            int bitsPerSample, float sampleRate, int numChannels,
            EncodingMode mode, int jpegQuality) :
        Avi(AviMainHeader(fps, width, height))
    {
        Flac::FlacEncodeOptions flacOptions(
            numChannels,
            bitsPerSample,
            sampleRate,
            Flac::FLAC_DEFAULT_BLOCKSIZE,
            Flac::FLAC_DEFAULT_LPCBITS,
            Flac::FLAC_DEFAULT_MINPRED,
            Flac::FLAC_DEFAULT_MAXPRED,
            Flac::FLAC_DEFAULT_MINPART,
            Flac::FLAC_DEFAULT_MAXPART,
            maxK[mode],
            encodingFlags[mode]
        );
        Jpeg::JpegSettings jpegSettings(
            std::pair<int, int>(width, height),
            nullptr,
            Jpeg::RELATIVE,
            std::pair<int, int>(1, 1),
            jpegQuality,
            jpegFlags[mode]
        );
        flac = std::make_unique<Flac::Flac>(flacOptions);
        jpeg = std::make_unique<Jpeg::Jpeg>(jpegSettings);
        addStream(AviMjpegStream(jpegSettings, fps));
        addStream(AviFlacStream(flacOptions));
    }
    
    FlacMjpegAvi::FlacMjpegAvi(
            const Jpeg::JpegSettings& jpegSettings,
            const Flac::FlacEncodeOptions& flacSettings,
            float fps) :
        Avi(AviMainHeader(fps, jpegSettings.size.first, jpegSettings.size.second)),
        flac {std::make_unique<Flac::Flac>(flacSettings)},
        jpeg {std::make_unique<Jpeg::Jpeg>(jpegSettings)}
    {
        addStream(AviMjpegStream(jpegSettings, fps));
        addStream(AviFlacStream(flacSettings));
    }
    
    void FlacMjpegAvi::writeSamples(std::ostream& stream)
    {
        while (!flac->empty()) {
            sstr << *flac;
            std::string str = sstr.str();
            AviStream& as = operator[](FLAC_STR);
            writeFrame(
                stream, FLAC_STR, as.getTime(), 0,
                reinterpret_cast<const std::uint8_t*>(str.data()), str.size());
            as.increment();
            sstr.str(std::string());
        }
    }
    
    void FlacMjpegAvi::prepare(std::ostream& stream)
    {
        writeBeforeFrames(stream);
    }
    
    void FlacMjpegAvi::finish(std::ostream& stream)
    {
        flac->finalize();
        writeSamples(stream);
        writeAfterFrames(stream);
    }
    
    void FlacMjpegAvi::writeVideoFrame(std::ostream& stream, const std::uint8_t *rgb)
    {
        jpeg->encodeRGB(rgb);
        jpeg->write(sstr);
        std::string str = sstr.str();
        AviStream& as = operator[](MJPG_STR);
        writeFrame(
            stream, MJPG_STR, as.getTime(), AVIIF_KEYFRAME,
            reinterpret_cast<const std::uint8_t*>(str.data()), str.size());
        as.increment();
        sstr.str(std::string());
    }
    
}