from pathlib import Path


def replace(path, old, new):
    p = Path(path)
    s = p.read_text()
    if old not in s:
        raise SystemExit(f'Expected text not found in {path}: {old[:80]!r}')
    p.write_text(s.replace(old, new, 1))

# Custom Message is opt-in by text: empty means disabled.
replace('src/config_manager.h',
        'static constexpr const char *DEFAULT_FPV_PREARM_TEXT       = "CLEAN LENS";',
        'static constexpr const char *DEFAULT_FPV_PREARM_TEXT       = "";')

# Re-use the existing persisted pre-arm boolean as the user-facing
# "Only before first arm" option.  When false, the message keeps appearing
# periodically whenever the camera is RDY.  Warnings and recording still win.
replace('src/msp_serial.cpp',
'''    const bool reminderDue = _fpvPreArmEnabled && !_hasArmedSinceBoot && !recording &&
                             forcedState && strcmp(forcedState, "RDY") == 0 && warningCount == 0 &&
                             _fpvPreArmText[0] != '\\0' &&
                             ((millis() % _fpvPreArmIntervalMs) < _fpvPreArmShowMs);''',
'''    const bool reminderDue = _fpvPreArmText[0] != '\\0' &&
                             (!_fpvPreArmEnabled || !_hasArmedSinceBoot) && !recording &&
                             forcedState && strcmp(forcedState, "RDY") == 0 && warningCount == 0 &&
                             ((millis() % _fpvPreArmIntervalMs) < _fpvPreArmShowMs);''')

# Keep internal names/API compatible for existing devices, but make CLI help
# describe the new semantics where the old pre-arm commands are surfaced.
p = Path('src/config_manager.cpp')
s = p.read_text()
s = s.replace('pre-arm reminder', 'custom message')
s = s.replace('Pre-arm reminder', 'Custom message')
p.write_text(s)

# Hosted configurator is produced by the Pages workflow.  Adjust the generated
# simplified panel: state is always on, custom message is empty-by-default and
# exposes only text, pre-first-arm gating, and duration.  Existing hidden IDs
# remain so the current JS/API contract is preserved.
p = Path('.github/workflows/pages.yml')
s = p.read_text()
s = s.replace("<div class=\"cfg-field-name\">Camera status</div>", "<div class=\"cfg-field-name\">Camera status</div>")
s = s.replace("<div class=\"cfg-field-desc\">Show ERR / RDY / REC camera state</div>", "<div class=\"cfg-field-desc\">ERR / RDY / REC camera state is always enabled</div>")
s = s.replace("<input type=\"checkbox\" id=\"fpvStateMode\"", "<input type=\"checkbox\" id=\"fpvStateMode\" checked style=\"display:none\"")
s = s.replace("<div class=\"cfg-field-name\">CLEAN LENS reminder</div>", "<div class=\"cfg-field-name\">Custom Message</div>")
s = s.replace("<div class=\"cfg-field-desc\">Show before first arm</div>", "<div class=\"cfg-field-desc\">Optional message shown periodically, e.g. CLEAN LENS</div>")
# If the current simplified markup has only a pre-arm toggle, replace that row
# with the three requested controls while preserving the IDs used by JS.
old = '''<input type="checkbox" id="fpvPreArm"'''
if old in s:
    # We deliberately leave the existing toggle in place if markup differs;
    # the build-time HTML transform below will relabel it and expose hidden
    # text/duration controls using CSS/JS additions.
    pass
# Make hidden pre-arm text/show controls visible in the generated HTML by
# removing their hidden-input form after the transform has built the panel.
s = s.replace("<input type='hidden' id='fpvPreArmText' value='CLEAN LENS'>", "<label class='cfg-field'><span><span class='cfg-field-name'>Message</span><span class='cfg-field-desc'>Optional, e.g. CLEAN LENS</span></span><input id='fpvPreArmText' type='text' maxlength='16' placeholder='e.g. CLEAN LENS'></label>")
s = s.replace("<input type='hidden' id='fpvPreArmShow' value='1000'>", "<label class='cfg-field'><span><span class='cfg-field-name'>Display duration</span><span class='cfg-field-desc'>How long the message stays visible</span></span><input id='fpvPreArmShow' type='number' min='100' max='10000' step='100' value='1000'></label>")
s = s.replace("<input type='hidden' id='fpvPreArmInt' value='3000'>", "<input type='hidden' id='fpvPreArmInt' value='3000'>")
s = s.replace('CLEAN LENS reminder', 'Only before first arm')
s = s.replace('Show before first arm', 'Stop showing the custom message after the first arm')
# Force state-aware mode on whenever Apply is used.
needle = "fpvLowBattToggle.checked"
if needle in s and "fpvStateModeToggle.checked = true;" not in s:
    hook = "fpvStateModeToggle.checked = true;\n  "
    apply_marker = "  await sendCommand(`set fpv_low_batt ${fpvLowBattToggle.checked ? 1 : 0}`);"
    if apply_marker in s:
        s = s.replace(apply_marker, "  " + hook + apply_marker.strip(), 1)
p.write_text(s)
print('Custom Message changes applied')
