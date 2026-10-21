// GOES Net: the modules of the onboard terminal, built into the game. The
// original ran each as a DOS program with its output redirected to
// GOESfile.txt and its requests to the ship in COMM.BIN; that stays the same.

#include "goesnet.h"
#include "goesdos.h"
#include "../platform.h"

#include <cctype>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace goesdos {

FILE *out;
FILE *stdprn;
int _argc;
char **_argv;

const char *map_path(const char *dos_path) {
    static char buf[4][512];
    static int next;
    char *r = buf[next];
    next = (next + 1) % 4;

    std::string p = dos_path;
    for (char &c : p) {
        if (c == '\\') {
            c = '/';
        }
    }
    while (p.compare(0, 3, "../") == 0) {
        p.erase(0, 3);
    }
    std::string dir, name = p;
    size_t slash = p.rfind('/');
    if (slash != std::string::npos) {
        dir  = p.substr(0, slash);
        name = p.substr(slash + 1);
        for (char &c : dir) {
            c = tolower(c);
        }
    }

    // DOS names are case-insensitive: use the file that is there, else lower case.
    std::string found;
    if (DIR *d = opendir(dir.empty() ? "." : dir.c_str())) {
        while (dirent *e = readdir(d)) {
            if (!strcasecmp(e->d_name, name.c_str())) {
                found = e->d_name;
                break;
            }
        }
        closedir(d);
    }
    if (found.empty()) {
        found = name;
        for (char &c : found) {
            c = tolower(c);
        }
    }
    snprintf(r, sizeof(buf[0]), "%s%s%s", dir.c_str(), dir.empty() ? "" : "/", found.c_str());
    return r;
}

// Borland's O_RDONLY 1, O_WRONLY 2, O_RDWR 4 (0: read).
int _open(const char *path, int access) {
    int flags = (access & 4) ? O_RDWR : (access & 2) ? O_WRONLY : O_RDONLY;
    return ::open(map_path(path), flags);
}

int _creat(const char *path, int) {
    return ::open(map_path(path), O_RDWR | O_CREAT | O_TRUNC, 0644);
}

int _read(int fh, void *buf, unsigned len) { return (int) ::read(fh, buf, len); }

int _write(int fh, const void *buf, unsigned len) { return (int) ::write(fh, buf, len); }

int _close(int fh) { return ::close(fh); }

long lseek(int fh, long offset, int whence) { return (long) ::lseek(fh, offset, whence); }

long tell(int fh) { return (long) ::lseek(fh, 0, SEEK_CUR); }

long filelength(int fh) {
    struct stat st;
    return ::fstat(fh, &st) == 0 ? (long) st.st_size : -1;
}

int chsize(int fh, long size) { return ::ftruncate(fh, size); }

int remove(const char *path) { return ::remove(map_path(path)); }

int rename(const char *from, const char *to) {
    std::string f = map_path(from);
    return ::rename(f.c_str(), map_path(to));
}

FILE *fopen(const char *path, const char *mode) { return ::fopen(map_path(path), mode); }

static void put_text(const char *s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s[i] == '\n') {
            fputc('\r', out);
        }
        fputc(s[i], out);
    }
}

int printf(const char *fmt, ...) {
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) {
        put_text(buf, (size_t) n < sizeof(buf) ? (size_t) n : sizeof(buf) - 1);
    }
    return n;
}

unsigned char vga[64000];

void vga_show() { goesnet_show_vga(vga); }

int kbhit() { return plat_key_ready(); }

int getch() { return plat_get_key(); }

static uint32_t rand_seed = 1;

void srand(unsigned seed) { rand_seed = (uint16_t) seed; }

int rand() {
    rand_seed = rand_seed * 0x015A4E35u + 1;
    return (rand_seed >> 16) & 0x7FFF;
}

char *strupr(char *s) {
    for (char *p = s; *p; p++) {
        *p = toupper(*p);
    }
    return s;
}

int puts(const char *s) {
    put_text(s, strlen(s));
    put_text("\n", 1);
    return 1;
}

} // namespace goesdos

// The modules (one file each in this folder).
#define GOES_MODULES(X) X(cat) X(cast) X(rep) X(dele) X(clean) X(where) X(pri) X(help) \
    X(st) X(sl) X(dl) X(par)
#define DECLARE(m) namespace goesdos { namespace goes_##m { void run(); } }
GOES_MODULES(DECLARE)
#undef DECLARE

static const GoesModule modules[] = {
#define ENTRY(m) {#m, goesdos::goes_##m::run},
    GOES_MODULES(ENTRY)
#undef ENTRY
};

bool goesnet_run(const char *command, const char *output_file) {
    // DOS splits the command line at blanks; argv[0] is the program.
    std::vector<std::string> words;
    std::string w;
    for (const char *p = command;; p++) {
        if (*p == ' ' || *p == '\t' || *p == 0) {
            if (!w.empty()) {
                words.push_back(w);
                w.clear();
            }
            if (*p == 0) {
                break;
            }
        } else {
            w += *p;
        }
    }
    if (words.empty()) {
        return false;
    }

    for (const GoesModule &m : modules) {
        if (strcasecmp(m.name, words[0].c_str())) {
            continue;
        }
        std::vector<char *> argv;
        for (std::string &s : words) {
            argv.push_back(&s[0]);
        }
        // Modules copy _argv[1] even when there is none: DOS read some bytes
        // at 0000:0000 then, an empty string here.
        static char none[1];
        argv.push_back(none);
        argv.push_back(none);
        goesnet_load_vga(goesdos::vga);
        goesdos::_argc = (int) words.size();
        goesdos::_argv = argv.data();
        goesdos::out   = ::fopen(output_file, "wb");
        if (!goesdos::out) {
            return false;
        }
        // The printer is a text file now, added to like paper in a printer.
        if (!strcmp(m.name, "pri")) {
            goesdos::stdprn = ::fopen("printout.txt", "ab");
        }
        m.run();
        if (goesdos::stdprn) {
            fclose(goesdos::stdprn);
            goesdos::stdprn = nullptr;
        }
        fclose(goesdos::out);
        goesdos::out = nullptr;
        return true;
    }
    return false;
}
