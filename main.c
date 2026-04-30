#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <wchar.h>

#include "config.h"

#define MAX_SECTION_COUNT 30
#define MAX_HISTORY_COUNT 5
#define WINDOW_CLASS_NAME L"Tgm4SectionTimerWindow"
#define BEST_TIME_DEFAULT_SECONDS 999.0
#define ID_BUTTON_HISTORY_PREV 1001
#define ID_BUTTON_HISTORY_NEXT 1002

typedef struct PointerConfig {
    const wchar_t *modeLabel;
    uintptr_t baseOffset;
    const uintptr_t *pointerOffsets;
    size_t pointerOffsetCount;
    uintptr_t timerBaseOffset;
    const uintptr_t *timerPointerOffsets;
    size_t timerPointerOffsetCount;
    int cursorValue;
    int menuCursorPosition;
    int theoreticalMaxLevel;
    int initialTimerFrames;
    const wchar_t *saveFileName;
    const wchar_t *maxLevelFileName;
} PointerConfig;

typedef struct RunSnapshot {
    bool valid;
    wchar_t modeLabel[32];
    int theoreticalMaxLevel;
    int maxLevel;
    int sectionCount;
    double sectionTimes[MAX_SECTION_COUNT];
    double sectionDeltas[MAX_SECTION_COUNT];
    double bestSectionTimes[MAX_SECTION_COUNT];
    int sectionGameTimerFrames[MAX_SECTION_COUNT];
    int backstepCounts[MAX_SECTION_COUNT];
    int tetrisCounts[MAX_SECTION_COUNT];
} RunSnapshot;

static uintptr_t ASUKA_POINTER_OFFSETS[] = {
    0x20,
    0x04,
    0x10,
    0x10,
    0x10,
    0x94
};

static uintptr_t ASUKA_TIMER_POINTER_OFFSETS[] = {
    0x20,
    0x04,
    0x10,
    0x10,
    0x10,
    0x9C
};

static uintptr_t NORMAL_POINTER_OFFSETS[] = {
    0x08,
    0x30,
    0x10,
    0x10,
    0x10,
    0x0C,
    0x98
};

static uintptr_t NORMAL_TIMER_POINTER_OFFSETS[] = {
    0x08,
    0x30,
    0x10,
    0x10,
    0x10,
    0x0C,
    0xA0
};

static uintptr_t NORMAL_EX_POINTER_OFFSETS[] = {
    0x08,
    0x30,
    0x2c,
    0x0C,
    0x0c,
    0x1a4
};

static uintptr_t NORMAL_EX_TIMER_POINTER_OFFSETS[] = {
    0x08,
    0x30,
    0x2C,
    0x0C,
    0x0C,
    0x1DC
};

static uintptr_t CURSOR_POINTER_OFFSETS[] = {
    0x08,
    0x254,
    0x30,
    0x2C,
    0x0C,
    0x1C
};

static uintptr_t MENU_CURSOR_POINTER_OFFSETS[] = {
    0x08,
    0x254,
    0x30,
    0x30,
    0x44,
    0x10,
    0x15
};

static PointerConfig POINTER_CONFIGS[] = {
    {
        L"NORMAL",
        0x00A7E528,
        NORMAL_POINTER_OFFSETS,
        sizeof(NORMAL_POINTER_OFFSETS) / sizeof(NORMAL_POINTER_OFFSETS[0]),
        0x00A7E528,
        NORMAL_TIMER_POINTER_OFFSETS,
        sizeof(NORMAL_TIMER_POINTER_OFFSETS) / sizeof(NORMAL_TIMER_POINTER_OFFSETS[0]),
        9,
        1,
        999,
        0,
        L"section_bests_normal.txt",
        L"max_level_normal.txt"
    },
    {
        L"NORMAL(1.1)",
        0x00A7E528,
        NORMAL_EX_POINTER_OFFSETS,
        sizeof(NORMAL_EX_POINTER_OFFSETS) / sizeof(NORMAL_EX_POINTER_OFFSETS[0]),
        0x00A7E528,
        NORMAL_EX_TIMER_POINTER_OFFSETS,
        sizeof(NORMAL_EX_TIMER_POINTER_OFFSETS) / sizeof(NORMAL_EX_TIMER_POINTER_OFFSETS[0]),
        15,
        1,
        999,
        0,
        L"section_bests_normal_1_1.txt",
        L"max_level_normal_1_1.txt"
    },
    {
        L"NORMAL(2.1)",
        0x00A7E528,
        NORMAL_EX_POINTER_OFFSETS,
        sizeof(NORMAL_EX_POINTER_OFFSETS) / sizeof(NORMAL_EX_POINTER_OFFSETS[0]),
        0x00A7E528,
        NORMAL_EX_TIMER_POINTER_OFFSETS,
        sizeof(NORMAL_EX_TIMER_POINTER_OFFSETS) / sizeof(NORMAL_EX_TIMER_POINTER_OFFSETS[0]),
        16,
        1,
        999,
        0,
        L"section_bests_normal_2_1.txt",
        L"max_level_normal_2_1.txt"
    },
    {
        L"NORMAL(3.1)",
        0x00A7E528,
        NORMAL_EX_POINTER_OFFSETS,
        sizeof(NORMAL_EX_POINTER_OFFSETS) / sizeof(NORMAL_EX_POINTER_OFFSETS[0]),
        0x00A7E528,
        NORMAL_EX_TIMER_POINTER_OFFSETS,
        sizeof(NORMAL_EX_TIMER_POINTER_OFFSETS) / sizeof(NORMAL_EX_TIMER_POINTER_OFFSETS[0]),
        17,
        1,
        2000,
        0,
        L"section_bests_normal_3_1.txt",
        L"max_level_normal_3_1.txt"
    },
    {
        L"NORMAL(4.1)",
        0x00A7E528,
        NORMAL_EX_POINTER_OFFSETS,
        sizeof(NORMAL_EX_POINTER_OFFSETS) / sizeof(NORMAL_EX_POINTER_OFFSETS[0]),
        0x00A7E528,
        NORMAL_EX_TIMER_POINTER_OFFSETS,
        sizeof(NORMAL_EX_TIMER_POINTER_OFFSETS) / sizeof(NORMAL_EX_TIMER_POINTER_OFFSETS[0]),
        18,
        1,
        999,
        0,
        L"section_bests_normal_4_1.txt",
        L"max_level_normal_4_1.txt"
    },
    {
        L"ASUKA",
        0x00A7CD9C,
        ASUKA_POINTER_OFFSETS,
        sizeof(ASUKA_POINTER_OFFSETS) / sizeof(ASUKA_POINTER_OFFSETS[0]),
        0x00A7CD9C,
        ASUKA_TIMER_POINTER_OFFSETS,
        sizeof(ASUKA_TIMER_POINTER_OFFSETS) / sizeof(ASUKA_TIMER_POINTER_OFFSETS[0]),
        5,
        2,
        1300,
        7 * 60 * 60,
        L"section_bests_asuka.txt",
        L"max_level_asuka.txt"
    },
    {
        L"ASUKAEASY",
        0x00A7CD9C,
        ASUKA_POINTER_OFFSETS,
        sizeof(ASUKA_POINTER_OFFSETS) / sizeof(ASUKA_POINTER_OFFSETS[0]),
        0x00A7CD9C,
        ASUKA_TIMER_POINTER_OFFSETS,
        sizeof(ASUKA_TIMER_POINTER_OFFSETS) / sizeof(ASUKA_TIMER_POINTER_OFFSETS[0]),
        10,
        2,
        999,
        30 * 60 * 60,
        L"section_bests_asukaeasy.txt",
        L"max_level_asukaeasy.txt"
    },
    {
        L"MASTER",
        0x00A7E528,
        NORMAL_EX_POINTER_OFFSETS,
        sizeof(NORMAL_EX_POINTER_OFFSETS) / sizeof(NORMAL_EX_POINTER_OFFSETS[0]),
        0x00A7E528,
        NORMAL_EX_TIMER_POINTER_OFFSETS,
        sizeof(NORMAL_EX_TIMER_POINTER_OFFSETS) / sizeof(NORMAL_EX_TIMER_POINTER_OFFSETS[0]),
        1,
        3,
        2600,
        0,
        L"section_bests_master.txt",
        L"max_level_master.txt"
    }
};

