from pathlib import Path
import re
import runpy

def read(path):
    return Path(path).read_text(encoding='utf-8')

def write(path, text):
    Path(path).write_text(text, encoding='utf-8')

source_config = read('web/config.html')
if '<title>FreeCLinker — FPSteVe Edition Config</title>' not in source_config:
    runpy.run_path('tools/build_fps_pages.py', run_name='__main__')

c = Path('web/config.html')
cs = read(c)
cs = re.sub(r'<style id="fps-demo-style">.*?<script id="fps-demo">.*?</script>', '', cs, flags=re.S)
legacy_scripts = ['fps-ui-v2.js','fps-osd-method.js','fps-osd-enhancements.js','fps-osd-adaptive.js','fps-config-state.js','fps-ui-clean.js','fps-ui-final-polish.js']
for script in legacy_scripts:
    cs = cs.replace(f'<script src="{script}"></script>', '')

cs = cs.replace("    if (target === 'config'  && port) sendCommand('show');\n", '')
cs = cs.replace("  // Populate Easy Config fields after a brief settle time\n  setTimeout(() => sendCommand('show'), 300);\n", '')

for script in ['fps-ui-rebuild.js','fps-demo-v1.js','fps-integrated-preview.js','fps-stage3-parity.js','fps-v102-experimental.js','fps-camera-labels.js','fps-autosync.js','fps-version-check.js','fps-read-completion-guard.js','experimental-banner.js']:
    tag = f'<script src="{script}"></script>'
    if tag not in cs:
        if '</body>' not in cs: raise SystemExit('Configurator has no body close')
        cs = cs.replace('</body>', tag + '\n</body>', 1)
write(c, cs)

h = Path('web/index.html')
hs = read(h)
for home_script in ['fps-home-updates.js','experimental-banner.js']:
    home = f'<script src="{home_script}"></script>'
    if home not in hs:
        if '</body>' not in hs: raise SystemExit('Homepage has no body close')
        hs = hs.replace('</body>', home + '\n</body>', 1)
write(h, hs)

final = read(c)
for script in legacy_scripts:
    if f'<script src="{script}"></script>' in final: raise SystemExit(f'Legacy configurator layer still active: {script}')
if 'href="test.html"' in final or 'osd-preview-tab' in final: raise SystemExit('Standalone OSD Preview navigation has been reintroduced')
if "setTimeout(() => sendCommand('show'), 300)" in final or "target === 'config'  && port" in final: raise SystemExit('Legacy automatic device read reintroduced')
for required in ['<script src="fps-ui-rebuild.js"></script>','<script src="fps-demo-v1.js"></script>','<script src="fps-integrated-preview.js"></script>','<script src="fps-stage3-parity.js"></script>','<script src="fps-v102-experimental.js"></script>','<script src="fps-camera-labels.js"></script>','<script src="fps-autosync.js"></script>','<script src="fps-version-check.js"></script>','<script src="fps-read-completion-guard.js"></script>','<script src="experimental-banner.js"></script>','fps-theme.css']:
    if required not in final: raise SystemExit(f'Generated configurator missing: {required}')

home_final = read(h)
if '<script src="fps-home-updates.js"></script>' not in home_final:
    raise SystemExit('Experimental homepage update layer is missing')
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
for marker in ['fpsMultiCamSync','fpsArmBoostPower','fpsArmBoostMs','fpsArmedPowerV102','fpsDisBoostPower','fpsDisBoostMs','fpsPowerMultiOnly']:
    if marker not in v102:
        raise SystemExit(f'V1.0.2 experimental UI missing: {marker}')
labels = read('web/fps-camera-labels.js')
for marker in ['MAX_LABEL=7','fpsCameraLabelsInfo','cameras label','BATT LOW','supportsLabels']:
    if marker not in labels:
        raise SystemExit(f'V1.0.2 camera-label UI missing: {marker}')
autosync = read('web/fps-autosync.js')
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

print('FPSteVe experimental web site generated and validated')
