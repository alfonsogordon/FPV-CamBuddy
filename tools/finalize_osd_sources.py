from pathlib import Path


def rep(path, old, new, label):
    p = Path(path)
    s = p.read_text(encoding='utf-8')
    if old not in s:
        raise SystemExit(f'Could not locate {label} in {path}')
    p.write_text(s.replace(old, new, 1), encoding='utf-8')

# ---- Config structure / persistence -------------------------------------------------
rep('src/config_manager.h',
'''        // Betaflight 4.5 has Pilot Name and Craft Name but not the Custom
        // Message 1-4 fields (added in 4.6). When enabled, the four osdN
        // templates above are not sent at all and pilotNameTpl/craftNameTpl
        // are sent to Pilot Name/Craft Name instead.''',
'''        // Betaflight 4.4 through 2025.12 uses Pilot Name and Craft Name.
        // Betaflight 2026.6+ provides Custom Message 1-4 via MSP2_SET_TEXT.
        // When legacy mode is enabled, the four osdN templates above are not
        // sent and pilotNameTpl/craftNameTpl are used instead.''', 'BF compatibility comment')
rep('src/config_manager.h',
'''        bool     fpvRecFlash;           // {state} flashes at 1 Hz while recording
        bool     fpvLowBatteryEnabled;''',
'''        bool     fpvRecFlash;           // {state} flashes at 1 Hz while recording
        uint8_t  fpvWarningTarget;       // 1..4, Custom Message destination on BF 2026.6+
        bool     fpvLowBatteryEnabled;''', 'warning target config field')
rep('src/config_manager.h',
'''    static constexpr bool     DEFAULT_FPV_REC_FLASH           = true;
    static constexpr bool     DEFAULT_FPV_LOW_BATTERY_ENABLED = true;''',
'''    static constexpr bool     DEFAULT_FPV_REC_FLASH           = true;
    static constexpr uint8_t  DEFAULT_FPV_WARNING_TARGET       = 1;
    static constexpr bool     DEFAULT_FPV_LOW_BATTERY_ENABLED = true;''', 'warning target default')
rep('src/config_manager.h',
'''    void setFpvRecFlash(bool v);
    void setFpvLowBatteryEnabled(bool v);''',
'''    void setFpvRecFlash(bool v);
    void setFpvWarningTarget(uint8_t v);
    void setFpvLowBatteryEnabled(bool v);''', 'warning target setter declaration')

rep('src/config_manager.cpp',
'''static constexpr const char *KEY_FPV_FLASH = "fpv_flash";
static constexpr const char *KEY_FPV_LOW_EN = "fpv_low_en";''',
'''static constexpr const char *KEY_FPV_FLASH = "fpv_flash";
static constexpr const char *KEY_FPV_WARN_DST = "fpv_warn_dst";
static constexpr const char *KEY_FPV_LOW_EN = "fpv_low_en";''', 'warning target NVS key')
rep('src/config_manager.cpp',
'''    _cfg.fpvRecFlash         = _prefs.getBool(KEY_FPV_FLASH, DEFAULT_FPV_REC_FLASH);
    _cfg.fpvLowBatteryEnabled = _prefs.getBool(KEY_FPV_LOW_EN, DEFAULT_FPV_LOW_BATTERY_ENABLED);''',
'''    _cfg.fpvRecFlash         = _prefs.getBool(KEY_FPV_FLASH, DEFAULT_FPV_REC_FLASH);
    _cfg.fpvWarningTarget    = static_cast<uint8_t>(_prefs.getUInt(KEY_FPV_WARN_DST, DEFAULT_FPV_WARNING_TARGET));
    if (_cfg.fpvWarningTarget < 1 || _cfg.fpvWarningTarget > 4) _cfg.fpvWarningTarget = DEFAULT_FPV_WARNING_TARGET;
    _cfg.fpvLowBatteryEnabled = _prefs.getBool(KEY_FPV_LOW_EN, DEFAULT_FPV_LOW_BATTERY_ENABLED);''', 'warning target load')