typedef struct AppState {
    DWORD processId;
    HANDLE processHandle;
    uintptr_t levelAddress;

    bool attached;
    bool timerRunning;
    bool levelReadable;
    bool modeDetected;
    bool clearResultsOnLevelAdvance;

    int currentLevel;
    int maxLevel;
    int previousLevel;
    int lastRecordedSection;
    int cursorValue;
    int runStartGameTimerFrames;

    ULONGLONG runStartMs;
    ULONGLONG lastPollMs;
    double sectionTimes[MAX_SECTION_COUNT];
    double sectionDeltas[MAX_SECTION_COUNT];
    double bestSectionTimes[MAX_SECTION_COUNT];
    int sectionGameTimerFrames[MAX_SECTION_COUNT];
    int backstepCounts[MAX_SECTION_COUNT];
    int tetrisCounts[MAX_SECTION_COUNT];
    int sectionCount;
    int currentConfigIndex;
    int currentGameTimerFrames;
    HWND historyPrevButton;
    HWND historyNextButton;
    RunSnapshot history[MAX_HISTORY_COUNT];
    int historyCount;
    int historyViewOffset;
    wchar_t saveDirectory[MAX_PATH];
    wchar_t saveFilePath[MAX_PATH];
    wchar_t maxLevelFilePath[MAX_PATH];
    wchar_t configFilePath[MAX_PATH];

    wchar_t statusText[128];
} AppState;

static AppState g_app = {
    0
};

static void close_process(void);
static void load_best_times(void);
static void load_max_level(void);
static void reset_run_preserving_mode(void);
static int section_count_for_max_level(int theoreticalMaxLevel);
static int clamp_section_index_for_level(int level, int theoreticalMaxLevel);
static int completed_section_count_for_level(int level, int theoreticalMaxLevel);
static void format_section_label(wchar_t *buffer, size_t bufferCount, int sectionIndex, int theoreticalMaxLevel);
static void format_game_timer(wchar_t *buffer, size_t bufferCount, int frames);
static void clear_section_results(void);
static void reset_tracking_state(void);
static void archive_current_results_if_any(void);
static const RunSnapshot *current_view_snapshot(void);
static const wchar_t *gm_requirement_text_for_mode(const wchar_t *modeLabel);
static void draw_multiline_text(HDC hdc, int x, int y, const wchar_t *text, COLORREF color);
static void load_pointer_configs(void);

static void copy_status_text(const wchar_t *text) {
    lstrcpynW(g_app.statusText, text, (int)(sizeof(g_app.statusText) / sizeof(g_app.statusText[0])));
}

static const PointerConfig *current_pointer_config(void) {
    if (g_app.currentConfigIndex < 0 ||
        g_app.currentConfigIndex >= (int)(sizeof(POINTER_CONFIGS) / sizeof(POINTER_CONFIGS[0]))) {
        return NULL;
    }
    return &POINTER_CONFIGS[g_app.currentConfigIndex];
}

static void update_button_labels(void) {
    if (g_app.historyPrevButton != NULL) {
        EnableWindow(g_app.historyPrevButton, g_app.historyCount > 0 && g_app.historyViewOffset < g_app.historyCount);
    }

    if (g_app.historyNextButton != NULL) {
        EnableWindow(g_app.historyNextButton, g_app.historyViewOffset > 0);
    }
}

