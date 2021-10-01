/*
aviutil.hpp
Yaakov Schectman, 2021
Utility for writing RIFF chunks for AVI files
*/

#ifndef _AVIUTIL_HPP
#define _AVIUTIL_HPP

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "jpegutil.hpp"
#include "flacutil.hpp"

namespace Riff {
    
    constexpr size_t FOURCC_SIZE = 4;
    constexpr size_t SIZE_OFFSET = 4;
    constexpr size_t LENGTH_SIZE = 4;
    
    class RiffChunk {
        protected:
            char fourCC[FOURCC_SIZE];
            size_t dataSize;
            std::streampos offset;
            RiffChunk(const char *fourCC, size_t dataSize = 0);
        public:
            /*
            Seek to the stored offset and overwrite the size parameter
            */
            void rewriteLength(std::ostream& stream);
            
            /*
            Add size to stored dataSize
            */
            inline void expand(size_t size)
            {
                dataSize += size;
            }
            
            inline size_t getSize() const
            {
                return dataSize;
            }
            
            inline std::streampos getOffset() const
            {
                return offset;
            }
            
            inline void markOffset(std::ostream& stream)
            {
                offset = stream.tellp();
            }
            
            void markSize(std::ostream& stream);
            
            virtual void writeTo(std::ostream& stream);
            
            inline const char* getFourCC() const
            {
                return fourCC;
            }
            
            friend inline std::ostream& operator<<(std::ostream& stream, RiffChunk& chunk);
    };
    
    inline std::ostream& operator<<(std::ostream& stream, RiffChunk& chunk)
    {
        chunk.writeTo(stream);
        return stream;
    }
    
    /*
    Any container that contains subchunks of some sort
    */
    class RiffContainer : public RiffChunk {
        protected:
            char containerType[FOURCC_SIZE];
        public:
            RiffContainer(const char *fourCC, const char *subCC);
            virtual void writeTo(std::ostream& stream);
    };
    
    /*
    A container with FourCC RIFF
    */
    class RiffFile : public RiffContainer {
        public:
            constexpr const static char *RIFF_FILE = "RIFF";
            RiffFile(const char *subCC) :
                RiffContainer(RIFF_FILE, subCC) {}
            void finalize(std::ostream& stream);
    };
    
    /*
    A container with FourCC LIST
    */
    class RiffList : public RiffContainer {
        public:
            constexpr const static char *LIST_FILE = "LIST";
            RiffList(const char *subCC) :
                RiffContainer(LIST_FILE, subCC) {}
    };
    
    class RiffConstList : public RiffList {
        protected:
            std::vector<RiffChunk> subChunks;
        public:
            RiffConstList(const char *subCC) :
                RiffList(subCC) {}
            virtual void writeTo(std::ostream& stream);
            void add(const RiffChunk& subChunk);
            inline RiffChunk& operator[](size_t index)
            {
                return subChunks[index];
            }
    };
    
    /*
    A chunk that holds data in it
    */
    class RiffData : public RiffChunk {
        protected:
            std::vector<std::uint8_t> data;
        public:
            RiffData(const char *fourCC, const std::vector<std::uint8_t>& data) :
                RiffChunk(fourCC, data.size()),
                data {data} {}
            RiffData(const char *fourCC, const std::uint8_t *data, size_t bytes) :
                RiffChunk(fourCC, bytes),
                data(data, data + bytes) {}
            virtual void writeTo(std::ostream& stream);
    };
    
    /*
    A chunk that has data written to the stream after it
    */
    class RiffHeaderOnly : public RiffChunk {
        public:
            RiffHeaderOnly(const char *fourCC) :
                RiffChunk(fourCC) {}
    };
    
}

namespace Avi {
    
    constexpr std::uint32_t AVIIF_KEYFRAME = 0x0010;
    constexpr std::uint32_t AVIIF_NO_TIME = 0x0100;
    constexpr std::uint32_t AVIIF_LIST = 0x0001;
    
    constexpr std::uint32_t AVIF_HASINDEX = 0x0010;
    constexpr std::uint32_t AVIF_MUSTUSEINDEX = 0x0020;
    constexpr std::uint32_t AVIF_ISINTERLEAVED = 0x0100;
    
    inline void toVectorLE(std::vector<std::uint8_t>& vector, std::uint32_t num, size_t bytes)
    {
        std::uint32_t le = htole32(num);
        for (size_t i = 0; i < bytes; i++) {
            vector.push_back(reinterpret_cast<const std::uint8_t*>(&num)[i]);
        }
    }
    
