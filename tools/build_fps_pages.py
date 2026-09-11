from pathlib import Path

p = Path('web/config.html')
s = p.read_text(encoding='utf-8')
s = s.replace('<title>FreeCLinker — Config</title>', '<title>FreeCLinker — FPSteVe Edition Config</title>')
s = s.replace('<span class="brand">FreeCLinker</span>', '<span class="brand">FreeCLinker — FPSteVe Edition</span>')
s = s.replace('</head>', '  <link rel="stylesheet" href="fps-theme.css">\n</head>', 1)

start_marker = '      <div class="cfg-field" style="flex-direction:column;align-items:flex-start;">\n        <div class="cfg-field-name">State-aware Craft Name</div>'
end_marker = '      <div class="card-footer">'
start = s.find(start_marker)
end = s.find(end_marker, start)
if start == -1 or end == -1:
    raise SystemExit('Could not locate FPSteVe state/warning section')

lines = [
    '      <div class="cfg-field" style="flex-direction:column;align-items:flex-start;">',
    '        <div class="cfg-field-name">FPSteVe Camera OSD</div>',
    '        <div class="cfg-field-desc">Camera status (ERR / RDY / REC) is always enabled. Optional custom message and camera warnings can be configured below.</div>',
    '        <div style="display:none"><input type="checkbox" id="fpvStateMode" checked></div>',
    '        <div style="width:100%;margin-top:16px;"><strong>Custom Message</strong><div class="cfg-field-desc">Optional message shown periodically, e.g. CLEAN LENS. Leave blank to disable.</div></div>',
    '        <div style="display:flex;align-items:center;gap:10px;width:100%;margin-top:10px;"><label for="fpvPreArmText" style="min-width:185px;">Message</label><input type="text" id="fpvPreArmText" maxlength="16" placeholder="e.g. CLEAN LENS" style="flex:1;min-width:0;"></div>',
    '        <div style="display:flex;align-items:center;justify-content:space-between;width:100%;margin-top:12px;"><div><strong>Only before first arm</strong><div class="cfg-field-desc">Stop showing the message after the first arm until the next power cycle.</div></div><label class="switch"><input type="checkbox" id="fpvPreArm"><span class="track"><span class="thumb"></span></span></label></div>',
    '        <div style="display:flex;align-items:center;gap:10px;width:100%;margin-top:12px;"><label for="fpvCustomDurationSec" style="min-width:185px;">Display duration</label><input type="number" id="fpvCustomDurationSec" min="0.1" max="2.5" step="0.1" value="1.0" style="width:90px;"><span class="num-unit">sec</span></div>',
    '        <div style="display:none"><input type="number" id="fpvPreArmShow" value="1000"><input type="number" id="fpvPreArmInt" value="3000"></div>',
    '        <div style="display:flex;align-items:center;justify-content:space-between;width:100%;margin-top:20px;"><div><strong>Camera warnings</strong><div class="cfg-field-desc">Low battery, low recording-time and camera-hot warnings.</div></div><label class="switch"><input type="checkbox" id="fpvLowBatt"><span class="track"><span class="thumb"></span></span></label></div>',
    '        <div style="display:flex;align-items:center;gap:10px;width:100%;margin-top:14px;"><label for="fpvLowPct" style="min-width:185px;">Low battery threshold</label><input type="number" id="fpvLowPct" min="0" max="100" step="1" value="10" style="width:90px;"><span class="num-unit">%</span></div>',
    '        <div style="display:flex;align-items:center;gap:10px;width:100%;margin-top:12px;"><label for="fpvRecLowMin" style="min-width:185px;">Low recording-time threshold</label><input type="number" id="fpvRecLowMin" min="0" max="999" step="1" value="5" style="width:90px;"><span class="num-unit">min</span></div>',
    '        <div class="cfg-field-desc" style="margin-top:8px;">Camera-hot uses the camera over-temperature status, so it needs no threshold.</div>',
    '        <div style="display:none">',
    '          <input type="checkbox" id="fpvError" checked><input id="fpvErrorText" value="{state} {batt} {rectf}">',
    '          <input type="checkbox" id="fpvReady" checked><input id="fpvReadyText" value="{state} {batt} {rectf}">',
    '          <input type="checkbox" id="fpvRecord" checked><input id="fpvRecordText" value="{state}"><input type="checkbox" id="fpvFlash" checked>',
    '          <input id="fpvLowText" value="BATT LOW"><input type="checkbox" id="fpvLowRdyFlash" checked><input type="checkbox" id="fpvLowRecText" checked>',
    '          <input type="checkbox" id="fpvRecLow" checked><input id="fpvRecLowText" value="REC LOW"><input type="checkbox" id="fpvRecLowReady" checked><input type="checkbox" id="fpvRecLowRecording" checked>',
    '          <input type="checkbox" id="fpvHot" checked><input id="fpvHotText" value="CAM HOT"><input type="checkbox" id="fpvHotReady" checked><input type="checkbox" id="fpvHotRecording" checked>',
    '        </div>',
    '      </div>',
    ''
]
s = s[:start] + '\n'.join(lines) + s[end:]