static void archive_current_results_if_any(void) {
    RunSnapshot snapshot;
    const PointerConfig *config;
    int i;

    if (g_app.sectionCount <= 0) {
        return;
    }

    config = current_pointer_config();
    if (config == NULL) {
        return;
    }

    ZeroMemory(&snapshot, sizeof(snapshot));
    snapshot.valid = true;
    lstrcpynW(snapshot.modeLabel, config->modeLabel, (int)(sizeof(snapshot.modeLabel) / sizeof(snapshot.modeLabel[0])));
    snapshot.theoreticalMaxLevel = config->theoreticalMaxLevel;
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
    update_button_labels();
}

static const RunSnapshot *current_view_snapshot(void) {
    if (g_app.historyViewOffset <= 0 || g_app.historyViewOffset > g_app.historyCount) {
        return NULL;
    }
    return &g_app.history[g_app.historyViewOffset - 1];
}

static double frames_to_seconds(int frames) {
    return (double)frames / 60.0;
}

static void initialize_best_times(void) {
    int i;

    for (i = 0; i < MAX_SECTION_COUNT; ++i) {
        g_app.bestSectionTimes[i] = BEST_TIME_DEFAULT_SECONDS;
    }
}

static void initialize_max_level(void) {
    g_app.maxLevel = 0;
}

static void build_save_paths(void) {
    DWORD length;
    wchar_t exePath[MAX_PATH];
    wchar_t *lastSlash;
    const PointerConfig *config;

    exePath[0] = L'\0';
    length = GetModuleFileNameW(NULL, exePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        copy_status_text(L"GetModuleFileName failed");
        return;
    }

    lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash == NULL) {
        copy_status_text(L"Executable path parse failed");
        return;
    }

    *lastSlash = L'\0';
    config = current_pointer_config();
    swprintf(g_app.configFilePath, MAX_PATH, L"%ls\\config.txt", exePath);
    if (config == NULL) {
        g_app.saveDirectory[0] = L'\0';
        g_app.saveFilePath[0] = L'\0';
        g_app.maxLevelFilePath[0] = L'\0';
        return;
    }
    swprintf(g_app.saveDirectory, MAX_PATH, L"%ls\\save", exePath);
    swprintf(g_app.saveFilePath, MAX_PATH, L"%ls\\%ls", g_app.saveDirectory, config->saveFileName);
    swprintf(g_app.maxLevelFilePath, MAX_PATH, L"%ls\\%ls", g_app.saveDirectory, config->maxLevelFileName);
}

static size_t split_csv_offsets(wchar_t *text, uintptr_t *offsets, size_t maxCount) {
    wchar_t *token;
    size_t count;

    count = 0;
    token = wcstok(text, L",");
    while (token != NULL && count < maxCount) {
        offsets[count] = (uintptr_t)wcstoul(token, NULL, 0);
        count += 1;
        token = wcstok(NULL, L",");
    }

    return count;
}

static void write_pointer_configs_to_file(void) {
    FILE *file;
    int i;
    size_t j;

    build_save_paths();
    if (g_app.configFilePath[0] == L'\0') {
        return;
    }

    file = _wfopen(g_app.configFilePath, L"w");
    if (file == NULL) {
        return;
    }

    fwprintf(file, L"mode\tlevel_base\tlevel_offsets\ttimer_base\ttimer_offsets\tcursor_value\tmenu_cursor_position\ttheoretical_max_level\tinitial_timer_frames\n");
    for (i = 0; i < (int)(sizeof(POINTER_CONFIGS) / sizeof(POINTER_CONFIGS[0])); ++i) {
        fwprintf(file, L"%ls\t0x%08IX\t", POINTER_CONFIGS[i].modeLabel, POINTER_CONFIGS[i].baseOffset);
        for (j = 0; j < POINTER_CONFIGS[i].pointerOffsetCount; ++j) {
            fwprintf(file, j == 0 ? L"0x%IX" : L",0x%IX", POINTER_CONFIGS[i].pointerOffsets[j]);
        }
        fwprintf(file, L"\t0x%08IX\t", POINTER_CONFIGS[i].timerBaseOffset);
        for (j = 0; j < POINTER_CONFIGS[i].timerPointerOffsetCount; ++j) {
            fwprintf(file, j == 0 ? L"0x%IX" : L",0x%IX", POINTER_CONFIGS[i].timerPointerOffsets[j]);
        }
        fwprintf(
            file,
            L"\t%d\t%d\t%d\t%d\n",
            POINTER_CONFIGS[i].cursorValue,
            POINTER_CONFIGS[i].menuCursorPosition,
            POINTER_CONFIGS[i].theoreticalMaxLevel,
            POINTER_CONFIGS[i].initialTimerFrames
        );
    }

    fclose(file);
}

