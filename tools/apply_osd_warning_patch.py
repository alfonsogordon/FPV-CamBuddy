from pathlib import Path

def rep(path, old, new):
    p=Path(path); s=p.read_text()
    if old not in s: raise SystemExit(f'missing patch marker in {path}: {old[:60]}')
    p.write_text(s.replace(old,new,1))

# Route the selected BF 2026.6+ Custom Message through the warning/reminder overlay.
rep('src/main.cpp', '''    const bool craftFlashRefresh =
        liveCfg.bf45Compat && liveCfg.craftNameEnabled && liveCfg.fpvStateMode &&
        (now - lastBattMs) >= 500UL;''', '''    const bool craftFlashRefresh =
        ((liveCfg.bf45Compat && liveCfg.craftNameEnabled && liveCfg.fpvStateMode) ||
         (!liveCfg.bf45Compat && (liveCfg.fpvLowBatteryEnabled || liveCfg.fpvLowRecTimeEnabled ||
                                  liveCfg.fpvHotWarningEnabled || liveCfg.fpvPreArmReminderText[0] != '\\0'))) &&
        (now - lastBattMs) >= 500UL;''')
rep('src/main.cpp', '''            mspSerial.sendCustomOSD1(currentCamera, cfg.osd1Tpl);
            mspSerial.sendCustomOSD2(currentCamera, cfg.osd2Tpl);
            mspSerial.sendCustomOSD3(currentCamera, cfg.osd3Tpl);
            mspSerial.sendCustomOSD4(currentCamera, cfg.osd4Tpl);''', '''            const uint8_t wt = mspSerial.warningTarget();
            if (wt == 1) mspSerial.sendCustomOSDWarnings(1, currentCamera, cfg.osd1Tpl); else mspSerial.sendCustomOSD1(currentCamera, cfg.osd1Tpl);
            if (wt == 2) mspSerial.sendCustomOSDWarnings(2, currentCamera, cfg.osd2Tpl); else mspSerial.sendCustomOSD2(currentCamera, cfg.osd2Tpl);
            if (wt == 3) mspSerial.sendCustomOSDWarnings(3, currentCamera, cfg.osd3Tpl); else mspSerial.sendCustomOSD3(currentCamera, cfg.osd3Tpl);
            if (wt == 4) mspSerial.sendCustomOSDWarnings(4, currentCamera, cfg.osd4Tpl); else mspSerial.sendCustomOSD4(currentCamera, cfg.osd4Tpl);''')
rep('src/main.cpp', '''            // Betaflight 4.5 has no Custom Message 1-4 OSD fields — sending
            // them would just be ignored, so use Pilot Name/Craft Name instead.''', '''            // Betaflight 4.4 through 2025.12 uses Pilot Name/Craft Name.
            // Betaflight 2026.6+ provides Custom Message 1-4 via MSP2_SET_TEXT.''')
# {stateonly} is the UI's "REC only while recording" token: blank while idle, REC while recording.
rep('src/msp_serial.cpp', '''    if (strcmp(tok, "state") == 0) {''', '''    if (strcmp(tok, "stateonly") == 0) {
        if (data.valid && data.has_recording && data.recording) snprintf(val, valLen, "REC");
    } else if (strcmp(tok, "state") == 0) {''')
# Correct stale version comments in config declaration.
rep('src/config_manager.h', '''        // Betaflight 4.5 has Pilot Name and Craft Name but not the Custom
        // Message 1-4 fields (added in 4.6). When enabled, the four osdN''', '''        // Betaflight 4.4 through 2025.12 uses Pilot Name and Craft Name.
        // Betaflight 2026.6+ adds Custom Message 1-4. When legacy mode is enabled, the four osdN''')
# Fix spacing in the compact declaration if needed.
p=Path('src/msp_serial.h'); s=p.read_text(); s=s.replace('const char*=nullptr','const char * = nullptr'); p.write_text(s)
print('OSD warning firmware patch applied')