    inline void toVectorBytes(
        std::vector<std::uint8_t>& vector, const std::uint8_t *data, size_t bytes)
    {
        for (size_t i = 0; i < bytes; i++) {
            vector.push_back(data[i]);
        }
    }
    
    class IndexEntry {
        private:
            constexpr const static char *OLDINDEX_ID = "idx1";
            float seconds;
            size_t offset;
            size_t size;
            std::uint32_t flags;
            char fourCC[Riff::FOURCC_SIZE];
        
        public:
            IndexEntry(
                float seconds,
                size_t offset, size_t size,
                std::uint32_t flags = 0) :
                    seconds {seconds},
                    offset {offset},
                    size {size},
                    flags {flags} {}
            
            void match(const Riff::RiffData& rd);
            
            inline bool operator<(const IndexEntry& other) const
            {
                return seconds < other.seconds;
            }
            
            friend std::vector<std::uint8_t>& operator<<(
                std::vector<std::uint8_t>& vector, const IndexEntry& ie);
    };
    
    std::vector<std::uint8_t>& operator<<(
                std::vector<std::uint8_t>& vector, const IndexEntry& ie);
    
    struct AviMainHeader : public Riff::RiffChunk {
        private:
            constexpr const static char *AVIMAIN_ID = "avih";
        public:
            float fps;
            size_t numFrames;
            unsigned int numStreams;
            unsigned int width;
            unsigned int height;
            AviMainHeader(float fps, unsigned int width, unsigned int height) :
                RiffChunk(AVIMAIN_ID),
                fps {fps},
                width {width},
                height {height},
                numFrames {0},
                numStreams {0} {}
            
            /*
            Fairly simply
            */
            virtual void writeTo(std::ostream& stream);
    };
    
    enum StreamType {
        AUDIO,
        MIDI,
        TEXT,
        VIDEO
    };
    
    constexpr const static char DEFAULT_HANDLER[4] = {1, 0, 0, 0};
    
    class AviStream : public Riff::RiffList {
        protected:
            constexpr const static char *STRL_ID = "strl";
            constexpr const static char *STRH_ID = "strh";
            constexpr const static char *STRF_ID = "strf";
            constexpr const static char *AUDIO_ID = "wb";
            constexpr const static char *RAW_VIDEO_ID = "db";
            constexpr const static char *VIDEO_ID = "dc";
            const char *idCode;
            unsigned int rate, scale;
            unsigned int width, height;
            size_t length;
            char handler[Riff::FOURCC_SIZE];
            float time;
        public:
            const StreamType type;
            size_t biggestChunk;
            AviStream(
                StreamType type, float fps,
                const char *handler = DEFAULT_HANDLER, unsigned int scale = 1,
                unsigned int width = 0, unsigned int height = 0);
            Riff::RiffData getStrhChunk() const;
            virtual Riff::RiffData getStrfChunk() = 0;
            Riff::RiffData dataToChunk(
                const std::uint8_t *data, size_t size, size_t streamNo) const;
            virtual void writeTo(std::ostream& stream);
            inline void updateChunkSize(size_t size)
            {
                biggestChunk = std::max(biggestChunk, size);
                length++;
            }
            inline float getTime() const
            {
                return time;
            }
            inline void increment()
            {
                time += (float)scale / rate;
            }
    };
    
    class AviMjpegStream : public AviStream {
        private:
            constexpr const static char *MJPEG_HANDLER = "MJPG";
            Jpeg::JpegSettings settings;
        public:
            AviMjpegStream(const Jpeg::JpegSettings& settings, float fps) :
                AviStream(VIDEO, fps, MJPEG_HANDLER, 1, settings.size.first, settings.size.second),
                settings {settings} {idCode = VIDEO_ID;}
            virtual Riff::RiffData getStrfChunk();
    };
    
    class AviFlacStream : public AviStream {
        private:
            Flac::FlacEncodeOptions settings;
            Flac::Flac flac;
            std::streampos strhOffset;
        public:
            AviFlacStream(const Flac::FlacEncodeOptions& settings, float sampleRate) :
                AviStream(AUDIO, sampleRate / settings.blockSize,
                    DEFAULT_HANDLER, settings.blockSize),
                flac(settings),
                settings {settings},
                strhOffset {0} {idCode = AUDIO_ID;}
            virtual Riff::RiffData getStrfChunk();
            inline Flac::Flac& getFlac()
            {
                return flac;
            }
            inline void markOffset(std::streampos offset)
            {
                strhOffset = offset;
            }
    };
    
