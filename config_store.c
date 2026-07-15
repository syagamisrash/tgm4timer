#include "app.h"

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

static void clear_pointer_fields(PointerConfig *config) {
    config->baseOffset = 0;
    config->pointerOffsetCount = 0;
    config->timerBaseOffset = 0;
    config->timerPointerOffsetCount = 0;
    config->cursorBaseOffset = 0;
    config->cursorPointerOffsetCount = 0;
    config->menuCursorBaseOffset = 0;
    config->menuCursorPointerOffsetCount = 0;
    ZeroMemory(config->pointerOffsets, sizeof(config->pointerOffsets));
    ZeroMemory(config->timerPointerOffsets, sizeof(config->timerPointerOffsets));
    ZeroMemory(config->cursorPointerOffsets, sizeof(config->cursorPointerOffsets));
    ZeroMemory(config->menuCursorPointerOffsets, sizeof(config->menuCursorPointerOffsets));
}

static void write_offsets(FILE *file, const uintptr_t *offsets, size_t count) {
    size_t i;

    for (i = 0; i < count; ++i) {
        fwprintf(file, i == 0 ? L"0x%IX" : L",0x%IX", offsets[i]);
    }
}

static void write_pointer_configs_to_file(void) {
    FILE *file;
    int i;

    build_save_paths();
    if (g_app.configFilePath[0] == L'\0') {
        return;
    }

    file = _wfopen(g_app.configFilePath, L"w");
    if (file == NULL) {
        return;
    }

    fwprintf(file, L"mode\tlevel_base\tlevel_offsets\ttimer_base\ttimer_offsets\tcursor_base\tcursor_offsets\tmenu_cursor_base\tmenu_cursor_offsets\tcursor_value\tmenu_cursor_position\ttheoretical_max_level\tinitial_timer_frames\n");
    for (i = 0; i < pointer_config_count(); ++i) {
        fwprintf(file, L"%ls\t0x%08IX\t", POINTER_CONFIGS[i].modeLabel, POINTER_CONFIGS[i].baseOffset);
        write_offsets(file, POINTER_CONFIGS[i].pointerOffsets, POINTER_CONFIGS[i].pointerOffsetCount);
        fwprintf(file, L"\t0x%08IX\t", POINTER_CONFIGS[i].timerBaseOffset);
        write_offsets(file, POINTER_CONFIGS[i].timerPointerOffsets, POINTER_CONFIGS[i].timerPointerOffsetCount);
        fwprintf(file, L"\t0x%08IX\t", POINTER_CONFIGS[i].cursorBaseOffset);
        write_offsets(file, POINTER_CONFIGS[i].cursorPointerOffsets, POINTER_CONFIGS[i].cursorPointerOffsetCount);
        fwprintf(file, L"\t0x%08IX\t", POINTER_CONFIGS[i].menuCursorBaseOffset);
        write_offsets(file, POINTER_CONFIGS[i].menuCursorPointerOffsets, POINTER_CONFIGS[i].menuCursorPointerOffsetCount);
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
    wchar_t line[2048];
    bool loadedAny;
    int i;

    build_save_paths();
    if (g_app.configFilePath[0] == L'\0') {
        return;
    }

    for (i = 0; i < pointer_config_count(); ++i) {
        clear_pointer_fields(&POINTER_CONFIGS[i]);
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

    loadedAny = false;
    while (fgetws(line, ARRAY_COUNT(line), file) != NULL) {
        wchar_t *fields[13];
        wchar_t *token;
        wchar_t levelOffsetsText[512];
        wchar_t timerOffsetsText[512];
        wchar_t cursorOffsetsText[512];
        wchar_t menuCursorOffsetsText[512];
        int fieldCount;

        fieldCount = 0;
        token = wcstok(line, L"\t\r\n");
        while (token != NULL && fieldCount < (int)ARRAY_COUNT(fields)) {
            fields[fieldCount] = token;
            fieldCount += 1;
            token = wcstok(NULL, L"\t\r\n");
        }

        if (fieldCount < 13) {
            continue;
        }

        for (i = 0; i < pointer_config_count(); ++i) {
            if (wcscmp(POINTER_CONFIGS[i].modeLabel, trim_quotes(fields[0])) != 0) {
                continue;
            }

            POINTER_CONFIGS[i].baseOffset = (uintptr_t)wcstoul(trim_quotes(fields[1]), NULL, 0);
            lstrcpynW(levelOffsetsText, trim_quotes(fields[2]), ARRAY_COUNT(levelOffsetsText));
            POINTER_CONFIGS[i].pointerOffsetCount = split_csv_offsets(levelOffsetsText, POINTER_CONFIGS[i].pointerOffsets, MAX_POINTER_OFFSET_COUNT);
            POINTER_CONFIGS[i].timerBaseOffset = (uintptr_t)wcstoul(trim_quotes(fields[3]), NULL, 0);
            lstrcpynW(timerOffsetsText, trim_quotes(fields[4]), ARRAY_COUNT(timerOffsetsText));
            POINTER_CONFIGS[i].timerPointerOffsetCount = split_csv_offsets(timerOffsetsText, POINTER_CONFIGS[i].timerPointerOffsets, MAX_POINTER_OFFSET_COUNT);
            POINTER_CONFIGS[i].cursorBaseOffset = (uintptr_t)wcstoul(trim_quotes(fields[5]), NULL, 0);
            lstrcpynW(cursorOffsetsText, trim_quotes(fields[6]), ARRAY_COUNT(cursorOffsetsText));
            POINTER_CONFIGS[i].cursorPointerOffsetCount = split_csv_offsets(cursorOffsetsText, POINTER_CONFIGS[i].cursorPointerOffsets, MAX_POINTER_OFFSET_COUNT);
            POINTER_CONFIGS[i].menuCursorBaseOffset = (uintptr_t)wcstoul(trim_quotes(fields[7]), NULL, 0);
            lstrcpynW(menuCursorOffsetsText, trim_quotes(fields[8]), ARRAY_COUNT(menuCursorOffsetsText));
            POINTER_CONFIGS[i].menuCursorPointerOffsetCount = split_csv_offsets(menuCursorOffsetsText, POINTER_CONFIGS[i].menuCursorPointerOffsets, MAX_POINTER_OFFSET_COUNT);
            POINTER_CONFIGS[i].cursorValue = _wtoi(trim_quotes(fields[9]));
            POINTER_CONFIGS[i].menuCursorPosition = _wtoi(trim_quotes(fields[10]));
            POINTER_CONFIGS[i].theoreticalMaxLevel = _wtoi(trim_quotes(fields[11]));
            POINTER_CONFIGS[i].initialTimerFrames = _wtoi(trim_quotes(fields[12]));
            loadedAny = true;
            break;
        }
    }

    fclose(file);
    if (!loadedAny) {
        write_pointer_configs_to_file();
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
