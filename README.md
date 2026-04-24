# TGM4 Section Timer Prototype

Tetris The Grand Master 4 process memory reader written in C for Windows.

## What it does

- Attaches to the target process by executable name
- Reads the in-game `Level` value from memory
- Starts timing when the observed level becomes `0`
- Records a section time every time the level crosses `100`, `200`, `300`...
- Shows current level, total run time, section split times, and best records in a larger window sized for sections up to level 1300
- Saves best section records in the `save` folder next to the executable
- Lets you switch between `ASUKA` and `NORMAL` memory layouts from a button
- Lets you pause and resume processing from a button

## Files

- `main.c`: Win32 window + process attach + memory read + section timing
- `config.h`: change process/module name and the level address settings here

## Address setup

Edit `config.h`.

### Option 1: direct address

If you already know the final virtual address that stores `Level`, set:

```c
static const uintptr_t LEVEL_VALUE_ADDRESS = 0x12345678;
```

When this is non-zero, the pointer-chain settings are ignored.

Current configured value:

```c
static const uintptr_t LEVEL_VALUE_ADDRESS = 0;
static const uintptr_t LEVEL_BASE_OFFSET = 0x00A7CD9C;

static const uintptr_t LEVEL_POINTER_OFFSETS[] = {
    0x94,
    0x10,
    0x10,
    0x10,
    0x04,
    0x20
};
```

### Option 2: module base + pointer chain

If Cheat Engine shows something like:

```text
TGM4.exe + 00123456 -> +20 -> +18 -> +10
```

set:

```c
static const uintptr_t LEVEL_VALUE_ADDRESS = 0;
static const uintptr_t LEVEL_BASE_OFFSET = 0x00123456;

static const uintptr_t LEVEL_POINTER_OFFSETS[] = {
    0x20,
    0x18,
    0x10
};
```

## Build examples

### MSVC

```bat
cl /W4 /O2 /DUNICODE /D_UNICODE main.c user32.lib gdi32.lib
```

### MinGW-w64

```bat
gcc -municode -O2 -Wall -Wextra -o tgm4_timer.exe main.c -lgdi32 -luser32
```

## Notes

- The prototype assumes `Level` is a 32-bit integer.
- If the game stores it as `short`, `float`, or BCD-style text, the read type must be changed.
- A big backward jump in level is treated as a reset and the timer returns to standby.
- A return to `Level 0` during a run is treated as a retry and restarts the timer immediately.
- If you cross multiple sections in one poll, the code records them with the same timestamp.
- The current setup uses the pointer chain `tgm4.exe+00A7CD9C, 94, 10, 10, 10, 4, 20`.
- `ASUKA` mode uses `tgm4.exe+00A7CD9C` with offsets `20, 4, 10, 10, 10, 94`.
- `NORMAL` mode uses `tgm4.exe+00A7E528` with offsets `8, 30, 10, c, 28, 10, 9c`.
- Best records are saved per mode in separate files under `save`:
  `section_bests_asuka.txt` and `section_bests_normal.txt`.
- If no save file exists yet, all best times start at `999.000` seconds.
- Current split lines are green when faster than the previous saved best and red when slower.

## Next recommended step

Find the real `Level` address or pointer chain in Cheat Engine / x64dbg and paste it into `config.h`.
Once that is known, this tool is ready for a first run.
