#include "app.h"

AppState g_app = { 0 };

static void close_process(void);
static void reset_run_preserving_mode(void);
static void clear_section_results(void);
static void reset_tracking_state(void);
static void reset_timer_state(void);
static void archive_current_results_if_any(void);
static void record_pace_sample(int level);
static void record_new_sections(int currentLevel);
static void update_timer_from_level(int level);
static void poll_target_process(void);
static bool open_target_process(void);
static bool detect_mode_from_cursor(void);
static bool read_level_value(int *levelOut);
static bool read_game_timer_frames_internal(int *framesOut);
static bool resolve_pointer_chain(uintptr_t baseOffset, const uintptr_t *offsets, size_t offsetCount, uintptr_t *resolvedAddress);
static bool resolve_level_address(uintptr_t *resolvedAddress);
static bool read_int_from_address(uintptr_t address, int *valueOut);
static bool read_byte_from_address(uintptr_t address, uint8_t *valueOut);
static DWORD find_process_id(const wchar_t *processName);
static uintptr_t find_module_base_address(DWORD processId, const wchar_t *moduleName);
static int find_config_index_for_cursor_value(int cursorValue);
static double frames_to_seconds(int frames);
static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static const PointerConfig *cursor_pointer_config(void) {
    if (g_app.currentConfigIndex >= 0) {
        return current_pointer_config();
    }
    if (pointer_config_count() > 0) {
        return &POINTER_CONFIGS[0];
    }
    return NULL;
}

void copy_status_text(const wchar_t *text) {
    lstrcpynW(g_app.statusText, text, ARRAY_COUNT(g_app.statusText));
}

static double frames_to_seconds(int frames) {
    return (double)frames / 60.0;
}

const RunSnapshot *current_view_snapshot(void) {
    if (g_app.historyViewOffset <= 0 || g_app.historyViewOffset > g_app.historyCount) {
        return NULL;
    }
    return &g_app.history[g_app.historyViewOffset - 1];
}

double current_levels_per_minute(void) {
    int oldestIndex;
    int newestIndex;
    ULONGLONG newestTime;

    if (!g_app.timerRunning || g_app.currentLevel < 0 || g_app.paceSampleCount <= 0) {
        return 0.0;
    }

    newestIndex = (g_app.paceSampleStart + g_app.paceSampleCount - 1) % PACE_SAMPLE_COUNT;
    newestTime = g_app.paceSamples[newestIndex].timeMs;

    while (g_app.paceSampleCount > 1) {
        oldestIndex = g_app.paceSampleStart;
        if (newestTime - g_app.paceSamples[oldestIndex].timeMs <= 60000ULL) {
            break;
        }
        g_app.paceSampleStart = (g_app.paceSampleStart + 1) % PACE_SAMPLE_COUNT;
        g_app.paceSampleCount -= 1;
    }

    if (g_app.paceSampleCount <= 1) {
        return 0.0;
    }

    oldestIndex = g_app.paceSampleStart;
    if (newestTime <= g_app.paceSamples[oldestIndex].timeMs) {
        return 0.0;
    }

    return (double)(g_app.paceSamples[newestIndex].level - g_app.paceSamples[oldestIndex].level)
        * 60000.0
        / (double)(newestTime - g_app.paceSamples[oldestIndex].timeMs);
}

static void record_pace_sample(int level) {
    int writeIndex;
    ULONGLONG nowMs;

    nowMs = GetTickCount64();
    if (g_app.paceSampleCount > 0) {
        int newestIndex = (g_app.paceSampleStart + g_app.paceSampleCount - 1) % PACE_SAMPLE_COUNT;
        if (g_app.paceSamples[newestIndex].timeMs == nowMs && g_app.paceSamples[newestIndex].level == level) {
            return;
        }
    }

    if (g_app.paceSampleCount < PACE_SAMPLE_COUNT) {
        writeIndex = (g_app.paceSampleStart + g_app.paceSampleCount) % PACE_SAMPLE_COUNT;
        g_app.paceSampleCount += 1;
    } else {
        writeIndex = g_app.paceSampleStart;
        g_app.paceSampleStart = (g_app.paceSampleStart + 1) % PACE_SAMPLE_COUNT;
    }

    g_app.paceSamples[writeIndex].timeMs = nowMs;
    g_app.paceSamples[writeIndex].level = level;
}

