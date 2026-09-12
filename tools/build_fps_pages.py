from pathlib import Path

p = Path('web/config.html')
s = p.read_text(encoding='utf-8')


def must_replace(old, new, label):
    global s
    if old not in s:
        raise SystemExit(f'Could not locate {label}')
    s = s.replace(old, new, 1)


must_replace('<title>FreeCLinker — Config</title>', '<title>FreeCLinker — FPSteVe Edition Config</title>', 'page title')
must_replace('<span class="brand">FreeCLinker</span>', '<span class="brand">FreeCLinker — FPSteVe Edition</span>', 'brand')
must_replace('</head>', '  <link rel="stylesheet" href="fps-theme.css">\n</head>', 'head close')
must_replace('  <button class="tab"        data-tab="cli">CLI</button>\n</div>', '  <button class="tab"        data-tab="cli">CLI</button>\n  <a class="tab osd-preview-tab" href="test.html">OSD Preview</a>\n</div>', 'OSD Preview tab')

start_marker = '      <div class="cfg-field" style="flex-direction:column;align-items:flex-start;">\n        <div class="cfg-field-name">State-aware Craft Name</div>'
end_marker = '      <div class="card-footer">'
start = s.find(start_marker)
end = s.find(end_marker, start)
if start == -1 or end == -1:
    raise SystemExit('Could not locate FPSteVe state/warning section')

