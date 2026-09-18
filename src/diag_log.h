#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>
#include <stdarg.h>

// Small persistent rolling diagnostic log for V1.0.2 AP bring-up.
// Writes only on meaningful state transitions, not in the fast loop.
static inline String diagRead() {
    Preferences p;
    if (!p.begin("diag", true)) return String();
    String s = p.getString("log", "");
    p.end();
    return s;
}

static inline void diagClear() {
    Preferences p;
    if (!p.begin("diag", false)) return;
    p.remove("log");
    p.end();
}

static inline void diagLog(const char *fmt, ...) {
    char msg[240];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    char line[300];
    snprintf(line, sizeof(line), "[%lu] %s\n", (unsigned long)millis(), msg);

    Preferences p;
    if (!p.begin("diag", false)) return;
    String s = p.getString("log", "");
    s += line;
    const size_t MAX_LOG = 3000;
    if (s.length() > MAX_LOG) {
        size_t cut = s.length() - MAX_LOG;
        int nl = s.indexOf('\n', cut);
        s = (nl >= 0) ? s.substring(nl + 1) : s.substring(cut);
    }
    p.putString("log", s);
    p.end();
}

static inline void diagLogBoot() {
    diagLog("BOOT reset_reason=%d heap=%u", (int)esp_reset_reason(), (unsigned)ESP.getFreeHeap());
}
