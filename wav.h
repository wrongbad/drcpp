#pragma once

#include "dr_libs/dr_wav.h"
#include <vector>

namespace drcpp {

struct wav
{   // index like: samples[chan_idx + frame_idx * channels]
    std::vector<float> samples;
    int sample_rate = 0;
    int channels = 0;
};

template<class Stream>
Stream & operator>>(Stream & in, wav & audiodata)
{
    auto read = +[] (void * ctx, void * buf, size_t count) {
        Stream & in = *(Stream*)ctx;
        in.read((char*)buf, count);
        return (size_t)in.gcount();
    };
    auto seek = +[] (void * ctx, int off, drwav_seek_origin drway) {
        Stream & in = *(Stream*)ctx;
        std::ios_base::seekdir way = (drway == drwav_seek_origin_start) ?
            std::ios_base::beg : std::ios_base::cur;
        in.seekg(off, way);
        return (uint32_t)(bool)in;
    };
    drwav reader;
    if(!drwav_init_ex(&reader, read, seek, nullptr, (void*)&in,
        nullptr, DRWAV_SEQUENTIAL, nullptr))
    {   in.setstate(std::ios::failbit);
        return in;
    }
    audiodata.frameSize = reader.channels;
    audiodata.frameRate = reader.sampleRate;
    size_t frames = reader.totalPCMFrameCount;
    audiodata.samples.resize(frames * audiodata.frameSize);

    frames = drwav_read_pcm_frames_f32(&reader, frames, audiodata.samples.data());
    audiodata.samples.resize(frames * audiodata.frameSize); // handle truncated files

    drwav_uninit(&reader);

    return in;
}

template<class Stream>
Stream & operator<<(Stream & out, wav const& audiodata)
{
    auto write = +[] (void * ctx, void const* buf, size_t count) {
        Stream & out = *(Stream*)ctx;
        out.write((char*)buf, count);
        return bool(out) ? count : size_t(0); // approx
    };
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = audiodata.frameSize;
    format.sampleRate = audiodata.frameRate;
    format.bitsPerSample = 32;
    drwav writer;
    if(!drwav_init_write_sequential(&writer, &format,
        audiodata.samples.size(), write, (void*)&out, nullptr))
    {   out.setstate(std::ios::failbit);
        return out;
    }
    drwav_write_pcm_frames(&writer,
        audiodata.samples.size()/audiodata.frameSize,
        audiodata.samples.data());
    drwav_uninit(&writer);
    return out;
}

} // namespace dr
