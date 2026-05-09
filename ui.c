#include "app.h"

static bool is_column_visible(TableColumnId columnId) {
    switch (columnId) {
    case COLUMN_GAME_TIME: return g_app.showColumnGameTime;
    case COLUMN_DELTA: return g_app.showColumnDelta;
    case COLUMN_SECTION_TIME: return g_app.showColumnSectionTime;
    case COLUMN_BEST: return g_app.showColumnBest;
    case COLUMN_BACK: return g_app.showColumnBack;
    case COLUMN_TET: return g_app.showColumnTet;
    }

    return false;
}

static void set_column_visible(TableColumnId columnId, bool visible) {
    switch (columnId) {
    case COLUMN_GAME_TIME: g_app.showColumnGameTime = visible; break;
    case COLUMN_DELTA: g_app.showColumnDelta = visible; break;
    case COLUMN_SECTION_TIME: g_app.showColumnSectionTime = visible; break;
    case COLUMN_BEST: g_app.showColumnBest = visible; break;
    case COLUMN_BACK: g_app.showColumnBack = visible; break;
    case COLUMN_TET: g_app.showColumnTet = visible; break;
    }
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

static int last_section_index_for_max_level(int theoreticalMaxLevel) {
    int sectionCount = section_count_for_max_level(theoreticalMaxLevel);
    if (sectionCount <= 0) {
        return 0;
    }
    return sectionCount - 1;
}

static void set_checkbox_state(HWND hwnd, bool checked) {
    if (hwnd != NULL) {
        SendMessageW(hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

static const wchar_t *gm_requirement_text_for_mode(const wchar_t *modeLabel) {
    if (wcscmp(modeLabel, L"NORMAL(1.1)") == 0) {
        return L"GM Requirements\nSCORE 280000+\nLv999 within 8:55\n6 tetrises in credit roll";
    }
    if (wcscmp(modeLabel, L"NORMAL(2.1)") == 0) {
        return L"GM Requirements\nSCORE 260000+\nLv999 within 5:00 (Lv500 with in 3:20)\n8 tetrises in credit roll";
    }
    if (wcscmp(modeLabel, L"NORMAL(3.1)") == 0) {
        return L"GM Requirements\nLv1000 within 4:01\nLv1300 within 4:30\nLv2000 within 6:46\n21 triples+ in credit roll";
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

static void paint_settings_screen(HDC hdc) {
    int y = 48;
    draw_text_line(hdc, &y, L"TGM4 Section Timer - Settings", RGB(240, 240, 240));
    y += 10;
    draw_text_line(hdc, &y, L"Visible Columns", RGB(255, 230, 160));
    y += 220;
    draw_text_line(hdc, &y, L"Reset Best Records", RGB(255, 230, 160));
    draw_text_line(hdc, &y, L"Select a mode and reset best section times to 999.000 s", RGB(200, 220, 255));
}

static double current_section_progress(int level, int theoreticalMaxLevel) {
    int sectionIndex;
    int sectionStart;
    int sectionEnd;

    if (level < 0 || theoreticalMaxLevel <= 0) {
        return 0.0;
    }
    if (level >= theoreticalMaxLevel) {
        return 1.0;
    }

    sectionIndex = clamp_section_index_for_level(level, theoreticalMaxLevel);
    sectionStart = sectionIndex * 100;
    sectionEnd = (sectionIndex == last_section_index_for_max_level(theoreticalMaxLevel)) ? theoreticalMaxLevel : (sectionIndex + 1) * 100;
    if (sectionEnd <= sectionStart) {
        return 0.0;
    }

    return (double)(level - sectionStart) / (double)(sectionEnd - sectionStart);
}

static int current_section_index_for_display(int level, int theoreticalMaxLevel) {
    if (level < 0 || theoreticalMaxLevel <= 0) {
        return 0;
    }
    return clamp_section_index_for_level(level, theoreticalMaxLevel);
}

void update_button_labels(void) {
    if (g_app.historyPrevButton != NULL) {
        EnableWindow(g_app.historyPrevButton, g_app.historyCount > 0 && g_app.historyViewOffset < g_app.historyCount);
    }
    if (g_app.historyNextButton != NULL) {
        EnableWindow(g_app.historyNextButton, g_app.historyViewOffset > 0);
    }
}

void apply_column_toggle_from_control(int controlId) {
    size_t i;
    HWND checkboxHwnds[COLUMN_COUNT];

    checkboxHwnds[COLUMN_GAME_TIME] = g_app.gameTimeCheck;
    checkboxHwnds[COLUMN_DELTA] = g_app.deltaCheck;
    checkboxHwnds[COLUMN_SECTION_TIME] = g_app.sectionTimeCheck;
    checkboxHwnds[COLUMN_BEST] = g_app.bestCheck;
    checkboxHwnds[COLUMN_BACK] = g_app.backColCheck;
    checkboxHwnds[COLUMN_TET] = g_app.tetCheck;

    for (i = 0; i < ARRAY_COUNT(TABLE_COLUMNS); ++i) {
        if (TABLE_COLUMNS[i].controlId == controlId) {
            set_column_visible(TABLE_COLUMNS[i].id, SendMessageW(checkboxHwnds[TABLE_COLUMNS[i].id], BM_GETCHECK, 0, 0) == BST_CHECKED);
            return;
        }
    }
}

void create_main_screen_controls(HWND hwnd, HINSTANCE instance) {
    g_app.settingsButton = CreateWindowW(L"BUTTON", L"Setting", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 410, 8, 80, 28, hwnd, (HMENU)ID_BUTTON_SETTINGS, instance, NULL);
    g_app.historyPrevButton = CreateWindowW(L"BUTTON", L"<-", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 500, 8, 50, 28, hwnd, (HMENU)ID_BUTTON_HISTORY_PREV, instance, NULL);
    g_app.historyNextButton = CreateWindowW(L"BUTTON", L"->", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 556, 8, 50, 28, hwnd, (HMENU)ID_BUTTON_HISTORY_NEXT, instance, NULL);
}

void create_settings_screen_controls(HWND hwnd, HINSTANCE instance) {
    g_app.backButton = CreateWindowW(L"BUTTON", L"Back", WS_CHILD | BS_PUSHBUTTON, 12, 8, 80, 28, hwnd, (HMENU)ID_BUTTON_BACK, instance, NULL);
    g_app.gameTimeCheck = CreateWindowW(L"BUTTON", L"Show GameTime", WS_CHILD | BS_AUTOCHECKBOX, 24, 110, 220, 24, hwnd, (HMENU)ID_CHECK_GAMETIME, instance, NULL);
    g_app.deltaCheck = CreateWindowW(L"BUTTON", L"Show Delta", WS_CHILD | BS_AUTOCHECKBOX, 24, 140, 220, 24, hwnd, (HMENU)ID_CHECK_DELTA, instance, NULL);
    g_app.sectionTimeCheck = CreateWindowW(L"BUTTON", L"Show SectionTime", WS_CHILD | BS_AUTOCHECKBOX, 24, 170, 220, 24, hwnd, (HMENU)ID_CHECK_SECTIONTIME, instance, NULL);
    g_app.bestCheck = CreateWindowW(L"BUTTON", L"Show Best", WS_CHILD | BS_AUTOCHECKBOX, 24, 200, 220, 24, hwnd, (HMENU)ID_CHECK_BEST, instance, NULL);
    g_app.backColCheck = CreateWindowW(L"BUTTON", L"Show Back", WS_CHILD | BS_AUTOCHECKBOX, 24, 230, 220, 24, hwnd, (HMENU)ID_CHECK_BACKCOL, instance, NULL);
    g_app.tetCheck = CreateWindowW(L"BUTTON", L"Show Tet", WS_CHILD | BS_AUTOCHECKBOX, 24, 260, 220, 24, hwnd, (HMENU)ID_CHECK_TET, instance, NULL);
    g_app.progressCheck = CreateWindowW(L"BUTTON", L"Show Progress Bar", WS_CHILD | BS_AUTOCHECKBOX, 24, 290, 220, 24, hwnd, (HMENU)ID_CHECK_PROGRESS, instance, NULL);
    g_app.resetModeCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 24, 370, 240, 200, hwnd, (HMENU)ID_COMBO_RESET_MODE, instance, NULL);
    g_app.resetBestButton = CreateWindowW(L"BUTTON", L"Reset Best To 999s", WS_CHILD | BS_PUSHBUTTON, 280, 370, 180, 28, hwnd, (HMENU)ID_BUTTON_RESET_BEST, instance, NULL);
}

void populate_reset_mode_combo(void) {
    int i;

    if (g_app.resetModeCombo == NULL) {
        return;
    }
    for (i = 0; i < pointer_config_count(); ++i) {
        SendMessageW(g_app.resetModeCombo, CB_ADDSTRING, 0, (LPARAM)POINTER_CONFIGS[i].modeLabel);
    }
    SendMessageW(g_app.resetModeCombo, CB_SETCURSEL, 0, 0);
}

void update_screen_controls(void) {
    BOOL showMain = g_app.currentScreen == SCREEN_MAIN ? TRUE : FALSE;
    BOOL showSettings = g_app.currentScreen == SCREEN_SETTINGS ? TRUE : FALSE;
    size_t i;
    HWND checkboxHwnds[COLUMN_COUNT];

    if (g_app.historyPrevButton != NULL) ShowWindow(g_app.historyPrevButton, showMain ? SW_SHOW : SW_HIDE);
    if (g_app.historyNextButton != NULL) ShowWindow(g_app.historyNextButton, showMain ? SW_SHOW : SW_HIDE);
    if (g_app.settingsButton != NULL) ShowWindow(g_app.settingsButton, showMain ? SW_SHOW : SW_HIDE);

    if (g_app.backButton != NULL) ShowWindow(g_app.backButton, showSettings ? SW_SHOW : SW_HIDE);
    if (g_app.gameTimeCheck != NULL) ShowWindow(g_app.gameTimeCheck, showSettings ? SW_SHOW : SW_HIDE);
    if (g_app.deltaCheck != NULL) ShowWindow(g_app.deltaCheck, showSettings ? SW_SHOW : SW_HIDE);
    if (g_app.sectionTimeCheck != NULL) ShowWindow(g_app.sectionTimeCheck, showSettings ? SW_SHOW : SW_HIDE);
    if (g_app.bestCheck != NULL) ShowWindow(g_app.bestCheck, showSettings ? SW_SHOW : SW_HIDE);
    if (g_app.backColCheck != NULL) ShowWindow(g_app.backColCheck, showSettings ? SW_SHOW : SW_HIDE);
    if (g_app.tetCheck != NULL) ShowWindow(g_app.tetCheck, showSettings ? SW_SHOW : SW_HIDE);
    if (g_app.progressCheck != NULL) ShowWindow(g_app.progressCheck, showSettings ? SW_SHOW : SW_HIDE);
    if (g_app.resetModeCombo != NULL) ShowWindow(g_app.resetModeCombo, showSettings ? SW_SHOW : SW_HIDE);
    if (g_app.resetBestButton != NULL) ShowWindow(g_app.resetBestButton, showSettings ? SW_SHOW : SW_HIDE);

    checkboxHwnds[COLUMN_GAME_TIME] = g_app.gameTimeCheck;
    checkboxHwnds[COLUMN_DELTA] = g_app.deltaCheck;
    checkboxHwnds[COLUMN_SECTION_TIME] = g_app.sectionTimeCheck;
    checkboxHwnds[COLUMN_BEST] = g_app.bestCheck;
    checkboxHwnds[COLUMN_BACK] = g_app.backColCheck;
    checkboxHwnds[COLUMN_TET] = g_app.tetCheck;

    for (i = 0; i < ARRAY_COUNT(TABLE_COLUMNS); ++i) {
        set_checkbox_state(checkboxHwnds[TABLE_COLUMNS[i].id], is_column_visible(TABLE_COLUMNS[i].id));
    }
    set_checkbox_state(g_app.progressCheck, g_app.showProgressBar);

    update_button_labels();
}

int section_count_for_max_level(int theoreticalMaxLevel) {
    return (theoreticalMaxLevel + 99) / 100;
}

int clamp_section_index_for_level(int level, int theoreticalMaxLevel) {
    int sectionIndex = level / 100;
    int lastSectionIndex = last_section_index_for_max_level(theoreticalMaxLevel);
    if (sectionIndex < 0) {
        return 0;
    }
    if (sectionIndex > lastSectionIndex) {
        return lastSectionIndex;
    }
    return sectionIndex;
}

int completed_section_count_for_level(int level, int theoreticalMaxLevel) {
    if (level >= theoreticalMaxLevel) {
        return section_count_for_max_level(theoreticalMaxLevel);
    }
    return level / 100;
}

void format_section_label(wchar_t *buffer, size_t bufferCount, int sectionIndex, int theoreticalMaxLevel) {
    int sectionStart = sectionIndex * 100;
    int sectionEnd = (sectionIndex == last_section_index_for_max_level(theoreticalMaxLevel)) ? theoreticalMaxLevel : (sectionIndex + 1) * 100;
    swprintf(buffer, bufferCount, L"%4d-%4d", sectionStart, sectionEnd);
}

void format_game_timer(wchar_t *buffer, size_t bufferCount, int frames) {
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

void format_seconds_as_game_time(wchar_t *buffer, size_t bufferCount, double secondsValue) {
    int totalCentiseconds;
    int minutes;
    int seconds;
    int centiseconds;

    if (secondsValue < 0.0) {
        swprintf(buffer, bufferCount, L"-");
        return;
    }

    totalCentiseconds = (int)(secondsValue * 100.0 + 0.5);
    minutes = totalCentiseconds / 6000;
    seconds = (totalCentiseconds / 100) % 60;
    centiseconds = totalCentiseconds % 100;
    if (minutes > 0) {
        swprintf(buffer, bufferCount, L"%dm%02d.%02ds", minutes, seconds, centiseconds);
    } else {
        swprintf(buffer, bufferCount, L"%d.%02ds", seconds, centiseconds);
    }
}

void paint_window(HWND hwnd) {
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
    int columnX[COLUMN_COUNT];
    bool columnVisible[COLUMN_COUNT];
    int columnWidths[COLUMN_COUNT];
    int visibleColumnCount;
    int cellPadding;
    int sectionWidth;
    int deltaWidth;
    int sectionTimeWidth;
    int bestWidth;
    int gameTimeWidth;
    int backWidth;
    int tetWidth;
    int progressBarWidth;
    int progressBarLeft;
    int progressBarTop;
    int progressBarFillWidth;
    RECT progressOuterRect;
    RECT progressFillRect;
    HBRUSH progressOuterBrush;
    HBRUSH progressFillBrush;
    HPEN progressPen;
    HPEN oldPen;
    HBRUSH oldBrush;
    int infoTop;
    const PointerConfig *config;
    const RunSnapshot *snapshot;
    const wchar_t *displayModeLabel;
    const wchar_t *gmRequirementText;
    int displayTheoreticalMaxLevel;
    int displayCurrentLevel;
    int displayMaxLevel;
    int displaySectionCount;
    double liveLevelsPerMinute;
    double progressRatio;
    int progressSectionIndex;
    int progressTetrisCount;
    wchar_t sectionLabel[32];
    wchar_t gameTimeText[32];

    hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &clientRect);

    memoryDc = CreateCompatibleDC(hdc);
    backBufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
    oldBitmap = (HBITMAP)SelectObject(memoryDc, backBufferBitmap);
    font = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    oldFont = (HFONT)SelectObject(memoryDc, font);

    backgroundBrush = CreateSolidBrush(RGB(24, 24, 24));
    FillRect(memoryDc, &clientRect, backgroundBrush);
    DeleteObject(backgroundBrush);
    SetBkMode(memoryDc, TRANSPARENT);

    config = current_pointer_config();
    snapshot = current_view_snapshot();

    if (g_app.currentScreen == SCREEN_SETTINGS) {
        paint_settings_screen(memoryDc);
        BitBlt(hdc, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, memoryDc, 0, 0, SRCCOPY);
        SelectObject(memoryDc, oldFont);
        SelectObject(memoryDc, oldBitmap);
        DeleteObject(font);
        DeleteObject(backBufferBitmap);
        DeleteDC(memoryDc);
        EndPaint(hwnd, &ps);
        return;
    }

    displayModeLabel = (config != NULL) ? config->modeLabel : L"-";
    displayTheoreticalMaxLevel = (config != NULL) ? config->theoreticalMaxLevel : 0;
    displayCurrentLevel = g_app.currentLevel;
    displayMaxLevel = g_app.maxLevel;
    displaySectionCount = g_app.sectionCount;
    liveLevelsPerMinute = current_levels_per_minute();

    if (snapshot != NULL && snapshot->valid) {
        displayModeLabel = snapshot->modeLabel;
        displayTheoreticalMaxLevel = snapshot->theoreticalMaxLevel;
        displayCurrentLevel = snapshot->finalLevel;
        displayMaxLevel = snapshot->maxLevel;
        displaySectionCount = snapshot->sectionCount;
    }
    gmRequirementText = gm_requirement_text_for_mode(displayModeLabel);
    for (i = 0; i < COLUMN_COUNT; ++i) {
        columnVisible[i] = is_column_visible(TABLE_COLUMNS[i].id);
    }

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
            swprintf(line, 128, L"Mode: %ls    Current Level: %d    Max Level: %d", displayModeLabel, displayCurrentLevel, displayMaxLevel);
            draw_text_line(memoryDc, &y, line, RGB(240, 240, 240));
        }
    } else {
        y += 22;
    }

    if (snapshot == NULL && g_app.modeDetected && g_app.timerRunning && g_app.levelReadable && g_app.currentGameTimerFrames >= 0) {
        format_game_timer(gameTimeText, ARRAY_COUNT(gameTimeText), g_app.currentGameTimerFrames);
        swprintf(line, 128, L"Run Time: %ls    Pace: %.1f lv/min    Max: %.1f lv/min", gameTimeText, liveLevelsPerMinute, g_app.maxLevelsPerMinute);
        draw_text_line(memoryDc, &y, line, RGB(180, 255, 180));
    } else {
        y += 22;
    }

    if (g_app.showProgressBar) {
        progressRatio = current_section_progress(displayCurrentLevel, displayTheoreticalMaxLevel);
        progressSectionIndex = current_section_index_for_display(displayCurrentLevel, displayTheoreticalMaxLevel);
        progressTetrisCount = 0;
        if (progressSectionIndex >= 0 && progressSectionIndex < MAX_SECTION_COUNT) {
            progressTetrisCount = snapshot != NULL ? snapshot->tetrisCounts[progressSectionIndex] : g_app.tetrisCounts[progressSectionIndex];
        }
        progressBarWidth = (int)((clientRect.right - clientRect.left - 24) * progressRatio);
        if (progressBarWidth < 0) progressBarWidth = 0;
        if (progressBarWidth > clientRect.right - clientRect.left - 24) progressBarWidth = clientRect.right - clientRect.left - 24;

        progressBarTop = y + 8;
        progressBarLeft = (clientRect.right - clientRect.left - progressBarWidth) / 2;
        progressBarFillWidth = progressBarWidth;
        progressOuterRect.left = 12;
        progressOuterRect.top = progressBarTop;
        progressOuterRect.right = clientRect.right - 12;
        progressOuterRect.bottom = progressBarTop + 32;
        progressFillRect.left = progressBarLeft;
        progressFillRect.top = progressBarTop;
        progressFillRect.right = progressBarLeft + progressBarFillWidth;
        progressFillRect.bottom = progressBarTop + 32;

        progressOuterBrush = CreateSolidBrush(RGB(40, 40, 40));
        FillRect(memoryDc, &progressOuterRect, progressOuterBrush);
        DeleteObject(progressOuterBrush);

        if (progressBarFillWidth > 0) {
            int red;
            int green;
            int blue;

            if (progressTetrisCount > 0) {
                if (progressRatio >= 0.96) {
                    red = 144; green = 238; blue = 144;
                } else {
                    red = 0 + (int)(120.0 * progressRatio);
                    green = 80 + (int)(160.0 * progressRatio);
                    blue = 180 + (int)(75.0 * progressRatio);
                }
            } else {
                red = 96 + (int)(159.0 * progressRatio);
                green = red;
                blue = red;
            }

            if (red > 255) red = 255;
            if (green > 255) green = 255;
            if (blue > 255) blue = 255;

            progressFillBrush = CreateSolidBrush(RGB(red, green, blue));
            FillRect(memoryDc, &progressFillRect, progressFillBrush);
            DeleteObject(progressFillBrush);
        }

        progressPen = CreatePen(PS_SOLID, 1, RGB(110, 110, 110));
        oldPen = (HPEN)SelectObject(memoryDc, progressPen);
        oldBrush = (HBRUSH)SelectObject(memoryDc, GetStockObject(NULL_BRUSH));
        Rectangle(memoryDc, progressOuterRect.left, progressOuterRect.top, progressOuterRect.right, progressOuterRect.bottom);
        SelectObject(memoryDc, oldBrush);
        SelectObject(memoryDc, oldPen);
        DeleteObject(progressPen);
        y = progressBarTop + 32;
    }

    if (!g_app.modeDetected && snapshot == NULL) {
        BitBlt(hdc, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, memoryDc, 0, 0, SRCCOPY);
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

    format_section_label(sectionLabel, ARRAY_COUNT(sectionLabel), visibleSectionCount - 1, displayTheoreticalMaxLevel);
    sectionWidth = max_int(measure_text_width(memoryDc, L"Section"), measure_text_width(memoryDc, sectionLabel)) + cellPadding;
    gameTimeWidth = max_int(measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_GAME_TIME].header), measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_GAME_TIME].sampleText)) + cellPadding;
    deltaWidth = max_int(measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_DELTA].header), measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_DELTA].sampleText)) + cellPadding;
    sectionTimeWidth = max_int(measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_SECTION_TIME].header), measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_SECTION_TIME].sampleText)) + cellPadding;
    bestWidth = max_int(measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_BEST].header), measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_BEST].sampleText)) + cellPadding;
    backWidth = max_int(measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_BACK].header), measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_BACK].sampleText)) + cellPadding;
    tetWidth = max_int(measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_TET].header), measure_text_width(memoryDc, TABLE_COLUMNS[COLUMN_TET].sampleText)) + cellPadding;

    columnWidths[COLUMN_GAME_TIME] = gameTimeWidth;
    columnWidths[COLUMN_DELTA] = deltaWidth;
    columnWidths[COLUMN_SECTION_TIME] = sectionTimeWidth;
    columnWidths[COLUMN_BEST] = bestWidth;
    columnWidths[COLUMN_BACK] = backWidth;
    columnWidths[COLUMN_TET] = tetWidth;

    visibleColumnCount = 0;
    tableRight = tableLeft + sectionWidth;
    for (i = 0; i < COLUMN_COUNT; ++i) {
        if (!columnVisible[i]) {
            continue;
        }
        columnX[visibleColumnCount] = tableRight;
        tableRight += columnWidths[i];
        visibleColumnCount += 1;
    }

    draw_table_grid(memoryDc, tableLeft, tableTop, tableRight, tableTop + rowHeight * (visibleSectionCount + 1), columnX, visibleColumnCount, rowHeight, visibleSectionCount + 1);
    draw_table_text(memoryDc, tableLeft + 10, tableTop + 6, L"Section", RGB(255, 230, 160));

    visibleColumnCount = 0;
    for (i = 0; i < COLUMN_COUNT; ++i) {
        if (!columnVisible[i]) {
            continue;
        }
        draw_table_text(memoryDc, columnX[visibleColumnCount++] + 10, tableTop + 6, TABLE_COLUMNS[i].header, RGB(255, 230, 160));
    }

    for (i = 0; i < visibleSectionCount; ++i) {
        int rowY = tableTop + rowHeight * (i + 1) + 6;
        double delta = snapshot != NULL ? snapshot->sectionDeltas[i] : g_app.sectionDeltas[i];
        wchar_t deltaSign = delta < 0.0 ? L'-' : L'+';

        format_section_label(line, ARRAY_COUNT(line), i, displayTheoreticalMaxLevel);
        draw_table_text(memoryDc, tableLeft + 10, rowY, line, RGB(240, 240, 240));

        visibleColumnCount = 0;
        if (columnVisible[COLUMN_GAME_TIME]) {
            if (i < displaySectionCount && (snapshot != NULL ? snapshot->sectionTimes[i] : g_app.sectionTimes[i]) >= 0.0) {
                format_game_timer(line, ARRAY_COUNT(line), snapshot != NULL ? snapshot->sectionGameTimerFrames[i] : g_app.sectionGameTimerFrames[i]);
                draw_table_text(memoryDc, columnX[visibleColumnCount] + 10, rowY, line, color_for_delta(delta));
            } else {
                draw_table_text(memoryDc, columnX[visibleColumnCount] + 10, rowY, L"-", RGB(140, 140, 140));
            }
            visibleColumnCount += 1;
        }
        if (columnVisible[COLUMN_DELTA]) {
            if (i < displaySectionCount && (snapshot != NULL ? snapshot->sectionTimes[i] : g_app.sectionTimes[i]) >= 0.0) {
                swprintf(line, ARRAY_COUNT(line), L"%lc%.3f s", deltaSign, delta < 0.0 ? -delta : delta);
                draw_table_text(memoryDc, columnX[visibleColumnCount] + 10, rowY, line, color_for_delta(delta));
            } else {
                draw_table_text(memoryDc, columnX[visibleColumnCount] + 10, rowY, L"-", RGB(140, 140, 140));
            }
            visibleColumnCount += 1;
        }
        if (columnVisible[COLUMN_SECTION_TIME]) {
            if (i < displaySectionCount && (snapshot != NULL ? snapshot->sectionTimes[i] : g_app.sectionTimes[i]) >= 0.0) {
                format_seconds_as_game_time(line, ARRAY_COUNT(line), snapshot != NULL ? snapshot->sectionTimes[i] : g_app.sectionTimes[i]);
                draw_table_text(memoryDc, columnX[visibleColumnCount] + 10, rowY, line, RGB(220, 220, 220));
            } else {
                draw_table_text(memoryDc, columnX[visibleColumnCount] + 10, rowY, L"-", RGB(140, 140, 140));
            }
            visibleColumnCount += 1;
        }
        if (columnVisible[COLUMN_BEST]) {
            swprintf(line, ARRAY_COUNT(line), L"%.3f s", snapshot != NULL ? snapshot->bestSectionTimes[i] : g_app.bestSectionTimes[i]);
            draw_table_text(memoryDc, columnX[visibleColumnCount] + 10, rowY, line, RGB(200, 200, 200));
            visibleColumnCount += 1;
        }
        if (columnVisible[COLUMN_BACK]) {
            swprintf(line, ARRAY_COUNT(line), L"%d", snapshot != NULL ? snapshot->backstepCounts[i] : g_app.backstepCounts[i]);
            draw_table_text(memoryDc, columnX[visibleColumnCount] + 10, rowY, line, RGB(240, 240, 240));
            visibleColumnCount += 1;
        }
        if (columnVisible[COLUMN_TET]) {
            swprintf(line, ARRAY_COUNT(line), L"%d", snapshot != NULL ? snapshot->tetrisCounts[i] : g_app.tetrisCounts[i]);
            draw_table_text(memoryDc, columnX[visibleColumnCount] + 10, rowY, line, RGB(240, 240, 240));
        }
    }

    infoTop = tableTop + rowHeight * (visibleSectionCount + 1) + 20;
    if (gmRequirementText != NULL) {
        draw_multiline_text(memoryDc, tableLeft, infoTop, gmRequirementText, RGB(200, 220, 255));
    }

    BitBlt(hdc, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, memoryDc, 0, 0, SRCCOPY);
    SelectObject(memoryDc, oldFont);
    SelectObject(memoryDc, oldBitmap);
    DeleteObject(font);
    DeleteObject(backBufferBitmap);
    DeleteDC(memoryDc);
    EndPaint(hwnd, &ps);
}
