#include "camera_registry.h"
#include <cstring>

static constexpr const char *NVS_NS     = "cam_reg";
static constexpr const char *KEY_CNT    = "cnt";
static constexpr const char *KEY_LAST   = "last";
static constexpr const char *KEY_LIST   = "list";
static constexpr const char *KEY_LABELS = "labels";
static constexpr const char *KEY_NUMS   = "nums";

static const char *cameraTypeName(uint8_t t) {
    switch (t) {
        case 1:  return "GoPro";
        case 2:  return "Caddx";
        case 3:  return "Sony";
        case 4:  return "Blackmagic";
        case 5:  return "Insta360";
        default: return "DJI";
    }
}

void CameraRegistry::begin() {
    _prefs.begin(NVS_NS, false);
    load();
}

void CameraRegistry::initialiseMetadataIfNeeded() {
    bool changed = false;
    bool used[CAMREG_MAX + 1] = {};

    for (uint8_t i = 0; i < _count; ++i) {
        const uint8_t n = _numbers[i];
        if (n == 0 || n > CAMREG_MAX || used[n]) {
            _numbers[i] = 0;
            changed = true;
        } else {
            used[n] = true;
        }
        _labels[i][CAMREG_LABEL_LEN - 1] = '\0';
    }

    for (uint8_t i = 0; i < _count; ++i) {
        if (_numbers[i] != 0) continue;
        for (uint8_t n = 1; n <= CAMREG_MAX; ++n) {
            if (!used[n]) {
                _numbers[i] = n;
                used[n] = true;
                changed = true;
                break;
            }
        }
    }

    if (changed) save();
}

void CameraRegistry::load() {
    memset(_labels, 0, sizeof(_labels));
    memset(_numbers, 0, sizeof(_numbers));

    _count   = _prefs.getUChar(KEY_CNT, 0);
    _lastIdx = (int8_t)_prefs.getChar(KEY_LAST, -1);

    if (_count > CAMREG_MAX) { _count = 0; _lastIdx = -1; return; }
    if (_lastIdx >= (int8_t)_count) _lastIdx = -1;

    if (_count > 0) {
        const size_t expectedEntries = (size_t)_count * sizeof(CameraEntry);
        const size_t gotEntries = _prefs.getBytes(KEY_LIST, _entries, expectedEntries);
        if (gotEntries != expectedEntries) {
            _count = 0;
            _lastIdx = -1;
            return;
        }

        const size_t expectedLabels = (size_t)_count * CAMREG_LABEL_LEN;
        const size_t expectedNums = (size_t)_count * sizeof(uint8_t);
        _prefs.getBytes(KEY_LABELS, _labels, expectedLabels);
        _prefs.getBytes(KEY_NUMS, _numbers, expectedNums);
        initialiseMetadataIfNeeded();
    }
}

void CameraRegistry::save() {
    _prefs.putUChar(KEY_CNT,  _count);
    _prefs.putChar (KEY_LAST, (char)_lastIdx);
    if (_count > 0) {
        _prefs.putBytes(KEY_LIST, _entries, (size_t)_count * sizeof(CameraEntry));
        _prefs.putBytes(KEY_LABELS, _labels, (size_t)_count * CAMREG_LABEL_LEN);
        _prefs.putBytes(KEY_NUMS, _numbers, (size_t)_count * sizeof(uint8_t));
    } else {
        _prefs.remove(KEY_LIST);
        _prefs.remove(KEY_LABELS);
        _prefs.remove(KEY_NUMS);
    }
}

int CameraRegistry::findByAddr(const char *addr) const {
    if (!addr) return -1;
    for (int i = 0; i < (int)_count; i++)
        if (strcmp(_entries[i].addr, addr) == 0) return i;
    return -1;
}

uint8_t CameraRegistry::nextCameraNumber() const {
    bool used[CAMREG_MAX + 1] = {};
    for (uint8_t i = 0; i < _count; ++i) {
        const uint8_t n = _numbers[i];
        if (n > 0 && n <= CAMREG_MAX) used[n] = true;
    }
    for (uint8_t n = 1; n <= CAMREG_MAX; ++n)
        if (!used[n]) return n;
    return 1;
}

