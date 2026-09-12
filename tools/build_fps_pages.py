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

config_demo_patch = r'''
<script id="fps-config-demo-mode">
(() => {
  const MODE_KEY = 'freeclinkerDemoMode';
  const CFG_KEY = 'freeclinkerDemoConfig';
  const PREVIEW_KEY = 'freeclinkerDemoPreviewConfig';
  let demoMode = localStorage.getItem(MODE_KEY) === '1';

  const header = document.querySelector('header');
  const banner = document.createElement('div');
  banner.id = 'demoModeBanner';
  banner.textContent = '🧪 PREVIEW / DEMO MODE — NO HARDWARE CONNECTED';
  banner.style.cssText = 'display:none;background:#3b1766;color:#f2e8ff;border-bottom:1px solid #8b5cf6;padding:8px 12px;text-align:center;font-weight:700;font-size:12px;letter-spacing:.04em;flex-shrink:0';
  document.body.insertBefore(banner, header);

  const demoBtn = document.createElement('button');
  demoBtn.id = 'demoModeBtn';
  demoBtn.type = 'button';
  demoBtn.textContent = demoMode ? 'Exit Demo' : 'Demo Mode';
  demoBtn.style.cssText = 'border-color:#8b5cf6!important;color:#d8b4fe!important';
  header.insertBefore(demoBtn, document.getElementById('connectBtn'));

  function snapshot() {
    const out = {};
    document.querySelectorAll('#panel-config input[id], #panel-config select[id]').forEach(el => {
      if (el.id === 'caddxPass') return;
      out[el.id] = el.type === 'checkbox' ? !!el.checked : el.value;
    });
    return out;
  }

  function restore(data) {
    if (!data) return;
    Object.entries(data).forEach(([id, value]) => {
      const el = document.getElementById(id);
      if (!el) return;
      if (el.type === 'checkbox') el.checked = !!value;
      else el.value = value;
    });
    try { updateCameraTypeState(); } catch {}
    try { updateDelayEquiv(); } catch {}
  }

  function buildPreviewConfig(data) {
    const cameraNames = ['DJI','GoPro','Caddx','Sony','Blackmagic','Insta360'];
    return {
      camera_type: cameraNames[parseInt(data.cameraType || '1')] || 'GoPro',
      camera_match: data.cameraMatch || '0',
      wake_guard: data.wakeGuard ? 'true' : 'false',
      debug_ble: data.debugBle ? 'true' : 'false',
      low_power: data.lowPower ? 'true' : 'false',
      wifi_ap_enabled: data.wifiApEnabled ? 'true' : 'false',
      wifi_ap_delay: data.wifiApDelay || '30',
      stop_on_disarm: data.stopOnDisarm ? 'true' : 'false',
      disarm_delay: data.disarmDelay || '0',
      aux_channel: data.auxChannel === '0' ? 'disabled' : `AUX${data.auxChannel || '1'}`,
      aux_mode: data.auxMode || '0',
      osd1: data.osd1 || 'BAT:{bat}',
      osd2: data.osd2 || '●{rec} {recdur}',
      osd3: data.osd3 || '{mode} {res}/{fps} {eis}',
      osd4: data.osd4 || '{rleft} {rcap}',
      bf45_compat: data.bf45Compat ? 'true' : 'false',
      pilot_en: data.pilotEn ? 'true' : 'false',
      pilot_tpl: data.pilotTpl || '',
      craft_en: data.craftEn ? 'true' : 'false',
      craft_tpl: data.craftTpl || '{state}',
      fpv_state_mode: data.fpvStateMode ? 'true' : 'false',
      fpv_error: data.fpvError ? 'true' : 'false',
      fpv_err_text: data.fpvErrorText || '{state} {batt} {rectf}',
      fpv_ready: data.fpvReady ? 'true' : 'false',
      fpv_ready_text: data.fpvReadyText || '{state} {batt} {rectf}',
      fpv_record: data.fpvRecord ? 'true' : 'false',
      fpv_record_text: data.fpvRecordText || '{state}',
      fpv_flash: data.fpvFlash ? 'true' : 'false',
      fpv_prearm: data.fpvPreArm ? 'true' : 'false',
      fpv_prearm_text: data.fpvPreArmText || '',
      fpv_prearm_show: data.fpvPreArmShow || '1000',
      fpv_prearm_int: data.fpvPreArmInt || '3000',
      fpv_low_batt: data.fpvLowBatt ? 'true' : 'false',
      fpv_low_pct: data.fpvLowPct || '10',
      fpv_low_text: data.fpvLowText || 'BATT LOW',
      fpv_low_rdyflash: data.fpvLowRdyFlash ? 'true' : 'false',
      fpv_low_rectext: data.fpvLowRecText ? 'true' : 'false',
      fpv_rect_warn: data.fpvRecLow ? 'true' : 'false',
      fpv_rect_min: data.fpvRecLowMin || '5',
      fpv_rect_text: data.fpvRecLowText || 'REC LOW',
      fpv_rect_ready: data.fpvRecLowReady ? 'true' : 'false',
      fpv_rect_record: data.fpvRecLowRecording ? 'true' : 'false',
      fpv_hot_warn: data.fpvHot ? 'true' : 'false',
      fpv_hot_text: data.fpvHotText || 'CAM HOT',
      fpv_hot_ready: data.fpvHotReady ? 'true' : 'false',
      fpv_hot_record: data.fpvHotRecording ? 'true' : 'false'
    };
  }

  function saveDemo() {
    if (!demoMode) return;
    const data = snapshot();
    localStorage.setItem(CFG_KEY, JSON.stringify(data));
    localStorage.setItem(PREVIEW_KEY, JSON.stringify(buildPreviewConfig(data)));
  }

  function setDemoUi() {
    banner.style.display = demoMode ? 'block' : 'none';
    document.body.style.outline = demoMode ? '2px solid #7c3aed' : '';
    document.body.style.outlineOffset = demoMode ? '-2px' : '';
    if (!demoMode) return;

    statusText.textContent = 'DEMO';
    statusText.classList.add('connected');
    dot.classList.add('connected');
    dot.style.background = '#8b5cf6';
    connectBtn.disabled = true;
    disconnectBtn.disabled = true;
    baudSelect.disabled = true;

    document.querySelectorAll('#panel-config input, #panel-config select, #panel-config button').forEach(el => {
      if (el.id !== 'caddxPass') el.disabled = false;
    });
    readBtn.textContent = 'Load Demo Settings';
    applyBtn.textContent = 'Save Demo';
    auxApplyBtn.textContent = 'Save Demo';
    osdApplyBtn.textContent = 'Save Demo';
    bf45ApplyBtn.textContent = 'Save Demo';
    if (caddxApplyBtn) caddxApplyBtn.textContent = 'Save Demo';

    camRefreshBtn.disabled = true;
    camClearBtn.disabled = true;
    cmdInput.disabled = true;
    sendBtn.disabled = true;
  }

  demoBtn.addEventListener('click', () => {
    if (!demoMode) {
      localStorage.setItem(MODE_KEY, '1');
      if (!localStorage.getItem(CFG_KEY)) {
        const initial = snapshot();
        localStorage.setItem(CFG_KEY, JSON.stringify(initial));
        localStorage.setItem(PREVIEW_KEY, JSON.stringify(buildPreviewConfig(initial)));
      }
    } else {
      saveDemo();
      localStorage.setItem(MODE_KEY, '0');
    }
    location.reload();
  });

  document.addEventListener('input', e => {
    if (demoMode && e.target.closest('#panel-config')) saveDemo();
  });
  document.addEventListener('change', e => {
    if (demoMode && e.target.closest('#panel-config')) saveDemo();
  });
  document.addEventListener('click', e => {
    if (!demoMode) return;
    if (e.target === readBtn) {
      e.preventDefault();
      try { restore(JSON.parse(localStorage.getItem(CFG_KEY) || '{}')); } catch {}
      saveDemo();
    }
    if ([applyBtn, auxApplyBtn, osdApplyBtn, bf45ApplyBtn, caddxApplyBtn].includes(e.target)) {
      setTimeout(saveDemo, 0);
    }
  }, true);

  if (demoMode) {
    try { restore(JSON.parse(localStorage.getItem(CFG_KEY) || '{}')); } catch {}
    setDemoUi();
    saveDemo();
    setInterval(setDemoUi, 400);
  }
})();
</script>
'''
if 'id="fps-config-demo-mode"' not in s:
    s = s.replace('</body>', config_demo_patch + '</body>', 1)
