from pathlib import Path


def replace_once(text, old, new, label):
    if old not in text:
        raise SystemExit(f'{label}: expected source marker not found')
    return text.replace(old, new, 1)

# The main generator owns the broad FPSteVe configurator conversion. This file
# performs only the small UI/state enhancements that are not yet in that source.
p = Path('web/config.html')
s = p.read_text(encoding='utf-8')

s = replace_once(s,
"const tokenDefs = [['Battery','{bat}'],['Recording','{rec}'],['Duration','{recdur}'],['Mode','{mode}'],['Resolution','{res}'],['FPS','{fps}'],['Stabilisation','{eis}'],['Time left','{rleft}'],['Storage','{rcap}']];",
"const tokenDefs = [['Status','{state}'],['Battery','{batt}'],['Recording Duration','{recdur}'],['Mode','{mode}'],['Resolution','{res}'],['FPS','{fps}'],['Stabilisation','{eis}'],['Time Remaining','{rectf}'],['Storage Remaining','{rcap}']];",
'OSD element vocabulary')
s = replace_once(s,
"const samples = {'{bat}':'69%','{rec}':'REC','{recdur}':'00:42','{mode}':'VIDEO','{res}':'4K','{fps}':'60','{eis}':'HS','{rleft}':'45m','{rcap}':'128GB'};",
"const samples = {'{state}':'RDY','{batt}':'B:69','{recdur}':'00:42','{mode}':'VIDEO','{res}':'4K','{fps}':'60','{eis}':'HS','{rectf}':'T:45m','{rcap}':'128GB'};",
'OSD preview samples')

old_builder = """    tokenDefs.forEach(([name,token])=>{ const b=document.createElement('button'); b.type='button'; b.className='osd-token'; b.textContent='+ '+name; b.title=token; b.onclick=()=>{ const spacer=input.value && !input.value.endsWith(' ')?' ':''; input.value+=spacer+token; input.dispatchEvent(new Event('input',{bubbles:true})); refresh(); }; grid.appendChild(b); });
    const clear=document.createElement('button'); clear.type='button'; clear.className='osd-token'; clear.textContent='Clear'; clear.onclick=()=>{input.value='';input.dispatchEvent(new Event('input',{bubbles:true}));refresh()}; grid.appendChild(clear);"""
new_builder = """    const picker=document.createElement('details'); picker.className='osd-multi';
    const summary=document.createElement('summary'); summary.textContent='Select OSD elements'; picker.appendChild(summary);
    const menu=document.createElement('div'); menu.className='osd-multi-menu'; picker.appendChild(menu); grid.appendChild(picker);
    const chips=document.createElement('div'); chips.className='osd-chip-row'; grid.appendChild(chips);
    const tokenRe=/(\\{(?:state|batt|recdur|mode|res|fps|eis|rectf|rcap)\\})/g;
    function tokensInOrder(){return String(input.value||'').match(tokenRe)||[]}
    function removeToken(token){const parts=String(input.value||'').split(tokenRe);let removed=false;input.value=parts.filter(part=>{if(!removed&&part===token){removed=true;return false}return true}).join('').replace(/ {2,}/g,' ').trim()}
    function syncPicker(){const active=tokensInOrder();menu.querySelectorAll('input[type=checkbox]').forEach(cb=>cb.checked=active.includes(cb.value));chips.innerHTML='';active.forEach(token=>{const def=tokenDefs.find(x=>x[1]===token);const chip=document.createElement('span');chip.className='osd-chip';chip.textContent=def?def[0]:token;chips.appendChild(chip)});summary.textContent=active.length?`OSD elements (${active.length})`:'Select OSD elements';document.dispatchEvent(new CustomEvent('fps-status-selection-changed'))}
    tokenDefs.forEach(([name,token])=>{const label=document.createElement('label');label.className='osd-check-item';const cb=document.createElement('input');cb.type='checkbox';cb.value=token;const text=document.createElement('span');text.textContent=name;label.append(cb,text);menu.appendChild(label);cb.onchange=()=>{if(cb.checked){if(!tokensInOrder().includes(token)){const spacer=input.value&&!input.value.endsWith(' ')?' ':'';input.value+=spacer+token}}else removeToken(token);input.dispatchEvent(new Event('input',{bubbles:true}));refresh();syncPicker()}});
    input.addEventListener('input',syncPicker);syncPicker();"""
s = replace_once(s, old_builder, new_builder, 'OSD checkbox builder')

