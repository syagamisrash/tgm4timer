#include "app.h"

PointerChains POINTER_CHAINS;

PointerConfig POINTER_CONFIGS[] = {
    { .modeLabel = L"NORMAL", .cursorValue = 9, .menuCursorPosition = 1, .theoreticalMaxLevel = 999, .initialTimerFrames = 0, .saveFileName = L"section_bests_normal.txt", .maxLevelFileName = L"max_level_normal.txt" },
    { .modeLabel = L"NORMAL(1.1)", .cursorValue = 15, .menuCursorPosition = 1, .theoreticalMaxLevel = 999, .initialTimerFrames = 0, .saveFileName = L"section_bests_normal_1_1.txt", .maxLevelFileName = L"max_level_normal_1_1.txt" },
    { .modeLabel = L"NORMAL(2.1)", .cursorValue = 16, .menuCursorPosition = 1, .theoreticalMaxLevel = 999, .initialTimerFrames = 0, .saveFileName = L"section_bests_normal_2_1.txt", .maxLevelFileName = L"max_level_normal_2_1.txt" },
    { .modeLabel = L"NORMAL(3.1)", .cursorValue = 17, .menuCursorPosition = 1, .theoreticalMaxLevel = 2000, .initialTimerFrames = 0, .saveFileName = L"section_bests_normal_3_1.txt", .maxLevelFileName = L"max_level_normal_3_1.txt" },
    { .modeLabel = L"NORMAL(4.1)", .cursorValue = 18, .menuCursorPosition = 1, .theoreticalMaxLevel = 999, .initialTimerFrames = 0, .saveFileName = L"section_bests_normal_4_1.txt", .maxLevelFileName = L"max_level_normal_4_1.txt" },
    { .modeLabel = L"ASUKA", .cursorValue = 5, .menuCursorPosition = 2, .theoreticalMaxLevel = 1300, .initialTimerFrames = 7 * 60 * 60, .saveFileName = L"section_bests_asuka.txt", .maxLevelFileName = L"max_level_asuka.txt" },
    { .modeLabel = L"ASUKAEASY", .cursorValue = 10, .menuCursorPosition = 2, .theoreticalMaxLevel = 999, .initialTimerFrames = 30 * 60 * 60, .saveFileName = L"section_bests_asukaeasy.txt", .maxLevelFileName = L"max_level_asukaeasy.txt" },
    { .modeLabel = L"MASTER", .cursorValue = 1, .menuCursorPosition = 3, .theoreticalMaxLevel = 2600, .initialTimerFrames = 0, .saveFileName = L"section_bests_master.txt", .maxLevelFileName = L"max_level_master.txt" }
};

const TableColumnDefinition TABLE_COLUMNS[COLUMN_COUNT] = {
    { COLUMN_GAME_TIME, ID_CHECK_GAMETIME, L"GameTime", L"99:59.99" },
    { COLUMN_DELTA, ID_CHECK_DELTA, L"Delta", L"+999.999 s" },
    { COLUMN_SECTION_TIME, ID_CHECK_SECTIONTIME, L"SectionTime", L"99m59.99s" },
    { COLUMN_BEST, ID_CHECK_BEST, L"Best", L"999.999 s" },
    { COLUMN_BACK, ID_CHECK_BACKCOL, L"Back", L"99" },
    { COLUMN_TET, ID_CHECK_TET, L"Tet", L"99" }
};

static void initialize_best_times(void) {
    int i;
    for (i = 0; i < MAX_SECTION_COUNT; ++i) {
        g_app.bestSectionTimes[i] = BEST_TIME_DEFAULT_SECONDS;
    }
}

static void initialize_max_level(void) {
    g_app.maxLevel = 1107;
}

static wchar_t *trim_quotes(wchar_t *text) {
    size_t len;

    if (text == NULL) {
        return NULL;
    }

    while (*text == L'"') {
        ++text;
    }

    len = wcslen(text);
    while (len > 0 && (text[len - 1] == L'"' || text[len - 1] == L'\r' || text[len - 1] == L'\n')) {
        text[len - 1] = L'\0';
        --len;
    }

    return text;
}

static size_t split_csv_offsets(wchar_t *text, uintptr_t *offsets, size_t maxCount) {
    wchar_t *token;
    size_t count;

    count = 0;
    token = wcstok(trim_quotes(text), L",");
    while (token != NULL && count < maxCount) {
        offsets[count] = (uintptr_t)wcstoul(trim_quotes(token), NULL, 0);
        count += 1;
        token = wcstok(NULL, L",");
    }

    return count;
}

static void clear_pointer_chains(void) {
    ZeroMemory(&POINTER_CHAINS, sizeof(POINTER_CHAINS));
}

static void write_offsets(FILE *file, const uintptr_t *offsets, size_t count) {
    size_t i;

    for (i = 0; i < count; ++i) {
        fwprintf(file, i == 0 ? L"0x%IX" : L",0x%IX", offsets[i]);
    }
}

