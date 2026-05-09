#include "app.h"

static uintptr_t ASUKA_POINTER_OFFSETS[] = {
    0x20, 0x04, 0x10, 0x10, 0x10, 0x94
};

static uintptr_t ASUKA_TIMER_POINTER_OFFSETS[] = {
    0x20, 0x04, 0x10, 0x10, 0x10, 0x9C
};

static uintptr_t NORMAL_POINTER_OFFSETS[] = {
    0x08, 0x30, 0x10, 0x10, 0x10, 0x0C, 0x98
};

static uintptr_t NORMAL_TIMER_POINTER_OFFSETS[] = {
    0x08, 0x30, 0x10, 0x10, 0x10, 0x0C, 0xA0
};

static uintptr_t NORMAL_EX_POINTER_OFFSETS[] = {
    0x08, 0x30, 0x2c, 0x0C, 0x0c, 0x1a4
};

static uintptr_t NORMAL_EX_TIMER_POINTER_OFFSETS[] = {
    0x08, 0x30, 0x2C, 0x0C, 0x0C, 0x1DC
};

PointerConfig POINTER_CONFIGS[] = {
    { L"NORMAL", 0x00A7E528, NORMAL_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_POINTER_OFFSETS), 0x00A7E528, NORMAL_TIMER_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_TIMER_POINTER_OFFSETS), 9, 1, 999, 0, L"section_bests_normal.txt", L"max_level_normal.txt" },
    { L"NORMAL(1.1)", 0x00A7E528, NORMAL_EX_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_EX_POINTER_OFFSETS), 0x00A7E528, NORMAL_EX_TIMER_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_EX_TIMER_POINTER_OFFSETS), 15, 1, 999, 0, L"section_bests_normal_1_1.txt", L"max_level_normal_1_1.txt" },
    { L"NORMAL(2.1)", 0x00A7E528, NORMAL_EX_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_EX_POINTER_OFFSETS), 0x00A7E528, NORMAL_EX_TIMER_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_EX_TIMER_POINTER_OFFSETS), 16, 1, 999, 0, L"section_bests_normal_2_1.txt", L"max_level_normal_2_1.txt" },
    { L"NORMAL(3.1)", 0x00A7E528, NORMAL_EX_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_EX_POINTER_OFFSETS), 0x00A7E528, NORMAL_EX_TIMER_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_EX_TIMER_POINTER_OFFSETS), 17, 1, 2000, 0, L"section_bests_normal_3_1.txt", L"max_level_normal_3_1.txt" },
    { L"NORMAL(4.1)", 0x00A7E528, NORMAL_EX_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_EX_POINTER_OFFSETS), 0x00A7E528, NORMAL_EX_TIMER_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_EX_TIMER_POINTER_OFFSETS), 18, 1, 999, 0, L"section_bests_normal_4_1.txt", L"max_level_normal_4_1.txt" },
    { L"ASUKA", 0x00A7CD9C, ASUKA_POINTER_OFFSETS, ARRAY_COUNT(ASUKA_POINTER_OFFSETS), 0x00A7CD9C, ASUKA_TIMER_POINTER_OFFSETS, ARRAY_COUNT(ASUKA_TIMER_POINTER_OFFSETS), 5, 2, 1300, 7 * 60 * 60, L"section_bests_asuka.txt", L"max_level_asuka.txt" },
    { L"ASUKAEASY", 0x00A7CD9C, ASUKA_POINTER_OFFSETS, ARRAY_COUNT(ASUKA_POINTER_OFFSETS), 0x00A7CD9C, ASUKA_TIMER_POINTER_OFFSETS, ARRAY_COUNT(ASUKA_TIMER_POINTER_OFFSETS), 10, 2, 999, 30 * 60 * 60, L"section_bests_asukaeasy.txt", L"max_level_asukaeasy.txt" },
    { L"MASTER", 0x00A7E528, NORMAL_EX_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_EX_POINTER_OFFSETS), 0x00A7E528, NORMAL_EX_TIMER_POINTER_OFFSETS, ARRAY_COUNT(NORMAL_EX_TIMER_POINTER_OFFSETS), 1, 3, 2600, 0, L"section_bests_master.txt", L"max_level_master.txt" }
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

