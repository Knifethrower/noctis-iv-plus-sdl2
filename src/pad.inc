// --- Gamepad (included by platform.cpp) -------------------------------------
// Native SDL game controller: sticks go to the game loops (plat_pad_sticks),
// buttons become the keys and mouse buttons of the original. Select opens an
// on-screen keyboard with every key the game uses.

static SDL_GameController *pad;
static int pad_ctx = PAD_SPACE, pad_menu = 1;
static bool pad_used;

struct OskKey {
    const char *label;
    int16_t code; // ASCII, or 0x100 + scan code for extended keys
};
static const OskKey osk_keys[OSK_COLS * OSK_ROWS] = {
    {"1", '1'}, {"2", '2'}, {"3", '3'}, {"4", '4'}, {"5", '5'},
    {"6", '6'}, {"7", '7'}, {"8", '8'}, {"9", '9'}, {"0", '0'},
    {"Q", 'q'}, {"W", 'w'}, {"E", 'e'}, {"R", 'r'}, {"T", 't'},
    {"Y", 'y'}, {"U", 'u'}, {"I", 'i'}, {"O", 'o'}, {"P", 'p'},
    {"A", 'a'}, {"S", 's'}, {"D", 'd'}, {"F", 'f'}, {"G", 'g'},
    {"H", 'h'}, {"J", 'j'}, {"K", 'k'}, {"L", 'l'}, {"ENT", 13},
    {"Z", 'z'}, {"X", 'x'}, {"C", 'c'}, {"V", 'v'}, {"B", 'b'},
    {"N", 'n'}, {"M", 'm'}, {".", '.'}, {"/", '/'}, {"BKS", 8},
    {"SPC", ' '}, {"-", '-'}, {"+", '+'}, {"*", '*'}, {":", ':'},
    {",", ','}, {"ESC", 27}, {"TAB", 9}, {"PGU", 0x100 + 73}, {"PGD", 0x100 + 81},
    {"F1", 0x100 + 0x3B}, {"F2", 0x100 + 0x3C}, {"F3", 0x100 + 0x3D}, {"C+", 0x100 + 144},
    {"C-", 0x100 + 142}, {"UP", 0x100 + 72}, {"DN", 0x100 + 80}, {"LT", 0x100 + 75},
    {"RT", 0x100 + 77}, {"DEL", 0x100 + 83},
};
static int osk_cursor = -1; // -1 = closed

// Held buttons repeat like a held key (BIOS typematic: 500 ms, then ~11/s).
static Uint32 held_since[SDL_CONTROLLER_BUTTON_MAX], held_next[SDL_CONTROLLER_BUTTON_MAX];
static bool trigger_down[2];

static void push_code(int16_t code) {
    if (code >= 0x100) {
        push_ext(code - 0x100);
    } else {
        keybuf.push_back(code);
    }
}

static void osk_move(int dx, int dy) {
    int col = osk_cursor % OSK_COLS, row = osk_cursor / OSK_COLS;
    col = (col + dx + OSK_COLS) % OSK_COLS;
    row = (row + dy + OSK_ROWS) % OSK_ROWS;
    osk_cursor = row * OSK_COLS + col;
}

static bool is_dpad(int b) {
    return b >= SDL_CONTROLLER_BUTTON_DPAD_UP && b <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
}

static bool repeats(int b) {
    if (osk_cursor >= 0) {
        return is_dpad(b);
    }
    return is_dpad(b) || (b == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER && pad_ctx == PAD_SURFACE);
}