void CameraRegistry::onConnected(const char *name, const char *addr,
                                   uint8_t addrType, uint8_t cameraType,
                                   const char *pass) {
    int idx = findByAddr(addr);
    const bool isNew = (idx < 0);
    if (isNew) {
        if (_count < CAMREG_MAX) {
            idx = (int)_count++;
        } else {
            // List full: evict oldest entry that isn't the last connected.
            int evict = (_lastIdx == 0) ? 1 : 0;
            memmove(&_entries[evict], &_entries[evict + 1],
                    ((size_t)_count - (size_t)evict - 1) * sizeof(CameraEntry));
            memmove(&_labels[evict], &_labels[evict + 1],
                    ((size_t)_count - (size_t)evict - 1) * CAMREG_LABEL_LEN);
            memmove(&_numbers[evict], &_numbers[evict + 1],
                    ((size_t)_count - (size_t)evict - 1) * sizeof(uint8_t));
            _count--;
            if (_lastIdx > evict) _lastIdx--;
            idx = (int)_count++;
        }
        memset(_labels[idx], 0, CAMREG_LABEL_LEN);
        _numbers[idx] = 0;
        _numbers[idx] = nextCameraNumber();
    }
    strlcpy(_entries[idx].name, name, CAMREG_NAME_LEN);
    strlcpy(_entries[idx].addr, addr, CAMREG_ADDR_LEN);
    _entries[idx].addrType   = addrType;
    _entries[idx].cameraType = cameraType;
    if (pass)       strlcpy(_entries[idx].pass, pass, CAMREG_PASS_LEN);
    else if (isNew) _entries[idx].pass[0] = '\0';
    _lastIdx     = (int8_t)idx;
    _selectedIdx = -1;
    save();
}

std::string CameraRegistry::preferredAddr() const {
    int idx = (_selectedIdx >= 0) ? _selectedIdx : _lastIdx;
    if (idx < 0 || idx >= (int)_count) return "";
    return std::string(_entries[idx].addr);
}

uint8_t CameraRegistry::preferredAddrType() const {
    int idx = (_selectedIdx >= 0) ? _selectedIdx : _lastIdx;
    if (idx < 0 || idx >= (int)_count) return 0;
    return _entries[idx].addrType;
}

bool CameraRegistry::preferredEntry(uint8_t wantType, CameraEntry &out) const {
    int idx = (_selectedIdx >= 0) ? _selectedIdx : _lastIdx;
    if (idx < 0 || idx >= (int)_count) return false;
    if (_entries[idx].cameraType != wantType) return false;
    out = _entries[idx];
    return true;
}

bool CameraRegistry::setPassword(uint8_t idx, const char *pass) {
    if (idx >= _count) return false;
    strlcpy(_entries[idx].pass, pass, CAMREG_PASS_LEN);
    save();
    return true;
}

bool CameraRegistry::setLabel(uint8_t idx, const char *labelText) {
    if (idx >= _count || !labelText) return false;

    while (*labelText == ' ') ++labelText;
    char clean[CAMREG_LABEL_LEN] = {};
    uint8_t w = 0;
    bool pendingSpace = false;
    for (const char *p = labelText; *p && w < CAMREG_LABEL_LEN - 1; ++p) {
        const unsigned char c = (unsigned char)*p;
        if (c < 32 || c > 126 || c == '[' || c == ']') continue;
        if (c == ' ') {
            pendingSpace = (w > 0);
            continue;
        }
        if (pendingSpace && w < CAMREG_LABEL_LEN - 1) clean[w++] = ' ';
        pendingSpace = false;
        if (w < CAMREG_LABEL_LEN - 1) clean[w++] = (char)c;
    }
    clean[w] = '\0';
    strlcpy(_labels[idx], clean, CAMREG_LABEL_LEN);
    save();
    return true;
}

const char *CameraRegistry::label(uint8_t idx) const {
    return idx < _count ? _labels[idx] : "";
}

uint8_t CameraRegistry::cameraNumber(uint8_t idx) const {
    return idx < _count ? _numbers[idx] : 0;
}

bool CameraRegistry::identityForAddr(const char *addr, uint8_t &number,
                                     char *labelOut, size_t labelOutLen) const {
    const int idx = findByAddr(addr);
    if (idx < 0) return false;
    number = _numbers[idx];
    if (labelOut && labelOutLen) strlcpy(labelOut, _labels[idx], labelOutLen);
    return number != 0;
}

