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

#if 0
#include <raylib.h>

#include <quickjs.h>

#include <minizip/zip.h>

//#define MINIMP3_ONLY_MP3
//#define MINIMP3_ONLY_SIMD
//#define MINIMP3_NO_SIMD
//#define MINIMP3_NONSTANDARD_BUT_LOGICAL
//#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include <minimp3/minimp3.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>

int main (int argc, char* argv[]) {
    static mp3dec_t mp3d;
    mp3dec_init(&mp3d);

    InitWindow(800, 450, "raylib");

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("This runs!", 200, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
#endif // Test code
