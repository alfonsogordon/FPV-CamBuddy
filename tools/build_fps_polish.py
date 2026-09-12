from pathlib import Path

p = Path('web/config.html')
s = p.read_text(encoding='utf-8')

old = """    tokenDefs.forEach(([name,token])=>{ const b=document.createElement('button'); b.type='button'; b.className='osd-token'; b.textContent='+ '+name; b.title=token; b.onclick=()=>{ const spacer=input.value && !input.value.endsWith(' ')?' ':''; input.value+=spacer+token; input.dispatchEvent(new Event('input',{bubbles:true})); refresh(); }; grid.appendChild(b); });
    const clear=document.createElement('button'); clear.type='button'; clear.className='osd-token'; clear.textContent='Clear'; clear.onclick=()=>{input.value='';input.dispatchEvent(new Event('input',{bubbles:true}));refresh()}; grid.appendChild(clear);"""
new = """    const picker=document.createElement('details'); picker.className='osd-multi';
    const summary=document.createElement('summary'); summary.textContent='Select OSD elements'; picker.appendChild(summary);
    const menu=document.createElement('div'); menu.className='osd-multi-menu'; picker.appendChild(menu); grid.appendChild(picker);
    const chips=document.createElement('div'); chips.className='osd-chip-row'; grid.appendChild(chips);
    const tokenRe=/(\\{(?:bat|rec|recdur|mode|res|fps|eis|rleft|rcap)\\})/g;
    function tokensInOrder(){return String(input.value||'').match(tokenRe)||[]}
    function removeToken(token){const parts=String(input.value||'').split(tokenRe);let removed=false;input.value=parts.filter(part=>{if(!removed&&part===token){removed=true;return false}return true}).join('').replace(/ {2,}/g,' ').trim()}
    function syncPicker(){const active=tokensInOrder();menu.querySelectorAll('input[type=checkbox]').forEach(cb=>cb.checked=active.includes(cb.value));chips.innerHTML='';active.forEach(token=>{const def=tokenDefs.find(x=>x[1]===token);const chip=document.createElement('span');chip.className='osd-chip';chip.textContent=def?def[0]:token;chips.appendChild(chip)});summary.textContent=active.length?`OSD elements (${active.length})`:'Select OSD elements'}
    tokenDefs.forEach(([name,token])=>{const label=document.createElement('label');label.className='osd-check-item';const cb=document.createElement('input');cb.type='checkbox';cb.value=token;const text=document.createElement('span');text.textContent=name;label.append(cb,text);menu.appendChild(label);cb.onchange=()=>{if(cb.checked){if(!tokensInOrder().includes(token)){const spacer=input.value&&!input.value.endsWith(' ')?' ':'';input.value+=spacer+token}}else removeToken(token);input.dispatchEvent(new Event('input',{bubbles:true}));refresh();syncPicker()}});
    input.addEventListener('input',syncPicker);syncPicker();"""
if old not in s: raise SystemExit('old OSD button builder not found')
s=s.replace(old,new,1)

s=s.replace(".osd-token-grid{display:flex;flex-wrap:wrap;gap:6px}", ".osd-token-grid{display:flex;flex-wrap:wrap;gap:7px;align-items:center}.osd-multi{position:relative;min-width:210px}.osd-multi>summary{list-style:none;cursor:pointer;background:#171122;color:#eee;border:1px solid #6d28d9;border-radius:4px;padding:7px 9px}.osd-multi>summary::-webkit-details-marker{display:none}.osd-multi>summary:after{content:' ▾';float:right;color:#c4b5fd}.osd-multi[open]>summary:after{content:' ▴'}.osd-multi-menu{position:absolute;z-index:30;top:calc(100% + 4px);left:0;min-width:230px;padding:6px;background:#100b18;border:1px solid #6d28d9;border-radius:6px;box-shadow:0 8px 24px #0009}.osd-check-item{display:flex;align-items:center;gap:8px;padding:7px 8px;border-radius:4px;cursor:pointer;color:#eee}.osd-check-item:hover{background:#2a1744}.osd-check-item input{accent-color:#8b5cf6}.osd-chip-row{display:flex;flex-wrap:wrap;gap:5px;width:100%}.osd-chip{padding:3px 7px;font-size:11px;border:1px solid #6d28d9;background:#2a1744;color:#e9d5ff;border-radius:12px}")

