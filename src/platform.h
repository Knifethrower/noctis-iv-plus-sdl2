#pragma once

// SDL2 platform layer: stands in for the VGA card, the BIOS keyboard buffer,
// the INT 33h mouse driver and the 18.2 Hz BIOS timer of the DOS original.

#include <cstddef>
#include <cstdint>

// --fullscreen: fill the display (handhelds); otherwise a window.
void plat_init(int argc, char **argv);

// Frame width: 320 (4:3) up to PLAT_MAX_W for wider displays, chosen at start.
#define PLAT_MAX_W 480
int plat_screen_width();

// Shows a (screen width)x200 8-bit frame with a 6-bit VGA palette (768 bytes);
// DOS pixels are 5:6, so 320 is 4:3.
void plat_present(const uint8_t *frame, const uint8_t *palette);

// What the original drew straight to video memory when taking a snapshot:
// the whole screen in color 0 (-1), or a line at the bottom in that color.
void screen_flash(int16_t line_color);

// BIOS timer ticks (18.2065 Hz), what clock() returned under Borland C++.
int32_t bios_clock();
// time() for the game's calendar (fixed in test mode).
#include <ctime>
time_t plat_time();
// Sleeps until bios_clock() moves on.
void wait_tick();

// Reads pending SDL events into the key buffer and the mouse counters.
void plat_pump();

// Key buffer: ASCII, or 0 followed by the scan code for extended keys.
bool plat_key_ready();
int16_t plat_get_key(); // blocks like getch()

// Mouse motion since the last call (mickeys) and buttons (bit 0 left, bit 1 right).
void plat_mouse(int16_t *dx, int16_t *dy, uint16_t *buttons);

// Test mode only: checksum of a buffer on stderr (x86 vs ARM checks).
void test_crc(const char *tag, const void *data, size_t len);

// Gamepad: the game loops say where the player is (and the console menu shown
// in the ship), and read the sticks (-1..1, deadzone applied) every frame.
enum { PAD_SPACE, PAD_SURFACE };
void plat_pad_context(int ctx, int menu);
void plat_pad_sticks(float *lx, float *ly, float *rx, float *ry);
bool plat_pad_used();

// On-screen keyboard (Select): cursor -1 when closed; the game draws it.
enum { OSK_COLS = 10, OSK_ROWS = 6 };
int plat_osk_cursor();
const char *plat_osk_label(int i);