s = replace_once(s, ".osd-token-grid{display:flex;flex-wrap:wrap;gap:6px}", ".osd-token-grid{display:flex;flex-wrap:wrap;gap:7px;align-items:center}.osd-multi{position:relative;min-width:210px}.osd-multi>summary{list-style:none;cursor:pointer;background:#171122;color:#eee;border:1px solid #6d28d9;border-radius:4px;padding:7px 9px}.osd-multi>summary::-webkit-details-marker{display:none}.osd-multi>summary:after{content:' ▾';float:right;color:#c4b5fd}.osd-multi[open]>summary:after{content:' ▴'}.osd-multi-menu{position:absolute;z-index:30;top:calc(100% + 4px);left:0;min-width:250px;padding:6px;background:#100b18;border:1px solid #6d28d9;border-radius:6px;box-shadow:0 8px 24px #0009}.osd-check-item{display:flex;align-items:center;gap:8px;padding:7px 8px;border-radius:4px;cursor:pointer;color:#eee}.osd-check-item:hover{background:#2a1744}.osd-check-item input{accent-color:#8b5cf6}.osd-chip-row{display:flex;flex-wrap:wrap;gap:5px;width:100%}.osd-chip{padding:3px 7px;font-size:11px;border:1px solid #6d28d9;background:#2a1744;color:#e9d5ff;border-radius:12px}.fps-global-status{display:none;margin:12px 0;padding:12px 14px;border:1px solid #56308d;border-radius:7px;background:#151022}.fps-global-status.show{display:block}.fps-global-status h4{margin:0 0 5px;color:#eee7f8}.fps-global-status p{margin:0 0 7px;color:#9f93ad;font-size:11px}", 'OSD builder CSS')
s = replace_once(s, "  ['osd1','osd2','osd3','osd4'].forEach((id,i) => {", "  ['osd1','osd2','osd3','osd4','pilotTpl','craftTpl'].forEach((id,i) => {", 'builder targets')
s = replace_once(s, '<span class="osd-builder-title">Message ${i+1} builder</span>', '<span class="osd-builder-title">${id===\'pilotTpl\'?\'Pilot Name\':id===\'craftTpl\'?\'Craft Name\':\'Message \'+(i+1)} builder</span>', 'builder title')

state_ui = r'''
<script id="fps-global-status-ui">
(() => {
 function boot(){
  if(document.getElementById('fpsGlobalStatus'))return;
  const host=document.getElementById('panel-config')||document.querySelector('.tab-panel')||document.body;
  const box=document.createElement('div');box.id='fpsGlobalStatus';box.className='fps-global-status';
  box.innerHTML='<h4>Camera Status</h4><p>Applies anywhere Status is selected.</p><label class="osd-check-item"><input type="checkbox" id="fpsRecOnly" checked><span>While recording show REC only</span></label><label class="osd-check-item"><input type="checkbox" id="fpsRecFlash" checked><span>Flash REC while recording</span></label>';
  const firstBuilder=document.querySelector('.osd-builder');(firstBuilder?.parentElement||host).insertBefore(box,firstBuilder||null);
  const recOnly=document.getElementById('fpsRecOnly'),flash=document.getElementById('fpsRecFlash');
  recOnly.checked=localStorage.getItem('freeclinkerRecOnly')!=='0';flash.checked=localStorage.getItem('freeclinkerRecFlash')!=='0';
  function activeInputs(){const bf45=!!document.getElementById('bf45Compat')?.checked;return bf45?['pilotTpl','craftTpl']:['osd1','osd2','osd3','osd4']}
  function hasStatus(){return activeInputs().some(id=>String(document.getElementById(id)?.value||'').includes('{state}'))}
  function syncFirmware(){
   const ready=document.getElementById('fpvReadyText'),err=document.getElementById('fpvErrorText'),rec=document.getElementById('fpvRecordText'),fpvFlash=document.getElementById('fpvFlash');
   const active=activeInputs().map(id=>document.getElementById(id)).find(e=>String(e?.value||'').includes('{state}'));
   const tpl=active?.value||'{state}'; if(ready)ready.value=tpl; if(err)err.value=tpl; if(rec)rec.value=recOnly.checked?'{state}':tpl; if(fpvFlash)fpvFlash.checked=flash.checked;
   localStorage.setItem('freeclinkerRecOnly',recOnly.checked?'1':'0');localStorage.setItem('freeclinkerRecFlash',flash.checked?'1':'0');box.classList.toggle('show',hasStatus());
  }
  document.addEventListener('fps-status-selection-changed',syncFirmware);document.getElementById('bf45Compat')?.addEventListener('change',syncFirmware);
  ['osd1','osd2','osd3','osd4','pilotTpl','craftTpl'].forEach(id=>document.getElementById(id)?.addEventListener('input',syncFirmware));
  recOnly.addEventListener('change',syncFirmware);flash.addEventListener('change',syncFirmware);syncFirmware();
 }
 if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',boot);else boot();
})();
</script>
'''
s = replace_once(s, '</body>', state_ui + '\n</body>', 'global status UI')

# Demo dependency rules should not re-enable controls from the inactive BF mode.
old_demo = """    fields().forEach(e=>e.disabled=false);
    [applyBtn,auxApplyBtn,osdApplyBtn,bf45ApplyBtn,caddxApplyBtn].filter(Boolean).forEach(e=>{e.disabled=false;e.textContent='Save Demo'});"""