static void archive_current_results_if_any(void) {
    RunSnapshot snapshot;
    const PointerConfig *config;
    int i;

    if (g_app.sectionCount <= 0 || g_app.resultsArchivedPendingClear) {
        return;
    }

    config = current_pointer_config();
    if (config == NULL) {
        return;
    }

    ZeroMemory(&snapshot, sizeof(snapshot));
    snapshot.valid = true;
    lstrcpynW(snapshot.modeLabel, config->modeLabel, ARRAY_COUNT(snapshot.modeLabel));
    snapshot.theoreticalMaxLevel = config->theoreticalMaxLevel;
    snapshot.finalLevel = g_app.currentLevel;
    snapshot.maxLevel = g_app.maxLevel;
    snapshot.sectionCount = g_app.sectionCount;

    for (i = 0; i < MAX_SECTION_COUNT; ++i) {
        snapshot.sectionTimes[i] = g_app.sectionTimes[i];
        snapshot.sectionDeltas[i] = g_app.sectionDeltas[i];
        snapshot.bestSectionTimes[i] = g_app.bestSectionTimes[i];
        snapshot.sectionGameTimerFrames[i] = g_app.sectionGameTimerFrames[i];
        snapshot.backstepCounts[i] = g_app.backstepCounts[i];
        snapshot.tetrisCounts[i] = g_app.tetrisCounts[i];
    }

    for (i = MAX_HISTORY_COUNT - 1; i > 0; --i) {
        g_app.history[i] = g_app.history[i - 1];
    }
    g_app.history[0] = snapshot;

    if (g_app.historyCount < MAX_HISTORY_COUNT) {
        g_app.historyCount += 1;
    }

    g_app.historyViewOffset = 0;
    g_app.resultsArchivedPendingClear = true;
    update_button_labels();
}

static DWORD find_process_id(const wchar_t *processName) {
    PROCESSENTRY32W entry;
    HANDLE snapshot;

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    entry.dwSize = sizeof(entry);
    if (!Process32FirstW(snapshot, &entry)) {
        CloseHandle(snapshot);
        return 0;
    }

    do {
        if (_wcsicmp(entry.szExeFile, processName) == 0) {
            CloseHandle(snapshot);
            return entry.th32ProcessID;
        }
    } while (Process32NextW(snapshot, &entry));

    CloseHandle(snapshot);
    return 0;
}

static uintptr_t find_module_base_address(DWORD processId, const wchar_t *moduleName) {
    MODULEENTRY32W entry;
    HANDLE snapshot;

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    entry.dwSize = sizeof(entry);
    if (!Module32FirstW(snapshot, &entry)) {
        CloseHandle(snapshot);
        return 0;
    }

    do {
        if (_wcsicmp(entry.szModule, moduleName) == 0) {
            CloseHandle(snapshot);
            return (uintptr_t)entry.modBaseAddr;
        }
    } while (Module32NextW(snapshot, &entry));

    CloseHandle(snapshot);
    return 0;
}

static void close_process(void) {
    if (g_app.processHandle != NULL) {
        CloseHandle(g_app.processHandle);
        g_app.processHandle = NULL;
    }

    g_app.processId = 0;
    g_app.levelAddress = 0;
    g_app.attached = false;
}

static void clear_section_results(void) {
    int i;

    g_app.runStartGameTimerFrames = -1;
    g_app.sectionCount = 0;
    g_app.currentGameTimerFrames = -1;
    g_app.resultsArchivedPendingClear = false;

    for (i = 0; i < MAX_SECTION_COUNT; ++i) {
        g_app.sectionTimes[i] = -1.0;
        g_app.sectionDeltas[i] = 0.0;
        g_app.sectionGameTimerFrames[i] = -1;
        g_app.backstepCounts[i] = 0;
        g_app.tetrisCounts[i] = 0;
    }
}

