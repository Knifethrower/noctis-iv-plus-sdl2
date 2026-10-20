#pragma once

// Noctis IV Plus additions (NOCTIS-2.H and noctis-0.h of the Plus source).

#include <cstdint>

// Saved in current.bin after the original 370 bytes.
extern int32_t lastSnapshot;
extern int8_t option_mouseLook; // 0 off, 1 on, 2 inverted Y
extern int16_t roofspeed;
extern int8_t hud_closed;
extern int8_t draw_hud;
extern int8_t lens_flare_mode; // 0 visor only, 1 always, -1 never
extern int8_t seamless_border;

extern int16_t openhudcount;
extern int16_t openhuddelta;
extern int8_t graphics_menu_status;
extern int8_t moviestat;
extern int8_t about;
extern char fcs_status_extended[42];

// Moviemaker
extern int8_t movie;
extern int8_t movieflashoff;
extern int32_t movienr;
extern int32_t moviefscap;
extern int32_t moviestime;
extern int32_t moviedeck;
extern int32_t movieexists;
extern float moviefps;

extern int8_t blinkhudlights;
extern int32_t last_autosave;
extern int8_t blinkhudlights_stay;

char *formatTriad(double value);
void CalculatemovieFPS(int32_t moviefscap, int32_t moviestime);
void ShowMovieSetup(int32_t moviefsec, int8_t movieflashoff, int32_t moviedeck);
void FitOutHudBuffer(int32_t min, int32_t max);
void ShowAboutPage(int8_t surface);
int32_t movie_deck_exists(int32_t deck);
void freeze();
