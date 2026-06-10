#pragma once

#include <string>
#include <string_view>
#include "robin_hood.hpp"

namespace ms {

    struct StaticStringHash {
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
     * @brief Static string hash set
     * Hash set with heterogeneous lookup for static strings (no storage).
     */
    using unordered_set_static = robin_hood::unordered_set<char const*, StaticStringHash, std::equal_to<>>;

}
