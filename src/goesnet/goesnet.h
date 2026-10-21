#pragma once

// Runs a GOES Net command line ("CAT FENIA") with its output in output_file.
// False if there is no such module.
bool goesnet_run(const char *command, const char *output_file);

// In noctis.cpp: the 320x200 VGA stand-in of the modules from and onto the
// middle of the game's frame.
void goesnet_load_vga(unsigned char *vga);
void goesnet_show_vga(const unsigned char *vga);
