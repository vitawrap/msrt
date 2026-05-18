#include "platform/windowing.hpp"

#ifdef PLATFORM_NT_MESSAGEBOXES

#include <Windows.h>

namespace ms {
namespace platform {

    void nt_showMessageBox(void* hwnd, char const* title, char const* message, IWindowManager::MessageBoxType mt) {
        int winType = MB_ICONINFORMATION;
        switch (mt) {
            case ms::platform::IWindowManager::MB_WARNING: winType = MB_ICONWARNING; break;
            case ms::platform::IWindowManager::MB_ERROR: winType = MB_ICONERROR; break;
            default: break;
        }
        HWND hWnd = (HWND) hwnd;
        MessageBoxA(hWnd, message, title, MB_OK|MB_APPLMODAL|winType);
    }

}
}

#endif