rep('src/config_manager.cpp',
'''    _prefs.putBool(KEY_FPV_FLASH, _cfg.fpvRecFlash);
    _prefs.putBool(KEY_FPV_LOW_EN, _cfg.fpvLowBatteryEnabled);''',
'''    _prefs.putBool(KEY_FPV_FLASH, _cfg.fpvRecFlash);
    _prefs.putUInt(KEY_FPV_WARN_DST, _cfg.fpvWarningTarget);
    _prefs.putBool(KEY_FPV_LOW_EN, _cfg.fpvLowBatteryEnabled);''', 'warning target save')
rep('src/config_manager.cpp',
'''    out.printf("[cfg] fpv_flash       = %s\\n", _cfg.fpvRecFlash ? "true" : "false");
    out.printf("[cfg] fpv_low_batt    = %s\\n", _cfg.fpvLowBatteryEnabled ? "true" : "false");''',
'''    out.printf("[cfg] fpv_flash       = %s\\n", _cfg.fpvRecFlash ? "true" : "false");
    out.printf("[cfg] fpv_warn_dst    = %u\\n", _cfg.fpvWarningTarget);
    out.printf("[cfg] fpv_low_batt    = %s\\n", _cfg.fpvLowBatteryEnabled ? "true" : "false");''', 'warning target show')
rep('src/config_manager.cpp',
'''        out.println("  Tokens: {state} {bat} {batn} {batt} {rect} {rectf} {rec} {recdur} {mode} {res} {fps} {eis} {rleft} {rcap}");
        out.println("  set bf45_compat <0|1>      - 1=target Betaflight 4.5: send pilot_tpl/craft_tpl to Pilot Name/Craft Name, stop sending osd1-4 (no Custom Message fields on 4.5)");''',
'''        out.println("  Tokens: {state} {stateonly} {bat} {batn} {batt} {rect} {rectf} {rec} {recdur} {mode} {res} {fps} {eis} {rleft} {rcap}");
        out.println("  set bf45_compat <0|1>      - 1=Betaflight 4.4-2025.12 Pilot/Craft Name; 0=Betaflight 2026.6+ Custom Message 1-4");''', 'BF help and stateonly token')
rep('src/config_manager.cpp',
'''        out.println("  set fpv_flash <0|1>        - flash {state} at 1 Hz while recording");
        out.println("  set fpv_prearm <0|1>       - custom message until first arm since boot");''',
'''        out.println("  set fpv_flash <0|1>        - flash {state} at 1 Hz while recording");
        out.println("  set fpv_warn_dst <1-4>     - Custom Message destination for warnings on Betaflight 2026.6+");
        out.println("  set fpv_prearm <0|1>       - custom message until first arm since boot");''', 'warning target help')
rep('src/config_manager.cpp',
'''        if (strncmp(rest, "fpv_low_batt ", 13) == 0) {''',
'''        if (strncmp(rest, "fpv_warn_dst ", 13) == 0) {
            const char *val = rest + 13; while (*val == ' ') val++;
            unsigned dst = strtoul(val, nullptr, 10); if (dst < 1) dst = 1; if (dst > 4) dst = 4;
            setFpvWarningTarget((uint8_t)dst);
            out.printf("[cfg] fpv_warn_dst = %u (saved)\\n", _cfg.fpvWarningTarget); return;
        }

        if (strncmp(rest, "fpv_low_batt ", 13) == 0) {''', 'warning target command')
rep('src/config_manager.cpp',
'''void ConfigManager::setFpvRecFlash(bool v) { _cfg.fpvRecFlash = v; _prefs.putBool(KEY_FPV_FLASH, v); }
void ConfigManager::setFpvLowBatteryEnabled(bool v)''',
'''void ConfigManager::setFpvRecFlash(bool v) { _cfg.fpvRecFlash = v; _prefs.putBool(KEY_FPV_FLASH, v); }
void ConfigManager::setFpvWarningTarget(uint8_t v) { if (v < 1) v = 1; if (v > 4) v = 4; _cfg.fpvWarningTarget = v; _prefs.putUInt(KEY_FPV_WARN_DST, v); }
void ConfigManager::setFpvLowBatteryEnabled(bool v)''', 'warning target setter')

