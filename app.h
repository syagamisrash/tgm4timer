#ifndef APP_H
#define APP_H

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <wchar.h>

#include "config.h"

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define MAX_SECTION_COUNT 30
#define MAX_HISTORY_COUNT 20
#define MAX_POINTER_OFFSET_COUNT 16
#define PACE_SAMPLE_COUNT 4096
#define COLUMN_COUNT 6
#define WINDOW_CLASS_NAME L"Tgm4SectionTimerWindow"
#define TARGET_PROCESS_NAME L"tgm4.exe"
#define TARGET_MODULE_NAME L"tgm4.exe"
#define BEST_TIME_DEFAULT_SECONDS 999.0

#define ID_BUTTON_HISTORY_PREV 1001
#define ID_BUTTON_HISTORY_NEXT 1002
#define ID_BUTTON_SETTINGS 1003
#define ID_BUTTON_BACK 1004
#define ID_CHECK_GAMETIME 1101
#define ID_CHECK_DELTA 1102
#define ID_CHECK_SECTIONTIME 1103
#define ID_CHECK_BEST 1104
#define ID_CHECK_BACKCOL 1105
#define ID_CHECK_TET 1106
#define ID_CHECK_PROGRESS 1107
#define ID_CHECK_DEBUG 1108
#define ID_COMBO_RESET_MODE 1201
#define ID_BUTTON_RESET_BEST 1202

typedef struct PointerConfig {
    const wchar_t *modeLabel;
    int cursorValue;
    int menuCursorPosition;
    int theoreticalMaxLevel;
    int initialTimerFrames;
    const wchar_t *saveFileName;
    const wchar_t *maxLevelFileName;
} PointerConfig;

typedef struct PointerChains {
    uintptr_t baseOffset;
    uintptr_t gameModeOffsets[MAX_POINTER_OFFSET_COUNT];
    size_t gameModeOffsetCount;
    uintptr_t menuCursorYOffsets[MAX_POINTER_OFFSET_COUNT];
    size_t menuCursorYOffsetCount;
    uintptr_t levelOffsets[MAX_POINTER_OFFSET_COUNT];
    size_t levelOffsetCount;
    uintptr_t timerOffsets[MAX_POINTER_OFFSET_COUNT];
    size_t timerOffsetCount;
} PointerChains;

typedef struct RunSnapshot {
    bool valid;
    wchar_t modeLabel[32];
    int theoreticalMaxLevel;
    int finalLevel;
    int maxLevel;
    int sectionCount;
    double sectionTimes[MAX_SECTION_COUNT];
    double sectionDeltas[MAX_SECTION_COUNT];
    double bestSectionTimes[MAX_SECTION_COUNT];
    int sectionGameTimerFrames[MAX_SECTION_COUNT];
    int backstepCounts[MAX_SECTION_COUNT];
    int tetrisCounts[MAX_SECTION_COUNT];
} RunSnapshot;

typedef struct PaceSample {
    ULONGLONG timeMs;
    int level;
} PaceSample;

typedef enum AppScreen {
    SCREEN_MAIN = 0,
    SCREEN_SETTINGS = 1
} AppScreen;

typedef enum TableColumnId {
    COLUMN_GAME_TIME = 0,
    COLUMN_DELTA = 1,
    COLUMN_SECTION_TIME = 2,
    COLUMN_BEST = 3,
    COLUMN_BACK = 4,
    COLUMN_TET = 5
} TableColumnId;

typedef struct TableColumnDefinition {
    TableColumnId id;
    int controlId;
    const wchar_t *header;
    const wchar_t *sampleText;
} TableColumnDefinition;

typedef struct AppState {
    DWORD processId;
    HANDLE processHandle;
    uintptr_t levelAddress;

    bool attached;
    bool timerRunning;
    bool levelReadable;
    bool timerReadable;
    bool modeDetected;
    bool clearResultsOnLevelAdvance;
    bool resultsArchivedPendingClear;

    int currentLevel;
    int maxLevel;
    int previousLevel;
    int lastRecordedSection;
    int cursorValue;
    int cursorYValue;
    int runStartGameTimerFrames;
    double maxLevelsPerMinute;

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
    uintptr_t cursorAddress;
    uintptr_t menuCursorAddress;
    uintptr_t timerAddress;
    PaceSample paceSamples[PACE_SAMPLE_COUNT];
    int paceSampleStart;
    int paceSampleCount;
    AppScreen currentScreen;
    bool showColumnGameTime;
    bool showColumnDelta;
    bool showColumnSectionTime;
    bool showColumnBest;
    bool showColumnBack;
    bool showColumnTet;
    bool showProgressBar;
    bool showDebugInfo;
    HWND historyPrevButton;
    HWND historyNextButton;
    HWND settingsButton;
    HWND backButton;
    HWND gameTimeCheck;
    HWND deltaCheck;
    HWND sectionTimeCheck;
    HWND bestCheck;
    HWND backColCheck;
    HWND tetCheck;
    HWND progressCheck;
    HWND debugCheck;
    HWND resetModeCombo;
    HWND resetBestButton;
    RunSnapshot history[MAX_HISTORY_COUNT];
    int historyCount;
    int historyViewOffset;
    wchar_t saveDirectory[MAX_PATH];
    wchar_t saveFilePath[MAX_PATH];
    wchar_t maxLevelFilePath[MAX_PATH];
    wchar_t configFilePath[MAX_PATH];
    wchar_t statusText[128];
    wchar_t levelReadStatus[128];
    wchar_t timerReadStatus[128];
} AppState;

extern PointerConfig POINTER_CONFIGS[];
extern PointerChains POINTER_CHAINS;
extern const TableColumnDefinition TABLE_COLUMNS[COLUMN_COUNT];
extern AppState g_app;

void copy_status_text(const wchar_t *text);

int pointer_config_count(void);
const PointerConfig *current_pointer_config(void);

void load_pointer_configs(void);
void load_best_times(void);
void save_best_times(void);
void load_max_level(void);
void save_max_level(void);
void build_save_paths(void);
void reset_best_times_for_config_index(int configIndex);

int section_count_for_max_level(int theoreticalMaxLevel);
int clamp_section_index_for_level(int level, int theoreticalMaxLevel);
int completed_section_count_for_level(int level, int theoreticalMaxLevel);
void format_section_label(wchar_t *buffer, size_t bufferCount, int sectionIndex, int theoreticalMaxLevel);
void format_game_timer(wchar_t *buffer, size_t bufferCount, int frames);
void format_seconds_as_game_time(wchar_t *buffer, size_t bufferCount, double secondsValue);

const RunSnapshot *current_view_snapshot(void);
double current_levels_per_minute(void);

void update_button_labels(void);
void update_screen_controls(void);
void apply_column_toggle_from_control(int controlId);
void create_main_screen_controls(HWND hwnd, HINSTANCE instance);
void create_settings_screen_controls(HWND hwnd, HINSTANCE instance);
void populate_reset_mode_combo(void);
void paint_window(HWND hwnd);

#endif