hook = "[fpvStateModeToggle, fpvErrorToggle, fpvReadyToggle, fpvRecordToggle, fpvFlashToggle, fpvLowBattToggle, fpvRecLowToggle, fpvHotToggle].forEach(el => el.addEventListener('change', updateFpvOptionState));"
replacement = hook + "\n\n// One simple warning switch controls all tested warning types.\nfpvLowBattToggle.addEventListener('change', () => {\n  fpvRecLowToggle.checked = fpvLowBattToggle.checked;\n  fpvHotToggle.checked = fpvLowBattToggle.checked;\n  updateFpvOptionState();\n});\n\nconst customDurationInput = document.getElementById('fpvCustomDurationSec');\ncustomDurationInput?.addEventListener('change', () => {\n  const sec = Math.min(2.5, Math.max(0.1, parseFloat(customDurationInput.value) || 1));\n  customDurationInput.value = sec.toFixed(1);\n  document.getElementById('fpvPreArmShow').value = Math.round(sec * 1000);\n});"
if hook not in s:
    raise SystemExit('Could not locate FPS warning event hook')
s = s.replace(hook, replacement, 1)

apply_hook = "  await sendCommand(`set fpv_low_batt ${fpvLowBattToggle.checked ? 1 : 0}`);"
forced = "  // FPSteVe simple UI: state is permanent and warning text/behaviour is fixed.\n  fpvStateModeToggle.checked = true;\n  fpvErrorToggle.checked = true;\n  fpvReadyToggle.checked = true;\n  fpvRecordToggle.checked = true;\n  fpvFlashToggle.checked = true;\n  fpvErrorTextInput.value = '{state} {batt} {rectf}';\n  fpvReadyTextInput.value = '{state} {batt} {rectf}';\n  fpvRecordTextInput.value = '{state}';\n  const durationSec = Math.min(2.5, Math.max(0.1, parseFloat(document.getElementById('fpvCustomDurationSec').value) || 1));\n  document.getElementById('fpvPreArmShow').value = Math.round(durationSec * 1000);\n  document.getElementById('fpvPreArmInt').value = 3000;\n  fpvRecLowToggle.checked = fpvLowBattToggle.checked;\n  fpvHotToggle.checked = fpvLowBattToggle.checked;\n  fpvLowTextInput.value = 'BATT LOW';\n  fpvRecLowTextInput.value = 'REC LOW';\n  fpvHotTextInput.value = 'CAM HOT';\n  fpvLowRdyFlashToggle.checked = true;\n  fpvLowRecTextToggle.checked = true;\n  fpvRecLowReadyToggle.checked = true;\n  fpvRecLowRecordingToggle.checked = true;\n  fpvHotReadyToggle.checked = true;\n  fpvHotRecordingToggle.checked = true;\n" + apply_hook
if apply_hook not in s:
    raise SystemExit('Could not locate warning apply hook')
s = s.replace(apply_hook, forced, 1)
s = s.replace('fpvRecLowMinInput.disabled = !connectedNow || !fpvRecLowToggle.checked;', 'fpvRecLowMinInput.disabled = !connectedNow || !fpvLowBattToggle.checked;')

load_hook = '          applyState(cfg);'
load_extra = load_hook + "\n          const customDuration = document.getElementById('fpvCustomDurationSec');\n          if (customDuration) customDuration.value = (Math.min(2500, Math.max(100, Number(document.getElementById('fpvPreArmShow').value) || 1000)) / 1000).toFixed(1);"
if load_hook not in s:
    raise SystemExit('Could not locate config load hook')
s = s.replace(load_hook, load_extra, 1)

p.write_text(s, encoding='utf-8')
print('FPSteVe configurator generated')
