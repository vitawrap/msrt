#pragma once

#include "static_set.hpp"

namespace ms {

    /**
     * @brief Const char string key-specific unordered map
     * fortunately for us, the robin_hood map supports heterogeneous lookup without the need for C++20.
     */
    template <typename V>
    using unordered_map_static = robin_hood::unordered_map<std::string_view, V, StaticStringHash, std::equal_to<>>;

}
