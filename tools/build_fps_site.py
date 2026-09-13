from pathlib import Path
import re
import runpy

runpy.run_path('tools/build_fps_pages.py', run_name='__main__')

def read(path):
    return Path(path).read_text(encoding='utf-8')

def write(path, text):
    Path(path).write_text(text, encoding='utf-8')

c = Path('web/config.html')
cs = read(c)
cs = re.sub(r'<style id="fps-demo-style">.*?<script id="fps-demo">.*?</script>', '', cs, flags=re.S)
legacy_scripts = ['fps-ui-v2.js','fps-osd-method.js','fps-osd-enhancements.js','fps-osd-adaptive.js','fps-config-state.js','fps-ui-clean.js','fps-ui-final-polish.js']
for script in legacy_scripts:
    cs = cs.replace(f'<script src="{script}"></script>', '')

# Device state is explicit in FPSteVe Edition: connecting or changing tabs must
# never silently read/overwrite the UI. Only READ SETTINGS is authoritative.
cs = cs.replace("    if (target === 'config'  && port) sendCommand('show');\n", '')
cs = cs.replace("  // Populate Easy Config fields after a brief settle time\n  setTimeout(() => sendCommand('show'), 300);\n", '')

for script in ['fps-ui-rebuild.js','fps-demo-v1.js','fps-integrated-preview.js','fps-stage3-parity.js','fps-autosync.js']:
    tag = f'<script src="{script}"></script>'
    if tag not in cs:
        if '</body>' not in cs: raise SystemExit('Configurator has no body close')
        cs = cs.replace('</body>', tag + '\n</body>', 1)
write(c, cs)

h = Path('web/index.html')
hs = read(h)
home = '<script src="fps-home-updates.js"></script>'
if home not in hs:
    if '</body>' not in hs: raise SystemExit('Homepage has no body close')
    hs = hs.replace('</body>', home + '\n</body>', 1)
write(h, hs)

final = read(c)
for script in legacy_scripts:
    if f'<script src="{script}"></script>' in final: raise SystemExit(f'Legacy configurator layer still active: {script}')
if 'href="test.html"' in final or 'osd-preview-tab' in final: raise SystemExit('Standalone OSD Preview navigation has been reintroduced')
if "setTimeout(() => sendCommand('show'), 300)" in final or "target === 'config'  && port" in final: raise SystemExit('Automatic device read reintroduced')
for required in ['<script src="fps-ui-rebuild.js"></script>','<script src="fps-demo-v1.js"></script>','<script src="fps-integrated-preview.js"></script>','<script src="fps-stage3-parity.js"></script>','<script src="fps-autosync.js"></script>','fps-theme.css']:
    if required not in final: raise SystemExit(f'Generated configurator missing: {required}')


# Pre-V1 UI regression guards
if 'id="disarmDelay"' not in final:
    raise SystemExit('Configurator lost Disarm Delay control')
ui = read('web/fps-ui-rebuild.js')
if "filter(r=>r!==delayRow)" not in ui:
    raise SystemExit('AUX section no longer excludes always-visible Disarm Delay row')
if "fps-config-read-complete" not in ui or "fpsRefreshUiVisibility" not in ui:
    raise SystemExit('Authoritative board read no longer refreshes master/child visibility')

print('FPSteVe production web site generated and validated')
