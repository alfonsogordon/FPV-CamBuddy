from pathlib import Path
import re
import runpy

def read(path):
    return Path(path).read_text(encoding='utf-8')

def write(path, text):
    Path(path).write_text(text, encoding='utf-8')

# build_fps_pages.py is a one-way source-to-FPSteVe transformer. The repository
# currently carries the already-generated FPSteVe configurator, so do not run
# that transformer a second time. Running it twice used to fail at the original
# page-title marker before validation could even begin.
source_config = read('web/config.html')
if '<title>FPV CamBuddy · by FPSteVe Config</title>' not in source_config:
    runpy.run_path('tools/build_fps_pages.py', run_name='__main__')

c = Path('web/config.html')
cs = read(c)
cs = re.sub(r'<style id="fps-demo-style">.*?<script id="fps-demo">.*?</script>', '', cs, flags=re.S)
legacy_scripts = ['fps-ui-v2.js','fps-osd-method.js','fps-osd-enhancements.js','fps-osd-adaptive.js','fps-config-state.js','fps-ui-clean.js','fps-ui-final-polish.js']
for script in legacy_scripts:
    cs = cs.replace(f'<script src="{script}"></script>', '')

# Device state is explicit in FPV CamBuddy: connecting or changing tabs must
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
# Remove the obsolete explanatory line below the homepage OSD preview from the
# generated HTML itself rather than relying on JavaScript to hide it at runtime.
hs = re.sub(r'<div class="demo-caption">.*?</div>', '', hs, count=1, flags=re.S)
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

home_final = read(h)
if 'class="demo-caption"' in home_final:
    raise SystemExit('Obsolete OSD preview caption is still present on homepage')

# Pre-V1 UI regression guards
if 'id="disarmDelay"' not in final:
    raise SystemExit('Configurator lost Disarm Delay control')
ui = read('web/fps-ui-rebuild.js')
if "filter(r=>r!==delayRow)" not in ui:
    raise SystemExit('AUX section no longer excludes always-visible Disarm Delay row')
if "fps-config-read-complete" not in ui or "fpsRefreshUiVisibility" not in ui:
    raise SystemExit('Authoritative board read no longer refreshes master/child visibility')

print('FPSteVe production web site generated and validated')
