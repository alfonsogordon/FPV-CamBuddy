(()=>{
'use strict';
const q=id=>document.getElementById(id), IDS=['osd1','osd2','osd3','osd4'];
const WARN_KEY='fpsWarningDestination';
function parse(v){const out=[];String(v||'').replace(/\{[^}]+\}|[^{}]+/g,x=>{if(x[0]==='{')out.push({t:'token',v:x});else if(x.trim())out.push({t:'text',v:x.trim()})});return out}
function serial(a){return a.map(x=>x.v).join(' ').replace(/\s+/g,' ').trim()}
function statusToken(v){return v==='{stateonly}'?'{state}':v}
function hasStatus(a){return a.some(x=>x.t==='token'&&(x.v==='{state}'||x.v==='{stateonly}'))}
function setStatusOnly(input,on){const a=parse(input.value).map(x=>x.t==='token'&&x.v===(on?'{state}':'{stateonly}')?{...x,v:on?'{stateonly}':'{state}'}:x);input.value=serial(a);input.dispatchEvent(new Event('input',{bubbles:true}))}
function enhanceBuilder(input){const box=input?.closest('.fps-builder');if(!box||box.dataset.customText==='1')return;box.dataset.customText='1';const menu=box.querySelector('.fps-menu');if(!menu)return;
 menu.querySelectorAll('input[type=checkbox]').forEach(cb=>{cb.onchange=()=>{let a=parse(input.value);const wanted=cb.value;if(cb.checked){if(!a.some(x=>x.t==='token'&&statusToken(x.v)===wanted))a.push({t:'token',v:wanted})}else a=a.filter(x=>!(x.t==='token'&&statusToken(x.v)===wanted));input.value=serial(a);input.dispatchEvent(new Event('input',{bubbles:true}))}});
 const label=document.createElement('label');label.innerHTML='<input type="checkbox" class="fps-custom-text-check"> Custom Text';menu.appendChild(label);const check=label.querySelector('input');
 const row=document.createElement('div');row.className='fps-custom-text-row fps-hidden';row.innerHTML='<input type="text" maxlength="16" class="fps-custom-text-input" placeholder="Custom text, e.g. GOPRO"><button type="button" class="fps-custom-text-add">Add text</button>';box.querySelector('.fps-chips')?.before(row);
 const txt=row.querySelector('input'),add=row.querySelector('button');check.onchange=()=>{row.classList.toggle('fps-hidden',!check.checked);if(check.checked)txt.focus()};add.onclick=()=>{const v=txt.value.trim();if(!v)return;const a=parse(input.value);a.push({t:'text',v});input.value=serial(a);input.dispatchEvent(new Event('input',{bubbles:true}));txt.value='';check.checked=false;row.classList.add('fps-hidden')};
 const recOnly=box.querySelector('.fps-rec-only');if(recOnly){recOnly.checked=parse(input.value).some(x=>x.v==='{stateonly}');recOnly.onchange=()=>setStatusOnly(input,recOnly.checked)}
 const render=()=>{const chips=box.querySelector('.fps-chips');if(!chips)return;const a=parse(input.value);chips.querySelectorAll('.fps-literal-chip').forEach(x=>x.remove());menu.querySelectorAll('input[type=checkbox][value]').forEach(cb=>cb.checked=a.some(x=>x.t==='token'&&statusToken(x.v)===cb.value));box.querySelector('.fps-local-status')?.classList.toggle('fps-hidden',!hasStatus(a));if(recOnly)recOnly.checked=a.some(x=>x.v==='{stateonly}');a.forEach((x,i)=>{if(x.t!=='text')return;const c=document.createElement('span');c.className='fps-chip fps-literal-chip';c.append(document.createTextNode('Text: '+x.v));const b=document.createElement('button');b.type='button';b.textContent='×';b.onclick=()=>{const n=parse(input.value);n.splice(i,1);input.value=serial(n);input.dispatchEvent(new Event('input',{bubbles:true}))};c.appendChild(b);chips.appendChild(c)})};
 input.addEventListener('input',()=>setTimeout(render,0));input.addEventListener('change',()=>setTimeout(render,0));render();
}
function warningUi(){
 const head=q('fpsWarningsHead');if(!head||q('fpsWarningDestination'))return;
 const row=document.createElement('div');row.className='cfg-field fps-warning-destination';
 row.innerHTML='<div class="cfg-field-text"><div class="cfg-field-name">Temporary message / warning destination</div><div class="cfg-field-desc">Select which Custom Message box is temporarily replaced by Custom Message reminders and camera warnings.</div></div><select id="fpsWarningDestination"><option value="1">Custom Message 1</option><option value="2">Custom Message 2</option><option value="3">Custom Message 3</option><option value="4">Custom Message 4</option></select>';
 head.after(row);const s=q('fpsWarningDestination'),low=q('fpvLowText');
 const encoded=(low?.value||'').match(/^@([1-4]):(.*)$/);s.value=encoded?.[1]||localStorage.getItem(WARN_KEY)||'1';if(encoded&&low)low.value=encoded[2];
 function saveDestination(){localStorage.setItem(WARN_KEY,s.value)}
 s.onchange=saveDestination;saveDestination();
 const warn=q('fpvLowBatt'),custom=q('fpsCustomEnable');
 const apply=()=>{const legacy=!!q('bf45Compat')?.checked;const anyEnabled=!!warn?.checked||!!custom?.checked;row.classList.toggle('fps-collapsed',!anyEnabled);s.disabled=legacy;row.querySelector('.cfg-field-desc').textContent=legacy?'Betaflight 4.4–2025.12 uses Craft Name for temporary messages and warnings.':'Select which Custom Message box is temporarily replaced by Custom Message reminders and camera warnings.'};
 warn?.addEventListener('change',apply);custom?.addEventListener('change',apply);q('bf45Compat')?.addEventListener('change',apply);apply();
}
function init(){IDS.forEach(id=>enhanceBuilder(q(id)));warningUi()}
const st=document.createElement('style');st.textContent='.fps-custom-text-row{display:flex;gap:8px;margin:8px 0}.fps-custom-text-row input{flex:1}.fps-custom-text-row button{white-space:nowrap}.fps-warning-destination{border-top:1px solid rgba(124,58,237,.18)}';document.head.appendChild(st);if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,120));else setTimeout(init,120);
})();