static void reset_tracking_state(void) {
    g_app.timerRunning = false;
    g_app.currentLevel = -1;
    g_app.previousLevel = -1;
    g_app.lastRecordedSection = -1;
    g_app.levelReadable = false;
    g_app.timerReadable = false;
    g_app.modeDetected = false;
    g_app.runStartMs = 0;
    g_app.runStartGameTimerFrames = -1;
    g_app.currentGameTimerFrames = -1;
    g_app.clearResultsOnLevelAdvance = false;
    g_app.maxLevelsPerMinute = 0.0;
    g_app.paceSampleStart = 0;
    g_app.paceSampleCount = 0;
}

static void reset_timer_state(void) {
    reset_tracking_state();
    clear_section_results();
}

static void reset_run_preserving_mode(void) {
    reset_timer_state();
    close_process();
}

static bool open_target_process(void) {
    DWORD processId;
    HANDLE processHandle;

    processId = find_process_id(TARGET_PROCESS_NAME);
    if (processId == 0) {
        close_process();
        copy_status_text(L"Waiting for tgm4.exe");
        return false;
    }

    if (g_app.attached && g_app.processId == processId && g_app.processHandle != NULL) {
        return true;
    }

    close_process();
    reset_tracking_state();

    processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (processHandle == NULL) {
        swprintf(g_app.statusText, ARRAY_COUNT(g_app.statusText), L"OpenProcess failed (pid=%lu)", processId);
        return false;
    }

    g_app.processId = processId;
    g_app.processHandle = processHandle;
    g_app.attached = true;
    swprintf(g_app.statusText, ARRAY_COUNT(g_app.statusText), L"Attached to tgm4.exe (pid=%lu)", processId);
    return true;
}

static bool resolve_pointer_chain(uintptr_t baseOffset, const uintptr_t *offsets, size_t offsetCount, uintptr_t *resolvedAddress) {
    uintptr_t address;
    SIZE_T bytesRead;
    size_t i;

    if (g_app.processHandle == NULL) {
        return false;
    }

    address = find_module_base_address(g_app.processId, TARGET_MODULE_NAME);
    if (address == 0) {
        copy_status_text(L"Module not found: tgm4.exe");
        return false;
    }

    address += baseOffset;
    for (i = 0; i < offsetCount; ++i) {
        uint32_t nextPtr32;

        if (!ReadProcessMemory(g_app.processHandle, (LPCVOID)address, &nextPtr32, sizeof(nextPtr32), &bytesRead) || bytesRead != sizeof(nextPtr32)) {
            swprintf(g_app.statusText, ARRAY_COUNT(g_app.statusText), L"Pointer read failed at step %zu", i);
            return false;
        }
        address = (uintptr_t)nextPtr32 + offsets[i];
    }

    *resolvedAddress = address;
    return true;
}

static bool resolve_level_address(uintptr_t *resolvedAddress) {
    const PointerConfig *config = current_pointer_config();
    if (config == NULL) {
        return false;
    }

    return resolve_pointer_chain(POINTER_CHAINS.baseOffset, POINTER_CHAINS.levelOffsets, POINTER_CHAINS.levelOffsetCount, resolvedAddress);
}

static bool read_int_from_address(uintptr_t address, int *valueOut) {
    SIZE_T bytesRead;

    if (!ReadProcessMemory(g_app.processHandle, (LPCVOID)address, valueOut, sizeof(*valueOut), &bytesRead) || bytesRead != sizeof(*valueOut)) {
        return false;
    }
    return true;
}

