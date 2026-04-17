#pragma once

#include "audio_stream.hpp"
#include "io/file.hpp"

namespace ms {
namespace res {

    class AudioStreamWAV : public AudioStream {
    public:
        // TODO
        DECLARE_LOADER { return nullptr; }
    };

}
}
