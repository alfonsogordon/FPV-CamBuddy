from pathlib import Path

p = Path('web/config.html')
s = p.read_text(encoding='utf-8')

old = """    tokenDefs.forEach(([name,token])=>{ const b=document.createElement('button'); b.type='button'; b.className='osd-token'; b.textContent='+ '+name; b.title=token; b.onclick=()=>{ const spacer=input.value && !input.value.endsWith(' ')?' ':''; input.value+=spacer+token; input.dispatchEvent(new Event('input',{bubbles:true})); refresh(); }; grid.appendChild(b); });
    const clear=document.createElement('button'); clear.type='button'; clear.className='osd-token'; clear.textContent='Clear'; clear.onclick=()=>{input.value='';input.dispatchEvent(new Event('input',{bubbles:true}));refresh()}; grid.appendChild(clear);"""
new = """    const select=document.createElement('select'); select.className='osd-add-select'; select.innerHTML='<option value="">Add OSD element…</option>'+tokenDefs.map(([name,token])=>`<option value="${token}">${name}</option>`).join(''); grid.appendChild(select);
    const chips=document.createElement('div'); chips.className='osd-chip-row'; grid.appendChild(chips);
    function redrawChips(){ chips.innerHTML=''; const re=/(\\{(?:bat|rec|recdur|mode|res|fps|eis|rleft|rcap)\\})/g; const parts=String(input.value||'').split(re); parts.forEach((part,idx)=>{ if(!samples[part])return; const def=tokenDefs.find(x=>x[1]===part); const chip=document.createElement('button'); chip.type='button'; chip.className='osd-chip'; chip.textContent=(def?def[0]:part)+' ×'; chip.title='Remove '+(def?def[0]:part); chip.onclick=()=>{parts[idx]=''; input.value=parts.join('').replace(/ {2,}/g,' ').trim(); input.dispatchEvent(new Event('input',{bubbles:true})); refresh(); redrawChips();}; chips.appendChild(chip); }); }
    select.onchange=()=>{ if(!select.value)return; const spacer=input.value && !input.value.endsWith(' ')?' ':''; input.value+=spacer+select.value; select.value=''; input.dispatchEvent(new Event('input',{bubbles:true})); refresh(); redrawChips(); };
    const clear=document.createElement('button'); clear.type='button'; clear.className='osd-token'; clear.textContent='Clear message'; clear.onclick=()=>{input.value='';input.dispatchEvent(new Event('input',{bubbles:true}));refresh();redrawChips()}; grid.appendChild(clear); redrawChips();"""
if old not in s: raise SystemExit('old OSD button builder not found')
s=s.replace(old,new,1)

s=s.replace(".osd-token-grid{display:flex;flex-wrap:wrap;gap:6px}", ".osd-token-grid{display:flex;flex-wrap:wrap;gap:7px;align-items:center}.osd-add-select{min-width:190px;background:#171122!important;color:#eee!important;border:1px solid #6d28d9!important;padding:6px 8px!important}.osd-chip-row{display:flex;flex-wrap:wrap;gap:5px;width:100%}.osd-chip{padding:4px 7px!important;font-size:11px!important;border:1px solid #6d28d9!important;background:#2a1744!important;color:#e9d5ff!important;border-radius:12px!important}")

# Also decorate BF4.5 Pilot/Craft templates with the same ordered dropdown/chip builder.
needle = "  ['osd1','osd2','osd3','osd4'].forEach((id,i) => {"
if needle not in s: raise SystemExit('builder loop not found')
# Expand IDs and use friendly labels.
s=s.replace(needle, "  ['osd1','osd2','osd3','osd4','pilotTpl','craftTpl'].forEach((id,i) => {",1)
s=s.replace("<span class=\"osd-builder-title\">Message ${i+1} builder</span>", "<span class=\"osd-builder-title\">${id==='pilotTpl'?'Pilot Name':id==='craftTpl'?'Craft Name':'Message '+(i+1)} builder</span>",1)

# Demo mode must respect BF4.5 dependency instead of blindly enabling all config controls.
old_demo = """    fields().forEach(e=>e.disabled=false);
    [applyBtn,auxApplyBtn,osdApplyBtn,bf45ApplyBtn,caddxApplyBtn].filter(Boolean).forEach(e=>{e.disabled=false;e.textContent='Save Demo'});"""