static bool read_game_timer_frames_internal(int *framesOut) {
    uintptr_t resolvedAddress;
    const PointerConfig *config = current_pointer_config();

    if (config == NULL || g_app.processHandle == NULL) {
        g_app.timerReadable = false;
        lstrcpynW(g_app.timerReadStatus, L"Timer skipped: no mode or process", ARRAY_COUNT(g_app.timerReadStatus));
        return false;
    }

    g_app.timerAddress = 0;
    g_app.timerReadable = false;
    if (!resolve_pointer_chain(POINTER_CHAINS.baseOffset, POINTER_CHAINS.timerOffsets, POINTER_CHAINS.timerOffsetCount, &resolvedAddress)) {
        lstrcpynW(g_app.timerReadStatus, L"Timer pointer resolve failed", ARRAY_COUNT(g_app.timerReadStatus));
        return false;
    }

    g_app.timerAddress = resolvedAddress;
    if (!read_int_from_address(resolvedAddress, framesOut)) {
        swprintf(g_app.timerReadStatus, ARRAY_COUNT(g_app.timerReadStatus), L"Timer read failed at 0x%p", (void *)resolvedAddress);
        return false;
    }

    g_app.timerReadable = true;
    lstrcpynW(g_app.timerReadStatus, L"Timer read OK", ARRAY_COUNT(g_app.timerReadStatus));
    return true;
}

static bool read_byte_from_address(uintptr_t address, uint8_t *valueOut) {
    SIZE_T bytesRead;

    if (!ReadProcessMemory(g_app.processHandle, (LPCVOID)address, valueOut, sizeof(*valueOut), &bytesRead) || bytesRead != sizeof(*valueOut)) {
        return false;
    }
    return true;
}

static int find_config_index_for_cursor_value(int cursorValue) {
    int i;
    for (i = 0; i < pointer_config_count(); ++i) {
        if (POINTER_CONFIGS[i].cursorValue == cursorValue) {
            return i;
        }
    }
    return -1;
}

/* During a game the menu pointers can become unavailable. Keep the last
 * confirmed mode so level and timer tracking can continue. */
static bool keep_last_confirmed_mode(const wchar_t *statusText) {
    if (g_app.currentConfigIndex < 0 || g_app.currentConfigIndex >= pointer_config_count()) {
        g_app.modeDetected = false;
        return false;
    }

    g_app.modeDetected = true;
    if (statusText != NULL) {
        copy_status_text(statusText);
    }
    return true;
}

