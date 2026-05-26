#pragma once

#include <string>
#include <string_view>
#include "robin_hood.hpp"

namespace ms {

    struct StringHash {
        using is_transparent = void;
        size_t operator()(const char *txt) const {
            return std::hash<std::string_view>{}(txt);
        }
        size_t operator()(std::string_view txt) const {
            return std::hash<std::string_view>{}(txt);
        }
        size_t operator()(const std::string &txt) const {
            return std::hash<std::string>{}(txt);
        }
    };

    /**
     * @brief String key-specific unordered map
     * fortunately for us, the robin_hood map supports heterogeneous lookup without the need for C++20.
     */
    template <typename V>
    using unordered_map = robin_hood::unordered_map<std::string, V, StringHash, std::equal_to<>>;

}
