from pathlib import Path
import hashlib
import re
import runpy

def read(path):
    return Path(path).read_text(encoding='utf-8')

def write(path, text):
    Path(path).write_text(text, encoding='utf-8')

def asset_version(name):
    p = Path('web') / name
    if not p.exists():
        raise SystemExit(f'Missing web asset: {name}')
    return hashlib.sha256(p.read_bytes()).hexdigest()[:12]

def versioned_src(name):
    return f'{name}?v={asset_version(name)}'

def replace_or_add_stylesheet(html, name):
    src = versioned_src(name)
    pattern = rf'<link\s+rel="stylesheet"\s+href="{re.escape(name)}(?:\?v=[^"]+)?">'
    tag = f'<link rel="stylesheet" href="{src}">'
    if re.search(pattern, html):
        return re.sub(pattern, tag, html, count=1)
    if '</head>' not in html:
        raise SystemExit('Configurator has no head close')
    return html.replace('</head>', tag + '\n</head>', 1)

def replace_or_add_script(html, name):
    src = versioned_src(name)
    pattern = rf'<script\s+src="{re.escape(name)}(?:\?v=[^"]+)?"></script>'
    html = re.sub(pattern, '', html)
    if '</body>' not in html:
        raise SystemExit('Configurator has no body close')
    return html.replace('</body>', f'<script src="{src}"></script>\n</body>', 1)

source_config = read('web/config.html')
if '<title>FPV CamBuddy Config</title>' not in source_config:
    runpy.run_path('tools/build_fps_pages.py', run_name='__main__')

c = Path('web/config.html')
cs = read(c)
cs = re.sub(r'<style id="fps-demo-style">.*?<script id="fps-demo">.*?</script>', '', cs, flags=re.S)
legacy_scripts = ['fps-ui-v2.js','fps-osd-method.js','fps-osd-enhancements.js','fps-osd-adaptive.js','fps-config-state.js','fps-ui-clean.js','fps-ui-final-polish.js']
for script in legacy_scripts:
    cs = re.sub(rf'<script\s+src="{re.escape(script)}(?:\?v=[^"]+)?"></script>', '', cs)

cs = cs.replace("    if (target === 'config'  && port) sendCommand('show');\n", '')
cs = cs.replace("  // Populate Easy Config fields after a brief settle time\n  setTimeout(() => sendCommand('show'), 300);\n", '')

# Experimental pages are frequently tested on mobile browsers which can keep
# external JS/CSS aggressively cached. Content hashes make every changed asset
# a new URL, so the deployed page cannot silently run an older helper file.
cs = replace_or_add_stylesheet(cs, 'fps-theme.css')
cs = replace_or_add_stylesheet(cs, 'fps-osd-cleanup.css')

scripts = ['fps-ui-rebuild.js','fps-demo-v1.js','fps-integrated-preview.js','fps-stage3-parity.js','fps-v102-experimental.js','fps-multicam-setup-info.js','fps-camera-labels.js','fps-multicam-ui.js','fps-multicam-osd.js','fps-multicam-osd-visibility-fix.js','fps-collapsible-sections.js','fps-osd-capacity-guard.js','fps-autosync.js','fps-version-check.js','fps-read-completion-guard.js','experimental-banner.js']
for script in scripts:
    cs = replace_or_add_script(cs, script)

build_marker = asset_version('fps-multicam-osd.js')
cs = re.sub(r'<meta\s+name="fps-experimental-build"\s+content="[^"]*">\s*', '', cs)
if '</head>' not in cs:
    raise SystemExit('Configurator has no head close')
cs = cs.replace('</head>', f'<meta name="fps-experimental-build" content="{build_marker}">\n</head>', 1)
write(c, cs)

h = Path('web/index.html')
hs = read(h)
hs = re.sub(r'<div class="demo-caption">.*?</div>', '', hs, count=1, flags=re.S)
for home_script in ['fps-home-updates.js','experimental-banner.js']:
    hs = replace_or_add_script(hs, home_script)
write(h, hs)

final = read(c)
for script in legacy_scripts:
    if re.search(rf'<script\s+src="{re.escape(script)}(?:\?v=[^"]+)?"></script>', final):
        raise SystemExit(f'Legacy configurator layer still active: {script}')
if 'href="test.html"' in final or 'osd-preview-tab' in final:
    raise SystemExit('Standalone OSD Preview navigation has been reintroduced')
if "setTimeout(() => sendCommand('show'), 300)" in final or "target === 'config'  && port" in final:
    raise SystemExit('Legacy automatic device read reintroduced')

for stylesheet in ['fps-theme.css','fps-osd-cleanup.css']:
    expected = f'href="{versioned_src(stylesheet)}"'
    if expected not in final:
        raise SystemExit(f'Generated configurator missing cache-busted stylesheet: {stylesheet}')
for script in scripts:
    expected = f'<script src="{versioned_src(script)}"></script>'
    if expected not in final:
        raise SystemExit(f'Generated configurator missing cache-busted script: {script}')
if f'<meta name="fps-experimental-build" content="{build_marker}">' not in final:
    raise SystemExit('Generated configurator missing experimental build marker')

