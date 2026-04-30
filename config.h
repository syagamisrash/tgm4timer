#ifndef CONFIG_H
#define CONFIG_H

/*
 * Runtime memory addresses are loaded from config.txt next to the executable.
 * This header keeps only app-wide timing and polling constants.
 */

static const int LEVEL_RESET_THRESHOLD = 50;
static const int POLL_INTERVAL_MS = 15;
static const int WINDOW_REFRESH_MS = 33;

#endif