static void load_pointer_configs(void) {
    FILE *file;
    wchar_t line[1024];

    build_save_paths();
    if (g_app.configFilePath[0] == L'\0') {
        return;
    }

    file = _wfopen(g_app.configFilePath, L"r");
    if (file == NULL) {
        write_pointer_configs_to_file();
        return;
    }

    if (fgetws(line, sizeof(line) / sizeof(line[0]), file) == NULL) {
        fclose(file);
        write_pointer_configs_to_file();
        return;
    }

    while (fgetws(line, sizeof(line) / sizeof(line[0]), file) != NULL) {
        wchar_t *fields[9];
        wchar_t *token;
        wchar_t levelOffsetsText[512];
        wchar_t timerOffsetsText[512];
        int fieldCount;
        int i;

        fieldCount = 0;
        token = wcstok(line, L"\t\r\n");
        while (token != NULL && fieldCount < 9) {
            fields[fieldCount++] = token;
            token = wcstok(NULL, L"\t\r\n");
        }

        if (fieldCount < 9) {
            continue;
        }

        for (i = 0; i < (int)(sizeof(POINTER_CONFIGS) / sizeof(POINTER_CONFIGS[0])); ++i) {
            if (wcscmp(POINTER_CONFIGS[i].modeLabel, fields[0]) != 0) {
                continue;
            }

            POINTER_CONFIGS[i].baseOffset = (uintptr_t)wcstoul(fields[1], NULL, 0);
            lstrcpynW(levelOffsetsText, fields[2], (int)(sizeof(levelOffsetsText) / sizeof(levelOffsetsText[0])));
            POINTER_CONFIGS[i].pointerOffsetCount = split_csv_offsets(
                levelOffsetsText,
                (uintptr_t *)POINTER_CONFIGS[i].pointerOffsets,
                POINTER_CONFIGS[i].pointerOffsetCount
            );
            POINTER_CONFIGS[i].timerBaseOffset = (uintptr_t)wcstoul(fields[3], NULL, 0);
            lstrcpynW(timerOffsetsText, fields[4], (int)(sizeof(timerOffsetsText) / sizeof(timerOffsetsText[0])));
            POINTER_CONFIGS[i].timerPointerOffsetCount = split_csv_offsets(
                timerOffsetsText,
                (uintptr_t *)POINTER_CONFIGS[i].timerPointerOffsets,
                POINTER_CONFIGS[i].timerPointerOffsetCount
            );
            POINTER_CONFIGS[i].cursorValue = _wtoi(fields[5]);
            POINTER_CONFIGS[i].menuCursorPosition = _wtoi(fields[6]);
            POINTER_CONFIGS[i].theoreticalMaxLevel = _wtoi(fields[7]);
            POINTER_CONFIGS[i].initialTimerFrames = _wtoi(fields[8]);
            break;
        }
    }

    fclose(file);
}

static void save_best_times(void) {
    FILE *file;
    int i;

    if (g_app.saveFilePath[0] == L'\0') {
        return;
    }

    CreateDirectoryW(g_app.saveDirectory, NULL);

    file = _wfopen(g_app.saveFilePath, L"w");
    if (file == NULL) {
        return;
    }

    for (i = 0; i < MAX_SECTION_COUNT; ++i) {
        fwprintf(file, L"%.3f\n", g_app.bestSectionTimes[i]);
    }

    fclose(file);
}

static void load_best_times(void) {
    FILE *file;
    int i;
    double value;

    initialize_best_times();
    build_save_paths();

    if (g_app.saveFilePath[0] == L'\0') {
        return;
    }

    CreateDirectoryW(g_app.saveDirectory, NULL);

    file = _wfopen(g_app.saveFilePath, L"r");
    if (file == NULL) {
        save_best_times();
        return;
    }

    for (i = 0; i < MAX_SECTION_COUNT; ++i) {
        if (fwscanf(file, L"%lf", &value) != 1) {
            break;
        }
        g_app.bestSectionTimes[i] = value;
    }

    fclose(file);
}

static void save_max_level(void) {
    FILE *file;

    if (g_app.maxLevelFilePath[0] == L'\0') {
        return;
    }

    CreateDirectoryW(g_app.saveDirectory, NULL);

    file = _wfopen(g_app.maxLevelFilePath, L"w");
    if (file == NULL) {
        return;
    }

    fwprintf(file, L"%d\n", g_app.maxLevel);
    fclose(file);
}

static void load_max_level(void) {
    FILE *file;
    int value;

    initialize_max_level();
    build_save_paths();

    if (g_app.maxLevelFilePath[0] == L'\0') {
        return;
    }

    CreateDirectoryW(g_app.saveDirectory, NULL);

    file = _wfopen(g_app.maxLevelFilePath, L"r");
    if (file == NULL) {
        save_max_level();
        return;
    }

    if (fwscanf(file, L"%d", &value) == 1 && value > 0) {
        g_app.maxLevel = value;
    }

    fclose(file);
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
    g_app.modeDetected = false;
    g_app.runStartMs = 0;
    g_app.runStartGameTimerFrames = -1;
    g_app.currentGameTimerFrames = -1;
    g_app.clearResultsOnLevelAdvance = false;
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

    processId = find_process_id(L"tgm4.exe");
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
        swprintf(g_app.statusText, 128, L"OpenProcess failed (pid=%lu)", processId);
        return false;
    }

    g_app.processId = processId;
    g_app.processHandle = processHandle;
    g_app.attached = true;
    swprintf(g_app.statusText, 128, L"Attached to tgm4.exe (pid=%lu)", processId);
    return true;
}

static bool resolve_pointer_chain(uintptr_t baseOffset, const uintptr_t *offsets, size_t offsetCount, uintptr_t *resolvedAddress) {
    uintptr_t address;
    SIZE_T bytesRead;
    size_t i;

    if (g_app.processHandle == NULL) {
        return false;
    }

    address = find_module_base_address(g_app.processId, L"tgm4.exe");
    if (address == 0) {
        copy_status_text(L"Module not found: tgm4.exe");
        return false;
    }

    address += baseOffset;

    for (i = 0; i < offsetCount; ++i) {
        uint32_t nextPtr32;

        if (!ReadProcessMemory(g_app.processHandle, (LPCVOID)address, &nextPtr32, sizeof(nextPtr32), &bytesRead) ||
            bytesRead != sizeof(nextPtr32)) {
            swprintf(g_app.statusText, 128, L"Pointer read failed at step %zu", i);
            return false;
        }

        address = (uintptr_t)nextPtr32 + offsets[i];
    }

    *resolvedAddress = address;
    return true;
}

static bool resolve_level_address(uintptr_t *resolvedAddress) {
    const PointerConfig *config;

    config = current_pointer_config();
    if (config == NULL) {
        return false;
    }

    return resolve_pointer_chain(
        config->baseOffset,
        config->pointerOffsets,
        config->pointerOffsetCount,
        resolvedAddress
    );
}

static bool read_int_from_address(uintptr_t address, int *valueOut) {
    SIZE_T bytesRead;

    if (!ReadProcessMemory(g_app.processHandle, (LPCVOID)address, valueOut, sizeof(*valueOut), &bytesRead) ||
        bytesRead != sizeof(*valueOut)) {
        return false;
    }

    return true;
}