# Validate the exact Advanced Multi Cam simulator structure and active styling,
# not just a broad helper filename. This catches the class of deployment/UI
# mismatch that previously passed CI while an older layout was still visible.
multicam_osd = read('web/fps-multicam-osd.js')
for marker in ['fpsAdvancedMultiCamOsd','Default Multi Cam OSD','Camera identifier','fpsMultiOsdSimulator','fps-multiosd-cam-title','fps-multiosd-toggle-row','fps-multiosd-value-row','fps-multiosd-cam-state','Time remaining']:
    if marker not in multicam_osd:
        raise SystemExit(f'Advanced Multi Cam OSD helper missing exact UI marker: {marker}')
theme = read('web/fps-theme.css')
for marker in ['.fps-multiosd-cam-title','.fps-multiosd-toggle-row','.fps-multiosd-value-row','.fps-multiosd-cam-state']:
    if marker not in theme:
        raise SystemExit(f'Active theme missing Advanced Multi Cam simulator rule: {marker}')

home_final = read(h)
for home_script in ['fps-home-updates.js','experimental-banner.js']:
    if f'<script src="{versioned_src(home_script)}"></script>' not in home_final:
        raise SystemExit(f'Experimental homepage helper missing/cache stale: {home_script}')
if 'class="demo-caption"' in home_final:
    raise SystemExit('Obsolete OSD preview caption is still present on experimental homepage')
home_updates = read('web/fps-home-updates.js')
for marker in ['fpsExperimentalFeatures','V1.0.2 · EXPERIMENTAL','Multi Cam Coordinator','Multi-Camera OSD','Advanced BLE TX Power','Experimental Multi Cam Settings','Hardware Validation']:
    if marker not in home_updates:
        raise SystemExit(f'Experimental homepage feature list missing: {marker}')

if 'id="disarmDelay"' not in final:
    raise SystemExit('Configurator lost Disarm Delay control')
ui = read('web/fps-ui-rebuild.js')
if "filter(r=>r!==delayRow)" not in ui:
    raise SystemExit('AUX section no longer excludes always-visible Disarm Delay row')
if "fps-config-read-complete" not in ui or "fpsRefreshUiVisibility" not in ui:
    raise SystemExit('Authoritative board read no longer refreshes master/child visibility')

v102 = read('web/fps-v102-experimental.js')
for marker in ['fpsMultiCamSync','fpsArmBoostPower','fpsArmBoostMs','fpsArmedPowerV102','fpsDisBoostPower','fpsDisBoostMs','fpsPowerMultiOnly','fpsMultiCamMatchInfo']:
    if marker not in v102:
        raise SystemExit(f'V1.0.2 experimental UI missing: {marker}')
setup_info = read('web/fps-multicam-setup-info.js')
for marker in ['Single Cam mode','Camera Type','power-cycle the C3','Automatic Camera Detection']:
    if marker not in setup_info:
        raise SystemExit(f'Multi Cam setup guidance missing: {marker}')
labels = read('web/fps-camera-labels.js')
for marker in ['MAX_LABEL=7','fpsCameraLabelsInfo','cameras label','BATT LOW','supportsLabels']:
    if marker not in labels:
        raise SystemExit(f'V1.0.2 camera-label UI missing: {marker}')
multicam_ui = read('web/fps-multicam-ui.js')
for marker in ['fps-cam-live-badge','fpsPreviewWarnCamera','BATT LOW','REC LOW','CAM HOT','fps-camera-registry-update']:
    if marker not in multicam_ui:
        raise SystemExit(f'V1.0.2 multi-camera UI helper missing: {marker}')
multicam_osd_fix = read('web/fps-multicam-osd-visibility-fix.js')
for marker in ['fpsAdvancedMultiOsdPanel','fpsOsdMaster','fpsMultiCamSync','insertAdjacentElement']:
    if marker not in multicam_osd_fix:
        raise SystemExit(f'Advanced Multi Cam OSD placement helper missing: {marker}')
collapsible = read('web/fps-collapsible-sections.js')
for marker in ['fps-menu-collapsed-target','fps-menu-enabled','fps-menu-collapse-arrow','fps-menu-collapsed','collapsed=!toggle.checked']:
    if marker not in collapsible:
        raise SystemExit(f'Collapsible menu UI missing: {marker}')
capacity = read('web/fps-osd-capacity-guard.js')
for marker in ['LIMIT=16','fps-osd-capacity-guard','fpsCapacityBlocked',"Won't fit",'stopImmediatePropagation']:
    if marker not in capacity:
        raise SystemExit(f'OSD capacity guard missing: {marker}')
# fps-autosync.js is now a small ordered loader; the actual save/apply commands
# live in fps-autosync-core.js. Validate both so CI checks the deployed structure.
autosync_loader = read('web/fps-autosync.js')
if 'fps-autosync-core.js' not in autosync_loader:
    raise SystemExit('Autosync loader no longer loads fps-autosync-core.js')
autosync = read('web/fps-autosync-core.js')
for command in ['set multi_cam','set arm_boost_dbm','set arm_boost_ms','set dis_boost_dbm','set dis_boost_ms','set power_multi_only']:
    if command not in autosync:
        raise SystemExit(f'V1.0.2 save/apply path missing: {command}')
read_guard = read('web/fps-read-completion-guard.js')
for marker in ['fps-config-read-complete','fpsRequestBoardRead','sendCommand(\'show\')']:
    if marker not in read_guard:
        raise SystemExit(f'Configurator read guard missing: {marker}')
flash = read('web/flash.html')
if 'firmware/experimental' not in flash or 'V1.0.2 EXPERIMENTAL' not in flash:
    raise SystemExit('Experimental flasher is not pinned to the validated experimental firmware path')

print(f'FPSteVe experimental web site generated and validated (build {build_marker})')
