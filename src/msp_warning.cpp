#include "msp_serial.h"

void MSPSerial::sendCustomOSDWarnings(uint8_t target, const CameraData &data, const char *tpl) {
    const uint8_t textType = target == 2 ? MSP_TEXT_CUSTOM_2 : target == 3 ? MSP_TEXT_CUSTOM_3 : target == 4 ? MSP_TEXT_CUSTOM_4 : MSP_TEXT_CUSTOM_1;
    const bool valid = data.valid;
    const bool recording = valid && data.has_recording && data.recording;
    const unsigned pct = data.percent > 100 ? 100 : data.percent;
    const bool lowBatt = _fpvLowBatteryEnabled && valid && data.has_battery && pct <= _fpvLowBatteryPct;
    const bool lowRec = _fpvLowRecEnabled && valid && data.has_remain_time && data.remain_time <= ((uint32_t)_fpvLowRecMinutes * 60UL);
    const bool hot = _fpvHotEnabled && valid && data.has_temperature && data.temp_over != 0;
    const char *warnings[3] = {nullptr, nullptr, nullptr};
    uint8_t count = 0;
    if (hot && ((recording && _fpvHotRecording) || (!recording && _fpvHotReady))) warnings[count++] = _fpvHotText;
    if (lowBatt && ((recording && _fpvLowBatteryRecText) || (!recording && _fpvLowBatteryReadyFlash))) warnings[count++] = _fpvLowBatteryText;
    if (lowRec && ((recording && _fpvLowRecRecording) || (!recording && _fpvLowRecReady))) warnings[count++] = _fpvLowRecText;
    const bool healthyReady = valid && data.has_recording && !recording && !(data.has_temperature && data.temp_over >= 2) && !(data.has_media_ready && !data.media_ready) && count == 0;
    const bool reminderDue = _fpvPreArmText[0] != '\0' && (!_fpvPreArmEnabled || !_hasArmedSinceBoot) && healthyReady && ((millis() % _fpvPreArmIntervalMs) < _fpvPreArmShowMs);
    if (reminderDue) { sendCustomText(textType, _fpvPreArmText); return; }
    const bool flashOn = (((millis() / 500UL) & 1U) == 0U);
    if (count && !flashOn) { sendCustomText(textType, warnings[(millis() / 1000UL) % count]); return; }
    sendCustomOSD(textType, data, tpl);
}
