#include "windowing.hpp"
#include "core/util.hpp"
#include "platform/windowing.hpp"
#include <raylib.h>

#ifdef PLATFORM_UNIX_MESSAGEBOXES
/* On unix platforms, try to get GTK to show message boxes. */
#include <gtk/gtk.h>
static bool unixMessageBoxInitialized = false;
#elif PLATFORM_NT_MESSAGEBOXES
#include <Windows.h>
#endif

namespace ms {
namespace platform {

    WindowManager::WindowManager() :
        m_hasWindow(false),
        m_windowSelected(false)
    {
#ifdef PLATFORM_UNIX_MESSAGEBOXES
        unixMessageBoxInitialized = gtk_init_check(0, nullptr);
#endif
    }

    int WindowManager::createWindow(char const* title) {
        if (!m_hasWindow) {
            InitWindow(640, 480, title);
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

    void WindowManager::setWindowSize(int w, int h) {
        if (m_windowSelected) {
            SetWindowSize(w, h);
        }
    }

    void WindowManager::destroyWindow() {
        if (m_hasWindow && m_windowSelected) {
            selectWindow(-1);
            CloseWindow();
        }
    }

    bool WindowManager::closeRequested() const {
        return WindowShouldClose();
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
        int winType = MB_ICONINFORMATION;
        switch (type) {
            case MB_WARNING: winType = MB_ICONWARNING; break;
            case MB_ERROR: winType = MB_ICONERROR; break;
            default: break;
        }
        HANDLE hWnd = (HANDLE) GetWindowHandle();
        MessageBoxA(hWnd, message, title, MB_OK|MB_APPLMODAL|winType);
#endif
    }

}
}
