#pragma once

#include <stdint.h>

#include "core/robin_hood.hpp"
#include "resource.hpp"
#include "core/iterable_stack.hpp"
#include "io/file.hpp"
#include "io/log.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <filesystem>
#include <utility>
#include <vector>

namespace ms {
namespace res {

    /**
     * TODO: Strong link with resource, update when the resource is reloaded.
     * The resource manager does not keep a weak ref to the resource, so as
     * to accurately control the lifetime of the resource.
     */
    template <typename TResource>
    struct ResourceHandle {
        std::shared_ptr<Resource> ref;

        ResourceHandle() : ref() {}    // null resource if default constructed.
        ResourceHandle(std::shared_ptr<Resource> ref) : ref(std::move(ref)) {}
        ResourceHandle(TResource* res) : ref(res) {}
        
        operator Resource*() const { return ref.get(); }
        explicit operator TResource*() const { return static_cast<TResource*>(ref.get()); }
        TResource* cast() { return (TResource*)(*this); }
        TResource* operator ->() { return cast(); }
        TResource const* operator ->() const { return (TResource const*)(*this); }
    };

    /**
     * Specialization for opaque resource handle
     */
    template <>
    struct ResourceHandle<Resource> {
        std::shared_ptr<Resource> ref;

        ResourceHandle(std::shared_ptr<Resource> ref) : ref(std::move(ref)) {}
        ResourceHandle(Resource* res) : ref(res) {}
        operator Resource*() const { return ref.get(); }
        Resource* cast() { return (Resource*)(*this); }   // not needed, but soothes mistaken uses

        template <typename T>
        ResourceHandle<T> as() const { return ResourceHandle<T>{std::static_pointer_cast<T>(ref)}; }
    };

    /**
     * Short-hand for type spec and opaque type (ms::res::Handle)
     */
    template <typename T>
    using HandleFor = ResourceHandle<T>;
    using Handle = ResourceHandle<Resource>;

    /**
     * std::hash specialization for std::filesystem::path does not exist
     * in c++11 (clang-11) so this is a more compatible approach.
     */
    struct CPathHashFunctor {
        std::size_t operator() (const std::filesystem::path& p) const noexcept {
            return std::filesystem::hash_value(p);
        }
    };

    /**
     * Resource manager for textures, sounds, models, etc.
     * This should be designed such that resource packs 
     * can change how those are fetched. Fortunately, the beta
     * does not do resource hot-reloading, so we can pass on that as well.
     */
    class ResourceManager {
    public:
        typedef Resource* (*TLoadingHandler) (io::File* file);

    protected:
        static ResourceManager s_manager;

        /**
         * Paths to resource root. (Modified by resource packs)
         * The default asset directory is always at the bottom of the stack.
         */
        CIterableStack<std::string, std::vector<std::string>> m_pathMods;

        /**
         * Resource storage (making use of filesystem::path hash specialization)
         */
        robin_hood::unordered_map<std::filesystem::path, std::shared_ptr<Resource>, CPathHashFunctor> m_resources;

        /**
         * Atomic lock for resource map
         */
        mutable std::mutex m_resourceLock;

        /**
         * Resource handler (ext -> loader function) map
         */
        robin_hood::unordered_map<std::string, TLoadingHandler> m_handlers;

        Resource* createFromHandler(std::string const& path, io::File* fp);
        Resource* loadResourceInternal(char const* filename, std::shared_ptr<Resource>* cached, bool noOverrides);
        void releaseResource(Resource* res);
        void resetPathStack();

        /**
         * Release all resources, regardless of usage metrics.
         * This can only safely be called if the app is no longer
         * making use of my resources.
         */
        void purge();

        template <typename TResource>
        ResourceHandle<TResource> validateResource(Resource* res, char const* filename) {
            TResource* tres = dynamic_cast<TResource*>(res);
            if (tres == nullptr) {
                if (res != nullptr)
                    releaseResource(res);
                LOG_MSGF("Loading resource at \"%s\" failed!\n", filename);
                return ResourceHandle<TResource>{nullptr};
            }
            // Returning inserted shared_ptr should give a clean ref with only 1 grab.
            auto ref = std::shared_ptr<Resource>(res);
            m_resourceLock.lock();
            m_resources.emplace(filename, ref);
            m_resourceLock.unlock();
            ref->initialize(this);
            return ResourceHandle<TResource>{ref};
        }


