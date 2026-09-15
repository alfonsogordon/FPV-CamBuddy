(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;

const TOKEN_RE=/\{[^}]+\}/g;
const FIELD_NAME={state:'Status',stateonly:'Status',batt:'Battery',recdur:'Recording Duration',mode:'Mode',res:'Resolution',fps:'FPS',eis:'Stabilisation',rectf:'Time Remaining',rcap:'Storage Remaining'};
const PREVIEW_DEFAULTS={1:{battery:82,remain:41},2:{battery:69,remain:74},3:{battery:76,remain:32},4:{battery:61,remain:55}};
const advanced=()=>!!window.fpsMultiCamOsd?.advancedOn?.();

function parse(tok){
 const raw=String(tok||'').replace(/^\{|\}$/g,'');
 const m=raw.match(/^([a-z0-9]+)(?:@(\d+)([nt]))?$/i);
 return m?{base:m[1],source:Number(m[2])||0,mode:m[3]||null}:null;
}
function cameraList(){
 const api=window.fpsMultiCamOsd,list=api?.cameraList?.();
 if(Array.isArray(list)&&list.length)return list;
 const map=api?.cameras;
 return map?.values?[...map.values()].sort((a,b)=>(a.number||0)-(b.number||0)):[];
}
function optionText(c){
 const tag=String(c?.label||'').trim();
 return `C${c.number}${tag?' · '+tag:''}${c.connected?' · connected':''}`;
}
function token(base,source){
 if(!source)return `{${base}}`;
 const mode=localStorage.getItem('fpsAdvancedMultiCamOsdIdentifier')==='tag'?'t':'n';
 return `{${base}@${source}${mode}}`;
}
function getTokens(input){return input.value.match(TOKEN_RE)||[]}
function setTokens(input,toks){
 input.value=toks.join(' ');
 input.dispatchEvent(new Event('input',{bubbles:true}));
 input.dispatchEvent(new Event('change',{bubbles:true}));
}
function changeSource(input,index,source){
 const toks=getTokens(input),p=parse(toks[index]);
 if(!p)return false;
 const next=token(p.base,source);
 if(toks.some((t,j)=>j!==index&&t===next))return false;
 toks[index]=next;setTokens(input,toks);return true;
}
function removeToken(input,index){
 const toks=getTokens(input);if(index<0||index>=toks.length)return;
 toks.splice(index,1);setTokens(input,toks);
}
function cameraSig(){return cameraList().map(c=>`${c.number}:${c.label||''}:${c.connected?1:0}`).join('|')}
function refillSelect(sel,source){
 const sig=cameraSig(),wanted=String(source||0);
 if(sel.dataset.cameraSig!==sig){
  sel.innerHTML='';sel.add(new Option('AUTO','0'));
  for(const c of cameraList())sel.add(new Option(optionText(c),String(c.number)));
  sel.dataset.cameraSig=sig;
 }
 if([...sel.options].some(o=>o.value===wanted))sel.value=wanted;
 sel.dataset.source=wanted;
}
function buildSelect(input,index,p){
 const sel=document.createElement('select');
 sel.className='fps-element-source';sel.title='Source camera for this OSD element';sel.dataset.index=String(index);refillSelect(sel,p.source);
 sel.addEventListener('change',e=>{
  e.stopPropagation();
  const previous=Number(sel.dataset.source)||0,next=Number(sel.value)||0;
  if(!changeSource(input,Number(sel.dataset.index),next)){
   sel.value=String(previous);sel.title='That element/source is already in this OSD line.';return;
  }
  sel.dataset.source=String(next);sel.title='Source camera for this OSD element';
  requestAnimationFrame(()=>refreshBuilder(sel.closest('.fps-builder')));
 });
 return sel;
}
function buildRemove(input,wrap){
 const close=document.createElement('span');
 close.className='fps-element-remove';close.textContent='×';close.title='Remove OSD element';close.setAttribute('role','button');close.tabIndex=0;
 const remove=()=>removeToken(input,Number(wrap.dataset.index));
 close.addEventListener('click',e=>{e.preventDefault();e.stopPropagation();remove()});
 close.addEventListener('keydown',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();remove()}});
 return close;
}
function makeChip(input,index,p){
 const wrap=document.createElement('span');wrap.className='fps-element-chip-wrap';wrap.dataset.index=String(index);
 const label=document.createElement('span');label.className='fps-element-label';label.textContent=FIELD_NAME[p.base]||p.base;
 wrap.append(label,buildSelect(input,index,p),buildRemove(input,wrap));
 return wrap;
}
function restoreLegacy(chipBox,input){
 const toks=getTokens(input);
 chipBox.replaceChildren(...toks.map((t,i)=>{
  const p=parse(t),b=document.createElement('button');b.type='button';b.textContent=(FIELD_NAME[p?.base]||p?.base||'Element')+' ×';b.addEventListener('click',()=>removeToken(input,i));return b;
 }));
}
function disableLegacyBuilder(builder){
 const tools=builder.querySelector('.fps-multiosd-builder-tools');
 if(!tools)return;
 const legacySelect=tools.querySelector('select');
 if(legacySelect)legacySelect.value='0';
 tools.remove();
 builder.dataset.fpsPerElementSources='1';
}
function refreshBuilder(builder){
 if(!builder)return;
 disableLegacyBuilder(builder);
 const input=builder.querySelector('input[id],textarea[id]'),chipBox=builder.querySelector('.fps-chips');
 if(!input||!chipBox)return;
 const toks=getTokens(input);
 if(!advanced()){
  if(chipBox.querySelector('.fps-element-chip-wrap'))restoreLegacy(chipBox,input);
  return;
 }
 const existing=[...chipBox.querySelectorAll(':scope > .fps-element-chip-wrap')];
 const cleanStructure=existing.length===toks.length&&[...chipBox.children].every(n=>n.classList?.contains('fps-element-chip-wrap'));
 if(!cleanStructure){
  chipBox.replaceChildren(...toks.map((t,i)=>{const p=parse(t);return p?makeChip(input,i,p):document.createTextNode('')}));
  return;
 }
 existing.forEach((wrap,i)=>{
  const p=parse(toks[i]);if(!p)return;
  wrap.dataset.index=String(i);
  const label=wrap.querySelector(':scope > .fps-element-label');if(label)label.textContent=FIELD_NAME[p.base]||p.base;
  const sel=wrap.querySelector(':scope > .fps-element-source');if(sel){sel.dataset.index=String(i);refillSelect(sel,p.source)}
 });
}
function syncStatusOptions(){
 const sec=document.querySelector('.fps-status-shared');if(!sec)return;
 const labels=[...sec.querySelectorAll('.fps-toggle-row strong')];
 const recOnly=labels.find(x=>/REC only when armed|Show only status during recording/i.test(x.textContent||''));
 if(recOnly)recOnly.textContent='Show only status during recording';
 const recDesc=recOnly?.parentElement?.querySelector('.fps-inline-desc');
 if(recDesc)recDesc.textContent='When enabled, camera status is shown only while the quad is armed and the camera confirms recording.';
 if(!advanced())return;
 const legacy=document.getElementById('fpsBfMode')?.value==='legacy';
 const ids=legacy?['pilotTpl','craftTpl']:['osd1','osd2','osd3','osd4'];
 const enabled=legacy?[document.getElementById('fpsPilotMaster')?.checked,document.getElementById('fpsCraftMaster')?.checked]:[1,2,3,4].map(n=>document.getElementById('fpsMsg'+n)?.checked);
 const used=ids.some((id,i)=>enabled[i]&&/\{state(?:only)?(?:@\d+[nt])?\}/i.test(String(document.getElementById(id)?.value||'')));
 sec.style.display=document.getElementById('fpsOsdMaster')?.checked&&used?'':'none';
}
function seedPreview(syncConnection=false){
 if(!advanced())return;
 const api=window.fpsMultiCamOsd,map=api?.cameras;if(!map?.get)return;
 let changed=false;
 document.querySelectorAll('#fpsMultiOsdSimulator .fps-multiosd-cam').forEach(card=>{
  const n=Number(card.dataset.cam),d=PREVIEW_DEFAULTS[n],c=map.get(n);if(!d||!c)return;
  const connected=card.querySelector('input[data-k="connected"]'),rec=card.querySelector('input[data-k="recording"]'),batt=card.querySelector('input[data-k="battery"]'),remain=card.querySelector('input[data-k="remain"]');
  if(syncConnection&&connected&&connected.checked!==!!c.connected){connected.checked=!!c.connected;changed=true}
  if(card.dataset.fpsPreviewSeeded!=='1'){
   if(!Number.isFinite(Number(c.battery))||Number(c.battery)<=0){c.battery=d.battery;if(batt)batt.value=String(d.battery);changed=true}else if(batt&&Number(batt.value)<=0)batt.value=String(c.battery);
   if(!Number.isFinite(Number(c.remain))||Number(c.remain)<=0){c.remain=d.remain;if(remain)remain.value=String(d.remain);changed=true}else if(remain&&Number(remain.value)<=0)remain.value=String(c.remain);
   card.dataset.fpsPreviewSeeded='1';
  }
  const isConnected=!!connected?.checked;card.classList.toggle('fps-preview-cam-off',!isConnected);
  if(rec)rec.disabled=!isConnected;if(batt)batt.disabled=!isConnected;
  if(remain){remain.disabled=!isConnected;remain.max='999';remain.title='Time remaining supports up to 999 minutes (3 digits).'}
  const state=card.querySelector('.fps-multiosd-cam-state');if(state)state.textContent=isConnected?'LIVE':'OFF';
 });
 if(changed)requestAnimationFrame(()=>api?.refresh?.());
}
function ensureStyle(){
 if(document.getElementById('fpsElementSourceUiStyle'))return;
 const s=document.createElement('style');s.id='fpsElementSourceUiStyle';s.textContent=`
 .fps-element-chip-wrap{display:inline-grid;grid-template-columns:minmax(92px,1fr) minmax(108px,1fr) 28px;align-items:stretch;border:1px solid #44445d;border-radius:999px;overflow:hidden;background:#151522;min-height:40px}
 .fps-element-label{display:flex;align-items:center;padding:0 14px;white-space:nowrap;color:#f2f2f6;border-right:1px solid #6d7040}
 .fps-element-chip-wrap .fps-element-source{display:block!important;width:100%!important;min-width:108px!important;max-width:none!important;margin:0!important;padding:0 30px 0 14px!important;border:0!important;border-radius:0!important;background:#3b4128!important;color:#f5ff00!important;box-shadow:none!important;font:inherit!important;font-size:12px!important;font-weight:800!important;cursor:pointer!important;outline:none!important}
 .fps-element-chip-wrap .fps-element-source:hover,.fps-element-chip-wrap .fps-element-source:focus{background:#454d2d!important;color:#ffff7a!important}
 .fps-element-remove{display:flex!important;align-items:center!important;justify-content:center!important;width:28px!important;min-width:28px!important;border-left:1px solid #4d5133!important;background:#22251c!important;color:#f2f2f6!important;cursor:pointer!important;user-select:none}
 .fps-element-remove:hover,.fps-element-remove:focus{background:#343827!important;color:#f5ff00!important;outline:none!important}
 #fpsMultiOsdSimulator .fps-preview-cam-off .fps-multiosd-toggle:not(:first-child),#fpsMultiOsdSimulator .fps-preview-cam-off .fps-multiosd-value-row{opacity:.38}
 #fpsMultiOsdSimulator .fps-preview-cam-off input:disabled{cursor:not-allowed}
 `;document.head.appendChild(s);
}
function refresh(){ensureStyle();document.querySelectorAll('.fps-builder').forEach(refreshBuilder);syncStatusOptions();seedPreview(false)}
let queued=false;const queue=()=>{if(queued)return;queued=true;requestAnimationFrame(()=>{queued=false;refresh()})};
new MutationObserver(queue).observe(document.documentElement,{childList:true,subtree:true});
document.addEventListener('input',queue,true);
function registrySync(){setTimeout(()=>{seedPreview(true);queue()},0)}
document.addEventListener('fps-camera-registry-update',registrySync);
document.addEventListener('fps-demo-camera-connections',registrySync);
document.addEventListener('fps-config-read-complete',registrySync);
window.addEventListener('fps-camera-registry-update',registrySync);
window.addEventListener('fps-config-read-complete',registrySync);
setInterval(()=>{syncStatusOptions();seedPreview(false)},1000);
queue();
})();
