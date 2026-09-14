(()=>{
'use strict';
const MAX_LABEL=7;
const $=id=>document.getElementById(id);
const slots=new Map();
const familyLive=new Map();
let cameras=[];
let selectedNumber=0;

function clean(v){return String(v||'').replace(/[\[\]\x00-\x1f\x7f]/g,'').trim().replace(/\s+/g,' ').slice(0,MAX_LABEL)}
function normAddr(v){return String(v||'').trim().toLowerCase()}
function metaFor(c){
 const tags=String(c?.tags||'');
 const n=tags.match(/\[cam=(\d+)\]/);
 const l=tags.match(/\[label=([^\]]*)\]/);
 return {number:n?parseInt(n[1],10):(Number(c?.idx)||0)+1,label:clean(l?l[1]:'')};
}
function displayName(c){const m=metaFor(c);return m.label||('C'+m.number)}
function connectedAddresses(){
 const out=new Set();
 for(const s of slots.values())if(s.ready&&s.addr)out.add(normAddr(s.addr));
 for(const [addr,on] of familyLive)if(on&&addr)out.add(normAddr(addr));
 return out;
}
function slotForAddress(addr){const a=normAddr(addr);for(const s of slots.values())if(s.ready&&normAddr(s.addr)===a)return s;return null}
function publishRegistry(){
 const live=connectedAddresses();
 const detail=cameras.map(c=>{const m=metaFor(c),s=slotForAddress(c.addr);return {idx:c.idx,number:m.number,label:m.label,name:c.name,addr:c.addr,type:c.type,connected:live.has(normAddr(c.addr)),battery:s?.battery??null,remain:s?.remain??null,recording:s?.recording??null,hot:s?.hot??null}});
 document.dispatchEvent(new CustomEvent('fps-camera-registry-update',{detail}));
 refreshSelector(detail);
}
function style(){if($('fpsMultiCamUiStyle'))return;const s=document.createElement('style');s.id='fpsMultiCamUiStyle';s.textContent=`
.fps-cam-live-status{white-space:nowrap;min-width:92px}.fps-cam-live-badge{display:inline-flex;align-items:center;gap:5px;padding:3px 7px;border-radius:999px;border:1px solid #495365;background:rgba(73,83,101,.12);color:#8b95a7;font-size:9px;font-weight:900;letter-spacing:.04em;text-transform:uppercase}.fps-cam-live-badge::before{content:'';width:7px;height:7px;border-radius:50%;background:#667085}.fps-cam-live-badge.connected{background:#dcfce7;color:#15803d;border-color:#86efac;box-shadow:0 0 7px rgba(34,197,94,.16)}.fps-cam-live-badge.connected::before{background:#22c55e;box-shadow:0 0 5px rgba(34,197,94,.7)}
.fps-cam-live-value{white-space:nowrap;font-size:10px;color:#cbd5e1}.fps-cam-live-value.hot{color:#f87171;font-weight:800}.fps-cam-live-value.rec{color:#fca5a5;font-weight:800}
#fpsPreviewWarnCameraRow{display:none}#fpsPreviewWarnCameraRow select{min-width:125px;max-width:180px}
#fpsPreviewWarnCameraHint{grid-column:1/-1;font-size:10px;line-height:1.35;color:#a78bfa;margin-top:-3px}
#fpsPreviewWarningSourceRow{display:none!important}
.fps-demo-active #fpsPreviewCameraCountRow{display:none!important}
`;document.head.appendChild(s)}
function ensureLiveColumns(table){
 const head=table?.querySelector('thead tr');if(!head)return;
 const cols=[['fps-cam-live-head','Status'],['fps-cam-batt-head','Battery'],['fps-cam-remain-head','Remain'],['fps-cam-state-head','State']];
 for(const [cls,label] of cols){if(!head.querySelector('.'+cls)){const th=document.createElement('th');th.className=cls;th.textContent=label;head.insertBefore(th,head.lastElementChild)}}
 const rows=[...table.querySelectorAll('tbody tr')];
 rows.forEach(row=>{
  const defs=[['fps-cam-live-status','<span class="fps-cam-live-badge">offline</span>'],['fps-cam-live-battery','<span class="fps-cam-live-value">—</span>'],['fps-cam-live-remain','<span class="fps-cam-live-value">—</span>'],['fps-cam-live-state','<span class="fps-cam-live-value">—</span>']];
  for(const [cls,html] of defs){if(!row.querySelector('.'+cls)){const td=document.createElement('td');td.className=cls;td.innerHTML=html;row.insertBefore(td,row.lastElementChild)}}
 });
}
function fmtRemain(sec){if(sec==null||sec<0)return '—';const m=Math.floor(sec/60),s=sec%60;return m>=60?`${Math.floor(m/60)}h ${m%60}m`:`${m}:${String(s).padStart(2,'0')}`}
function updateBadges(){
 const live=connectedAddresses();
 const table=document.querySelector('#camTableWrap .cam-table');if(!table){publishRegistry();return}
 ensureLiveColumns(table);
 const rows=[...table.querySelectorAll('tbody tr')];
 rows.forEach((row,i)=>{
  const c=cameras[i];if(!c)return;
  const s=slotForAddress(c.addr),on=live.has(normAddr(c.addr));
  const statusCell=row.querySelector('.fps-cam-live-status');if(statusCell){let badge=statusCell.querySelector('.fps-cam-live-badge');if(!badge){badge=document.createElement('span');badge.className='fps-cam-live-badge';statusCell.appendChild(badge)}badge.classList.toggle('connected',on);badge.textContent=on?'connected':'offline'}
  const batt=row.querySelector('.fps-cam-live-battery .fps-cam-live-value');if(batt)batt.textContent=on&&s?.battery!=null&&s.battery>=0?`${s.battery}%`:'—';
  const rem=row.querySelector('.fps-cam-live-remain .fps-cam-live-value');if(rem)rem.textContent=on&&s?fmtRemain(s?.remain):'—';
  const state=row.querySelector('.fps-cam-live-state .fps-cam-live-value');if(state){state.classList.toggle('rec',!!(on&&s?.recording===1));state.classList.toggle('hot',!!(on&&s?.hot===1));state.textContent=!on?'—':s?.hot===1?'HOT':s?.recording===1?'REC':s?.recording===0?'RDY':'LIVE'}
 });
 publishRegistry();
}
function consume(line){
 const t=String(line||'');let m;
 m=t.match(/^\[MULTI\] Slot (\d+) found .* \(([^)]+)\) rssi=/);
 if(m){const n=Number(m[1]);const s=slots.get(n)||{};s.addr=normAddr(m[2]);slots.set(n,s);return}
 m=t.match(/^\[MULTI\] Slot (\d+) status registration .*-> READY/);
 if(m){const n=Number(m[1]);const s=slots.get(n)||{};s.ready=true;slots.set(n,s);updateBadges();return}
 m=t.match(/^\[MULTI\] Slot (\d+) keep-alive addr=(\S+)/);
 if(m){const n=Number(m[1]);const s=slots.get(n)||{};s.addr=normAddr(m[2]);s.ready=true;slots.set(n,s);updateBadges();return}
 m=t.match(/^\[MULTI\] LIVE slot=(\d+) addr=(\S+) bat=(-?\d+) remain=(-?\d+) rec=(-?\d+) hot=(-?\d+)/);
 if(m){const n=Number(m[1]);const s=slots.get(n)||{};s.addr=normAddr(m[2]);s.ready=true;s.battery=Number(m[3]);s.remain=Number(m[4]);s.recording=Number(m[5]);s.hot=Number(m[6]);slots.set(n,s);updateBadges();return}
 m=t.match(/^\[MULTI\] GoPro slot (\d+) disconnected/);
 if(m){const n=Number(m[1]);const s=slots.get(n)||{};s.ready=false;slots.set(n,s);updateBadges();return}
 m=t.match(/^\[MULTI\] FAMILY LIVE family=(\d+) addr=(.+) connected=([01])$/);
 if(m){familyLive.set(normAddr(m[2]),m[3]==='1');updateBadges();return}
}
function scanExistingLogs(){document.querySelectorAll('#terminal .line').forEach(n=>consume(n.textContent))}
function watchLogs(){const t=$('terminal');if(!t)return;scanExistingLogs();new MutationObserver(ms=>ms.forEach(mu=>mu.addedNodes.forEach(n=>{if(n.nodeType===1&&n.classList?.contains('line'))consume(n.textContent)}))).observe(t,{childList:true})}
function hookCameraTable(){
 if(typeof window.renderCameraTable!=='function'||window.renderCameraTable.__fpsMultiUi)return false;
 const original=window.renderCameraTable;
 const wrapped=function(list){cameras=Array.isArray(list)?list.slice():[];const r=original.apply(this,arguments);setTimeout(()=>{const table=document.querySelector('#camTableWrap .cam-table');ensureLiveColumns(table);scanExistingLogs();updateBadges()},35);return r};
 wrapped.__fpsMultiUi=true;window.renderCameraTable=wrapped;return true;
}
function syncPrimaryWarningSource(number){
 const primary=$('fpsPreviewWarningSource');if(!primary)return;
 const wanted=String(Math.max(1,Number(number)||1));
 if([...primary.options].some(o=>o.value===wanted)){
  primary.value=wanted;
  primary.dispatchEvent(new Event('change',{bubbles:true}));
 }
}
function ensurePreviewControl(){
 const sim=document.querySelector('#fpsIntegratedPreview .fps-preview-sim');if(!sim||$('fpsPreviewWarnCameraRow'))return;
 const row=document.createElement('label');row.id='fpsPreviewWarnCameraRow';row.innerHTML='<span>Warning camera</span><select id="fpsPreviewWarnCamera"><option value="0">Camera 1</option></select>';
 const hint=document.createElement('div');hint.id='fpsPreviewWarnCameraHint';hint.textContent='Select which saved camera should be blamed for simulated BATT LOW / REC LOW / CAM HOT warnings.';
 sim.append(row,hint);$('fpsPreviewWarnCamera').addEventListener('change',e=>{selectedNumber=Number(e.target.value)||1;syncPrimaryWarningSource(selectedNumber)});
 refreshSelector();
}
function refreshSelector(detail){
 const sel=$('fpsPreviewWarnCamera'),row=$('fpsPreviewWarnCameraRow');if(!sel)return;
 const list=Array.isArray(detail)?detail:cameras.map(c=>{const m=metaFor(c);return {number:m.number,label:m.label,connected:connectedAddresses().has(normAddr(c.addr))}});
 const previous=selectedNumber||Number(sel.value)||0;sel.innerHTML='';
 if(list.length){for(const c of list){const o=document.createElement('option');o.value=String(c.number);o.textContent=`C${c.number}${c.label?' · '+c.label:''}${c.connected?' · connected':''}`;sel.appendChild(o)}sel.value=[...sel.options].some(o=>Number(o.value)===previous)?String(previous):sel.options[0].value;selectedNumber=Number(sel.value)||1}else{sel.add(new Option('C1','1'));selectedNumber=1}
 syncPrimaryWarningSource(selectedNumber);
 const multi=!!$('fpsMultiCamSync')?.checked||localStorage.getItem('fpsExperimentalMultiCamSync')==='1';if(row)row.style.display=multi?'flex':'none';
}
function selectedSuffix(){
 const c=cameras.map(c=>({c,m:metaFor(c)})).find(x=>x.m.number===selectedNumber);if(c)return c.m.label||('CAM'+c.m.number);
 return selectedNumber?('CAM'+selectedNumber):'';
}
function patchPreviewWarnings(){
 const suffix=selectedSuffix();
 if(suffix){
  document.querySelectorAll('#fpsPreviewRows .fps-preview-line').forEach(line=>{
   const raw=line.textContent.trim();
   const m=raw.match(/^(BATT LOW|REC LOW|CAM HOT)(?:\s+.*)?$/);
   if(m)line.textContent=(m[1]+' '+suffix).slice(0,16);
  });
 }
 requestAnimationFrame(patchPreviewWarnings);
}
function init(){style();watchLogs();let tries=0;const t=setInterval(()=>{ensurePreviewControl();const hooked=hookCameraTable();if((hooked||window.renderCameraTable?.__fpsMultiUi)&&$('fpsIntegratedPreview')){clearInterval(t);scanExistingLogs();setTimeout(updateBadges,50)}else if(++tries>60)clearInterval(t)},100);document.addEventListener('fps-camera-registry-update',e=>refreshSelector(e.detail));document.addEventListener('change',e=>{if(e.target?.id==='fpsMultiCamSync'||e.target?.id==='fpsExperimentalMaster')setTimeout(()=>refreshSelector(),0)},true);requestAnimationFrame(patchPreviewWarnings)}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();