    public:
        ResourceManager();

        /**
         * Load resource, expecting an output type, which is released
         * instantly if the retrieved type does not match.
         * @param filename Filename (partial)
         * @param noOverrides if true, only test the first path
         */
        template<typename TResource>
        ResourceHandle<TResource> loadResource(char const* filename, bool noOverrides = false)
        {
            std::shared_ptr<Resource> cache(nullptr);
            Resource* res = loadResourceInternal(filename, &cache, noOverrides);
            if (cache) {
                return ResourceHandle<TResource>{cache};
            }
            return validateResource<TResource>(res, filename);
        }

        auto loadResourceOpaque(char const* filename, bool noOverrides = false)
        {
            return loadResource<Resource>(filename, noOverrides);
        }

        /**
         * Load resource from a file already in memory. Again, the
         * memory is released instantly if there is a type mismatch
         * @param file Filename (partial)
         */
        template<typename TResource>
        ResourceHandle<TResource> loadResource(io::File* file)
        {
            auto cached = getCached<TResource>(file->path().c_str());
            if (cached) return cached;

            Resource* res = createFromHandler(file->path().c_str(), file);
            return validateResource<TResource>(res, file->path().c_str());
        }

        auto loadResourceOpaque(io::File* file)
        {
            return loadResource<Resource>(file);
        }

        /**
         * Check if a resource is loaded/cached for this path.
         */
        bool isCached(char const* filename);

        /**
         * Get cached resource at path (null ptr if unknown)
         */
        template<typename TResource>
        ResourceHandle<TResource> getCached(char const* filename) const
        {
            // Try finding cached resource with this path (as atomic operation)
            m_resourceLock.lock();
            auto const res = m_resources.find(filename);
            m_resourceLock.unlock();
            if (res != m_resources.end()) {
                return ResourceHandle<TResource>{(*res).second};
            }
            return ResourceHandle<TResource>{nullptr};
        }

        /**
         * Register a new resource in this manager under an arbitrary path
         */
        template<typename TResource>
        ResourceHandle<TResource> cacheResource(char const* filename, Resource* res) {
            if (isCached(filename)) {
                LOG_MSGF("Can't overwrite cached resource at \"%s\"!\n", filename);
                return ResourceHandle<TResource>{nullptr};
            }
            // Returning inserted shared_ptr should give a clean ref with only 1 grab.
            auto ref = std::shared_ptr<Resource>(res);
            m_resourceLock.lock();
            m_resources.emplace(filename, ref);
            m_resourceLock.unlock();
            return ResourceHandle<TResource>{ref};
        }

        /**
         * Add handler for resource extension.
         * Ext chains are allowed: .tar.gz, .geo.json...
         */
        void addHandler(char const* ext, TLoadingHandler handler);

        /**
         * Purge all currently unused resources.
         */
        void purgeUnused();

        /**
         * Read a file manifest and add all lines in cache.
         * The file is a simple nl/crnl separated list of paths.
         */
        bool precache(char const* manifestFilename);

        /**
         * Get global manager instance
         */
        static ResourceManager* get() { return &s_manager; }
    };

}
}

/* Macros */

// Shortcut to load resource with handler from global resource manager.
#define PRECACHE_FILE(rtype, file) ms::res::ResourceManager::get()->loadResource<rtype>(file)

// Shortcut to load resource with handler from global resource manager, without applying path modifiers.
#define PRECACHE_BASE_FILE(rtype, file) ms::res::ResourceManager::get()->loadResource<rtype>(file, true)

// Shortcut to register a loader for an ext and resource type
#define PRECACHE_REGISTER_EXT(extstr, type) ms::res::ResourceManager::get()->addHandler(extstr, type ::loadingHandler)
