#include "app.hpp"

#include <chrono>
#include <thread>

#include "core/events.hpp"
#include "io/log.hpp"
#include "core/util.hpp"
#include "resources/project.hpp"
#include "resources/resource_manager.hpp"
#include "resources/audio_stream_mp3.hpp"
#include "resources/audio_stream_wav.hpp"

// DEBUG!!!!
#include <raylib.h>

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
            return true;
        }
        return false;
    }

    void Application::unloadProject() {
        m_project.close();

        // TODO: More to do later, like flushing resource cache?
    }

    void Application::init() {
        Thread::setMainThread();
        io::LogDispatcher::get().addStandardOutput();
        m_browser.init();
        m_browser.getScriptHost()->installRuntime();

        // add all supported resource types so far
        PRECACHE_REGISTER_EXT(".wav", res::AudioStreamWAV);
        PRECACHE_REGISTER_EXT(".mp3", res::AudioStreamMP3);
    }

    void Application::free() {
        m_browser.free();
    }

    int Application::run() {
        // at this point the project should be loaded (externally or internally)
        if (!m_project.isOpen()) {
            LOG_MSG("No project loaded in runtime! Aborting.");
            return 1;
        }

        // make sure reentance doesn't leak another instance later with threads
        (void)EventQueue::get();

        if (m_wm.createWindow(m_project.getSettings().title.c_str()) >= 0) {

            // main event loop
            while (! m_wm.closeRequested()) {

                BeginDrawing();
                EndDrawing();

                // browser repaint cycle
                m_browser.processRepaintNotifications();
                m_browser.repaint(&m_wm);

                // flush main thread global event notifications
                EventQueue::get()->flushNotifications();

                // do not overload CPU and give other processes their share
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            m_wm.destroyWindow();
            this->free();
            return 0;
        }
        LOG_MSG("Failed to create app window!");
        return 1;
    }
}
