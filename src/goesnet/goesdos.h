#pragma once

// What the GOES Net modules (DOS programs of the original, now built into the
// game) need from DOS: Borland io.h on POSIX files, DOS paths mapped into data/,
// printf to the GOES output in text mode, and the command line as _argc/_argv.

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace goesdos {

extern FILE *out;      // the module's standard output
extern FILE *stdprn;   // the printer (PRI): printout.txt in the game folder
extern int _argc;
extern char **_argv;

int _open(const char *path, int access);
int _creat(const char *path, int attrib);
int _read(int fh, void *buf, unsigned len);
int _write(int fh, const void *buf, unsigned len);
int _close(int fh);
long lseek(int fh, long offset, int whence);
long tell(int fh);
long filelength(int fh);
int chsize(int fh, long size);
int remove(const char *path);
int rename(const char *from, const char *to);
FILE *fopen(const char *path, const char *mode);

// printf to the module's output; DOS text mode writes \n as \r\n.
int printf(const char *fmt, ...);
int puts(const char *s);
char *strupr(char *s);

// A DOS path (..\DATA\STARMAP.BIN) as a path in the game folder (data/STARMAP.BIN).
const char *map_path(const char *dos_path);

} // namespace goesdos

// A module (namespace goesdos::goes_<name>): run with the command line in _argc/_argv.
struct GoesModule {
    const char *name;
    void (*run)();
};
