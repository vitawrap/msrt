#pragma once

#include "platform.hpp"

namespace ms {
namespace platform {

    class IWindowManager {
    public:
        enum MessageBoxType {
            MB_INFO,
            MB_WARNING,
            MB_ERROR
        };

        /** Must be available from implementers */
        IWindowManager() {}

        /** Get max supported windows for this implementation. */
        virtual int getMaxSupportedWindows() { return 0xFFFFFFFF; }

        /** Get number of windows managed by this instance. */
        virtual int getWindowCount() const = 0;
        
        /** Select a window on which to run subsequent commands. */
        virtual bool selectWindow(int i) = 0;

        /**
         * Create and select a new window in this manager.
         * The windowing manager shall take care of the render context.
         * @returns The new window's index
         */
        virtual int createWindow(char const* title = "") = 0;

        // TODO: void setWindowIcon(ms::graphics::Image& const image);

        /**
         * Set the selected window's client size
         */
        virtual void setWindowSize(int w, int h) = 0;

        virtual int getWindowWidth() const = 0;
        virtual int getWindowHeight() const = 0;

        /** Destroy currently selected window. */
        virtual void destroyWindow() = 0;

        /** If whatever main window in this manager decides the app needs to close */
        virtual bool closeRequested() const = 0;

        /**
         * Show a blocking OS message box
         */
        virtual void showMessageBox(char const* title, char const* message, MessageBoxType type = MB_INFO) = 0;
    };

}
}

#if __has_include("abstracted/windowing.hpp")
#include "abstracted/windowing.hpp"
#elif defined(PLATFORM_UNIX)
#include "unix/windowing.hpp"
#elif defined(PLATFORM_NT)
#include "nt/windowing.hpp"
#elif defined(PLATFORM_DARWIN)
#include "darwin/windowing.hpp"
#elif defined(PLATFORM_BROWSER)
#include "browser/windowing.hpp"
#else
#error "TODO: Windowing for this platform"
#endif
