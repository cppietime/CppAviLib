/*
avitest.cpp
*/

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include "aviutil.hpp"

#include "jpegutil.hpp"
#include "flacutil.hpp"

int main(int argc, char** argv)
{
    int mode = Avi::NORMAL;
    int bitsPerSample = 8;
    int numChannels = 1;
    if (argc > 1) {
        bitsPerSample = atoi(argv[1]);
    }
    int sampleNorm = (1 << (bitsPerSample - 1)) - 1;
    int width = 100, height = 100;
    float fps = 12;
    float sampleRate = 44100;
    
    std::uint8_t *rgb = new std::uint8_t[3 * width * height];
    
    std::ofstream out("test.avi", std::ios_base::out | std::ios_base::binary);
    float duration = 30;

    std::stringstream jpegStream;
    Avi::FlacMjpegAvi fmavi(width, height, fps, bitsPerSample, sampleRate, numChannels, static_cast<Avi::EncodingMode>(mode));
    fmavi.prepare(out);
    
    std::vector<std::int16_t> samples;
    for (int i = 0; i < sampleRate * duration; i++) {
        for (int j = 0; j < numChannels; j++) {
            samples.push_back((std::int16_t)(sampleNorm / 2 * sin(i * M_PI * 440.0 / sampleRate)));
        }
    }
    fmavi.writeSamples(out, samples);
    
    int phase = 0;
    for (int i = 0; i < fps * duration; i++) {
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                size_t index = y * width + x;
                std::uint8_t c = ((x + 5 * i) ^ y) & 0xff;
                rgb[index * 3] = rgb[index * 3 + 1] = c;
                rgb[index * 3 + 2] = 0;
            }
        }
        fmavi.writeVideoFrame(out, rgb);
        
        std::cout << "Written frame #" << i << std::endl;
    }

    fmavi.finish(out);
    out.close();
    delete[] rgb;
}
