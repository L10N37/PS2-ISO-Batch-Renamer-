#define SDL_MAIN_HANDLED
#include "app/App.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

int runApplication() {
    SDL_SetMainReady();
    ps2br::App app;
    if (!app.initialize()) {
        return 1;
    }
    return app.run();
}

} // namespace

#if defined(_WIN32)
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    return runApplication();
}
#else
int main() {
    return runApplication();
}
#endif