// One press (or repeat) of a button.
static void pad_press(int b) {
    if (osk_cursor >= 0) {
        switch (b) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP: osk_move(0, -1); break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: osk_move(0, 1); break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: osk_move(-1, 0); break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: osk_move(1, 0); break;
        case SDL_CONTROLLER_BUTTON_A: push_code(osk_keys[osk_cursor].code); break;
        case SDL_CONTROLLER_BUTTON_B: keybuf.push_back(8); break;
        case SDL_CONTROLLER_BUTTON_Y: keybuf.push_back(13); break;
        case SDL_CONTROLLER_BUTTON_X: keybuf.push_back(' '); break;
        case SDL_CONTROLLER_BUTTON_BACK:
        case SDL_CONTROLLER_BUTTON_START: osk_cursor = -1; break;
        default: break;
        }
        return;
    }

    static const char menus[] = "5rdx"; // flight control, devices, prefs, screen off
    switch (b) {
    case SDL_CONTROLLER_BUTTON_A:
        if (pad_ctx == PAD_SURFACE) {
            keybuf.push_back('j');
        } else {
            mouse_buttons |= 1;
            mouse_pressed |= 1;
        }
        break;
    case SDL_CONTROLLER_BUTTON_B:
        mouse_buttons |= 2;
        mouse_pressed |= 2;
        break;
    case SDL_CONTROLLER_BUTTON_X: keybuf.push_back('m'); break;
    case SDL_CONTROLLER_BUTTON_Y: keybuf.push_back(13); break;
    case SDL_CONTROLLER_BUTTON_START: push_ext(0x3B); break;
    case SDL_CONTROLLER_BUTTON_BACK: osk_cursor = 0; break;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK: push_ext(0x3C); break;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK: push_ext(0x3D); break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP: push_ext(72); break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: push_ext(80); break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: push_ext(75); break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: push_ext(77); break;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        if (pad_ctx == PAD_SURFACE) {
            keybuf.push_back('l');
        } else {
            keybuf.push_back(menus[(pad_menu + 2) % 4]);
        }
        break;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        if (pad_ctx == PAD_SURFACE) {
            keybuf.push_back(' ');
        } else {
            keybuf.push_back(menus[pad_menu % 4]);
        }
        break;
    default: break;
    }
}

static void pad_release(int b) {
    if (b == SDL_CONTROLLER_BUTTON_A && pad_ctx != PAD_SURFACE) {
        mouse_buttons &= ~1;
    }
    if (b == SDL_CONTROLLER_BUTTON_B) {
        mouse_buttons &= ~2;
    }
}

static void pad_event(const SDL_Event &e) {
    switch (e.type) {
    case SDL_CONTROLLERDEVICEADDED:
        if (!pad) {
            pad = SDL_GameControllerOpen(e.cdevice.which);
        }
        break;
    case SDL_CONTROLLERDEVICEREMOVED:
        if (pad && e.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad))) {
            SDL_GameControllerClose(pad);
            pad = nullptr;
        }
        break;
    case SDL_CONTROLLERBUTTONDOWN:
        pad_used = true;
        held_since[e.cbutton.button] = SDL_GetTicks();
        held_next[e.cbutton.button]  = held_since[e.cbutton.button] + 500;
        pad_press(e.cbutton.button);
        break;
    case SDL_CONTROLLERBUTTONUP:
        held_since[e.cbutton.button] = 0;
        pad_release(e.cbutton.button);
        break;
    default: break;
    }
}

// Typematic repeat, and the triggers as PgUp / PgDn (HUD visor).
static void pad_poll() {
    if (!pad) {
        return;
    }
    Uint32 now = SDL_GetTicks();
    for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX; b++) {
        if (held_since[b] && repeats(b) && (int32_t) (now - held_next[b]) >= 0) {
            held_next[b] = now + 92;
            pad_press(b);
        }
    }
    const SDL_GameControllerAxis triggers[2] = {SDL_CONTROLLER_AXIS_TRIGGERLEFT,
                                                SDL_CONTROLLER_AXIS_TRIGGERRIGHT};
    for (int i = 0; i < 2; i++) {
        bool down = SDL_GameControllerGetAxis(pad, triggers[i]) > 16000;
        if (down && !trigger_down[i] && osk_cursor < 0) {
            pad_used = true;
            push_ext(i == 0 ? 73 : 81);
        }
        trigger_down[i] = down;
    }
}

static float axis(SDL_GameControllerAxis a) {
    float v = SDL_GameControllerGetAxis(pad, a) / 32767.0f;
    const float dz = 0.2f;
    if (fabsf(v) < dz) {
        return 0;
    }
    v = (fabsf(v) - dz) / (1 - dz) * (v < 0 ? -1 : 1);
    return v > 1 ? 1 : v < -1 ? -1 : v;
}

void plat_pad_context(int ctx, int menu) {
    pad_ctx  = ctx;
    pad_menu = menu;
}

void plat_pad_sticks(float *lx, float *ly, float *rx, float *ry) {
    *lx = *ly = *rx = *ry = 0;
    if (!pad || osk_cursor >= 0) {
        return;
    }
    *lx = axis(SDL_CONTROLLER_AXIS_LEFTX);
    *ly = axis(SDL_CONTROLLER_AXIS_LEFTY);
    *rx = axis(SDL_CONTROLLER_AXIS_RIGHTX);
    *ry = axis(SDL_CONTROLLER_AXIS_RIGHTY);
    if (*lx || *ly || *rx || *ry) {
        pad_used = true;
    }
}

bool plat_pad_used() { return pad_used; }

int plat_osk_cursor() { return osk_cursor; }

const char *plat_osk_label(int i) { return osk_keys[i].label; }