needle = "  ['osd1','osd2','osd3','osd4'].forEach((id,i) => {"
if needle not in s: raise SystemExit('builder loop not found')
s=s.replace(needle, "  ['osd1','osd2','osd3','osd4','pilotTpl','craftTpl'].forEach((id,i) => {",1)
s=s.replace("<span class=\"osd-builder-title\">Message ${i+1} builder</span>", "<span class=\"osd-builder-title\">${id==='pilotTpl'?'Pilot Name':id==='craftTpl'?'Craft Name':'Message '+(i+1)} builder</span>",1)

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

t=Path('web/test.html')
x=t.read_text(encoding='utf-8')
x=x.replace('</style></head>', '.demo-banner{display:none;background:#3b1766;color:#f2e8ff;border-bottom:1px solid #8b5cf6;padding:8px 12px;text-align:center;font-weight:700;font-size:12px;letter-spacing:.04em}body.demo-on{outline:2px solid #7c3aed;outline-offset:-2px}body.demo-on .demo-banner{display:block}</style></head>',1)
x=x.replace('<header>', '<div id="demoBanner" class="demo-banner">🧪 PREVIEW / DEMO MODE — NO HARDWARE CONNECTED</div><header>',1)
x=x.replace("const $=x=>document.getElementById(x),D=", "const $=x=>document.getElementById(x),DEMO=localStorage.getItem('freeclinkerDemoMode')==='1',D=",1)
x=x.replace("function load(){try{c={...D,...JSON.parse(localStorage.getItem('freeclinkerC3Config')||'{}')}}catch{c={...D}}summary()}", "function load(){try{const demoCfg=DEMO?JSON.parse(localStorage.getItem('freeclinkerDemoConfig')||'{}'):{};c={...D,...JSON.parse(localStorage.getItem('freeclinkerC3Config')||'{}'),...demoCfg}}catch{c={...D}}summary();if(DEMO){document.body.classList.add('demo-on');$('pill').textContent='DEMO';$('pill').className='pill';$('statusText').textContent='Demo Mode';$('source').textContent='DEMO DATA';$('connect').disabled=true;$('disconnect').disabled=true;$('read').disabled=false;$('arm').disabled=false;$('power').disabled=false;document.querySelectorAll('.tab[href^=\"config.html\"]').forEach(a=>a.href=a.getAttribute('href')+(a.getAttribute('href').includes('?')?'&':'?')+'demo=1')}}",1)
x=x.replace("await send(`sim arm ${armed?1:0}`)", "if(!DEMO)await send(`sim arm ${armed?1:0}`)")
x=x.replace("await send('sim off')", "if(!DEMO)await send('sim off')")
x=x.replace("$('arm').disabled=!live", "$('arm').disabled=!live&&!DEMO")
x=x.replace("$('power').disabled=!live", "$('power').disabled=!live&&!DEMO")
x=x.replace("$('read').disabled=!live", "$('read').disabled=!live&&!DEMO")
x=x.replace("$('source').textContent=useLive()?'LIVE CAMERA DATA':'PREVIEW VALUES'", "$('source').textContent=useLive()?'LIVE CAMERA DATA':DEMO?'DEMO DATA':'PREVIEW VALUES'")
x=x.replace("${useLive()?'LIVE CAMERA DATA':'PREVIEW DATA'}", "${useLive()?'LIVE CAMERA DATA':DEMO?'DEMO DATA':'PREVIEW DATA'}")
t.write_text(x,encoding='utf-8')
print('FPSteVe checkbox dropdown polish applied')