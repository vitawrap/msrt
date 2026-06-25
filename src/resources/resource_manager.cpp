#include "resource_manager.hpp"
#include <ms/core/util.hpp>
#include "resource.hpp"
#include <stdio.h>
#include <string.h>
#include <filesystem>
#include <string>
#include <vector>

namespace ms {
namespace res {

    ResourceManager ResourceManager::s_manager{};

    ResourceManager::ResourceManager()
    {
        resetPathStack();
    }

    Resource* ResourceManager::createFromHandler(std::string const& path, io::File* fp) {
        // small workaround because std::filesystem::path only retrieves the last extension component.
        // Yes, this currently also catches dotfiles, that'll also need fixing. :)
        auto const name = path;
        auto ext = name.rfind('.');
        auto const& handler = m_handlers.find(ext == std::string::npos? "" : name.substr(ext));
        if (handler == m_handlers.end())
            return nullptr; // can't continue here, if we don't have a handler it's really bad.

        return handler->second(fp);   // note: the handler still has chances to fail and return null here.
    }

    Resource* ResourceManager::loadResourceInternal(const char *filename, std::shared_ptr<Resource>* cached, bool noOverrides) {
        // Try finding cached resource with this path (as atomic operation)
        m_resourceLock.lock();
        auto const res = m_resources.find(filename);
        m_resourceLock.unlock();
        if (res != m_resources.end() && cached)
        {
            *cached = res->second;
            return nullptr;
        }
        
        // Not cached, search for it
        std::vector<std::string> first;
        auto const& cont = m_pathMods.container(); // non-standard
        if (noOverrides)
            first.push_back(cont[0]);
        auto const& vect = noOverrides? first : cont;
        for (auto itr = vect.crbegin(); itr != vect.crend(); itr++)
        {
            std::filesystem::path assetPath(*itr);
            assetPath.append(filename);

            // do sanity checks before trying to load from disk
            if (!assetPath.has_extension()) {
                LOG_MSGF("Path %s has no extension, can't determine resource type!\n", assetPath.c_str());
                return nullptr;
            }

            // If the file doesn't exist, just bail and try next path
            auto const pathString = assetPath.u8string();
            if (io::FileSystem::exists(reinterpret_cast<const char*>(pathString.data())))
            {
                auto fp = io::FileSystem::openForRead(reinterpret_cast<const char*>(pathString.data()));
                if (!fp.get())
                    continue;
                    
                return createFromHandler(assetPath.filename().string(), fp.get());
            }
        }
        return nullptr; // None of the path modifiers matched a valid file...
    }

    bool ResourceManager::isCached(char const* filename) {
        return getCached<Resource>(filename) != nullptr;
    }

    void ResourceManager::releaseResource(Resource* res) {
        // so far that's all we need...
        delete res;
    }

    void ResourceManager::resetPathStack() {
        while (!m_pathMods.empty())
            m_pathMods.pop();
        m_pathMods.push(".");
    }

    void ResourceManager::purgeUnused() {
        std::scoped_lock<std::recursive_mutex> lock(m_resourceLock);

        // All "unused" resources should have a single grab
        // so just auto-kill them by destroying last ref.
        auto it = m_resources.begin();
        while (it != m_resources.end()) {
            if (it->second.use_count() == 1)
            {
                it = m_resources.erase(it);
                continue;
            }
            ++it;
        }
    }

    void ResourceManager::addHandler(const char *_ext, TLoadingHandler handler) {
        DEBUG_ASSERT(_ext);
        char fixExt[256]{}; // fix ext to ideal format for std::filesystem::path
        if (_ext[0] != '.')
        {
            fixExt[0] = '.';
            strncpy(fixExt + 1, _ext, 253);
        }
        m_handlers.emplace(fixExt[0]? fixExt : _ext, handler);
    }

    bool ResourceManager::precache(const char *manifestFilename) {
        io::TFile file = io::FileSystem::openForRead(manifestFilename);
        if (file->status() != io::File::OK)
            return false;

        char line[1024];
        while (! file->eof())
        {
            size_t len = file->readLine(line, 1024);
            if (line[len-2] == '\r')        line[len-2] = 0;
            else if(line[len-1] == '\n')    line[len-1] = 0;
            auto handle = loadResource<Resource>(line);
        }
        return true;
    }

}
}