# ---- MSP OSD engine -----------------------------------------------------------
rep('src/msp_serial.h',
'''    void sendCustomOSD4(const CameraData &data, const char *tpl);
    void sendPilotName(const CameraData &data, const char *tpl);''',
'''    void sendCustomOSD4(const CameraData &data, const char *tpl);
    void sendCustomOSDWarnings(uint8_t target, const CameraData &data, const char *tpl);
    void sendPilotName(const CameraData &data, const char *tpl);''', 'warning sender declaration')
rep('src/msp_serial.cpp',
'''    if (strcmp(tok, "state") == 0) {''',
'''    if (strcmp(tok, "stateonly") == 0) {
        if (data.valid && data.has_recording && data.recording) {
            if (!fpvRecFlash || flashOn) snprintf(val, valLen, "REC");
        }
    } else if (strcmp(tok, "state") == 0) {''', 'stateonly token')

# ---- Runtime routing ----------------------------------------------------------
rep('src/main.cpp',
'''    const bool craftFlashRefresh =
        liveCfg.bf45Compat && liveCfg.craftNameEnabled && liveCfg.fpvStateMode &&
        (now - lastBattMs) >= 500UL;''',
'''    const bool craftFlashRefresh =
        ((liveCfg.bf45Compat && liveCfg.craftNameEnabled && liveCfg.fpvStateMode) ||
         (!liveCfg.bf45Compat && (liveCfg.fpvLowBatteryEnabled || liveCfg.fpvLowRecTimeEnabled ||
                                  liveCfg.fpvHotWarningEnabled || liveCfg.fpvPreArmReminderText[0] != '\\0'))) &&
        (now - lastBattMs) >= 500UL;''', 'warning refresh cadence')
rep('src/main.cpp',
'''            // Betaflight 4.5 has no Custom Message 1-4 OSD fields — sending
            // them would just be ignored, so use Pilot Name/Craft Name instead.''',
'''            // Betaflight 4.4 through 2025.12 uses Pilot Name/Craft Name.
            // Betaflight 2026.6+ provides Custom Message 1-4 via MSP2_SET_TEXT.''', 'runtime BF comment')
rep('src/main.cpp',
'''            mspSerial.sendCustomOSD1(currentCamera, cfg.osd1Tpl);
            mspSerial.sendCustomOSD2(currentCamera, cfg.osd2Tpl);
            mspSerial.sendCustomOSD3(currentCamera, cfg.osd3Tpl);
            mspSerial.sendCustomOSD4(currentCamera, cfg.osd4Tpl);''',
'''            const uint8_t wt = cfg.fpvWarningTarget;
            if (wt == 1) mspSerial.sendCustomOSDWarnings(1, currentCamera, cfg.osd1Tpl); else mspSerial.sendCustomOSD1(currentCamera, cfg.osd1Tpl);
            if (wt == 2) mspSerial.sendCustomOSDWarnings(2, currentCamera, cfg.osd2Tpl); else mspSerial.sendCustomOSD2(currentCamera, cfg.osd2Tpl);
            if (wt == 3) mspSerial.sendCustomOSDWarnings(3, currentCamera, cfg.osd3Tpl); else mspSerial.sendCustomOSD3(currentCamera, cfg.osd3Tpl);
            if (wt == 4) mspSerial.sendCustomOSDWarnings(4, currentCamera, cfg.osd4Tpl); else mspSerial.sendCustomOSD4(currentCamera, cfg.osd4Tpl);''', 'current BF warning routing')

print('Final OSD firmware sources prepared')
