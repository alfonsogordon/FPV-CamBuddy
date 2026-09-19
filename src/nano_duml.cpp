#include "nano_duml.h"
#include <cstring>

uint8_t nano_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x77;  // reflected form of DJI init 0xEE
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; ++i)
            crc = (crc & 1U) ? (uint8_t)((crc >> 1) ^ 0x8C) : (uint8_t)(crc >> 1);
    }
    return crc;
}

uint16_t nano_crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0x3692;  // reflected form of DJI init 0x496C
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; ++i)
            crc = (crc & 1U) ? (uint16_t)((crc >> 1) ^ 0x8408) : (uint16_t)(crc >> 1);
    }
    return crc;
}

uint16_t nano_duml_build(uint8_t *out, uint16_t outSize,
                         uint16_t target, uint16_t id,
                         uint8_t flags, uint8_t cmdSet, uint8_t cmdId,
                         const uint8_t *payload, uint16_t payloadLen) {
    const uint16_t total = (uint16_t)(13U + payloadLen);
    if (!out || total > outSize || total > 255U) return 0;

    uint16_t o = 0;
    out[o++] = 0x55;
    out[o++] = (uint8_t)total;
    out[o++] = 0x04;
    out[o++] = nano_crc8(out, 3);

    out[o++] = (uint8_t)(target & 0xFF);
    out[o++] = (uint8_t)(target >> 8);
    out[o++] = (uint8_t)(id & 0xFF);
    out[o++] = (uint8_t)(id >> 8);
    out[o++] = flags;
    out[o++] = cmdSet;
    out[o++] = cmdId;

    if (payload && payloadLen) {
        memcpy(out + o, payload, payloadLen);
        o += payloadLen;
    }

    const uint16_t crc = nano_crc16(out, o);
    out[o++] = (uint8_t)(crc & 0xFF);
    out[o++] = (uint8_t)(crc >> 8);
    return o;
}

bool nano_duml_parse(const uint8_t *data, uint16_t len, NanoDumlFrame &f) {
    if (!data || len < 13 || data[0] != 0x55) return false;
    if (data[1] != len || data[2] != 0x04) return false;
    if (nano_crc8(data, 3) != data[3]) return false;

    const uint16_t stored = (uint16_t)data[len - 2] | ((uint16_t)data[len - 1] << 8);
    if (nano_crc16(data, len - 2) != stored) return false;

    f.target = (uint16_t)data[4] | ((uint16_t)data[5] << 8);
    f.id = (uint16_t)data[6] | ((uint16_t)data[7] << 8);
    f.flags = data[8];
    f.cmdSet = data[9];
    f.cmdId = data[10];
    f.payload = (len > 13) ? data + 11 : nullptr;
    f.payloadLen = (uint16_t)(len - 13);
    return true;
}

uint16_t nano_pack_string(uint8_t *out, uint16_t outSize, const char *s) {
    if (!out || !s) return 0;
    const size_t n = strlen(s);
    if (n > 255 || outSize < n + 1) return 0;
    out[0] = (uint8_t)n;
    if (n) memcpy(out + 1, s, n);
    return (uint16_t)(n + 1);
}
