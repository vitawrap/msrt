#ifndef PLATFORM_NO_MAIN
#include "platform/abstracted/windowing.hpp"
#include "platform/platform.hpp"
#include "core/app.hpp"

#ifdef PLATFORM_NT
#include "platform/nt/nt_utils.hpp"

struct DeferFree {
    void* m_ptr;
    DeferFree(void* ptr) : m_ptr(ptr) {}
    ~DeferFree() { ::free(m_ptr); }
};

// Windows needs a specific entry point to treat the app as a GUI app (with no console)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
    int argc = 0;
    char** argv = nullptr;
    ms::nt::getArgvUTF8 (pCmdLine, &argc, &argv);
    DeferFree _((void*)argv);
#else
int main(int argc, char* argv[])
#endif
{
    auto* app = ms::Application::get();
    try {
    
        if (argc > 1) {
            app->loadProject(argv[1]);
        }
        int code = app->run();
        return code;
    } catch (std::runtime_error const& err) {
        auto* wm = app->getWindowManager();
        wm->showMessageBox("Error", err.what(), ms::platform::WindowManager::MB_ERROR);
    }
}

#endif
