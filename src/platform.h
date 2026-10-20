#pragma once

// SDL2 platform layer: stands in for the VGA card, the BIOS keyboard buffer,
// the INT 33h mouse driver and the 18.2 Hz BIOS timer of the DOS original.

#include <cstdint>

void plat_init();

// Shows a 320x200 8-bit frame with a 6-bit VGA palette (768 bytes), at 4:3.
void plat_present(const uint8_t *frame, const uint8_t *palette);

// BIOS timer ticks (18.2065 Hz), what clock() returned under Borland C++.
int32_t bios_clock();
// Sleeps until bios_clock() moves on.
void wait_tick();

// Reads pending SDL events into the key buffer and the mouse counters.
void plat_pump();

// Key buffer: ASCII, or 0 followed by the scan code for extended keys.
bool plat_key_ready();
int16_t plat_get_key(); // blocks like getch()

// Mouse motion since the last call (mickeys) and buttons (bit 0 left, bit 1 right).
void plat_mouse(int16_t *dx, int16_t *dy, uint16_t *buttons);