static bool read_game_timer_frames(int *framesOut) {
    uintptr_t resolvedAddress;
    const PointerConfig *config;

    config = current_pointer_config();
    if (config == NULL || g_app.processHandle == NULL) {
        return false;
    }

    if (!resolve_pointer_chain(
            config->timerBaseOffset,
            config->timerPointerOffsets,
            config->timerPointerOffsetCount,
            &resolvedAddress)) {
        return false;
    }

    return read_int_from_address(resolvedAddress, framesOut);
}

static bool read_byte_from_address(uintptr_t address, uint8_t *valueOut) {
    SIZE_T bytesRead;

    if (!ReadProcessMemory(g_app.processHandle, (LPCVOID)address, valueOut, sizeof(*valueOut), &bytesRead) ||
        bytesRead != sizeof(*valueOut)) {
        return false;
    }

    return true;
}

static int find_config_index_for_cursor_value(int cursorValue) {
    int i;

    for (i = 0; i < (int)(sizeof(POINTER_CONFIGS) / sizeof(POINTER_CONFIGS[0])); ++i) {
        if (POINTER_CONFIGS[i].cursorValue == cursorValue) {
            return i;
        }
    }

    return -1;
}

static bool detect_mode_from_cursor(void) {
    uintptr_t cursorAddress;
    uintptr_t menuCursorAddress;
    int cursorValue;
    uint8_t menuCursorPosition;
    int newConfigIndex;
    const PointerConfig *config;

    if (!resolve_pointer_chain(
            0x00A7E528,
            CURSOR_POINTER_OFFSETS,
            sizeof(CURSOR_POINTER_OFFSETS) / sizeof(CURSOR_POINTER_OFFSETS[0]),
            &cursorAddress)) {
        g_app.modeDetected = false;
        return false;
    }

    if (!read_int_from_address(cursorAddress, &cursorValue)) {
        copy_status_text(L"Cursor read failed");
        g_app.modeDetected = false;
        return false;
    }

    if (!resolve_pointer_chain(
            0x00A7E528,
            MENU_CURSOR_POINTER_OFFSETS,
            sizeof(MENU_CURSOR_POINTER_OFFSETS) / sizeof(MENU_CURSOR_POINTER_OFFSETS[0]),
            &menuCursorAddress)) {
        if (g_app.currentConfigIndex >= 0) {
            g_app.modeDetected = true;
            return true;
        }
        g_app.modeDetected = false;
        return false;
    }

    if (!read_byte_from_address(menuCursorAddress, &menuCursorPosition)) {
        copy_status_text(L"Menu cursor read failed");
        if (g_app.currentConfigIndex >= 0) {
            g_app.modeDetected = true;
            return true;
        }
        g_app.modeDetected = false;
        return false;
    }

    g_app.cursorValue = cursorValue;
    newConfigIndex = find_config_index_for_cursor_value(cursorValue);
    if (newConfigIndex < 0) {
        if (g_app.currentConfigIndex >= 0) {
            g_app.modeDetected = true;
            return true;
        }
        g_app.modeDetected = false;
        return false;
    }

    if (g_app.currentConfigIndex != newConfigIndex) {
        archive_current_results_if_any();
        g_app.currentConfigIndex = newConfigIndex;
        reset_run_preserving_mode();
        load_best_times();
        load_max_level();
    }

    config = current_pointer_config();
    if (config == NULL) {
        g_app.modeDetected = false;
        return false;
    }

    if (config->menuCursorPosition != (int)menuCursorPosition) {
        if (g_app.currentConfigIndex >= 0) {
            g_app.modeDetected = true;
            return true;
        }
        g_app.modeDetected = false;
        return false;
    }

    g_app.modeDetected = true;
    return true;
}

static bool read_level_value(int *levelOut) {
    uintptr_t resolvedAddress;
    SIZE_T bytesRead;

    if (g_app.processHandle == NULL) {
        return false;
    }

    if (!resolve_level_address(&resolvedAddress)) {
        g_app.levelReadable = false;
        return false;
    }

    g_app.levelAddress = resolvedAddress;

    if (!ReadProcessMemory(g_app.processHandle, (LPCVOID)g_app.levelAddress, levelOut, sizeof(*levelOut), &bytesRead) ||
        bytesRead != sizeof(*levelOut)) {
        g_app.levelAddress = 0;
        g_app.levelReadable = false;
        swprintf(g_app.statusText, 128, L"Level read failed, retrying address resolve");
        return false;
    }

    g_app.levelReadable = true;
    return true;
}

