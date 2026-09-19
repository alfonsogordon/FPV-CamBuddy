#pragma once
#include <Arduino.h>
#include <stdint.h>

// DJI Osmo Nano media/control BLE DUML framing.
// This is distinct from DJI's 0xAA R-SDK framing used by existing Action support.
//
// Frame:
//   55 len 04 crc8 targetLE idLE flags cmdSet cmdId payload crc16LE
// where flags=0x40 request, 0xC0 response, 0x00 notify.

struct NanoDumlFrame {
    uint16_t target = 0;
    uint16_t id = 0;
    uint8_t flags = 0;
    uint8_t cmdSet = 0;
    uint8_t cmdId = 0;
    const uint8_t *payload = nullptr;
    uint16_t payloadLen = 0;
};

uint8_t nano_crc8(const uint8_t *data, size_t len);
uint16_t nano_crc16(const uint8_t *data, size_t len);

uint16_t nano_duml_build(uint8_t *out, uint16_t outSize,
                         uint16_t target, uint16_t id,
                         uint8_t flags, uint8_t cmdSet, uint8_t cmdId,
                         const uint8_t *payload, uint16_t payloadLen);

bool nano_duml_parse(const uint8_t *data, uint16_t len, NanoDumlFrame &f);

// DJI PackString: [length:u8][UTF-8/ASCII bytes]. Returns bytes written.
uint16_t nano_pack_string(uint8_t *out, uint16_t outSize, const char *s);
