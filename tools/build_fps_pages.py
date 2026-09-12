from pathlib import Path

p = Path('web/config.html')
s = p.read_text(encoding='utf-8')
if '<title>FreeCLinker — Config</title>' in s:
    s = s.replace('<title>FreeCLinker — Config</title>', '<title>FreeCLinker — FPSteVe Edition Config</title>', 1)
if '<span class="brand">FreeCLinker</span>' in s:
    s = s.replace('<span class="brand">FreeCLinker</span>', '<span class="brand">FreeCLinker — FPSteVe Edition</span>', 1)
if 'fps-theme.css' not in s:
    s = s.replace('</head>', '  <link rel="stylesheet" href="fps-theme.css">\n</head>', 1)
if 'osd-preview-tab' not in s:
    s = s.replace('  <button class="tab"        data-tab="cli">CLI</button>\n</div>', '  <button class="tab"        data-tab="cli">CLI</button>\n  <button class="tab osd-preview-tab" type="button" onclick="window.location.href=\'test.html\'">OSD Preview</button>\n</div>', 1)
p.write_text(s, encoding='utf-8')

t = Path('web/test.html')
h = t.read_text(encoding='utf-8')
patch = r'''
<script id="fps-bench-ux-fix">
(() => {
  const ids = ['arm','power','read','battery','time','hot','media'];
  let alertEl = document.getElementById('cameraAlert');
  if (!alertEl) {
    alertEl = document.createElement('div');
    alertEl.id = 'cameraAlert';
    alertEl.style.cssText = 'font-weight:700;font-size:12px;letter-spacing:.04em;margin:0 0 10px;color:#ff7b86';
    const telemetry = document.querySelector('.livehead');
    if (telemetry) telemetry.insertAdjacentElement('afterend', alertEl);
  }

  // Disconnected demo represents the cropped left-hand portion of the OSD,
  // so all four current-Betaflight Custom Messages are stacked on the left.
  const screen = document.querySelector('.screen');
  const demo = document.createElement('div');
  demo.id = 'bf-latest-demo';
  demo.innerHTML = '<div>CAM:69%</div><div>●REC 00:42</div><div>VIDEO 4K/60 HS</div><div>45m 128GB</div>';
  demo.style.cssText = 'position:absolute;top:10%;left:4%;pointer-events:none;font:700 18px/1.45 Courier New,monospace;color:#fff;text-align:left;text-shadow:2px 2px 2px #000,-1px -1px 2px #000';
  if (screen) screen.appendChild(demo);

  function refreshBenchUi() {
    const board = !!live;
    const camera = board && !!liveData.connected;
    ids.forEach(id => { const el = document.getElementById(id); if (el) el.disabled = !board; });
    demo.style.display = board ? 'none' : 'block';
    const craft = document.getElementById('craft');
    const pilot = document.getElementById('pilot');
    if (craft) craft.style.visibility = board ? 'visible' : 'hidden';
    if (pilot) pilot.style.visibility = board ? 'visible' : 'hidden';
    if (!board) {
      alertEl.textContent = 'NO C3 CONNECTED';
      alertEl.style.color = '#ff7b86';
    } else if (!camera) {
      alertEl.textContent = 'NO CAMERA CONNECTED';
      alertEl.style.color = '#ff7b86';
    } else {
      alertEl.textContent = 'CAMERA CONNECTED';
      alertEl.style.color = '#65d69b';
    }
  }

  document.getElementById('arm').onclick = async () => {
    if (!live) return;
    const cameraConnected = !!liveData.connected;
    armed = !armed;
    if (armed) armedOnce = true;
    if (!cameraConnected) rec = armed;
    try {
      await send(armed ? 'sim arm 1' : 'sim arm 0');
      document.getElementById('serial').textContent = armed
        ? (cameraConnected ? 'Bench ARM sent — camera should start recording.' : 'Bench ARM sent — expected recording state previewed; no camera connected.')
        : (cameraConnected ? 'Bench DISARM sent — configured stop delay applies.' : 'Bench DISARM sent — preview returned to idle; no camera connected.');
      setTimeout(poll, 250);
    } catch (e) {
      armed = !armed;
      if (!cameraConnected) rec = armed;
      document.getElementById('serial').textContent = e.message;
    }
  };

  setInterval(refreshBenchUi, 250);
  refreshBenchUi();
})();
</script>
'''
if 'id="fps-bench-ux-fix"' not in h:
    h = h.replace('</body>', patch + '</body>', 1)
t.write_text(h, encoding='utf-8')
print('FPSteVe Pages generated and OSD bench UX patched')
