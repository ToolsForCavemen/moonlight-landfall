#include "landfalldrop.h"
#include "streamutils.h"

#include <QtGlobal>
#include <QByteArray>
#include <QStringList>
#include <QUrl>

#include <atomic>
#include <cstdio>
#include <cstring>

#ifdef Q_OS_WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#ifdef Q_OS_DARWIN
#include <mach-o/dyld.h>
#endif

namespace {

// SDL2 drop events have no position, but the SDL3 underneath sdl2-compat has it
// (on X11, Wayland, Windows and macOS). These are the fields of SDL3's
// SDL_DropEvent we need, read through an SDL3 event watch.
struct Sdl3DropEvent
{
    Uint32 type;
    Uint32 reserved;
    Uint64 timestamp;
    Uint32 windowID;
    float x;
    float y;
    const char* source;
    const char* data;
};
const Uint32 SDL3_EVENT_DROP_POSITION = 0x1004;

typedef bool (SDLCALL *Sdl3EventFilter)(void* userdata, void* event);
typedef bool (SDLCALL *Sdl3AddEventWatch)(Sdl3EventFilter filter, void* userdata);
typedef void (SDLCALL *Sdl3RemoveEventWatch)(Sdl3EventFilter filter, void* userdata);

std::atomic<bool> s_HavePosition(false);
std::atomic<float> s_X(0), s_Y(0);
std::atomic<bool> s_Watching(false);
QStringList s_Files;

bool SDLCALL watchDrops(void*, void* event)
{
    auto drop = static_cast<const Sdl3DropEvent*>(event);
    if (drop->type == SDL3_EVENT_DROP_POSITION) {
        s_X = drop->x;
        s_Y = drop->y;
        s_HavePosition = true;
    }
    return true;
}

// Looks up a function in the SDL3 library that sdl2-compat already loaded.
void* sdl3Function(const char* name)
{
#if defined(Q_OS_WIN32)
    HMODULE module = GetModuleHandleW(L"SDL3.dll");
    return module ? reinterpret_cast<void*>(GetProcAddress(module, name)) : nullptr;
#else
    void* library = nullptr;
#if defined(Q_OS_DARWIN)
    for (uint32_t i = 0; i < _dyld_image_count() && library == nullptr; i++) {
        const char* image = _dyld_get_image_name(i);
        if (image != nullptr && strstr(image, "SDL3") != nullptr && strstr(image, "SDL3_") == nullptr) {
            library = dlopen(image, RTLD_LAZY | RTLD_NOLOAD);
        }
    }
#else
    library = dlopen("libSDL3.so.0", RTLD_LAZY | RTLD_NOLOAD);
#endif
    return library ? dlsym(library, name) : nullptr;
#endif
}

void report(SDL_Window* window, int streamWidth, int streamHeight)
{
    float x, y;
    if (s_HavePosition) {
        x = s_X;
        y = s_Y;
    }
    else {
        // No SDL3 underneath: use where the pointer is now
        int globalX, globalY, windowX, windowY;
        SDL_GetGlobalMouseState(&globalX, &globalY);
        SDL_GetWindowPosition(window, &windowX, &windowY);
        x = globalX - windowX;
        y = globalY - windowY;
    }

    // Same video region as absolute mouse mode (SdlInputHandler::handleMouseMotionEvent)
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    SDL_Rect src, dst;
    src.x = src.y = 0;
    src.w = streamWidth;
    src.h = streamHeight;
    dst.x = dst.y = 0;
    dst.w = windowWidth;
    dst.h = windowHeight;
    StreamUtils::scaleSourceToDestinationSurface(&src, &dst);

    int fx = qBound(0, (int)((x - dst.x) * 1000 / qMax(dst.w, 1)), 1000);
    int fy = qBound(0, (int)((y - dst.y) * 1000 / qMax(dst.h, 1)), 1000);

    QByteArray line = "LANDFALL-SOLTAR " + QByteArray::number(fx) + " " + QByteArray::number(fy);
    for (const QString& file : s_Files) {
        line += " " + QUrl::toPercentEncoding(file);
    }
    line += "\n";
    fwrite(line.constData(), 1, line.size(), stderr);
    fflush(stderr);
}

}

void LandfallDrop::start()
{
    s_Files.clear();
    s_HavePosition = false;
    auto addEventWatch = reinterpret_cast<Sdl3AddEventWatch>(sdl3Function("SDL_AddEventWatch"));
    if (addEventWatch != nullptr && addEventWatch(watchDrops, nullptr)) {
        s_Watching = true;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Landfall: drops %s", s_Watching ? "with position" : "at pointer");
}

void LandfallDrop::stop()
{
    if (s_Watching) {
        auto removeEventWatch = reinterpret_cast<Sdl3RemoveEventWatch>(sdl3Function("SDL_RemoveEventWatch"));
        if (removeEventWatch != nullptr) {
            removeEventWatch(watchDrops, nullptr);
        }
        s_Watching = false;
    }
}

bool LandfallDrop::handleEvent(SDL_Event* event, SDL_Window* window, int streamWidth, int streamHeight)
{
    switch (event->type) {
    case SDL_DROPBEGIN:
        s_Files.clear();
        return true;
    case SDL_DROPFILE:
        if (event->drop.file != nullptr) {
            s_Files << QString::fromUtf8(event->drop.file);
        }
        SDL_free(event->drop.file);
        return true;
    case SDL_DROPTEXT:
        SDL_free(event->drop.file);
        return true;
    case SDL_DROPCOMPLETE:
        if (!s_Files.isEmpty()) {
            report(window, streamWidth, streamHeight);
        }
        s_Files.clear();
        s_HavePosition = false;
        return true;
    default:
        return false;
    }
}
