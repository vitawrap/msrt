#pragma once

#include "resources/resource.hpp"
#include "io/file.hpp"

namespace ms {
namespace res {

    class Script : public Resource {
        std::string m_path;
        std::string m_text;
    public:

        /** Get path inside project archive of script */
        std::string const& getPath() const { return m_path; }

        /** Get text within script */
        std::string const& getText() const { return m_text; }

        DECLARE_LOADER;
    };

}
}
