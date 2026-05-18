#pragma once

#include "../windowing.hpp"

namespace ms {
namespace platform {

    /**
     * @brief Concrete window manager for abstract platform
     * This windowing manager uses raylib under the hood.
     */
    class WindowManager : public virtual IWindowManager {
        bool m_hasWindow;
        bool m_windowSelected; // bool here because raylib only supports one window
        int m_windowWidth, m_windowHeight;
    
    public:
        WindowManager();

        void pollWindowEvents() override;
        int getMaxSupportedWindows() override { return 1; }
        int getWindowCount() const override { return m_hasWindow; }
        bool selectWindow(int i) override { return m_windowSelected = (i == 0); }

        int createWindow(char const* title) override;
        void setWindowSize(int w, int h) override;
        void destroyWindow() override;

        int getWindowWidth() const override;
        int getWindowHeight() const override;

        bool closeRequested() const override;

        void showMessageBox(char const* title, char const* message, MessageBoxType type) override;
    };

} // namespace platform
}

#ifdef PLATFORM_NT_MESSAGEBOXES
namespace ms { namespace platform {
    extern void nt_showMessageBox(void* hwnd, char const* title, char const* message, IWindowManager::MessageBoxType);
}}
#endif