bool CameraRegistry::identityForType(uint8_t cameraType, uint8_t &number,
                                     char *labelOut, size_t labelOutLen) const {
    // The most recently connected camera is the best exact answer for the
    // single-instance camera families used by the multi-brand coordinator.
    if (_lastIdx >= 0 && _lastIdx < (int8_t)_count &&
        _entries[_lastIdx].cameraType == cameraType) {
        number = _numbers[_lastIdx];
        if (labelOut && labelOutLen) strlcpy(labelOut, _labels[_lastIdx], labelOutLen);
        return number != 0;
    }

    // Fallback for a family whose connection happened before another family.
    // This remains deterministic for the normal one-camera-per-non-GoPro-family
    // case; multi-GoPro uses identityForAddr() per slot instead.
    for (int i = (int)_count - 1; i >= 0; --i) {
        if (_entries[i].cameraType != cameraType) continue;
        number = _numbers[i];
        if (labelOut && labelOutLen) strlcpy(labelOut, _labels[i], labelOutLen);
        return number != 0;
    }
    return false;
}

void CameraRegistry::selectCamera(uint8_t idx) {
    if (idx < _count) _selectedIdx = (int8_t)idx;
}

void CameraRegistry::clearSelection() {
    _selectedIdx = -1;
}

bool CameraRegistry::getEntry(uint8_t idx, CameraEntry &out) const {
    if (idx >= _count) return false;
    out = _entries[idx];
    return true;
}

bool CameraRegistry::remove(uint8_t idx) {
    if (idx >= _count) return false;
    memmove(&_entries[idx], &_entries[idx + 1],
            ((size_t)_count - idx - 1) * sizeof(CameraEntry));
    memmove(&_labels[idx], &_labels[idx + 1],
            ((size_t)_count - idx - 1) * CAMREG_LABEL_LEN);
    memmove(&_numbers[idx], &_numbers[idx + 1],
            ((size_t)_count - idx - 1) * sizeof(uint8_t));
    _count--;
    memset(&_entries[_count], 0, sizeof(CameraEntry));
    memset(&_labels[_count], 0, CAMREG_LABEL_LEN);
    _numbers[_count] = 0;
    if (_lastIdx    == (int8_t)idx) _lastIdx = -1;
    else if (_lastIdx    > (int8_t)idx) _lastIdx--;
    if (_selectedIdx == (int8_t)idx) _selectedIdx = -1;
    else if (_selectedIdx > (int8_t)idx) _selectedIdx--;
    save();
    return true;
}

void CameraRegistry::clear() {
    memset(_entries, 0, sizeof(_entries));
    memset(_labels, 0, sizeof(_labels));
    memset(_numbers, 0, sizeof(_numbers));
    _count       = 0;
    _lastIdx     = -1;
    _selectedIdx = -1;
    save();
}

void CameraRegistry::printList(Stream &out) const {
    if (_count == 0) { out.println("[reg] No cameras saved"); return; }
    out.printf("[reg] %u camera(s) saved:\n", _count);
    for (uint8_t i = 0; i < _count; i++) {
        const char *tag = "";
        bool isLast = (i == (uint8_t)_lastIdx);
        bool isSel  = (i == (uint8_t)_selectedIdx);
        if (isLast && isSel)  tag = " [last+sel]";
        else if (isLast)      tag = " [last]";
        else if (isSel)       tag = " [selected]";
        // Two literal spaces (not one) before addr: %-28s only pads up to
        // its width, so a name at or past 28 chars would otherwise leave
        // just one space — the browser parser needs a 2+-space boundary.
        out.printf("[reg]  %2u: %-28s  %s  %s%s [cam=%u] [label=%s]\n", i,
                   _entries[i].name, _entries[i].addr,
                   cameraTypeName(_entries[i].cameraType), tag,
                   _numbers[i], _labels[i]);
    }
}

String CameraRegistry::toJson() const {
    String out = "[";
    for (uint8_t i = 0; i < _count; i++) {
        if (i) out += ',';
        out += F("{\"idx\":");       out += i;
        out += F(",\"name\":\"");    out += _entries[i].name;
        out += F("\",\"addr\":\"");  out += _entries[i].addr;
        out += F("\",\"type\":");    out += _entries[i].cameraType;
        out += F(",\"cam\":");       out += _numbers[i];
        out += F(",\"label\":\"");   out += _labels[i];
        out += F("\",\"last\":");    out += (i == (uint8_t)_lastIdx    ? F("true") : F("false"));
        out += F(",\"sel\":");       out += (i == (uint8_t)_selectedIdx ? F("true") : F("false"));
        out += '}';
    }
    out += ']';
    return out;
}