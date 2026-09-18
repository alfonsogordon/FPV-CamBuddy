#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <stdarg.h>

// Persistent rolling diagnostic log for V1.0.2 AP bring-up.
// Keep it below the practical NVS string ceiling and write only on meaningful
// transitions / deliberately sparse health snapshots.
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
    char msg[320];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    char line[380];
    snprintf(line, sizeof(line), "[%lu] %s\n", (unsigned long)millis(), msg);

    Preferences p;
    if (!p.begin("diag", false)) return;
    String s = p.getString("log", "");
    s += line;
    const size_t MAX_LOG = 3800;
    if (s.length() > MAX_LOG) {
        size_t cut = s.length() - MAX_LOG;
        int nl = s.indexOf('\n', cut);
        s = (nl >= 0) ? s.substring(nl + 1) : s.substring(cut);
    }
    p.putString("log", s);
    p.end();
}

static inline void diagLogBoot() {
    Preferences p;
    uint32_t bootNo = 1;
    if (p.begin("diag", false)) {
        bootNo = p.getUInt("boots", 0) + 1;
        p.putUInt("boots", bootNo);
        p.end();
    }

    diagLog("=== BOOT #%lu reset_reason=%d chip=%s rev=%u cpu=%uMHz sdk=%s flash=%u heap=%u min_heap=%u largest8=%u ===",
            (unsigned long)bootNo,
            (int)esp_reset_reason(),
            ESP.getChipModel(),
            (unsigned)ESP.getChipRevision(),
            (unsigned)ESP.getCpuFreqMHz(),
            ESP.getSdkVersion(),
            (unsigned)ESP.getFlashChipSize(),
            (unsigned)ESP.getFreeHeap(),
            (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
            (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
