#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

/*
 * Change these values to match the TGM4 process you want to read.
 *
 * ADDRESS MODE:
 *   1) LEVEL_VALUE_ADDRESS: absolute virtual address inside the target process
 *   2) MODULE_NAME + LEVEL_BASE_OFFSET + LEVEL_POINTER_OFFSETS: module base + pointer chain
 *
 * Leave LEVEL_VALUE_ADDRESS as 0 to use the pointer-chain mode.
 */

static const wchar_t *TARGET_PROCESS_NAME = L"tgm4.exe";
static const wchar_t *TARGET_MODULE_NAME = L"tgm4.exe";

static const uintptr_t LEVEL_VALUE_ADDRESS = 0;
static const uintptr_t LEVEL_BASE_OFFSET = 0x00A7CD9C;
static const uintptr_t LEVEL_POINTER_OFFSETS[] = {
    0x20,
    0x04,
    0x10,
    0x10,
    0x10,
    0x94
};
static const size_t LEVEL_POINTER_OFFSET_COUNT =
    sizeof(LEVEL_POINTER_OFFSETS) / sizeof(LEVEL_POINTER_OFFSETS[0]);

static const int LEVEL_RESET_THRESHOLD = 50;
static const int POLL_INTERVAL_MS = 15;
static const int WINDOW_REFRESH_MS = 33;

#endif