new_demo = """    fields().forEach(e=>e.disabled=false);
    const bf45 = !!document.getElementById('bf45Compat')?.checked;
    ['osd1','osd2','osd3','osd4'].forEach(id=>{const e=document.getElementById(id);if(e)e.disabled=bf45});
    document.querySelectorAll('.osd-builder').forEach(box=>{const raw=box.querySelector('.tpl-input');if(raw&&/^osd[1-4]$/.test(raw.id)){box.style.opacity=bf45?'.35':'1';box.style.pointerEvents=bf45?'none':''}});
    const pe=document.getElementById('pilotEn'),ce=document.getElementById('craftEn'),pt=document.getElementById('pilotTpl'),ct=document.getElementById('craftTpl');
    if(pe)pe.disabled=!bf45;if(ce)ce.disabled=!bf45;if(pt)pt.disabled=!bf45||!pe?.checked;if(ct)ct.disabled=!bf45||!ce?.checked;
    [applyBtn,auxApplyBtn,osdApplyBtn,bf45ApplyBtn,caddxApplyBtn].filter(Boolean).forEach(e=>{e.disabled=false;e.textContent='Save Demo'});"""
s = replace_once(s, old_demo, new_demo, 'Demo BF dependency block')

# Validation is about the generated page, not brittle implementation strings.
for marker in ['fps-global-status-ui', 'Select OSD elements', "['Status','{state}']", 'freeclinkerDemoMode']:
    if marker not in s:
        raise SystemExit('Generated configurator missing: ' + marker)
p.write_text(s, encoding='utf-8')

# Preview Demo Mode is patched deterministically from the checked-in preview.
t = Path('web/test.html')
x = t.read_text(encoding='utf-8')
x = replace_once(x, '</style></head>', '.demo-banner{display:none;background:#3b1766;color:#f2e8ff;border-bottom:1px solid #8b5cf6;padding:8px 12px;text-align:center;font-weight:700;font-size:12px;letter-spacing:.04em}body.demo-on{outline:2px solid #7c3aed;outline-offset:-2px}body.demo-on .demo-banner{display:block}</style></head>', 'preview demo CSS')
x = replace_once(x, '<header>', '<div id="demoBanner" class="demo-banner">DEMO MODE - NO HARDWARE CONNECTED</div><header>', 'preview demo banner')
x = replace_once(x, "const $=x=>document.getElementById(x),D=", "const $=x=>document.getElementById(x),DEMO=localStorage.getItem('freeclinkerDemoMode')==='1'||new URLSearchParams(location.search).get('demo')==='1',D=", 'preview DEMO constant')
x = replace_once(x, "function load(){try{c={...D,...JSON.parse(localStorage.getItem('freeclinkerC3Config')||'{}')}}catch{c={...D}}summary()}", "function load(){try{const dc=DEMO?JSON.parse(localStorage.getItem('freeclinkerDemoConfig')||'{}'):{};c={...D,...JSON.parse(localStorage.getItem('freeclinkerC3Config')||'{}'),...dc}}catch{c={...D}}summary();if(DEMO){localStorage.setItem('freeclinkerDemoMode','1');document.body.classList.add('demo-on');$('pill').textContent='DEMO';$('statusText').textContent='Demo Mode';$('source').textContent='DEMO DATA';$('connect').disabled=true;$('disconnect').disabled=true;$('read').disabled=false;$('arm').disabled=false;$('power').disabled=false;document.querySelectorAll('a[href^=\"config.html\"]').forEach(a=>{const h=a.getAttribute('href').split('#');a.href=h[0]+'?demo=1'+(h[1]?'#'+h[1]:'')})}}", 'preview demo load')
x = replace_once(x, "await send(`sim arm ${armed?1:0}`);", "if(!DEMO)await send(`sim arm ${armed?1:0}`);", 'preview ARM simulation')
x = replace_once(x, "if(live)try{await send('sim off')}catch{}", "if(live&&!DEMO)try{await send('sim off')}catch{}", 'preview disconnect simulation')
x = replace_once(x, "if(live)try{await send('sim off')}catch{}armed=false", "if(live&&!DEMO)try{await send('sim off')}catch{}armed=false", 'preview reset simulation')
x = x.replace("$('arm').disabled=!live", "$('arm').disabled=!live&&!DEMO").replace("$('power').disabled=!live", "$('power').disabled=!live&&!DEMO").replace("$('read').disabled=!live", "$('read').disabled=!live&&!DEMO")
x = x.replace("$('source').textContent=useLive()?'LIVE CAMERA DATA':'PREVIEW VALUES'", "$('source').textContent=useLive()?'LIVE CAMERA DATA':DEMO?'DEMO DATA':'PREVIEW VALUES'")
x = x.replace("${useLive()?'LIVE CAMERA DATA':'PREVIEW DATA'}", "${useLive()?'LIVE CAMERA DATA':DEMO?'DEMO DATA':'PREVIEW DATA'}")
for marker in ["const $=x=>document.getElementById(x),DEMO=", 'DEMO MODE - NO HARDWARE CONNECTED', "if(!DEMO)await send(`sim arm"]:
    if marker not in x:
        raise SystemExit('Generated OSD Preview missing: ' + marker)
t.write_text(x, encoding='utf-8')
print('FPSteVe Pages polish generated and validated')