p.write_text(s, encoding='utf-8')

t = Path('web/test.html')
h = t.read_text(encoding='utf-8')
patch = r'''
<script id="fps-bench-ux-fix">
(() => {
  const MODE_KEY = 'freeclinkerDemoMode';
  const PREVIEW_KEY = 'freeclinkerDemoPreviewConfig';
  let demoMode = localStorage.getItem(MODE_KEY) === '1';
  let demoCfg = {};
  try { demoCfg = JSON.parse(localStorage.getItem(PREVIEW_KEY) || '{}'); } catch {}
  if (demoMode && Object.keys(demoCfg).length) {
    c = {...c, ...demoCfg};
    try { summary(); } catch {}
  }

  const header = document.querySelector('header');
  const banner = document.createElement('div');
  banner.textContent = '🧪 PREVIEW / DEMO MODE — NO HARDWARE CONNECTED';
  banner.style.cssText = 'display:none;background:#3b1766;color:#f2e8ff;border-bottom:1px solid #8b5cf6;padding:8px 12px;text-align:center;font-weight:700;font-size:12px;letter-spacing:.04em;flex-shrink:0';
  document.body.insertBefore(banner, header);

  const demoBtn = document.createElement('button');
  demoBtn.type = 'button';
  demoBtn.textContent = demoMode ? 'Exit Demo' : 'Demo Mode';
  demoBtn.style.cssText = 'border-color:#8b5cf6;color:#d8b4fe';
  header.insertBefore(demoBtn, document.getElementById('connect'));
  demoBtn.onclick = () => {
    localStorage.setItem(MODE_KEY, demoMode ? '0' : '1');
    location.reload();
  };

  const ids = ['arm','power','read','battery','time','hot','media'];
  let alertEl = document.getElementById('cameraAlert');
  if (!alertEl) {
    alertEl = document.createElement('div');
    alertEl.id = 'cameraAlert';
    alertEl.style.cssText = 'font-weight:700;font-size:12px;letter-spacing:.04em;margin:0 0 10px;color:#ff7b86';
    const telemetry = document.querySelector('.livehead');
    if (telemetry) telemetry.insertAdjacentElement('afterend', alertEl);
  }

  const screen = document.querySelector('.screen');
  const demo = document.createElement('div');
  demo.id = 'bf-latest-demo';
  demo.style.cssText = 'position:absolute;top:50%;left:4%;transform:translateY(-50%);pointer-events:none;font:700 18px/1.45 Courier New,monospace;color:#fff;text-align:left;text-shadow:2px 2px 2px #000,-1px -1px 2px #000';
  if (screen) screen.appendChild(demo);

  function demoExpand(tpl, recording) {
    const battery = +document.getElementById('battery').value || 69;
    const left = +document.getElementById('time').value || 45;
    const dur = recording ? '00:42' : '00:00';
    return String(tpl || '')
      .replaceAll('{bat}', `${battery}%`)
      .replaceAll('{batn}', `${battery}`)
      .replaceAll('{rec}', recording ? 'REC' : 'RDY')
      .replaceAll('{recdur}', dur)
      .replaceAll('{state}', recording ? 'REC' : 'RDY')
      .replaceAll('{rect}', `${left}`)
      .replaceAll('{mode}', 'VIDEO')
      .replaceAll('{res}', '4K')
      .replaceAll('{fps}', '60')
      .replaceAll('{eis}', 'HS')
      .replaceAll('{rleft}', `${left}m`)
      .replaceAll('{rcap}', '128GB')
      .slice(0, 32);
  }

  function updateFourMessageDemo() {
    const recording = demoMode ? rec : true;
    const defaults = ['BAT:{bat}','●{rec} {recdur}','{mode} {res}/{fps} {eis}','{rleft} {rcap}'];
    const vals = [c.osd1 || defaults[0], c.osd2 || defaults[1], c.osd3 || defaults[2], c.osd4 || defaults[3]];
    demo.innerHTML = vals.map(v => `<div>${demoExpand(v, recording)}</div>`).join('');
  }

  function refreshBenchUi() {
    const board = demoMode || !!live;
    const camera = demoMode || (live && !!liveData.connected);
    const bf45 = demoMode && String(c.bf45_compat) === 'true';
    ids.forEach(id => { const el = document.getElementById(id); if (el) el.disabled = !board; });
    updateFourMessageDemo();
    demo.style.display = (!board || (demoMode && !bf45)) ? 'block' : 'none';
    const craft = document.getElementById('craft');
    const pilot = document.getElementById('pilot');
    if (craft) craft.style.visibility = (board && (!demoMode || bf45)) ? 'visible' : 'hidden';
    if (pilot) pilot.style.visibility = (board && (!demoMode || bf45)) ? 'visible' : 'hidden';

    banner.style.display = demoMode ? 'block' : 'none';
    document.body.style.outline = demoMode ? '2px solid #7c3aed' : '';
    document.body.style.outlineOffset = demoMode ? '-2px' : '';

    if (demoMode) {
      alertEl.textContent = 'DEMO CAMERA CONNECTED';
      alertEl.style.color = '#c084fc';
      document.getElementById('source').textContent = 'DEMO DATA';
      document.getElementById('source').className = 'liveflag';
      document.getElementById('source').style.color = '#c084fc';
      document.getElementById('pill').textContent = 'DEMO';
      document.getElementById('pill').style.color = '#d8b4fe';
      document.getElementById('statusText').textContent = 'DEMO';
      document.getElementById('dot').classList.add('connected');
      document.getElementById('dot').style.background = '#8b5cf6';
      document.getElementById('connect').disabled = true;
      document.getElementById('disconnect').disabled = true;
      setText('lcam', c.camera_type || 'GoPro');
      setText('lconn', 'SIMULATED');
      setText('lbat', `${+document.getElementById('battery').value || 69}%`);
      setText('lrec', rec ? 'YES' : 'NO');
      setText('lrtime', rec ? '42s' : '0s');
      setText('lremain', `${+document.getElementById('time').value || 45}m`);
      setText('lcap', '128 GB');
      setText('ltemp', document.getElementById('hot').checked ? 'WARN' : 'OK');
      setText('lmedia', document.getElementById('media').checked ? 'READY' : 'ERROR');
      setText('lage', 'SIMULATED');
    } else if (!board) {
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
    if (demoMode) {
      armed = !armed;
      if (armed) armedOnce = true;
      rec = armed;
      document.getElementById('serial').textContent = armed
        ? 'DEMO ARM — simulated camera recording started.'
        : 'DEMO DISARM — simulated camera returned to idle.';
      return;
    }
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
print('FPSteVe Pages generated with full-board Demo Mode and OSD bench UX')