new_demo = """    fields().forEach(e=>e.disabled=false);
    const bf45 = !!document.getElementById('bf45Compat')?.checked;
    ['osd1','osd2','osd3','osd4'].forEach(id=>{const e=document.getElementById(id);if(e)e.disabled=bf45});
    document.querySelectorAll('.osd-builder').forEach(box=>{const raw=box.querySelector('.tpl-input');if(raw&&/^osd[1-4]$/.test(raw.id)){box.style.opacity=bf45?'.35':'1';box.style.pointerEvents=bf45?'none':''}});
    const p=document.getElementById('pilotEn'),ct=document.getElementById('craftEn'),pt=document.getElementById('pilotTpl'),cft=document.getElementById('craftTpl');
    if(p)p.disabled=!bf45;if(ct)ct.disabled=!bf45;if(pt)pt.disabled=!bf45||!p?.checked;if(cft)cft.disabled=!bf45||!ct?.checked;
    [applyBtn,auxApplyBtn,osdApplyBtn,bf45ApplyBtn,caddxApplyBtn].filter(Boolean).forEach(e=>{e.disabled=false;e.textContent='Save Demo'});"""
if old_demo not in s: raise SystemExit('demo enable block not found')
s=s.replace(old_demo,new_demo,1)

p.write_text(s,encoding='utf-8')

# OSD Preview: persist demo state, virtual ARM/DISARM, clear visual demo status, and demo-aware navigation.
t=Path('web/test.html')
x=t.read_text(encoding='utf-8')
x=x.replace('</style></head>', '.demo-banner{display:none;background:#3b1766;color:#f2e8ff;border-bottom:1px solid #8b5cf6;padding:8px 12px;text-align:center;font-weight:700;font-size:12px;letter-spacing:.04em}body.demo-on{outline:2px solid #7c3aed;outline-offset:-2px}body.demo-on .demo-banner{display:block}</style></head>',1)
x=x.replace('<header>', '<div id="demoBanner" class="demo-banner">🧪 PREVIEW / DEMO MODE — NO HARDWARE CONNECTED</div><header>',1)
x=x.replace("const $=x=>document.getElementById(x),D=", "const $=x=>document.getElementById(x),DEMO=localStorage.getItem('freeclinkerDemoMode')==='1',D=",1)
x=x.replace("function load(){try{c={...D,...JSON.parse(localStorage.getItem('freeclinkerC3Config')||'{}')}}catch{c={...D}}summary()}", "function load(){try{const demoCfg=DEMO?JSON.parse(localStorage.getItem('freeclinkerDemoConfig')||'{}'):{};c={...D,...JSON.parse(localStorage.getItem('freeclinkerC3Config')||'{}'),...demoCfg}}catch{c={...D}}summary();if(DEMO){document.body.classList.add('demo-on');$('pill').textContent='DEMO';$('pill').className='pill';$('statusText').textContent='Demo Mode';$('source').textContent='DEMO DATA';$('connect').disabled=true;$('disconnect').disabled=true;$('read').disabled=false;$('arm').disabled=false;$('power').disabled=false;document.querySelectorAll('.tab[href^=\"config.html\"]').forEach(a=>a.href=a.getAttribute('href')+(a.getAttribute('href').includes('?')?'&':'?')+'demo=1')}}",1)
# Make ARM usable in demo without send() trying serial.
x=x.replace("$('arm').onclick=async()=>{", "$('arm').onclick=async()=>{",1)
x=x.replace("await send(`sim arm ${armed?1:0}`)", "if(!DEMO)await send(`sim arm ${armed?1:0}`)")
x=x.replace("await send('sim off')", "if(!DEMO)await send('sim off')")
# No-C3 gating should not disable demo controls.
x=x.replace("$('arm').disabled=!live", "$('arm').disabled=!live&&!DEMO")
x=x.replace("$('power').disabled=!live", "$('power').disabled=!live&&!DEMO")
x=x.replace("$('read').disabled=!live", "$('read').disabled=!live&&!DEMO")
# Ensure preview badge stays explicitly demo when no live camera.
x=x.replace("$('source').textContent=useLive()?'LIVE CAMERA DATA':'PREVIEW VALUES'", "$('source').textContent=useLive()?'LIVE CAMERA DATA':DEMO?'DEMO DATA':'PREVIEW VALUES'")
x=x.replace("${useLive()?'LIVE CAMERA DATA':'PREVIEW DATA'}", "${useLive()?'LIVE CAMERA DATA':DEMO?'DEMO DATA':'PREVIEW DATA'}")
t.write_text(x,encoding='utf-8')
print('FPSteVe polish applied')