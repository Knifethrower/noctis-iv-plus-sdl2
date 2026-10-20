#include "platform.h"

#include <SDL.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <string>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *frame_tex;  // 320x200, nearest
static SDL_Texture *scaled_tex; // integer multiple of 320x200, linear to the screen
static int scaled_k;

static std::deque<int16_t> keybuf;
static int mouse_dx, mouse_dy;
static uint16_t mouse_buttons;
static uint16_t mouse_pressed; // clicks shorter than a frame still count
static float mouse_scale = 1.0f;

// Test mode (PC checks, x86 vs ARM): NOCTIS_FAKETIME=<unix time> makes the
// clock advance one BIOS tick per frame from that date; NOCTIS_DUMP=<dir> with
// NOCTIS_DUMPFRAMES="<n> ..." writes those frames as PPM and quits after the last.
static bool fake_clock;
static int32_t fake_ticks;
static time_t fake_base;
static int32_t frames;
static const char *dump_dir;
static const char *dump_frames;

void plat_init(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        exit(1);
    }

    Uint32 flags = SDL_WINDOW_RESIZABLE;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--fullscreen")) {
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        }
    }
    // Largest 320x240 multiple that fits the desktop.
    int k = 3;
    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) == 0) {
        k = SDL_min((dm.w - 64) / 320, (dm.h - 96) / 240);
        k = SDL_max(k, 1);
    }
    window = SDL_CreateWindow("Noctis IV Plus", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 320 * k, 240 * k, flags);
    if (!window) {
        SDL_Log("SDL_CreateWindow: %s", SDL_GetError());
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC |
                                      SDL_RENDERER_TARGETTEXTURE);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer: %s", SDL_GetError());
        exit(1);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    frame_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING, 320, 200);

    if (const char *s = getenv("NOCTIS_FAKETIME")) {
        fake_clock = true;
        fake_base  = (time_t) atoll(s);
    }
    dump_dir    = getenv("NOCTIS_DUMP");
    dump_frames = getenv("NOCTIS_DUMPFRAMES");

    if (const char *s = getenv("NOCTIS_MOUSE_SCALE")) {
        mouse_scale = (float) atof(s);
    }
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_ShowCursor(SDL_DISABLE);
}

// Sharp scaling: nearest to the next integer multiple, then linear down to
// the 4:3 picture, so uneven factors (2x2.4 on 640x480) don't band.
static void draw_scaled(const SDL_Rect &dst) {
    int k = (int) ceil(fmax(dst.w / 320.0, dst.h / 200.0));
    if (k < 1) {
        k = 1;
    }
    if (k != scaled_k) {
        if (scaled_tex) {
            SDL_DestroyTexture(scaled_tex);
        }
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        scaled_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_TARGET, 320 * k, 200 * k);
        scaled_k = k;
    }
    if (!scaled_tex || SDL_SetRenderTarget(renderer, scaled_tex) != 0) {
        SDL_RenderCopy(renderer, frame_tex, nullptr, &dst);
        return;
    }
    SDL_RenderCopy(renderer, frame_tex, nullptr, nullptr);
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_RenderCopy(renderer, scaled_tex, nullptr, &dst);
}

static uint8_t last_frame[64000], last_palette[768];

static void dump_frame(const uint8_t *frame, const uint8_t *palette) {
    char want[16];
    snprintf(want, sizeof(want), " %d ", (int) frames);
    std::string list = std::string(" ") + dump_frames + " ";
    if (list.find(want) == std::string::npos) {
        return;
    }

    test_crc("frame", frame, 64000);
    test_crc("palette", palette, 768);

    char name[512];
    snprintf(name, sizeof(name), "%s/f%d.ppm", dump_dir, (int) frames);
    if (FILE *f = fopen(name, "wb")) {
        fprintf(f, "P6\n320 200\n255\n");
        for (int i = 0; i < 64000; i++) {
            const uint8_t *c = palette + frame[i] * 3;
            uint8_t rgb[3] = {(uint8_t) ((c[0] & 63) << 2), (uint8_t) ((c[1] & 63) << 2),
                              (uint8_t) ((c[2] & 63) << 2)};
            fwrite(rgb, 1, 3, f);
        }
        fclose(f);
    }

    size_t last = list.find_last_not_of(' ');
    size_t start = list.find_last_of(' ', last) + 1;
    if (atoi(list.c_str() + start) == frames) {
        exit(0);
    }
}