static void write_pointer_configs_to_file(void) {
    FILE *file;

    build_save_paths();
    if (g_app.configFilePath[0] == L'\0') {
        return;
    }

    file = _wfopen(g_app.configFilePath, L"w");
    if (file == NULL) {
        return;
    }

    fwprintf(file, L"# Shared pointer settings for every game mode.\n");
    fwprintf(file, L"# Update only these values after a TGM4 update.\n");
    fwprintf(file, L"base_address\t0x%08IX\n", POINTER_CHAINS.baseOffset);
    fwprintf(file, L"game_mode_offsets\t");
    write_offsets(file, POINTER_CHAINS.gameModeOffsets, POINTER_CHAINS.gameModeOffsetCount);
    fwprintf(file, L"\nmenu_cursor_y_offsets\t");
    write_offsets(file, POINTER_CHAINS.menuCursorYOffsets, POINTER_CHAINS.menuCursorYOffsetCount);
    fwprintf(file, L"\nlevel_offsets\t");
    write_offsets(file, POINTER_CHAINS.levelOffsets, POINTER_CHAINS.levelOffsetCount);
    fwprintf(file, L"\ntimer_offsets\t");
    write_offsets(file, POINTER_CHAINS.timerOffsets, POINTER_CHAINS.timerOffsetCount);
    fwprintf(file, L"\n");
    fclose(file);
}
int pointer_config_count(void) {
    return (int)ARRAY_COUNT(POINTER_CONFIGS);
}

const PointerConfig *current_pointer_config(void) {
    if (g_app.currentConfigIndex < 0 || g_app.currentConfigIndex >= pointer_config_count()) {
        return NULL;
    }
    return &POINTER_CONFIGS[g_app.currentConfigIndex];
}

void build_save_paths(void) {
    wchar_t exePath[MAX_PATH];
    wchar_t *lastSlash;
    DWORD length;
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

void reset_best_times_for_config_index(int configIndex) {
    int oldConfigIndex;
    int i;

    if (configIndex < 0 || configIndex >= pointer_config_count()) {
        return;
    }

    oldConfigIndex = g_app.currentConfigIndex;
    g_app.currentConfigIndex = configIndex;
    build_save_paths();
    for (i = 0; i < MAX_SECTION_COUNT; ++i) {
        g_app.bestSectionTimes[i] = BEST_TIME_DEFAULT_SECONDS;
    }
    save_best_times();
    if (oldConfigIndex == configIndex) {
        load_best_times();
    }
    g_app.currentConfigIndex = oldConfigIndex;
}

void load_pointer_configs(void) {
    FILE *file;
    wchar_t line[2048];
    bool hasBaseAddress;
    bool hasGameModeOffsets;
    bool hasMenuCursorYOffsets;
    bool hasLevelOffsets;
    bool hasTimerOffsets;

    build_save_paths();
    if (g_app.configFilePath[0] == L'\0') {
        return;
    }

    clear_pointer_chains();
    file = _wfopen(g_app.configFilePath, L"r");
    if (file == NULL) {
        write_pointer_configs_to_file();
        return;
    }

    hasBaseAddress = false;
    hasGameModeOffsets = false;
    hasMenuCursorYOffsets = false;
    hasLevelOffsets = false;
    hasTimerOffsets = false;
    while (fgetws(line, ARRAY_COUNT(line), file) != NULL) {
        wchar_t *key;
        wchar_t *value;

        key = wcstok(line, L"\t\r\n");
        value = wcstok(NULL, L"\t\r\n");
        if (key == NULL || value == NULL || key[0] == L'#') {
            continue;
        }

        key = trim_quotes(key);
        value = trim_quotes(value);
        if (wcscmp(key, L"base_address") == 0) {
            POINTER_CHAINS.baseOffset = (uintptr_t)wcstoul(value, NULL, 0);
            hasBaseAddress = true;
        } else if (wcscmp(key, L"game_mode_offsets") == 0) {
            POINTER_CHAINS.gameModeOffsetCount = split_csv_offsets(value, POINTER_CHAINS.gameModeOffsets, MAX_POINTER_OFFSET_COUNT);
            hasGameModeOffsets = POINTER_CHAINS.gameModeOffsetCount > 0;
        } else if (wcscmp(key, L"menu_cursor_y_offsets") == 0) {
            POINTER_CHAINS.menuCursorYOffsetCount = split_csv_offsets(value, POINTER_CHAINS.menuCursorYOffsets, MAX_POINTER_OFFSET_COUNT);
            hasMenuCursorYOffsets = POINTER_CHAINS.menuCursorYOffsetCount > 0;
        } else if (wcscmp(key, L"level_offsets") == 0) {
            POINTER_CHAINS.levelOffsetCount = split_csv_offsets(value, POINTER_CHAINS.levelOffsets, MAX_POINTER_OFFSET_COUNT);
            hasLevelOffsets = POINTER_CHAINS.levelOffsetCount > 0;
        } else if (wcscmp(key, L"timer_offsets") == 0) {
            POINTER_CHAINS.timerOffsetCount = split_csv_offsets(value, POINTER_CHAINS.timerOffsets, MAX_POINTER_OFFSET_COUNT);
            hasTimerOffsets = POINTER_CHAINS.timerOffsetCount > 0;
        }
    }

    fclose(file);
    if (!hasBaseAddress || !hasGameModeOffsets || !hasMenuCursorYOffsets || !hasLevelOffsets || !hasTimerOffsets) {
        clear_pointer_chains();
        write_pointer_configs_to_file();
        copy_status_text(L"config.txt was invalid; created the shared pointer template");
    }
}
void save_best_times(void) {
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

void load_best_times(void) {
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

void save_max_level(void) {
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

void load_max_level(void) {
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
