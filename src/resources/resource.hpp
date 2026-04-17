#pragma once

namespace ms {
namespace res {

    class ResourceManager;

    /*!
    * @see ResourceManager::TLoadingHandler
    */
    #define DECLARE_LOADER static Resource* loadingHandler(class ms::io::File* file)

    /**
     * A resource is a managed object whose data is tied to a filesystem path.
     */
    class Resource {
    protected:
        friend ResourceManager;
        ResourceManager* m_manager;

        /**
         * Call performed by resource manager after being cached
         */
        virtual void initializeResource() {}

        void initialize(ResourceManager* man) {
            m_manager = man;
            initializeResource();
        }

    public:
        ResourceManager* getManager() const { return m_manager; }

        virtual ~Resource() { /* TODO */ }
    };

}
}