void plat_present(const uint8_t *frame, const uint8_t *palette) {
    if (frame != last_frame) {
        memcpy(last_frame, frame, sizeof(last_frame));
    }
    memcpy(last_palette, palette, sizeof(last_palette));

    uint32_t argb[256];
    for (int i = 0; i < 256; i++) {
        uint32_t r = palette[i * 3] & 63, g = palette[i * 3 + 1] & 63,
                 b = palette[i * 3 + 2] & 63;
        r = (r << 2) | (r >> 4);
        g = (g << 2) | (g >> 4);
        b = (b << 2) | (b >> 4);
        argb[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }

    void *pixels;
    int pitch;
    if (SDL_LockTexture(frame_tex, nullptr, &pixels, &pitch) == 0) {
        for (int y = 0; y < 200; y++) {
            uint32_t *row = (uint32_t *) ((uint8_t *) pixels + y * pitch);
            const uint8_t *src = frame + y * 320;
            for (int x = 0; x < 320; x++) {
                row[x] = argb[src[x]];
            }
        }
        SDL_UnlockTexture(frame_tex);
    }

    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    SDL_Rect dst;
    if (w * 3 > h * 4) {
        dst.h = h;
        dst.w = h * 4 / 3;
    } else {
        dst.w = w;
        dst.h = w * 3 / 4;
    }
    dst.x = (w - dst.w) / 2;
    dst.y = (h - dst.h) / 2;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    draw_scaled(dst);
    SDL_RenderPresent(renderer);

    frames++;
    if (dump_dir && dump_frames) {
        dump_frame(frame, palette);
    }

    if (fake_clock) {
        fake_ticks++;
        plat_pump();
        return;
    }

    // Loops that never wait for the timer (roof speed) get 4x the DOS rate at most.
    static Uint32 last;
    Uint32 now = SDL_GetTicks();
    if (now - last < 13) {
        SDL_Delay(13 - (now - last));
    }
    last = SDL_GetTicks();

    plat_pump();
}

int32_t bios_clock() {
    if (fake_clock) {
        return fake_ticks;
    }
    return (int32_t) ((uint64_t) SDL_GetTicks() * 1193182ull / 65536000ull);
}

time_t plat_time() {
    if (fake_clock) {
        return fake_base + (time_t) ((int64_t) fake_ticks * 65536 / 1193182);
    }
    return time(nullptr);
}

void wait_tick() {
    if (fake_clock) {
        fake_ticks++;
        return;
    }
    int32_t t = bios_clock();
    while (bios_clock() == t) {
        plat_pump();
        SDL_Delay(1);
    }
}

static void push_ext(int16_t scan) {
    keybuf.push_back(0);
    keybuf.push_back(scan);
}

// US layout, what the DOS keyboard driver gave by default.
static int16_t ascii_for(SDL_Keycode sym, bool shift) {
    if (sym >= SDLK_a && sym <= SDLK_z) {
        return shift ? sym - 32 : sym;
    }
    static const char plain[] = "1234567890-=[];'`\\,./";
    static const char shifted[] = "!@#$%^&*()_+{}:\"~|<>?";
    for (int i = 0; plain[i]; i++) {
        if (sym == plain[i]) {
            return shift ? shifted[i] : plain[i];
        }
    }
    if (sym == SDLK_SPACE) {
        return ' ';
    }
    return -1;
}

static void key_down(const SDL_KeyboardEvent &k) {
    SDL_Keycode sym = k.keysym.sym;
    bool shift = k.keysym.mod & KMOD_SHIFT;
    bool ctrl = k.keysym.mod & KMOD_CTRL;
    bool alt = k.keysym.mod & KMOD_ALT;

    if (alt && (sym == SDLK_RETURN || sym == SDLK_KP_ENTER)) {
        bool fs = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
        SDL_SetWindowFullscreen(window, fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
        return;
    }

    switch (sym) {
    case SDLK_RETURN:
    case SDLK_KP_ENTER: keybuf.push_back(13); return;
    case SDLK_ESCAPE: keybuf.push_back(27); return;
    case SDLK_BACKSPACE: keybuf.push_back(8); return;
    case SDLK_TAB: keybuf.push_back(9); return;
    case SDLK_F1: case SDLK_F2: case SDLK_F3: case SDLK_F4: case SDLK_F5:
    case SDLK_F6: case SDLK_F7: case SDLK_F8: case SDLK_F9: case SDLK_F10:
        push_ext(0x3B + (sym - SDLK_F1));
        return;
    case SDLK_UP: push_ext(72); return;
    case SDLK_DOWN: push_ext(80); return;
    case SDLK_LEFT: push_ext(75); return;
    case SDLK_RIGHT: push_ext(77); return;
    case SDLK_PAGEUP: push_ext(73); return;
    case SDLK_PAGEDOWN: push_ext(81); return;
    case SDLK_HOME: push_ext(71); return;
    case SDLK_END: push_ext(79); return;
    case SDLK_INSERT: push_ext(82); return;
    case SDLK_DELETE: push_ext(83); return;
    case SDLK_KP_PLUS:
        if (ctrl) push_ext(144); else keybuf.push_back('+');
        return;
    case SDLK_KP_MINUS:
        if (ctrl) push_ext(142); else keybuf.push_back('-');
        return;
    case SDLK_KP_MULTIPLY: keybuf.push_back('*'); return;
    case SDLK_KP_DIVIDE: keybuf.push_back('/'); return;
    case SDLK_KP_PERIOD: keybuf.push_back('.'); return;
    default: break;
    }
    if (sym >= SDLK_KP_1 && sym <= SDLK_KP_0) {
        keybuf.push_back(sym == SDLK_KP_0 ? '0' : '1' + (sym - SDLK_KP_1));
        return;
    }
    // Ctrl with the main-row +/- for keyboards without a keypad.
    if (ctrl && sym == SDLK_EQUALS) {
        push_ext(144);
        return;
    }
    if (ctrl && sym == SDLK_MINUS) {
        push_ext(142);
        return;
    }
    if (ctrl || alt) {
        return;
    }
    int16_t a = ascii_for(sym, shift);
    if (a >= 0) {
        keybuf.push_back(a);
    }
}

static void pad_event(const SDL_Event &e);
static void pad_poll();

void plat_pump() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        pad_event(e);
        switch (e.type) {
        case SDL_QUIT: exit(0);
        case SDL_KEYDOWN: key_down(e.key); break;
        case SDL_MOUSEMOTION:
            mouse_dx += e.motion.xrel;
            mouse_dy += e.motion.yrel;
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            uint16_t bit = e.button.button == SDL_BUTTON_LEFT    ? 1
                           : e.button.button == SDL_BUTTON_RIGHT ? 2
                                                                 : 0;
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                mouse_buttons |= bit;
                mouse_pressed |= bit;
            } else {
                mouse_buttons &= ~bit;
            }
            break;
        }
        default: break;
        }
    }
    pad_poll();
}

bool plat_key_ready() {
    plat_pump();
    return !keybuf.empty();
}

int16_t plat_get_key() {
    while (!plat_key_ready()) {
        SDL_Delay(5);
    }
    int16_t k = keybuf.front();
    keybuf.pop_front();
    return k;
}

void plat_mouse(int16_t *dx, int16_t *dy, uint16_t *buttons) {
    plat_pump();
    *dx = (int16_t) lrintf(mouse_dx * mouse_scale);
    *dy = (int16_t) lrintf(mouse_dy * mouse_scale);
    *buttons = mouse_buttons | mouse_pressed;
    mouse_pressed = 0;
    mouse_dx = mouse_dy = 0;
}

void screen_flash(int16_t line_color) {
    if (line_color < 0) {
        memset(last_frame, 0, sizeof(last_frame));
    } else {
        memset(last_frame + 198 * 320 + 1, line_color, 318);
    }
    plat_present(last_frame, last_palette);
}

void test_crc(const char *tag, const void *data, size_t len) {
    if (!dump_dir) {
        return;
    }
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        h = (h ^ ((const uint8_t *) data)[i]) * 16777619u;
    }
    fprintf(stderr, "crc %s %08x\n", tag, h);
}

#include "pad.inc"
