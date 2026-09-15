(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const TOKEN_RE=/\{[^}]+\}/g;
const FIELD_NAME={state:'Status',stateonly:'Status',batt:'Battery',recdur:'Recording Duration',mode:'Mode',res:'Resolution',fps:'FPS',eis:'Stabilisation',rectf:'Time Remaining',rcap:'Storage Remaining'};
const advanced=()=>!!window.fpsMultiCamOsd?.advancedOn?.();
function parse(tok){const raw=String(tok||'').replace(/^\{|\}$/g,'');const m=raw.match(/^([a-z0-9]+)(?:@(\d+)([nt]))?$/i);return m?{base:m[1],source:Number(m[2])||0,mode:m[3]||null}:null}
function cameraList(){const api=window.fpsMultiCamOsd,list=api?.cameraList?.();if(Array.isArray(list)&&list.length)return list;const map=api?.cameras;if(map?.values)return [...map.values()].sort((a,b)=>(a.number||0)-(b.number||0));return[]}
function optionText(c){const tag=String(c?.label||'').trim();return `C${c.number}${tag?' · '+tag:''}${c.connected?' · connected':''}`}
function token(base,source){if(!source)return `{${base}}`;const mode=localStorage.getItem('fpsAdvancedMultiCamOsdIdentifier')==='tag'?'t':'n';return `{${base}@${source}${mode}}`}
function changeSource(input,index,source){const toks=input.value.match(TOKEN_RE)||[];if(!toks[index])return;const p=parse(toks[index]);if(!p)return;toks[index]=token(p.base,source);input.value=toks.join(' ');input.dispatchEvent(new Event('input',{bubbles:true}));input.dispatchEvent(new Event('change',{bubbles:true}))}
function decorate(builder){
 const tools=builder.querySelector('.fps-multiosd-builder-tools');if(tools)tools.style.display='none';
 const input=builder.querySelector('input[id],textarea[id]');const chipBox=builder.querySelector('.fps-chips');if(!input||!chipBox)return;
 const toks=input.value.match(TOKEN_RE)||[],chips=[...chipBox.querySelectorAll('button')];
 chips.forEach((chip,i)=>{const p=parse(toks[i]);if(!p)return;if(!advanced()){if(chip.dataset.fpsElementSource==='1'){chip.dataset.fpsElementSource='0';chip.textContent=(FIELD_NAME[p.base]||p.base)+' ×'}return}
 const sig=`${p.base}|${p.source}|${cameraList().map(c=>`${c.number}:${c.label||''}:${c.connected?1:0}`).join(',')}`;if(chip.dataset.fpsSourceSig===sig)return;
 chip.dataset.fpsElementSource='1';chip.dataset.fpsSourceSig=sig;chip.textContent='';
 const name=document.createElement('span');name.className='fps-element-name';name.textContent=(FIELD_NAME[p.base]||p.base)+' · ';
 const sel=document.createElement('select');sel.className='fps-element-source';sel.title='Source camera for this OSD element';sel.add(new Option('AUTO','0'));for(const c of cameraList())sel.add(new Option(optionText(c),String(c.number)));sel.value=String(p.source||0);
 const close=document.createElement('span');close.className='fps-element-remove';close.textContent=' ×';
 for(const ev of ['pointerdown','mousedown','click'])sel.addEventListener(ev,e=>e.stopPropagation());sel.addEventListener('change',e=>{e.stopPropagation();changeSource(input,i,Number(sel.value)||0)});
 chip.append(name,sel,close)
 })
}
function refresh(){document.querySelectorAll('.fps-builder').forEach(decorate)}
let queued=false;const queue=()=>{if(queued)return;queued=true;requestAnimationFrame(()=>{queued=false;refresh()})};
new MutationObserver(queue).observe(document.documentElement,{childList:true,subtree:true,characterData:true});
document.addEventListener('input',queue,true);document.addEventListener('change',queue,true);window.addEventListener('fps-camera-registry-update',queue);window.addEventListener('fps-config-read-complete',queue);setInterval(refresh,500);queue();
})();
