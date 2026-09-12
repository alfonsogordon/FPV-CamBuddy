from pathlib import Path

def one(text, old, new, label):
    if old not in text: raise SystemExit(label + ': source marker not found')
    return text.replace(old,new,1)

p=Path('web/config.html'); s=p.read_text(encoding='utf-8')

old_tokens="""  const tokenDefs = [
    ['Battery','{bat}'],['Recording','{rec}'],['Duration','{recdur}'],['Mode','{mode}'],['Resolution','{res}'],['FPS','{fps}'],['Stabilisation','{eis}'],['Time left','{rleft}'],['Storage','{rcap}']
  ];"""
new_tokens="""  const tokenDefs = [
    ['Status','{state}'],['Battery','{batt}'],['Recording Duration','{recdur}'],['Mode','{mode}'],['Resolution','{res}'],['FPS','{fps}'],['Stabilisation','{eis}'],['Time Remaining','{rectf}'],['Storage Remaining','{rcap}']
  ];"""
s=one(s,old_tokens,new_tokens,'OSD element vocabulary')
s=one(s,"const samples = {'{bat}':'69%','{rec}':'REC','{recdur}':'00:42','{mode}':'VIDEO','{res}':'4K','{fps}':'60','{eis}':'HS','{rleft}':'45m','{rcap}':'128GB'};","const samples = {'{state}':'RDY','{batt}':'B:69','{recdur}':'00:42','{mode}':'VIDEO','{res}':'4K','{fps}':'60','{eis}':'HS','{rectf}':'T:45m','{rcap}':'128GB'};",'samples')
s=one(s,".osd-token-grid{display:flex;flex-wrap:wrap;gap:6px}",".osd-token-grid{display:flex;flex-wrap:wrap;gap:7px;align-items:center}.osd-multi{position:relative;min-width:210px}.osd-multi>summary{list-style:none;cursor:pointer;background:#171122;color:#eee;border:1px solid #6d28d9;border-radius:4px;padding:7px 9px}.osd-multi>summary::-webkit-details-marker{display:none}.osd-multi>summary:after{content:' ▾';float:right;color:#c4b5fd}.osd-multi-menu{position:absolute;z-index:30;top:calc(100% + 4px);left:0;min-width:250px;padding:6px;background:#100b18;border:1px solid #6d28d9;border-radius:6px;box-shadow:0 8px 24px #0009}.osd-check-item{display:flex;align-items:center;gap:8px;padding:7px 8px;border-radius:4px;cursor:pointer;color:#eee}.osd-check-item:hover{background:#2a1744}.osd-check-item input{accent-color:#8b5cf6}.osd-chip-row{display:flex;flex-wrap:wrap;gap:5px;width:100%}.osd-chip{padding:3px 7px;font-size:11px;border:1px solid #6d28d9;background:#2a1744;color:#e9d5ff;border-radius:12px}.fps-global-status{display:none;margin:12px 0;padding:12px 14px;border:1px solid #56308d;border-radius:7px;background:#151022}.fps-global-status.show{display:block}",'builder css')
s=one(s,"  ['osd1','osd2','osd3','osd4'].forEach((id,i) => {","  ['osd1','osd2','osd3','osd4','pilotTpl','craftTpl'].forEach((id,i) => {",'builder targets')
s=one(s,'<span class="osd-builder-title">Message ${i+1} builder</span>','<span class="osd-builder-title">${id===\'pilotTpl\'?\'Pilot Name\':id===\'craftTpl\'?\'Craft Name\':\'Message \'+(i+1)} builder</span>','builder title')
old_buttons="""    tokenDefs.forEach(([name,token])=>{ const b=document.createElement('button'); b.type='button'; b.className='osd-token'; b.textContent='+ '+name; b.title=token; b.onclick=()=>{ const spacer=input.value && !input.value.endsWith(' ')?' ':''; input.value+=spacer+token; input.dispatchEvent(new Event('input',{bubbles:true})); refresh(); }; grid.appendChild(b); });
    const clear=document.createElement('button'); clear.type='button'; clear.className='osd-token'; clear.textContent='Clear'; clear.onclick=()=>{input.value='';input.dispatchEvent(new Event('input',{bubbles:true}));refresh()}; grid.appendChild(clear);"""
new_buttons="""    const picker=document.createElement('details');picker.className='osd-multi';const summary=document.createElement('summary');summary.textContent='Select OSD elements';picker.appendChild(summary);const menu=document.createElement('div');menu.className='osd-multi-menu';picker.appendChild(menu);grid.appendChild(picker);const chips=document.createElement('div');chips.className='osd-chip-row';grid.appendChild(chips);
    const tokenRe=/(\\{(?:state|batt|recdur|mode|res|fps|eis|rectf|rcap)\\})/g;const toks=()=>String(input.value||'').match(tokenRe)||[];
    function sync(){const a=toks();menu.querySelectorAll('input').forEach(c=>c.checked=a.includes(c.value));chips.innerHTML='';a.forEach(t=>{const d=tokenDefs.find(v=>v[1]===t),z=document.createElement('span');z.className='osd-chip';z.textContent=d?d[0]:t;chips.appendChild(z)});summary.textContent=a.length?`OSD elements (${a.length})`:'Select OSD elements';document.dispatchEvent(new CustomEvent('fps-status-selection-changed'))}
    tokenDefs.forEach(([name,token])=>{const l=document.createElement('label');l.className='osd-check-item';const c=document.createElement('input');c.type='checkbox';c.value=token;l.append(c,document.createTextNode(name));menu.appendChild(l);c.onchange=()=>{if(c.checked){if(!toks().includes(token))input.value+=(input.value&&!input.value.endsWith(' ')?' ':'')+token}else{let done=false;input.value=input.value.split(tokenRe).filter(v=>{if(!done&&v===token){done=true;return false}return true}).join('').replace(/ {2,}/g,' ').trim()}input.dispatchEvent(new Event('input',{bubbles:true}));refresh();sync()}});input.addEventListener('input',sync);sync();"""
