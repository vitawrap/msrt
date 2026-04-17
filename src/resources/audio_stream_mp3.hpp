#pragma once

#include "audio_stream.hpp"
#include "io/file.hpp"

namespace ms {
namespace res {

    class AudioStreamMP3 : public AudioStream {
    public:
        // TODO
        DECLARE_LOADER { return nullptr; }
    };

}
}
