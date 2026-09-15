(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const TOKEN_RE=/\{[^}]+\}/g;
const FIELD_NAME={state:'Status',stateonly:'Status',batt:'Battery',recdur:'Recording Duration',mode:'Mode',res:'Resolution',fps:'FPS',eis:'Stabilisation',rectf:'Time Remaining',rcap:'Storage Remaining'};
const PREVIEW_DEFAULTS={1:{battery:82,remain:41},2:{battery:69,remain:74},3:{battery:76,remain:32},4:{battery:61,remain:55}};
const advanced=()=>!!window.fpsMultiCamOsd?.advancedOn?.();
function parse(tok){const raw=String(tok||'').replace(/^\{|\}$/g,'');const m=raw.match(/^([a-z0-9]+)(?:@(\d+)([nt]))?$/i);return m?{base:m[1],source:Number(m[2])||0,mode:m[3]||null}:null}
function cameraList(){const api=window.fpsMultiCamOsd,list=api?.cameraList?.();if(Array.isArray(list)&&list.length)return list;const map=api?.cameras;if(map?.values)return [...map.values()].sort((a,b)=>(a.number||0)-(b.number||0));return[]}
function optionText(c){const tag=String(c?.label||'').trim();return `C${c.number}${tag?' · '+tag:''}${c.connected?' · connected':''}`}
function token(base,source){if(!source)return `{${base}}`;const mode=localStorage.getItem('fpsAdvancedMultiCamOsdIdentifier')==='tag'?'t':'n';return `{${base}@${source}${mode}}`}
function changeSource(input,index,source){const toks=input.value.match(TOKEN_RE)||[];if(!toks[index])return;const p=parse(toks[index]);if(!p)return;const next=token(p.base,source);if(toks.some((t,j)=>j!==index&&t===next))return false;toks[index]=next;input.value=toks.join(' ');input.dispatchEvent(new Event('input',{bubbles:true}));input.dispatchEvent(new Event('change',{bubbles:true}));return true}
function redecorateAfterLegacyRefresh(builder){
 /* The original Advanced OSD helper redraws chip text after an input event. That
    legacy redraw is still useful for the underlying builder, but textContent also
    removes our inline select. Re-apply the per-element control after each of those
    queued redraws so the selector remains part of the chip. */
 [0,16,50,150,350].forEach(ms=>setTimeout(()=>decorate(builder),ms));
}
function buildSelect(chip,input,index,p){
 const sel=document.createElement('select');sel.className='fps-element-source';sel.title='Source camera for this OSD element';sel.add(new Option('AUTO','0'));for(const c of cameraList())sel.add(new Option(optionText(c),String(c.number)));sel.value=String(p.source||0);
 for(const ev of ['pointerdown','mousedown','click'])sel.addEventListener(ev,e=>e.stopPropagation());
 sel.addEventListener('change',e=>{e.stopPropagation();const previous=p.source||0,next=Number(sel.value)||0,builder=chip.closest('.fps-builder');if(!changeSource(input,index,next)){sel.value=String(previous);sel.title='That element/source is already in this OSD line.';return}sel.title='Source camera for this OSD element';redecorateAfterLegacyRefresh(builder)});
 return sel
}
function decorate(builder){
 if(!builder)return;const tools=builder.querySelector('.fps-multiosd-builder-tools');if(tools)tools.style.display='none';
 const input=builder.querySelector('input[id],textarea[id]');const chipBox=builder.querySelector('.fps-chips');if(!input||!chipBox)return;
 const toks=input.value.match(TOKEN_RE)||[],chips=[...chipBox.querySelectorAll('button')];
 chips.forEach((chip,i)=>{const p=parse(toks[i]);if(!p)return;if(!advanced()){if(chip.dataset.fpsElementSource==='1'){chip.dataset.fpsElementSource='0';chip.dataset.fpsSourceSig='';chip.textContent=(FIELD_NAME[p.base]||p.base)+' ×'}return}
 const sig=`${p.base}|${p.source}|${cameraList().map(c=>`${c.number}:${c.label||''}:${c.connected?1:0}`).join(',')}`;
 const current=chip.querySelector('.fps-element-source');if(chip.dataset.fpsElementSource==='1'&&current){if(current.value!==String(p.source||0))current.value=String(p.source||0);if(chip.dataset.fpsSourceSig===sig)return}
 chip.dataset.fpsElementSource='1';chip.dataset.fpsSourceSig=sig;chip.textContent='';
 const name=document.createElement('span');name.className='fps-element-name';name.textContent=(FIELD_NAME[p.base]||p.base)+' · ';
 const close=document.createElement('span');close.className='fps-element-remove';close.textContent=' ×';
 chip.append(name,buildSelect(chip,input,i,p),close)
 })
}
function seedPreview(){if(!advanced())return;const api=window.fpsMultiCamOsd,map=api?.cameras;if(!map?.get)return;document.querySelectorAll('#fpsMultiOsdSimulator .fps-multiosd-cam').forEach(card=>{if(card.dataset.fpsPreviewSeeded==='1')return;const n=Number(card.dataset.cam),d=PREVIEW_DEFAULTS[n];if(!d)return;const c=map.get(n);if(!c)return;const batt=card.querySelector('input[data-k="battery"]'),remain=card.querySelector('input[data-k="remain"]');if(Number(batt?.value)===0&&Number(c.battery)===0){c.battery=d.battery;batt.value=String(d.battery)}if(Number(remain?.value)===0&&Number(c.remain)===0){c.remain=d.remain;remain.value=String(d.remain)}if(remain){remain.max='999';remain.title='Time remaining supports up to 999 minutes (3 digits).'}card.dataset.fpsPreviewSeeded='1'});api?.refresh?.()}
function refresh(){document.querySelectorAll('.fps-builder').forEach(decorate);seedPreview()}
let queued=false;const queue=()=>{if(queued)return;queued=true;requestAnimationFrame(()=>{queued=false;refresh()})};
new MutationObserver(queue).observe(document.documentElement,{childList:true,subtree:true,characterData:true});
document.addEventListener('input',queue,true);document.addEventListener('change',queue,true);window.addEventListener('fps-camera-registry-update',queue);window.addEventListener('fps-config-read-complete',queue);setInterval(refresh,250);queue();
})();