static bool detect_mode_from_cursor(void) {
    uintptr_t cursorAddress;
    uintptr_t menuCursorAddress;
    int cursorValue;
    uint8_t menuCursorPosition;
    int newConfigIndex;
    const PointerConfig *config;

    config = cursor_pointer_config();
    if (config == NULL) {
        return false;
    }

    if (!resolve_pointer_chain(POINTER_CHAINS.baseOffset, POINTER_CHAINS.gameModeOffsets, POINTER_CHAINS.gameModeOffsetCount, &cursorAddress)) {
        return keep_last_confirmed_mode(L"Game mode pointer unavailable; using last confirmed mode");
    }
    if (!read_int_from_address(cursorAddress, &cursorValue)) {
        return keep_last_confirmed_mode(L"Game mode value unavailable; using last confirmed mode");
    }

    g_app.cursorAddress = cursorAddress;
    if (!resolve_pointer_chain(POINTER_CHAINS.baseOffset, POINTER_CHAINS.menuCursorYOffsets, POINTER_CHAINS.menuCursorYOffsetCount, &menuCursorAddress)) {
        return keep_last_confirmed_mode(L"Menu cursor pointer unavailable; using last confirmed mode");
    }
    if (!read_byte_from_address(menuCursorAddress, &menuCursorPosition)) {
        return keep_last_confirmed_mode(L"Menu cursor value unavailable; using last confirmed mode");
    }

    g_app.menuCursorAddress = menuCursorAddress;
    g_app.cursorValue = cursorValue;
    g_app.cursorYValue = (int)menuCursorPosition;
    newConfigIndex = find_config_index_for_cursor_value(cursorValue);
    if (newConfigIndex < 0) {
        return keep_last_confirmed_mode(L"Unknown game mode value; using last confirmed mode");
    }

    /* A mode change is valid only when both independent menu values agree. */
    config = &POINTER_CONFIGS[newConfigIndex];
    if (config->menuCursorPosition != (int)menuCursorPosition) {
        return keep_last_confirmed_mode(L"Game mode and menu cursor do not match; using last confirmed mode");
    }

    if (g_app.currentConfigIndex != newConfigIndex) {
        archive_current_results_if_any();
        g_app.currentConfigIndex = newConfigIndex;
        reset_run_preserving_mode();
        load_best_times();
        load_max_level();
    }

    g_app.modeDetected = true;
    return true;
}
static bool read_level_value(int *levelOut) {
    uintptr_t resolvedAddress;
    SIZE_T bytesRead;
    uint16_t levelValue;

    if (g_app.processHandle == NULL) {
        g_app.levelReadable = false;
        lstrcpynW(g_app.levelReadStatus, L"Level skipped: no process", ARRAY_COUNT(g_app.levelReadStatus));
        return false;
    }
    g_app.levelAddress = 0;
    g_app.levelReadable = false;
    if (!resolve_level_address(&resolvedAddress)) {
        lstrcpynW(g_app.levelReadStatus, L"Level pointer resolve failed", ARRAY_COUNT(g_app.levelReadStatus));
        return false;
    }

    g_app.levelAddress = resolvedAddress;
    if (!ReadProcessMemory(g_app.processHandle, (LPCVOID)g_app.levelAddress, &levelValue, sizeof(levelValue), &bytesRead) || bytesRead != sizeof(levelValue)) {
        swprintf(g_app.levelReadStatus, ARRAY_COUNT(g_app.levelReadStatus), L"Level read failed at 0x%p", (void *)resolvedAddress);
        swprintf(g_app.statusText, ARRAY_COUNT(g_app.statusText), L"Level read failed, retrying address resolve");
        return false;
    }

    *levelOut = (int)levelValue;
    g_app.levelReadable = true;
    lstrcpynW(g_app.levelReadStatus, L"Level read OK", ARRAY_COUNT(g_app.levelReadStatus));
    return true;
}
static void record_new_sections(int currentLevel) {
    int completedSectionCount;
    int sectionIndex;
    double splitTime;
    double previousBestTime;
    int previousFrames;
    int frameDelta;
    const PointerConfig *config = current_pointer_config();

    if (config == NULL) {
        return;
    }

    if (g_app.runStartGameTimerFrames < 0) {
        g_app.runStartGameTimerFrames = config->initialTimerFrames;
    }

    completedSectionCount = completed_section_count_for_level(currentLevel, config->theoreticalMaxLevel);
    while (g_app.lastRecordedSection < completedSectionCount - 1) {
        sectionIndex = g_app.lastRecordedSection + 1;

        if (sectionIndex >= 0 && sectionIndex < MAX_SECTION_COUNT && g_app.currentGameTimerFrames >= 0 && g_app.runStartGameTimerFrames >= 0) {
            previousFrames = g_app.runStartGameTimerFrames;
            if (sectionIndex > 0 && g_app.sectionGameTimerFrames[sectionIndex - 1] >= 0) {
                previousFrames = g_app.sectionGameTimerFrames[sectionIndex - 1];
            }

            frameDelta = g_app.currentGameTimerFrames - previousFrames;
            if (frameDelta < 0) {
                frameDelta = -frameDelta;
            }

            splitTime = frames_to_seconds(frameDelta);
            if (splitTime < 2.0) {
                break;
            }

            g_app.lastRecordedSection = sectionIndex;
            g_app.sectionTimes[sectionIndex] = splitTime;
            g_app.sectionGameTimerFrames[sectionIndex] = g_app.currentGameTimerFrames;
            previousBestTime = g_app.bestSectionTimes[sectionIndex];
            g_app.sectionDeltas[sectionIndex] = splitTime - previousBestTime;

            if (splitTime < g_app.bestSectionTimes[sectionIndex]) {
                g_app.bestSectionTimes[sectionIndex] = splitTime;
                save_best_times();
            }

            if (sectionIndex + 1 > g_app.sectionCount) {
                g_app.sectionCount = sectionIndex + 1;
            }
        } else {
            break;
        }
    }
}