static size_t split_csv_offsets(wchar_t *text, uintptr_t *offsets, size_t maxCount) {
    wchar_t *token;
    size_t count = 0;

    token = wcstok(text, L",");
    while (token != NULL && count < maxCount) {
        offsets[count++] = (uintptr_t)wcstoul(token, NULL, 0);
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
    for (i = 0; i < pointer_config_count(); ++i) {
        fwprintf(file, L"%ls\t0x%08IX\t", POINTER_CONFIGS[i].modeLabel, POINTER_CONFIGS[i].baseOffset);
        for (j = 0; j < POINTER_CONFIGS[i].pointerOffsetCount; ++j) {
            fwprintf(file, j == 0 ? L"0x%IX" : L",0x%IX", POINTER_CONFIGS[i].pointerOffsets[j]);
        }
        fwprintf(file, L"\t0x%08IX\t", POINTER_CONFIGS[i].timerBaseOffset);
        for (j = 0; j < POINTER_CONFIGS[i].timerPointerOffsetCount; ++j) {
            fwprintf(file, j == 0 ? L"0x%IX" : L",0x%IX", POINTER_CONFIGS[i].timerPointerOffsets[j]);
        }
        fwprintf(file, L"\t%d\t%d\t%d\t%d\n", POINTER_CONFIGS[i].cursorValue, POINTER_CONFIGS[i].menuCursorPosition, POINTER_CONFIGS[i].theoreticalMaxLevel, POINTER_CONFIGS[i].initialTimerFrames);
    }

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

    if (fgetws(line, ARRAY_COUNT(line), file) == NULL) {
        fclose(file);
        write_pointer_configs_to_file();
        return;
    }

    while (fgetws(line, ARRAY_COUNT(line), file) != NULL) {
        wchar_t *fields[9];
        wchar_t *token;
        wchar_t levelOffsetsText[512];
        wchar_t timerOffsetsText[512];
        int fieldCount = 0;
        int i;

        token = wcstok(line, L"\t\r\n");
        while (token != NULL && fieldCount < (int)ARRAY_COUNT(fields)) {
            fields[fieldCount++] = token;
            token = wcstok(NULL, L"\t\r\n");
        }

        if (fieldCount < (int)ARRAY_COUNT(fields)) {
            continue;
        }

        for (i = 0; i < pointer_config_count(); ++i) {
            if (wcscmp(POINTER_CONFIGS[i].modeLabel, fields[0]) != 0) {
                continue;
            }

            POINTER_CONFIGS[i].baseOffset = (uintptr_t)wcstoul(fields[1], NULL, 0);
            lstrcpynW(levelOffsetsText, fields[2], ARRAY_COUNT(levelOffsetsText));
            POINTER_CONFIGS[i].pointerOffsetCount = split_csv_offsets(levelOffsetsText, (uintptr_t *)POINTER_CONFIGS[i].pointerOffsets, POINTER_CONFIGS[i].pointerOffsetCount);
            POINTER_CONFIGS[i].timerBaseOffset = (uintptr_t)wcstoul(fields[3], NULL, 0);
            lstrcpynW(timerOffsetsText, fields[4], ARRAY_COUNT(timerOffsetsText));
            POINTER_CONFIGS[i].timerPointerOffsetCount = split_csv_offsets(timerOffsetsText, (uintptr_t *)POINTER_CONFIGS[i].timerPointerOffsets, POINTER_CONFIGS[i].timerPointerOffsetCount);
            POINTER_CONFIGS[i].cursorValue = _wtoi(fields[5]);
            POINTER_CONFIGS[i].menuCursorPosition = _wtoi(fields[6]);
            POINTER_CONFIGS[i].theoreticalMaxLevel = _wtoi(fields[7]);
            POINTER_CONFIGS[i].initialTimerFrames = _wtoi(fields[8]);
            break;
        }
    }

    fclose(file);
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
