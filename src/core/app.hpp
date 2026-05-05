#pragma once

#include "browser/context.hpp"
#include "platform/windowing.hpp"
#include "resources/project.hpp"

namespace ms {

    /**
     * @brief Microstudio native device singleton
     * This is the root class of the program structure.
     */
    class Application {
        static Application* s_instance;
        
        res::Project m_project;
        browser::Context m_browser;
        platform::WindowManager m_wm;
        bool m_programExit;

        void init();

        void free();

        Application() {
            init();
        }
    public:
        platform::WindowManager* getWindowManager() { return &m_wm; }

        /**
         * @brief Load project from file
         * @param filename The path to the archive
         * @return Whether loading succeeded or failed
         */
        bool loadProject(char const* filename);

        /**
         * @brief Free all resources tied to a project
         * Unload a project and all the assets/scripts it owns.
         */
        void unloadProject();

        res::Project const* getProject() const { return &m_project; }
        res::Project* getProject() { return &m_project; }

        browser::Context const* getBrowserContext() const { return &m_browser; }
        browser::Context* getBrowserContext() { return &m_browser; }

        /**
         * @brief Application lifecycle
         * Step simulation, process and dispatch events, render, etc.
         */
        int run();

        /**
         * @brief Request to quit
         * Send the application a request to soft quit
         */
        void quit();

        /** Get global application pointer */
        static Application* get() {
            if (!s_instance)
                s_instance = new Application();
            return s_instance;
        }
    };

}