static void update_timer_from_level(int level) {
    int currentSectionIndex;
    const PointerConfig *config = current_pointer_config();

    if (config == NULL) {
        return;
    }

    if (level > config->theoreticalMaxLevel) {
        swprintf(g_app.statusText, ARRAY_COUNT(g_app.statusText), L"%ls ignoring out-of-range level %d", config->modeLabel, level);
        return;
    }

    if (!g_app.timerRunning) {
        if (level == 0) {
            reset_tracking_state();
            g_app.timerRunning = true;
            g_app.currentLevel = 0;
            g_app.previousLevel = 0;
            g_app.runStartMs = GetTickCount64();
            g_app.runStartGameTimerFrames = config->initialTimerFrames;
            g_app.clearResultsOnLevelAdvance = true;
            g_app.modeDetected = true;
            copy_status_text(L"Run started at level 0");
        } else {
            g_app.currentLevel = level;
            g_app.previousLevel = level;
            swprintf(g_app.statusText, ARRAY_COUNT(g_app.statusText), L"Waiting for level 0 (current=%d)", level);
        }
        return;
    }

    if (level == 0 && g_app.previousLevel > 0) {
        archive_current_results_if_any();
        reset_tracking_state();
        g_app.timerRunning = true;
        g_app.currentLevel = 0;
        g_app.previousLevel = 0;
        g_app.runStartMs = GetTickCount64();
        g_app.runStartGameTimerFrames = config->initialTimerFrames;
        g_app.clearResultsOnLevelAdvance = true;
        g_app.modeDetected = true;
        copy_status_text(L"Retry detected, run restarted at level 0");
        return;
    }

    if (g_app.clearResultsOnLevelAdvance && g_app.previousLevel == 0 && level > 0) {
        archive_current_results_if_any();
        clear_section_results();
        g_app.lastRecordedSection = -1;
        g_app.clearResultsOnLevelAdvance = false;
        g_app.runStartGameTimerFrames = config->initialTimerFrames;
    }

    if (g_app.previousLevel > 0 && level > 0 && level == g_app.previousLevel - 1) {
        currentSectionIndex = clamp_section_index_for_level(level, config->theoreticalMaxLevel);
        g_app.backstepCounts[currentSectionIndex] += 1;
    }

    if (g_app.previousLevel >= 0 && level >= g_app.previousLevel + 4) {
        int previousSectionIndex = clamp_section_index_for_level(g_app.previousLevel, config->theoreticalMaxLevel);
        currentSectionIndex = clamp_section_index_for_level(level, config->theoreticalMaxLevel);
        if (currentSectionIndex > previousSectionIndex) {
            currentSectionIndex = previousSectionIndex;
        }
        g_app.tetrisCounts[currentSectionIndex] += 1;
    }

    if (g_app.previousLevel >= 0 && level + LEVEL_RESET_THRESHOLD < g_app.previousLevel) {
        reset_tracking_state();
        g_app.currentLevel = level;
        g_app.previousLevel = level;
        g_app.modeDetected = true;
        swprintf(g_app.statusText, ARRAY_COUNT(g_app.statusText), L"Detected level reset, waiting for level 0");
        return;
    }

    g_app.currentLevel = level;
    record_pace_sample(level);
    if (level > g_app.maxLevel) {
        g_app.maxLevel = level;
        save_max_level();
    }
    if (current_levels_per_minute() > g_app.maxLevelsPerMinute) {
        g_app.maxLevelsPerMinute = current_levels_per_minute();
    }
    record_new_sections(level);
    g_app.previousLevel = level;
    swprintf(g_app.statusText, ARRAY_COUNT(g_app.statusText), L"%ls tracking (level=%d)", config->modeLabel, level);
}