static void record_new_sections(int currentLevel) {
    int completedSectionCount;
    int sectionIndex;
    double splitTime;
    double previousBestTime;
    int previousFrames;
    int frameDelta;
    const PointerConfig *config;

    config = current_pointer_config();
    if (config == NULL) {
        return;
    }

    if (g_app.runStartGameTimerFrames < 0) {
        g_app.runStartGameTimerFrames = config->initialTimerFrames;
    }

    completedSectionCount = completed_section_count_for_level(currentLevel, config->theoreticalMaxLevel);
    while (g_app.lastRecordedSection < completedSectionCount - 1) {
        sectionIndex = g_app.lastRecordedSection + 1;

        if (sectionIndex >= 0 &&
            sectionIndex < MAX_SECTION_COUNT &&
            g_app.currentGameTimerFrames >= 0 &&
            g_app.runStartGameTimerFrames >= 0) {
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
    const PointerConfig *config;

    config = current_pointer_config();
    if (config == NULL) {
        return;
    }

    if (level > config->theoreticalMaxLevel) {
        swprintf(
            g_app.statusText,
            128,
            L"%ls ignoring out-of-range level %d",
            config->modeLabel,
            level
        );
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
            swprintf(g_app.statusText, 128, L"Waiting for level 0 (current=%d)", level);
        }
        return;
    }

    if (level == 0 && g_app.previousLevel > 0) {
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
        currentSectionIndex = clamp_section_index_for_level(level, config->theoreticalMaxLevel);
        g_app.tetrisCounts[currentSectionIndex] += 1;
    }

    if (g_app.previousLevel >= 0 && level + LEVEL_RESET_THRESHOLD < g_app.previousLevel) {
        reset_tracking_state();
        g_app.currentLevel = level;
        g_app.previousLevel = level;
        g_app.modeDetected = true;
        swprintf(g_app.statusText, 128, L"Detected level reset, waiting for level 0");
        return;
    }

    g_app.currentLevel = level;
    if (level > g_app.maxLevel) {
        g_app.maxLevel = level;
        save_max_level();
    }
    record_new_sections(level);
    g_app.previousLevel = level;
    swprintf(g_app.statusText, 128, L"%ls tracking (level=%d)", config->modeLabel, level);
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

    if (!read_game_timer_frames(&g_app.currentGameTimerFrames)) {
        g_app.currentGameTimerFrames = -1;
    }

    if (g_app.timerRunning && g_app.runStartGameTimerFrames < 0 && config != NULL) {
        g_app.runStartGameTimerFrames = config->initialTimerFrames;
    }

    g_app.lastPollMs = GetTickCount64();
    update_timer_from_level(level);
}

static void draw_text_line(HDC hdc, int *y, const wchar_t *text, COLORREF color) {
    SetTextColor(hdc, color);
    TextOutW(hdc, 12, *y, text, (int)wcslen(text));
    *y += 22;
}

static COLORREF color_for_delta(double deltaSeconds) {
    if (deltaSeconds < -0.0005) {
        return RGB(120, 255, 120);
    }
    if (deltaSeconds > 0.0005) {
        return RGB(255, 120, 120);
    }
    return RGB(240, 240, 240);
}

static void draw_table_grid(HDC hdc, int left, int top, int right, int bottom, const int *columns, int columnCount, int rowHeight, int rowCount) {
    HPEN pen;
    HPEN oldPen;
    HBRUSH oldBrush;
    int i;
    int y;

    pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
    oldPen = (HPEN)SelectObject(hdc, pen);
    oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    Rectangle(hdc, left, top, right, bottom);

    for (i = 0; i < columnCount; ++i) {
        MoveToEx(hdc, columns[i], top, NULL);
        LineTo(hdc, columns[i], bottom);
    }

    for (i = 1; i < rowCount; ++i) {
        y = top + rowHeight * i;
        MoveToEx(hdc, left, y, NULL);
        LineTo(hdc, right, y);
    }

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

static void draw_table_text(HDC hdc, int x, int y, const wchar_t *text, COLORREF color) {
    SetTextColor(hdc, color);
    TextOutW(hdc, x, y, text, (int)wcslen(text));
}

static int measure_text_width(HDC hdc, const wchar_t *text) {
    SIZE size;

    if (!GetTextExtentPoint32W(hdc, text, (int)wcslen(text), &size)) {
        return 0;
    }

    return size.cx;
}

static int max_int(int a, int b) {
    return a > b ? a : b;
}

static int section_count_for_max_level(int theoreticalMaxLevel) {
    return (theoreticalMaxLevel + 99) / 100;
}

static int last_section_index_for_max_level(int theoreticalMaxLevel) {
    int sectionCount;

    sectionCount = section_count_for_max_level(theoreticalMaxLevel);
    if (sectionCount <= 0) {
        return 0;
    }
    return sectionCount - 1;
}

static int clamp_section_index_for_level(int level, int theoreticalMaxLevel) {
    int sectionIndex;
    int lastSectionIndex;

    sectionIndex = level / 100;
    lastSectionIndex = last_section_index_for_max_level(theoreticalMaxLevel);
    if (sectionIndex < 0) {
        return 0;
    }
    if (sectionIndex > lastSectionIndex) {
        return lastSectionIndex;
    }
    return sectionIndex;
}

static int completed_section_count_for_level(int level, int theoreticalMaxLevel) {
    if (level >= theoreticalMaxLevel) {
        return section_count_for_max_level(theoreticalMaxLevel);
    }
    return level / 100;
}

static void format_section_label(wchar_t *buffer, size_t bufferCount, int sectionIndex, int theoreticalMaxLevel) {
    int sectionStart;
    int sectionEnd;

    sectionStart = sectionIndex * 100;
    sectionEnd = (sectionIndex == last_section_index_for_max_level(theoreticalMaxLevel))
        ? theoreticalMaxLevel
        : (sectionIndex + 1) * 100;

    swprintf(buffer, bufferCount, L"%4d-%4d", sectionStart, sectionEnd);
}

static void format_game_timer(wchar_t *buffer, size_t bufferCount, int frames) {
    int totalCentiseconds;
    int minutes;
    int seconds;
    int centiseconds;

    if (frames < 0) {
        swprintf(buffer, bufferCount, L"-");
        return;
    }

    totalCentiseconds = (frames * 100 + 30) / 60;
    minutes = totalCentiseconds / 6000;
    seconds = (totalCentiseconds / 100) % 60;
    centiseconds = totalCentiseconds % 100;
    swprintf(buffer, bufferCount, L"%d:%02d.%02d", minutes, seconds, centiseconds);
}

static const wchar_t *gm_requirement_text_for_mode(const wchar_t *modeLabel) {
    if (wcscmp(modeLabel, L"NORMAL(1.1)") == 0) {
        return
            L"GM Requirements\n"
            L"SCORE 280000+\n"
            L"Lv999 within 8:55\n"
            L"6 tetrises in credit roll";
    }

    if (wcscmp(modeLabel, L"NORMAL(2.1)") == 0) {
        return
            L"GM Requirements\n"
            L"SCORE 260000+\n"
            L"Lv999 within 5:00 (Lv500 with in 3:20)\n"
            L"8 tetrises in credit roll";
    }

    if (wcscmp(modeLabel, L"NORMAL(3.1)") == 0) {
        return
            L"GM Requirements\n"
            L"Lv1000 within 4:01\n"
            L"Lv1300 within 4:30\n"
            L"Lv2000 within 6:46\n"
            L"21 triples+ in credit roll";
    }

    return NULL;
}

static void draw_multiline_text(HDC hdc, int x, int y, const wchar_t *text, COLORREF color) {
    const wchar_t *lineStart;
    const wchar_t *lineEnd;
    int currentY;

    if (text == NULL) {
        return;
    }

    SetTextColor(hdc, color);
    lineStart = text;
    currentY = y;

    while (*lineStart != L'\0') {
        lineEnd = wcschr(lineStart, L'\n');
        if (lineEnd == NULL) {
            TextOutW(hdc, x, currentY, lineStart, (int)wcslen(lineStart));
            break;
        }

        TextOutW(hdc, x, currentY, lineStart, (int)(lineEnd - lineStart));
        currentY += 22;
        lineStart = lineEnd + 1;
    }
}

static void paint_window(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc;
    HDC memoryDc;
    RECT clientRect;
    HBRUSH backgroundBrush;
    HBITMAP backBufferBitmap;
    HBITMAP oldBitmap;
    HFONT font;
    HFONT oldFont;
    int y;
    wchar_t line[128];
    int i;
    int tableLeft;
    int tableTop;
    int tableRight;
    int rowHeight;
    int visibleSectionCount;
    int columns[5];
    int cellPadding;
    int sectionWidth;
    int deltaWidth;
    int bestWidth;
    int gameTimeWidth;
    int backWidth;
    int tetWidth;
    int infoTop;
    const PointerConfig *config;
    const RunSnapshot *snapshot;
    const wchar_t *displayModeLabel;
    const wchar_t *gmRequirementText;
    int displayTheoreticalMaxLevel;
    int displayMaxLevel;
    int displaySectionCount;
    wchar_t sectionLabel[32];
    wchar_t gameTimeText[32];

    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &clientRect);

    memoryDc = CreateCompatibleDC(hdc);
    backBufferBitmap = CreateCompatibleBitmap(
        hdc,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top
    );
    oldBitmap = (HBITMAP)SelectObject(memoryDc, backBufferBitmap);
    font = CreateFontW(
        -18,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_MODERN,
        L"Consolas"
    );
    oldFont = (HFONT)SelectObject(memoryDc, font);

    backgroundBrush = CreateSolidBrush(RGB(24, 24, 24));
    FillRect(memoryDc, &clientRect, backgroundBrush);
    DeleteObject(backgroundBrush);

    SetBkMode(memoryDc, TRANSPARENT);
    config = current_pointer_config();
    snapshot = current_view_snapshot();
    displayModeLabel = (config != NULL) ? config->modeLabel : L"-";
    gmRequirementText = NULL;
    displayTheoreticalMaxLevel = (config != NULL) ? config->theoreticalMaxLevel : 0;
    displayMaxLevel = g_app.maxLevel;
    displaySectionCount = g_app.sectionCount;

    if (snapshot != NULL && snapshot->valid) {
        displayModeLabel = snapshot->modeLabel;
        displayTheoreticalMaxLevel = snapshot->theoreticalMaxLevel;
        displayMaxLevel = snapshot->maxLevel;
        displaySectionCount = snapshot->sectionCount;
    }
    gmRequirementText = gm_requirement_text_for_mode(displayModeLabel);

    y = 12;
    draw_text_line(memoryDc, &y, L"TGM4 Section Timer", RGB(240, 240, 240));
    if (snapshot != NULL && snapshot->valid) {
        swprintf(line, 128, L"Viewing History: %d/%d", g_app.historyViewOffset, g_app.historyCount);
        draw_text_line(memoryDc, &y, line, RGB(255, 210, 140));
    } else if (g_app.modeDetected) {
        draw_text_line(memoryDc, &y, g_app.statusText, RGB(160, 220, 255));
    } else {
        y += 22;
    }

    if (g_app.modeDetected || (snapshot != NULL && snapshot->valid)) {
        if (displayTheoreticalMaxLevel > 0) {
            swprintf(
                line,
                128,
                L"Mode: %ls    Current Level: %d    Max Level: %d",
                displayModeLabel,
                snapshot != NULL ? displayMaxLevel : g_app.currentLevel,
                displayMaxLevel
            );
            draw_text_line(memoryDc, &y, line, RGB(240, 240, 240));
        }
    } else {
        y += 22;
    }

    if (snapshot == NULL && g_app.modeDetected && g_app.timerRunning && g_app.levelReadable && g_app.currentGameTimerFrames >= 0) {
        format_game_timer(gameTimeText, sizeof(gameTimeText) / sizeof(gameTimeText[0]), g_app.currentGameTimerFrames);
        swprintf(line, 128, L"Run Time: %ls", gameTimeText);
        draw_text_line(memoryDc, &y, line, RGB(180, 255, 180));
    } else {
        y += 22;
    }

    if (!g_app.modeDetected && snapshot == NULL) {
        BitBlt(
            hdc,
            0,
            0,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top,
            memoryDc,
            0,
            0,
            SRCCOPY
        );

        SelectObject(memoryDc, oldFont);
        SelectObject(memoryDc, oldBitmap);
        DeleteObject(font);
        DeleteObject(backBufferBitmap);
        DeleteDC(memoryDc);

        EndPaint(hwnd, &ps);
        return;
    }

    y += 8;
    draw_text_line(memoryDc, &y, L"Section Times", RGB(255, 230, 160));

    tableLeft = 12;
    tableTop = y;
    rowHeight = 26;
    visibleSectionCount = section_count_for_max_level(displayTheoreticalMaxLevel);
    if (visibleSectionCount > MAX_SECTION_COUNT) {
        visibleSectionCount = MAX_SECTION_COUNT;
    }
    cellPadding = 20;

    format_section_label(sectionLabel, sizeof(sectionLabel) / sizeof(sectionLabel[0]), visibleSectionCount - 1, displayTheoreticalMaxLevel);
    sectionWidth = max_int(measure_text_width(memoryDc, L"Section"), measure_text_width(memoryDc, sectionLabel)) + cellPadding;
    deltaWidth = max_int(measure_text_width(memoryDc, L"Delta"), measure_text_width(memoryDc, L"+999.999 s")) + cellPadding;
    bestWidth = max_int(measure_text_width(memoryDc, L"Best"), measure_text_width(memoryDc, L"999.999 s")) + cellPadding;
    gameTimeWidth = max_int(measure_text_width(memoryDc, L"GameTime"), measure_text_width(memoryDc, L"99:59.99")) + cellPadding;
    backWidth = max_int(measure_text_width(memoryDc, L"Back"), measure_text_width(memoryDc, L"99")) + cellPadding;
    tetWidth = max_int(measure_text_width(memoryDc, L"Tet"), measure_text_width(memoryDc, L"99")) + cellPadding;

    columns[0] = tableLeft + sectionWidth;
    columns[1] = columns[0] + gameTimeWidth;
    columns[2] = columns[1] + deltaWidth;
    columns[3] = columns[2] + bestWidth;
    columns[4] = columns[3] + backWidth;
    tableRight = columns[4] + tetWidth;

    draw_table_grid(
        memoryDc,
        tableLeft,
        tableTop,
        tableRight,
        tableTop + rowHeight * (visibleSectionCount + 1),
        columns,
        5,
        rowHeight,
        visibleSectionCount + 1
    );

    draw_table_text(memoryDc, tableLeft + 10, tableTop + 6, L"Section", RGB(255, 230, 160));
    draw_table_text(memoryDc, columns[0] + 10, tableTop + 6, L"GameTime", RGB(255, 230, 160));
    draw_table_text(memoryDc, columns[1] + 10, tableTop + 6, L"Delta", RGB(255, 230, 160));
    draw_table_text(memoryDc, columns[2] + 10, tableTop + 6, L"Best", RGB(255, 230, 160));
    draw_table_text(memoryDc, columns[3] + 10, tableTop + 6, L"Back", RGB(255, 230, 160));
    draw_table_text(memoryDc, columns[4] + 10, tableTop + 6, L"Tet", RGB(255, 230, 160));

    for (i = 0; i < visibleSectionCount; ++i) {
        int rowY;
        double delta;
        wchar_t deltaSign;

        rowY = tableTop + rowHeight * (i + 1) + 6;
        delta = snapshot != NULL ? snapshot->sectionDeltas[i] : g_app.sectionDeltas[i];
        deltaSign = delta < 0.0 ? L'-' : L'+';

        format_section_label(line, 128, i, displayTheoreticalMaxLevel);
        draw_table_text(memoryDc, tableLeft + 10, rowY, line, RGB(240, 240, 240));

        if (i < displaySectionCount && (snapshot != NULL ? snapshot->sectionTimes[i] : g_app.sectionTimes[i]) >= 0.0) {
            format_game_timer(line, 128, snapshot != NULL ? snapshot->sectionGameTimerFrames[i] : g_app.sectionGameTimerFrames[i]);
            draw_table_text(memoryDc, columns[0] + 10, rowY, line, color_for_delta(delta));

            swprintf(line, 128, L"%lc%.3f s", deltaSign, delta < 0.0 ? -delta : delta);
            draw_table_text(memoryDc, columns[1] + 10, rowY, line, color_for_delta(delta));
        } else {
            draw_table_text(memoryDc, columns[0] + 10, rowY, L"-", RGB(140, 140, 140));
            draw_table_text(memoryDc, columns[1] + 10, rowY, L"-", RGB(140, 140, 140));
        }

        swprintf(line, 128, L"%.3f s", snapshot != NULL ? snapshot->bestSectionTimes[i] : g_app.bestSectionTimes[i]);
        draw_table_text(memoryDc, columns[2] + 10, rowY, line, RGB(200, 200, 200));

        swprintf(line, 128, L"%d", snapshot != NULL ? snapshot->backstepCounts[i] : g_app.backstepCounts[i]);
        draw_table_text(memoryDc, columns[3] + 10, rowY, line, RGB(240, 240, 240));

        swprintf(line, 128, L"%d", snapshot != NULL ? snapshot->tetrisCounts[i] : g_app.tetrisCounts[i]);
        draw_table_text(memoryDc, columns[4] + 10, rowY, line, RGB(240, 240, 240));
    }

    infoTop = tableTop + rowHeight * (visibleSectionCount + 1) + 20;
    if (gmRequirementText != NULL) {
        draw_multiline_text(memoryDc, tableLeft, infoTop, gmRequirementText, RGB(200, 220, 255));
    }

    BitBlt(
        hdc,
        0,
        0,
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top,
        memoryDc,
        0,
        0,
        SRCCOPY
    );

    SelectObject(memoryDc, oldFont);
    SelectObject(memoryDc, oldBitmap);
    DeleteObject(font);
    DeleteObject(backBufferBitmap);
    DeleteDC(memoryDc);

    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_app.historyPrevButton = CreateWindowW(
            L"BUTTON",
            L"<-",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            500,
            8,
            50,
            28,
            hwnd,
            (HMENU)ID_BUTTON_HISTORY_PREV,
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL
        );
        g_app.historyNextButton = CreateWindowW(
            L"BUTTON",
            L"->",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            556,
            8,
            50,
            28,
            hwnd,
            (HMENU)ID_BUTTON_HISTORY_NEXT,
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL
        );
        update_button_labels();
        SetTimer(hwnd, 1, POLL_INTERVAL_MS, NULL);
        SetTimer(hwnd, 2, WINDOW_REFRESH_MS, NULL);
        return 0;

    case WM_COMMAND:
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
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        630,
        900,
        NULL,
        NULL,
        instance,
        NULL
    );

    if (hwnd == NULL) {
        return 1;
    }

    ShowWindow(hwnd, showCommand);

    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
