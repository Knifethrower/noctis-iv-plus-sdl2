# Noctis IV Plus, SDL2

A native SDL2 build of [Noctis IV Plus](https://github.com/jorisvddonk/Noctis-IV-Plus) (release 2.3, by
Mega, Neuzd, SL and others), the modified version of Alessandro Ghignola's space exploration simulator
Noctis IV. Made for [PortMaster](https://portmaster.games) handhelds; it also runs on desktop Linux.

It starts from [Noctis IV LR](https://github.com/dgcole/noctis-iv-lr) by Dyllan Cole and contributors, at
its last SDL2 commit (2cd4cda, January 2020), where the 16-bit assembly of the original had been rewritten
in C++. On top of that:

- the Noctis IV Plus 2.3 changes (jetpack, jumping, mouselook, moviemaker, visual effects menu, HUD visor,
  snapshot numbering, roof speed, Omega Drive, target data), with the original's controls (LR's WASD and
  click-to-select removed);
- an SDL2 platform layer standing in for the VGA card, the BIOS keyboard buffer, the mouse driver and the
  18.2 Hz BIOS timer, so the game runs at its original speed;
- native gamepad support with an on-screen keyboard, autosave every 30 seconds;
- the same results on every CPU: x86 float conversions, sin/cos/atan/pow from musl (`src/detmath`), the
  DOS heap layout for the buffers.

## Building

```
sudo apt install build-essential cmake libsdl2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/noctis
```

Run it from the folder that holds `data/` (the repository root); `--fullscreen` fills the screen. Snapshots go to `gallery/`.

## License

Noctis IV is Copyright (C) 1996-2002 Alessandro Ghignola, under the WTOF Public License ([LICENSE.md](LICENSE.md)).
Modified versions are published under Alessandro Ghignola's general conditions for distribution of modified
versions: free, pure exploration, the original copyright message and the full license kept. The files in
`src/detmath` are from musl libc (MIT, `src/detmath/COPYRIGHT.musl`).