static void poll_target_process(void) {
    int level;
    const PointerConfig *config;

    if (!open_target_process()) {
        return;
    }
    if (!detect_mode_from_cursor()) {
        g_app.levelReadable = false;
        g_app.currentLevel = -1;
        g_app.previousLevel = -1;
        g_app.timerRunning = false;
        copy_status_text(L"Unsupported cursor selection");
        return;
    }

    config = current_pointer_config();
    if (config == NULL) {
        return;
    }
    if (!read_level_value(&level)) {
        return;
    }
    if (!read_game_timer_frames_internal(&g_app.currentGameTimerFrames)) {
        g_app.currentGameTimerFrames = -1;
    }
    if (g_app.timerRunning && g_app.runStartGameTimerFrames < 0) {
        g_app.runStartGameTimerFrames = config->initialTimerFrames;
    }

    g_app.lastPollMs = GetTickCount64();
    update_timer_from_level(level);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        create_main_screen_controls(hwnd, ((LPCREATESTRUCT)lParam)->hInstance);
        create_settings_screen_controls(hwnd, ((LPCREATESTRUCT)lParam)->hInstance);
        populate_reset_mode_combo();
        update_button_labels();
        update_screen_controls();
        SetTimer(hwnd, 1, POLL_INTERVAL_MS, NULL);
        SetTimer(hwnd, 2, WINDOW_REFRESH_MS, NULL);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BUTTON_SETTINGS) {
            g_app.currentScreen = SCREEN_SETTINGS;
            update_screen_controls();
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        if (LOWORD(wParam) == ID_BUTTON_BACK) {
            g_app.currentScreen = SCREEN_MAIN;
            update_screen_controls();
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        if (LOWORD(wParam) == ID_BUTTON_HISTORY_PREV) {
            if (g_app.historyViewOffset < g_app.historyCount) {
                g_app.historyViewOffset += 1;
                update_button_labels();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        if (LOWORD(wParam) == ID_BUTTON_HISTORY_NEXT) {
            if (g_app.historyViewOffset > 0) {
                g_app.historyViewOffset -= 1;
                update_button_labels();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        apply_column_toggle_from_control(LOWORD(wParam));
        if (LOWORD(wParam) == ID_CHECK_PROGRESS) {
            g_app.showProgressBar = SendMessageW(g_app.progressCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
        }
        if (LOWORD(wParam) == ID_CHECK_DEBUG) {
            g_app.showDebugInfo = SendMessageW(g_app.debugCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        if (LOWORD(wParam) == ID_BUTTON_RESET_BEST) {
            int selection = (int)SendMessageW(g_app.resetModeCombo, CB_GETCURSEL, 0, 0);
            if (selection >= 0) {
                reset_best_times_for_config_index(selection);
            }
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        if (HIWORD(wParam) == BN_CLICKED) {
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        return 0;

    case WM_TIMER:
        if (wParam == 1) {
            poll_target_process();
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_PAINT:
        paint_window(hwnd);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        KillTimer(hwnd, 2);
        close_process();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR commandLine, int showCommand) {
    WNDCLASSW wc;
    HWND hwnd;
    MSG msg;

    (void)previousInstance;
    (void)commandLine;

    g_app.currentConfigIndex = -1;
    g_app.currentScreen = SCREEN_MAIN;
    g_app.showColumnGameTime = true;
    g_app.showColumnDelta = true;
    g_app.showColumnSectionTime = true;
    g_app.showColumnBest = true;
    g_app.showColumnBack = true;
    g_app.showColumnTet = true;
    g_app.showProgressBar = true;
    reset_timer_state();
    load_pointer_configs();
    copy_status_text(L"Starting...");

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        WINDOW_CLASS_NAME,
        L"TGM4 Section Timer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_THICKFRAME | WS_MAXIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        730,
        900,
        NULL,
        NULL,
        instance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