s=one(s,old_buttons,new_buttons,'checkbox builder')

status=r'''<script id="fps-global-status-ui">(()=>{function boot(){if(document.getElementById('fpsGlobalStatus'))return;const first=document.querySelector('.osd-builder'),box=document.createElement('div');box.id='fpsGlobalStatus';box.className='fps-global-status';box.innerHTML='<strong>Camera Status</strong><div class="cfg-field-desc">Applies anywhere Status is selected.</div><label class="osd-check-item"><input type="checkbox" id="fpsRecOnly" checked>While recording show REC only</label><label class="osd-check-item"><input type="checkbox" id="fpsRecFlash" checked>Flash REC while recording</label>';(first?.parentElement||document.body).insertBefore(box,first||null);const ro=document.getElementById('fpsRecOnly'),fl=document.getElementById('fpsRecFlash');ro.checked=localStorage.getItem('freeclinkerRecOnly')!=='0';fl.checked=localStorage.getItem('freeclinkerRecFlash')!=='0';function ids(){return document.getElementById('bf45Compat')?.checked?['pilotTpl','craftTpl']:['osd1','osd2','osd3','osd4']}function sync(){const es=ids().map(id=>document.getElementById(id)),active=es.find(e=>String(e?.value||'').includes('{state}')),tpl=active?.value||'{state}';box.classList.toggle('show',!!active);const r=document.getElementById('fpvReadyText'),e=document.getElementById('fpvErrorText'),q=document.getElementById('fpvRecordText'),f=document.getElementById('fpvFlash');if(r)r.value=tpl;if(e)e.value=tpl;if(q)q.value=ro.checked?'{state}':tpl;if(f)f.checked=fl.checked;localStorage.setItem('freeclinkerRecOnly',ro.checked?'1':'0');localStorage.setItem('freeclinkerRecFlash',fl.checked?'1':'0')}document.addEventListener('fps-status-selection-changed',sync);document.getElementById('bf45Compat')?.addEventListener('change',sync);ro.addEventListener('change',sync);fl.addEventListener('change',sync);sync()}document.readyState==='loading'?document.addEventListener('DOMContentLoaded',boot):boot()})();</script>'''
s=one(s,'</body>',status+'\n</body>','status ui')

# Demo controls must respect active BF mode.
old="""    fields().forEach(e=>e.disabled=false);
    [applyBtn,auxApplyBtn,osdApplyBtn,bf45ApplyBtn,caddxApplyBtn].filter(Boolean).forEach(e=>{e.disabled=false;e.textContent='Save Demo'});"""
new="""    fields().forEach(e=>e.disabled=false);
    const bf45=!!document.getElementById('bf45Compat')?.checked;['osd1','osd2','osd3','osd4'].forEach(id=>{const e=document.getElementById(id);if(e)e.disabled=bf45});
    [applyBtn,auxApplyBtn,osdApplyBtn,bf45ApplyBtn,caddxApplyBtn].filter(Boolean).forEach(e=>{e.disabled=false;e.textContent='Save Demo'});"""
s=one(s,old,new,'demo dependencies')
for m in ['fps-global-status-ui','Select OSD elements',"['Status','{state}']"]:
    if m not in s: raise SystemExit('config validation failed: '+m)
p.write_text(s,encoding='utf-8')

# Patch preview demo mode from its stable checked-in source.
t=Path('web/test.html');x=t.read_text(encoding='utf-8')
x=one(x,'</style></head>','.demo-banner{display:none;background:#3b1766;color:#f2e8ff;padding:8px;text-align:center;font-weight:700}body.demo-on .demo-banner{display:block}</style></head>','preview css')
x=one(x,'<header>','<div class="demo-banner">DEMO MODE - NO HARDWARE CONNECTED</div><header>','preview banner')
x=one(x,"const $=x=>document.getElementById(x),D=","const $=x=>document.getElementById(x),DEMO=localStorage.getItem('freeclinkerDemoMode')==='1'||new URLSearchParams(location.search).get('demo')==='1',D=",'preview mode')
x=one(x,"function load(){try{c={...D,...JSON.parse(localStorage.getItem('freeclinkerC3Config')||'{}')}}catch{c={...D}}summary()}","function load(){try{const dc=DEMO?JSON.parse(localStorage.getItem('freeclinkerDemoConfig')||'{}'):{};c={...D,...JSON.parse(localStorage.getItem('freeclinkerC3Config')||'{}'),...dc}}catch{c={...D}}summary();if(DEMO){localStorage.setItem('freeclinkerDemoMode','1');document.body.classList.add('demo-on');$('pill').textContent='DEMO';$('statusText').textContent='Demo Mode';$('source').textContent='DEMO DATA';$('connect').disabled=true;$('disconnect').disabled=true;$('read').disabled=false;$('arm').disabled=false;$('power').disabled=false}}",'preview load')
x=one(x,"await send(`sim arm ${armed?1:0}`);","if(!DEMO)await send(`sim arm ${armed?1:0}`);",'preview arm')
x=x.replace("$('arm').disabled=!live","$('arm').disabled=!live&&!DEMO").replace("$('power').disabled=!live","$('power').disabled=!live&&!DEMO").replace("$('read').disabled=!live","$('read').disabled=!live&&!DEMO")
for m in ['DEMO MODE - NO HARDWARE CONNECTED',"if(!DEMO)await send(`sim arm"]:
    if m not in x: raise SystemExit('preview validation failed: '+m)
t.write_text(x,encoding='utf-8')
print('FPSteVe polish generated and validated')