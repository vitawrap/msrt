
#include <ms/core/util.hpp>
#include "resources/ms_image.hpp"
#include "platform/abstracted/windowing.hpp"
#include "platform/windowing.hpp"
#include <raylib.h>

#ifdef PLATFORM_UNIX_MESSAGEBOXES
/* On unix platforms, try to get GTK to show message boxes. */
#include <gtk/gtk.h>
static bool unixMessageBoxInitialized = false;
#endif

namespace ms {
namespace platform {

    WindowManager::WindowManager() :
        m_hasWindow(false),
        m_windowSelected(false),
        m_windowWidth(640),
        m_windowHeight(480)
    {
#ifdef PLATFORM_UNIX_MESSAGEBOXES
        unixMessageBoxInitialized = gtk_init_check(0, nullptr);
#endif
    }

    int WindowManager::createWindow(char const* title) {
        if (!m_hasWindow) {
            InitWindow(m_windowWidth, m_windowHeight, title);
            m_hasWindow = IsWindowReady();
            if (m_hasWindow) {
                SetWindowFocused();
                selectWindow(0);
                return 0;
            }
            return -1;
        }
        return 0;
    }

    void WindowManager::setWindowIcon(res::Image const* image) {
        if (m_windowSelected) {
            ::Image* rImage = reinterpret_cast<::Image*>(image->getPlatformImage());
            if (IsImageValid(*rImage))
                SetWindowIcon(*rImage);
        }
    }

    void WindowManager::setWindowSize(int w, int h, bool invokeEvent) {
        m_windowWidth = w;
        m_windowHeight = h;
        if (m_windowSelected) {
            SetWindowSize(w, h);
            if (invokeEvent)
                resized.invoke(0, w, h);
        }
    }

    void WindowManager::destroyWindow() {
        if (m_hasWindow && m_windowSelected) {
            selectWindow(-1);
            CloseWindow();
        }
    }

    int WindowManager::getWindowWidth() const {
        return m_windowWidth;
    }

    int WindowManager::getWindowHeight() const {
        return m_windowHeight;
    }

    bool WindowManager::closeRequested() const {
        return WindowShouldClose();
    }

    void WindowManager::pollWindowEvents() {
        // window resized last frame
        if (IsWindowResized()) {
            m_windowWidth = GetScreenWidth();
            m_windowHeight = GetScreenHeight();
            resized.invoke(0, m_windowWidth, m_windowHeight);
        }
    }

    void WindowManager::showMessageBox(char const* title, char const* message, MessageBoxType type) {
#ifdef PLATFORM_UNIX_MESSAGEBOXES
        DEBUG_ASSERT(unixMessageBoxInitialized);
        int flags = GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL;
        int gtk_type = GTK_MESSAGE_INFO;
        switch (type) {
            case MB_WARNING: gtk_type = GTK_MESSAGE_WARNING; break;
            case MB_ERROR: gtk_type = GTK_MESSAGE_ERROR; break;
            default: break;
        }
        auto* dialog = gtk_message_dialog_new (nullptr,
                                        (GtkDialogFlags) flags,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        "%s", message);
        gtk_window_set_title(GTK_WINDOW(dialog), title);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        // gtk needs its main context to run for the dialog to disappear
        while (g_main_context_iteration(nullptr, false));
#elif PLATFORM_NT_MESSAGEBOXES
        nt_showMessageBox(GetWindowHandle(), title, message, type);
#endif
    }

}
}
