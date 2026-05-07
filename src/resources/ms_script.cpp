#include "ms_script.hpp"

namespace ms {
namespace res {
    
    /** TODO: More thorough error management */
    Resource* Script::loadingHandler(class ms::io::File *file) {
        if (file->status() != io::File::OK)
            return nullptr;

        std::string text = file->readString();

        Script* script = new Script;
        script->m_path = file->path();
        script->m_text = text;
        return script;
    }
}
}
