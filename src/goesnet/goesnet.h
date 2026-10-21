#pragma once

// Runs a GOES Net command line ("CAT FENIA") with its output in output_file.
// False if there is no such module.
bool goesnet_run(const char *command, const char *output_file);
