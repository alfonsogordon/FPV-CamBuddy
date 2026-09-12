from pathlib import Path
import re
import runpy

# First apply the existing source-to-config transform. This keeps the tested
# firmware/configurator field wiring intact while we migrate away from older
# one-off UI layers.
runpy.run_path('tools/build_fps_pages.py', run_name='__main__')


def read(path):
    return Path(path).read_text(encoding='utf-8')


def write(path, text):
    Path(path).write_text(text, encoding='utf-8')


# Canonicalise the generated configurator so production always has exactly one
# UI owner and one Demo Mode owner.
c = Path('web/config.html')
cs = read(c)
cs = re.sub(r'<style id="fps-demo-style">.*?<script id="fps-demo">.*?</script>', '', cs, flags=re.S)
legacy_scripts = [
    'fps-ui-v2.js',
    'fps-osd-method.js',
    'fps-osd-enhancements.js',
    'fps-osd-adaptive.js',
    'fps-config-state.js',
    'fps-ui-clean.js',
    'fps-ui-final-polish.js',
]
for script in legacy_scripts:
    cs = cs.replace(f'<script src="{script}"></script>', '')
for script in ['fps-ui-rebuild.js', 'fps-demo-v1.js']:
    tag = f'<script src="{script}"></script>'
    if tag not in cs:
        if '</body>' not in cs:
            raise SystemExit('Configurator has no body close')
        cs = cs.replace('</body>', tag + '\n</body>', 1)
write(c, cs)

# OSD Preview helper.
p = Path('web/test.html')
ps = read(p)
preview = '<script src="fps-preview-demo.js"></script>'
if preview not in ps:
    if '</body>' not in ps:
        raise SystemExit('OSD Preview has no body close')
    ps = ps.replace('</body>', preview + '\n</body>', 1)
write(p, ps)

# Landing-page FPSteVe updates.
h = Path('web/index.html')
hs = read(h)
home = '<script src="fps-home-updates.js"></script>'
if home not in hs:
    if '</body>' not in hs:
        raise SystemExit('Homepage has no body close')
    hs = hs.replace('</body>', home + '\n</body>', 1)
write(h, hs)

# Fail loudly if the production output drifts back toward one of the retired
# helper layers.
final = read(c)
for script in legacy_scripts:
    if f'<script src="{script}"></script>' in final:
        raise SystemExit(f'Legacy configurator layer still active: {script}')
for required in [
    '<script src="fps-ui-rebuild.js"></script>',
    '<script src="fps-demo-v1.js"></script>',
    'fps-theme.css',
    'OSD Preview',
]:
    if required not in final:
        raise SystemExit(f'Generated configurator missing: {required}')

print('FPSteVe production web site generated and validated')
