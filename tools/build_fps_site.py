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

# Bake the Quick Start + wiring into the generated homepage itself. This used to
# be injected only by fps-home-updates.js, which meant a script/cache issue could
# leave the live page with no wiring information at all.
quick_wiring = '''<section class="fps-start-wiring"><div class="section-head"><h2>Quick Start + Wiring</h2><p>The shortest route from a fresh ESP32-C3 Super Mini to a working FPV CamBuddy.</p></div><div class="wide-grid"><div class="panel fps-start-card"><h3>⚡ Quick Start</h3><ol><li><strong>Flash</strong> FPV CamBuddy in the browser.</li><li><strong>Power-cycle</strong> the C3 once after flashing.</li><li><strong>Wire</strong> the C3 to one spare FC UART using the table opposite.</li><li>In Betaflight <strong>Ports</strong>, enable <strong>MSP</strong> on that UART at 115200.</li><li>Open <strong>Easy Config</strong>, connect, read the board and choose your camera/OSD options.</li><li>In Betaflight <strong>OSD</strong>, place Pilot Name for BF 4.5 or the chosen Custom Messages for BF 2026.6+.</li><li>Bench-test with props removed: camera reaches <strong>RDY</strong>, arm starts <strong>REC</strong>, disarm stops after the configured delay.</li></ol><div class="fps-start-links"><a class="btn primary" href="flash.html">1 · Flash</a><a class="btn" href="config.html">2 · Easy Config</a><a class="btn" href="https://github.com/alfonsogordon/FPV-CamBuddy/blob/main/QUICKSTART.md" target="_blank">Full Quick Start</a></div></div><div class="panel fps-start-card"><h3>🔌 ESP32-C3 Super Mini wiring</h3><table><thead><tr><th>C3</th><th>Flight controller</th></tr></thead><tbody><tr><td><strong>GPIO4 / TX</strong></td><td>UART <strong>RX</strong></td></tr><tr><td><strong>GPIO5 / RX</strong></td><td>UART <strong>TX</strong></td></tr><tr><td><strong>GND</strong></td><td>GND</td></tr><tr><td><strong>5V</strong></td><td>Suitable 5V supply</td></tr></tbody></table><p class="fps-wire-note"><strong>TX → RX, RX → TX.</strong> The C3 and FC must share ground. Board layouts vary, so follow the labels on your actual C3 Super Mini.</p></div></div></section>'''
quick_pattern = r'<section><div class="panel quick">.*?</div></section>'
if re.search(quick_pattern, hs, flags=re.S):
    hs = re.sub(quick_pattern, quick_wiring, hs, count=1, flags=re.S)
elif 'class="fps-start-wiring"' not in hs:
    help_marker = '<section><div class="panel help">'
    if help_marker not in hs: raise SystemExit('Homepage has no Quick Start or Help insertion point')
    hs = hs.replace(help_marker, quick_wiring + '\n' + help_marker, 1)

home_style = '''<style id="fps-start-wiring-style">.fps-start-card{padding:22px}.fps-start-card h3{font-size:15px;color:#eee7f8;margin-bottom:12px}.fps-start-card ol{padding-left:22px;color:#a397b5;font-size:11px}.fps-start-card li{margin:6px 0}.fps-start-links{display:flex;gap:8px;flex-wrap:wrap;margin-top:18px}.fps-start-card table{width:100%;border-collapse:collapse;font-size:12px;margin-top:5px}.fps-start-card th,.fps-start-card td{text-align:left;padding:9px 10px;border-bottom:1px solid #34294a}.fps-start-card th{color:#c4b5fd}.fps-wire-note{font-size:11px;color:#a397b5;margin-top:14px}</style>'''
if 'id="fps-start-wiring-style"' not in hs:
    hs = hs.replace('</head>', home_style + '\n</head>', 1)

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
if 'class="fps-start-wiring"' not in home_final or 'GPIO4 / TX' not in home_final or 'GPIO5 / RX' not in home_final:
    raise SystemExit('Homepage Quick Start + wiring section is missing')

# Pre-V1 UI regression guards
if 'id="disarmDelay"' not in final:
    raise SystemExit('Configurator lost Disarm Delay control')
ui = read('web/fps-ui-rebuild.js')
if "fps-disarm-delay-row" not in ui:
    raise SystemExit('AUX section lost always-visible stop-delay handling')
if "section('Camera Mode Switch'" in ui:
    raise SystemExit('Legacy Camera Mode Switch is being rebuilt into Easy Config')
if "fps-config-read-complete" not in ui or "fpsRefreshUiVisibility" not in ui:
    raise SystemExit('Authoritative board read no longer refreshes master/child visibility')

print('FPSteVe production web site generated and validated')