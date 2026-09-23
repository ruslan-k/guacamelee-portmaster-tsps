#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>

typedef uint32_t gua_sdl_uint32;
typedef int (*gua_sdl_num_joysticks_fn)(void);
typedef int (*gua_sdl_num_gamecontrollers_fn)(void);
typedef gua_sdl_uint32 (*gua_sdl_was_init_fn)(gua_sdl_uint32);
typedef int (*gua_sdl_init_subsystem_fn)(gua_sdl_uint32);

enum {
    GUA_SDL_INIT_JOYSTICK = 0x00000200u,
    GUA_SDL_INIT_GAMECONTROLLER = 0x00002000u
};

static void ensure_controller_subsystems(void)
{
    static gua_sdl_was_init_fn was_init;
    static gua_sdl_init_subsystem_fn init_subsystem;
    if (!was_init)
        was_init = (gua_sdl_was_init_fn)dlsym(RTLD_NEXT, "SDL_WasInit");
    if (!init_subsystem)
        init_subsystem = (gua_sdl_init_subsystem_fn)dlsym(RTLD_NEXT, "SDL_InitSubSystem");
    if (!was_init || !init_subsystem)
        return;

    const gua_sdl_uint32 wanted = GUA_SDL_INIT_JOYSTICK | GUA_SDL_INIT_GAMECONTROLLER;
    const gua_sdl_uint32 active = was_init(0);
    const gua_sdl_uint32 missing = wanted & ~active;
    if (missing)
        (void)init_subsystem(missing);
}

int SDL_NumJoysticks(void)
{
    static gua_sdl_num_joysticks_fn real_fn;
    if (!real_fn)
        real_fn = (gua_sdl_num_joysticks_fn)dlsym(RTLD_NEXT, "SDL_NumJoysticks");
    if (!real_fn)
        return 0;
    ensure_controller_subsystems();
    return real_fn();
}

int SDL_NumGameControllers(void)
{
    static gua_sdl_num_gamecontrollers_fn real_fn;
    if (!real_fn)
        real_fn = (gua_sdl_num_gamecontrollers_fn)dlsym(RTLD_NEXT, "SDL_NumGameControllers");
    if (!real_fn)
        return 0;
    ensure_controller_subsystems();
    return real_fn();
}