lines = [
    '      <div class="cfg-field" style="flex-direction:column;align-items:flex-start;">',
    '        <div style="display:none"><input type="checkbox" id="fpvStateMode" checked></div>',
    '',
    '        <div style="width:100%;"><strong>Custom Message</strong><div class="cfg-field-desc">Optional message shown periodically, e.g. CLEAN LENS. Leave blank to disable.</div></div>',
    '        <div style="display:flex;align-items:center;gap:10px;width:100%;margin-top:10px;"><label for="fpvPreArmText" style="min-width:185px;">Message</label><input class="tpl-input" type="text" id="fpvPreArmText" maxlength="16" placeholder="e.g. CLEAN LENS" spellcheck="false" style="margin-top:0;flex:1;min-width:0;"></div>',
    '        <div style="display:flex;align-items:center;justify-content:space-between;width:100%;margin-top:12px;"><div><strong>Only before first arm</strong><div class="cfg-field-desc">Stop showing the message after the first arm until the next power cycle.</div></div><label class="switch"><input type="checkbox" id="fpvPreArm"><span class="track"><span class="thumb"></span></span></label></div>',
    '        <div style="display:flex;align-items:center;gap:10px;width:100%;margin-top:12px;"><label for="fpvCustomDurationSec" style="min-width:185px;">Display duration</label><input type="number" id="fpvCustomDurationSec" min="0.1" max="2.5" step="0.1" value="1.0" style="width:90px;"><span class="num-unit">sec</span></div>',
    '        <div style="display:none"><input type="number" id="fpvPreArmShow" value="1000"><input type="number" id="fpvPreArmInt" value="3000"></div>',
    '',
    '        <div style="display:flex;align-items:center;justify-content:space-between;width:100%;margin-top:20px;"><div><strong>Camera warnings</strong><div class="cfg-field-desc">Low battery, low recording-time and camera-hot warnings.</div></div><label class="switch"><input type="checkbox" id="fpvLowBatt"><span class="track"><span class="thumb"></span></span></label></div>',
    '        <div style="display:flex;align-items:center;gap:10px;width:100%;margin-top:14px;"><label for="fpvLowPct" style="min-width:185px;">Low battery threshold</label><input type="number" id="fpvLowPct" min="0" max="100" step="1" value="10" style="width:90px;"><span class="num-unit">%</span></div>',
    '        <div style="display:flex;align-items:center;gap:10px;width:100%;margin-top:12px;"><label for="fpvRecLowMin" style="min-width:185px;">Low recording-time threshold</label><input type="number" id="fpvRecLowMin" min="0" max="999" step="1" value="5" style="width:90px;"><span class="num-unit">min</span></div>',
    '        <div class="cfg-field-desc" style="margin-top:8px;">Camera-hot uses the camera over-temperature status, so it needs no threshold.</div>',
    '',
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
replacement = hook + "\n\n// FPSteVe simplified controls.\nfpvLowBattToggle.addEventListener('change', () => {\n  fpvRecLowToggle.checked = fpvLowBattToggle.checked;\n  fpvHotToggle.checked = fpvLowBattToggle.checked;\n  updateFpvOptionState();\n});\n\nconst customDurationInput = document.getElementById('fpvCustomDurationSec');\ncustomDurationInput.addEventListener('input', () => {\n  const sec = Math.min(2.5, Math.max(0.1, parseFloat(customDurationInput.value) || 1));\n  fpvPreArmShowInput.value = Math.round(sec * 1000);\n});"
must_replace(hook, replacement, 'FPS option event hook')

must_replace("  const key = m[1];\n  const val = m[2].trim().replace(/\\s*\\(saved.*/, '');", "  const key = m[1];\n  const val = m[2].trim().replace(/\\s*\\(saved.*/, '');\n  try { const c = JSON.parse(localStorage.getItem('freeclinkerC3Config') || '{}'); c[key] = val; localStorage.setItem('freeclinkerC3Config', JSON.stringify(c)); } catch {}", 'config cache hook')
must_replace("} else if (key === 'fpv_state_mode') { fpvStateModeToggle.checked = (val === 'true');", "} else if (key === 'fpv_state_mode') { fpvStateModeToggle.checked = true;", 'fpv_state_mode parser')
must_replace("} else if (key === 'fpv_prearm_show') { fpvPreArmShowInput.value = parseInt(val) || 1000;", "} else if (key === 'fpv_prearm_show') { fpvPreArmShowInput.value = parseInt(val) || 1000; const d = document.getElementById('fpvCustomDurationSec'); if (d) d.value = (Math.min(2500, Math.max(100, fpvPreArmShowInput.value)) / 1000).toFixed(1);", 'custom duration parser')

fixed_commands = [
    ("  await sendCommand(`set fpv_state_mode ${fpvStateModeToggle.checked ? 1 : 0}`);", "  await sendCommand('set fpv_state_mode 1');", 'state mode apply'),
    ("  await sendCommand(`set fpv_error ${fpvErrorToggle.checked ? 1 : 0}`);", "  await sendCommand('set fpv_error 1');", 'error enable apply'),
    ("  await sendCommand(`set fpv_err_text ${fpvErrorTextInput.value}`);", "  await sendCommand('set fpv_err_text {state} {batt} {rectf}');", 'error text apply'),
    ("  await sendCommand(`set fpv_ready ${fpvReadyToggle.checked ? 1 : 0}`);", "  await sendCommand('set fpv_ready 1');", 'ready enable apply'),
    ("  await sendCommand(`set fpv_ready_text ${fpvReadyTextInput.value}`);", "  await sendCommand('set fpv_ready_text {state} {batt} {rectf}');", 'ready text apply'),
    ("  await sendCommand(`set fpv_record ${fpvRecordToggle.checked ? 1 : 0}`);", "  await sendCommand('set fpv_record 1');", 'record enable apply'),
    ("  await sendCommand(`set fpv_record_text ${fpvRecordTextInput.value}`);", "  await sendCommand('set fpv_record_text {state}');", 'record text apply'),
    ("  await sendCommand(`set fpv_flash ${fpvFlashToggle.checked ? 1 : 0}`);", "  await sendCommand('set fpv_flash 1');", 'record flash apply'),
    ("  await sendCommand(`set fpv_low_rdyflash ${fpvLowRdyFlashToggle.checked ? 1 : 0}`);", "  await sendCommand('set fpv_low_rdyflash 1');", 'low battery ready behavior'),
    ("  await sendCommand(`set fpv_low_rectext ${fpvLowRecTextToggle.checked ? 1 : 0}`);", "  await sendCommand('set fpv_low_rectext 1');", 'low battery record behavior'),
    ("  await sendCommand(`set fpv_low_text ${fpvLowTextInput.value}`);", "  await sendCommand('set fpv_low_text BATT LOW');", 'low battery text'),
    ("  await sendCommand(`set fpv_rect_warn ${fpvRecLowToggle.checked ? 1 : 0}`);", "  await sendCommand(`set fpv_rect_warn ${fpvLowBattToggle.checked ? 1 : 0}`);", 'low record master'),
    ("  await sendCommand(`set fpv_rect_ready ${fpvRecLowReadyToggle.checked ? 1 : 0}`);", "  await sendCommand('set fpv_rect_ready 1');", 'low record ready behavior'),
    ("  await sendCommand(`set fpv_rect_record ${fpvRecLowRecordingToggle.checked ? 1 : 0}`);", "  await sendCommand('set fpv_rect_record 1');", 'low record record behavior'),
    ("  await sendCommand(`set fpv_rect_text ${fpvRecLowTextInput.value}`);", "  await sendCommand('set fpv_rect_text REC LOW');", 'low record text'),
    ("  await sendCommand(`set fpv_hot_warn ${fpvHotToggle.checked ? 1 : 0}`);", "  await sendCommand(`set fpv_hot_warn ${fpvLowBattToggle.checked ? 1 : 0}`);", 'hot warning master'),
    ("  await sendCommand(`set fpv_hot_ready ${fpvHotReadyToggle.checked ? 1 : 0}`);", "  await sendCommand('set fpv_hot_ready 1');", 'hot ready behavior'),
    ("  await sendCommand(`set fpv_hot_record ${fpvHotRecordingToggle.checked ? 1 : 0}`);", "  await sendCommand('set fpv_hot_record 1');", 'hot record behavior'),
    ("  await sendCommand(`set fpv_hot_text ${fpvHotTextInput.value}`);", "  await sendCommand('set fpv_hot_text CAM HOT');", 'hot text'),
]
for old, new, label in fixed_commands:
    must_replace(old, new, label)

must_replace("  await sendCommand(`set fpv_prearm_show ${Math.max(100, parseInt(fpvPreArmShowInput.value) || 1000)}`);", "  const customMsgSec = Math.min(2.5, Math.max(0.1, parseFloat(document.getElementById('fpvCustomDurationSec').value) || 1));\n  await sendCommand(`set fpv_prearm_show ${Math.round(customMsgSec * 1000)}`);", 'custom duration apply')
must_replace("  await sendCommand(`set fpv_prearm_int ${Math.max(100, parseInt(fpvPreArmIntInput.value) || 3000)}`);", "  await sendCommand('set fpv_prearm_int 3000');", 'custom interval apply')
s = s.replace('fpvRecLowMinInput.disabled = !connectedNow || !fpvRecLowToggle.checked;', 'fpvRecLowMinInput.disabled = !connectedNow || !fpvLowBattToggle.checked;')

# Add a visual builder without changing the firmware's token/template format.
osd_builder = r'''
<style id="fps-osd-builder-style">
.osd-builder{margin:10px 0 16px;padding:12px;border:1px solid #4c1d95;border-radius:10px;background:rgba(76,29,149,.10)}
.osd-builder-head{display:flex;justify-content:space-between;align-items:center;gap:8px;margin-bottom:8px}.osd-builder-title{font-weight:700}.osd-builder-preview{font-family:monospace;color:#d8b4fe;font-size:12px}
.osd-token-grid{display:flex;flex-wrap:wrap;gap:6px}.osd-token{padding:5px 8px!important;font-size:11px!important;border:1px solid #6d28d9!important;background:#21152f!important;color:#eee!important;border-radius:7px!important}.osd-token:hover{border-color:#a78bfa!important}.osd-advanced{margin-top:9px;font-size:11px}.osd-advanced summary{cursor:pointer;color:#c4b5fd}
.demo-banner{display:none;background:#3b1766;color:#f2e8ff;border-bottom:1px solid #8b5cf6;padding:8px 12px;text-align:center;font-weight:700;font-size:12px;letter-spacing:.04em}.demo-on .demo-banner{display:block}.demo-on{outline:2px solid #7c3aed;outline-offset:-2px}.demo-badge{display:none;color:#d8b4fe;font-weight:800}.demo-on .demo-badge{display:inline}
</style>
<script id="fps-osd-builder-demo">
(() => {
  const tokenDefs = [
    ['Battery','{bat}'],['Recording','{rec}'],['Duration','{recdur}'],['Mode','{mode}'],['Resolution','{res}'],['FPS','{fps}'],['Stabilisation','{eis}'],['Time left','{rleft}'],['Storage','{rcap}']
  ];
  const samples = {'{bat}':'69%','{rec}':'REC','{recdur}':'00:42','{mode}':'VIDEO','{res}':'4K','{fps}':'60','{eis}':'HS','{rleft}':'45m','{rcap}':'128GB'};
  function sample(t){ let out=t || ''; for(const [k,v] of Object.entries(samples)) out=out.split(k).join(v); return out.replace(/\s+/g,' ').trim() || '(empty)'; }
  ['osd1','osd2','osd3','osd4'].forEach((id,i) => {
    const input=document.getElementById(id); if(!input) return;
    const box=document.createElement('div'); box.className='osd-builder';
    box.innerHTML=`<div class="osd-builder-head"><span class="osd-builder-title">Message ${i+1} builder</span><span class="osd-builder-preview"></span></div><div class="osd-token-grid"></div><details class="osd-advanced"><summary>Advanced / raw template</summary><div class="raw-slot"></div></details>`;
    input.parentNode.insertBefore(box,input);
    box.querySelector('.raw-slot').appendChild(input);
    const grid=box.querySelector('.osd-token-grid'), preview=box.querySelector('.osd-builder-preview');
    const refresh=()=>{preview.textContent=sample(input.value)};
    tokenDefs.forEach(([name,token])=>{ const b=document.createElement('button'); b.type='button'; b.className='osd-token'; b.textContent='+ '+name; b.title=token; b.onclick=()=>{ const spacer=input.value && !input.value.endsWith(' ')?' ':''; input.value+=spacer+token; input.dispatchEvent(new Event('input',{bubbles:true})); refresh(); }; grid.appendChild(b); });
    const clear=document.createElement('button'); clear.type='button'; clear.className='osd-token'; clear.textContent='Clear'; clear.onclick=()=>{input.value='';input.dispatchEvent(new Event('input',{bubbles:true}));refresh()}; grid.appendChild(clear);
    input.addEventListener('input',refresh); refresh();
  });

  const MODE='freeclinkerDemoMode', CFG='freeclinkerDemoConfig';
  let demo=localStorage.getItem(MODE)==='1';
  const banner=document.createElement('div'); banner.className='demo-banner'; banner.textContent='🧪 PREVIEW / DEMO MODE — NO HARDWARE CONNECTED'; document.body.insertBefore(banner,document.body.firstChild);
  const btn=document.createElement('button'); btn.type='button'; btn.id='demoModeBtn'; btn.textContent=demo?'Exit Demo':'Demo Mode'; btn.style.cssText='border-color:#8b5cf6!important;color:#d8b4fe!important';
  const connect=document.getElementById('connectBtn'); if(connect) connect.parentNode.insertBefore(btn,connect);
  const badge=document.createElement('span'); badge.className='demo-badge'; badge.textContent='DEMO'; if(connect) connect.parentNode.insertBefore(badge,btn);
  const fields=()=>[...document.querySelectorAll('#panel-config input[id],#panel-config select[id]')].filter(e=>e.id!=='caddxPass');
  const snap=()=>Object.fromEntries(fields().map(e=>[e.id,e.type==='checkbox'?e.checked:e.value]));
  function save(){if(demo){localStorage.setItem(CFG,JSON.stringify(snap()));localStorage.setItem('freeclinkerC3Config',JSON.stringify(Object.fromEntries(['osd1','osd2','osd3','osd4'].map(id=>[id,document.getElementById(id)?.value||'']))));}}
  function restore(){try{const d=JSON.parse(localStorage.getItem(CFG)||'{}'); for(const [id,v] of Object.entries(d)){const e=document.getElementById(id);if(e){if(e.type==='checkbox')e.checked=!!v;else e.value=v; e.dispatchEvent(new Event('input',{bubbles:true}));}}}catch{}}
  function demoUi(){
    document.body.classList.toggle('demo-on',demo); if(!demo)return;
    if(typeof statusText!=='undefined'){statusText.textContent='DEMO';statusText.className='connected'} if(typeof dot!=='undefined'){dot.classList.add('connected');dot.style.background='#8b5cf6'}
    if(connectBtn)connectBtn.disabled=true;if(disconnectBtn)disconnectBtn.disabled=true;if(baudSelect)baudSelect.disabled=true;
    fields().forEach(e=>e.disabled=false);
    [applyBtn,auxApplyBtn,osdApplyBtn,bf45ApplyBtn,caddxApplyBtn].filter(Boolean).forEach(e=>{e.disabled=false;e.textContent='Save Demo'});
    if(readBtn){readBtn.disabled=false;readBtn.textContent='Load Demo Settings'}
    if(camRefreshBtn)camRefreshBtn.disabled=true;if(camClearBtn)camClearBtn.disabled=true;if(cmdInput)cmdInput.disabled=true;if(sendBtn)sendBtn.disabled=true;
  }
  btn.onclick=()=>{ if(!demo){localStorage.setItem(MODE,'1');localStorage.setItem(CFG,JSON.stringify(snap()));}else{save();localStorage.setItem(MODE,'0');} location.reload(); };
  document.addEventListener('input',e=>{if(demo&&e.target.closest('#panel-config'))save()}); document.addEventListener('change',e=>{if(demo&&e.target.closest('#panel-config'))save()});
  document.addEventListener('click',e=>{if(!demo)return;if(e.target===readBtn){e.preventDefault();e.stopImmediatePropagation();restore();save()}if([applyBtn,auxApplyBtn,osdApplyBtn,bf45ApplyBtn,caddxApplyBtn].includes(e.target)){e.preventDefault();e.stopImmediatePropagation();save();}},true);
  if(demo){restore();demoUi();save();setInterval(demoUi,300)}
})();
</script>
'''
must_replace('</body>', osd_builder + '\n</body>', 'body close for builder/demo')

# Preserve demo state when following the OSD Preview link.
s = s.replace('href="test.html">OSD Preview</a>', 'href="test.html" onclick="if(localStorage.getItem(\'freeclinkerDemoMode\')===\'1\') localStorage.setItem(\'freeclinkerDemoMode\',\'1\')">OSD Preview</a>')

for marker in ['id="fpvCustomDurationSec"','placeholder="e.g. CLEAN LENS"','Only before first arm',"await sendCommand('set fpv_state_mode 1')",'set fpv_rect_warn','set fpv_hot_warn','Camera warnings','freeclinkerC3Config','class="tab osd-preview-tab"','fps-osd-builder-demo','freeclinkerDemoMode','Message 1 builder']:
    if marker not in s:
        raise SystemExit(f'Generated configurator missing expected marker: {marker}')
if '<strong>CLEAN LENS reminder</strong>' in s or '<div>Camera status (ERR / RDY / REC)</div>' in s or 'FPSteVe Camera OSD' in s or '>OSD Simulator<' in s:
    raise SystemExit('Generated configurator still contains obsolete FPSteVe controls')

p.write_text(s, encoding='utf-8')
print('FPSteVe configurator generated and validated')