    // class AviStrl : public Riff::RiffList {
        // private:
            // constexpr const static char *STRL_ID = "strl";
            // std::vector<std::unique_ptr<AviStream>> streams;
        // public:
            // AviStrl() : RiffList(STRL_ID) {}
            // inline AviStream& operator[](size_t index)
            // {
                // return *streams[index];
            // }
            // /*
            // Writes this chunk's header, then for each stream:
                // Marks its offset, writes its header, and increments its own length
            // Then rewrite its own size
            // */
            // virtual void writeTo(std::ostream& stream);
            
            // inline void addStream(AviStream *stream)
            // {
                // streams.push_back(std::unique_ptr<AviStream>(stream));
            // }
    // };
    
    class AviHdrl : public Riff::RiffList {
        private:
            constexpr const static char *HDRL_ID = "hdrl";
            std::vector<std::unique_ptr<AviStream>> streams;
        public:
            AviMainHeader avih;
            // AviStrl strl;
            AviHdrl(const AviMainHeader& avih) : 
                RiffList(HDRL_ID),
                avih {avih} {}
            inline AviStream& operator[](size_t index)
            {
                return *streams[index];
            }
            /*
            Writes the header of this chunk, writes avih, writes strl, then updates its own size
            */
            virtual void writeTo(std::ostream& stream);
            
            inline void addStream(AviStream *stream)
            {
                streams.push_back(std::unique_ptr<AviStream>(stream));
            }
    };
    
    class Avi : public Riff::RiffFile {
        private:
            constexpr const static char *AVI_ID = "AVI ";
            constexpr const static char *MOVI_ID = "movi";
            std::vector<IndexEntry> indexEntries;
            AviHdrl headerList;
            Riff::RiffList moviList;
            size_t moviOffset;
        public:
            Avi(const AviMainHeader& avih) :
                Riff::RiffFile(AVI_ID), 
                headerList(avih),
                moviList(MOVI_ID),
                moviOffset {0} {}
            inline AviStream& operator[](size_t index)
            {
                return headerList[index];
            }
            /*
            Writes the chunk to the stream,
            Pushes the indexentry into the index,
            Updates the stream's length and biggestChunk params
            */
            void writeFrame(
                std::ostream& stream,
                size_t streamNo,
                float seconds, std::uint32_t flags, 
                const std::uint8_t *data, size_t size);
            inline void writeFrame(
                std::ostream& stream,
                size_t streamNo,
                float seconds, std::uint32_t flags, 
                const std::vector<std::uint8_t>& data)
            {
                writeFrame(stream, streamNo, seconds, flags, data.data(), data.size());
            }
            void addStream(AviStream * stream);
            /*
            Writes the file header chunk,
            Then writes the hdrl chunk list
            Then writes the header for the movi list
            */
            void writeBeforeFrames(std::ostream& stream);
            /*
            Rewrites the avih with updated length
            Updates/rewrites the movi list length
            Updates/rewrites the length of every stream
            Writes the idx1 chunk
            Finalizes the file chunk
            */
            void writeAfterFrames(std::ostream& stream);
    };
    
    enum EncodingMode {
        FAST = 0,
        NORMAL = 1,
        CAREFUL = 2,
        FRUGAL = 3,
        ENCODING_MODES = 4
    };
    
    class FlacMjpegAvi : public Avi {
        protected:
            constexpr static const int FLAC_STR = 1;
            constexpr static const int MJPG_STR = 0;
            std::stringstream sstr;
            std::unique_ptr<Flac::Flac> flac;
            std::unique_ptr<Jpeg::Jpeg> jpeg;
            void writeSamples(std::ostream& stream);
        public:
            FlacMjpegAvi(
                int width, int height, float fps,
                int bitsPerSample, float sampleRate, int numChannels,
                EncodingMode mode);
            
            void prepare(std::ostream& stream);
            
            void finish(std::ostream& stream);
            
            void writeVideoFrame(std::ostream& stream, const std::uint8_t *rgb);
            
            template <class T>
            inline void writeSamples(std::ostream& stream, const std::vector<T>& samples)
            {
                *flac << samples;
                writeSamples(stream);
            }
            
            inline void writeVideoFrame(std::ostream& stream, const std::vector<std::uint8_t>& rgb)
            {
                writeVideoFrame(stream, rgb.data());
            }
            
    };
    
    
}

#endif