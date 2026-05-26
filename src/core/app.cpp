#include "app.hpp"

#include <chrono>
#include <thread>
#include <ms/core/util.hpp>

#include "core/events.hpp"
#include "io/log.hpp"
#include "resources/project.hpp"
#include "resources/resource_manager.hpp"
#include "resources/audio_stream_mp3.hpp"
#include "resources/audio_stream_wav.hpp"

namespace ms {

    /** Implement util */

    namespace Time {
        static auto ProcessStart = std::chrono::steady_clock::now();
        static double CurrentTime = 0.0;

        double now() {
            const auto end = std::chrono::steady_clock::now();
            return (end - ProcessStart).count();
        }

        double frameNow() {
            return CurrentTime;
        }

        void updateFrameNow() {
            CurrentTime = now();
        }
    }

    namespace Thread {
        thread_local volatile bool main;

        bool isMainThread() { return main; }
        void setMainThread() { main = true; }
    }

    /** App */

    Application* Application::s_instance = nullptr;

    bool Application::loadProject(char const* filename) {
        unloadProject();

        if (m_project.open(filename)) {
            LOG_MSGF("opened project %s\n", filename);
            LOG_MSGF("project name: %s\n", m_project.getSettings().title.c_str());

            // do project setup tasks
            m_project.buildAtlas();
            return true;
        }
        return false;
    }

    void Application::unloadProject() {
        m_project.close();

        // TODO: More to do later, like flushing resource cache?
    }

    void Application::init() {
        m_programExit = false;
        Thread::setMainThread();
        io::LogDispatcher::get().addStandardOutput();

        // add all supported resource types so far
        PRECACHE_REGISTER_EXT(".ms", res::Script);
        PRECACHE_REGISTER_EXT(".png", res::Image);
        PRECACHE_REGISTER_EXT(".wav", res::AudioStreamWAV);
        PRECACHE_REGISTER_EXT(".mp3", res::AudioStreamMP3);
    }

    void Application::free() {
        m_browser.free();
    }

    void Application::setWindowIcon() {
        auto icon = m_project.getIcon();
        if (icon)
            m_wm.setWindowIcon(icon.operator->());
    }

    int Application::run() {
        // at this point the project should be loaded (externally or internally)
        if (!m_project.isOpen()) {
            m_wm.showMessageBox("microStudioRT Error",
                "No project loaded in runtime! Usage:\n$ msrt <path-to-zip-export>", platform::IWindowManager::MB_ERROR);
            LOG_MSG("No project loaded in runtime! Aborting.\n");
            return 1;
        }

        // make sure reentance doesn't leak another instance later with threads
        (void)EventQueue::get();

        if (m_wm.createWindow(m_project.getSettings().title.c_str()) >= 0) {
            // set icon (createWindow auto-selects the new window)
            setWindowIcon();

            // start up browser and script layer
            m_browser.init();
            m_browser.getScriptHost()->installRuntime(m_project.getLanguageEnum());
            m_browser.getScriptHost()->onLoaded();

            // main event loop
            while (! m_wm.closeRequested()) {
                if (m_programExit) break;

                // poll window manager events
                m_wm.pollWindowEvents();

                // poll input events
                m_input.pollEvents();

                // run all async script jobs
                m_browser.getScriptHost()->flushJobs();

                // browser repaint cycle
                m_browser.setupRepaint(&m_wm);
                m_browser.processRepaintNotifications();
                m_browser.repaint();

                // flush main thread global event notifications
                EventQueue::get()->flushNotifications();

                // do not overload CPU and give other processes their share
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            m_wm.destroyWindow();
            unloadProject();
            this->free();
            return 0;
        }
        LOG_MSG("Failed to create app window!");
        return 1;
    }

    void Application::quit() {
        m_programExit = true;
    }
}
