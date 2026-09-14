(()=>{
'use strict';
const MODE='freeclinkerDemoMode', CFG='freeclinkerDemoConfig', STORE='fpsUiV2State', LABELS='freeclinkerDemoCameraLabels', CONNECTIONS='freeclinkerDemoCameraConnections';
const demo=()=>localStorage.getItem(MODE)==='1';
const multi=()=>!!document.getElementById('fpsMultiCamSync')?.checked||localStorage.getItem('fpsExperimentalMultiCamSync')==='1';
const dummyCameras=[
 {idx:0,name:'GoPro HERO11 Black Mini',addr:'DE:MO:00:00:00:01',type:'GoPro',tags:'[cam=1][label=FRONT]'},
 {idx:1,name:'GoPro MAX2',addr:'DE:MO:00:00:00:02',type:'GoPro',tags:'[cam=2][label=REAR]'},
 {idx:2,name:'DJI Osmo Action 5 Pro',addr:'DE:MO:00:00:00:03',type:'DJI',tags:'[cam=3][label=]'},
 {idx:3,name:'Insta360 X5',addr:'DE:MO:00:00:00:04',type:'Insta360',tags:'[cam=4][label=360CAM]'}
];
let connected=new Set();
function parse(k){try{return JSON.parse(localStorage.getItem(k)||'{}')}catch{return{}}}
function cleanLabel(v){return String(v||'').replace(/[\[\]\x00-\x1f\x7f]/g,'').trim().replace(/\s+/g,' ').slice(0,7)}
function setCameraLabel(cam,label){const clean=cleanLabel(label),number=cam.idx+1;cam.tags=`[cam=${number}][label=${clean}]`;return clean}
function cameraMeta(cam){const n=String(cam.tags||'').match(/\[cam=(\d+)\]/),l=String(cam.tags||'').match(/\[label=([^\]]*)\]/);return {number:n?Number(n[1]):cam.idx+1,label:cleanLabel(l?l[1]:'')}}
function loadDummyLabels(){const saved=parse(LABELS);dummyCameras.forEach(cam=>{if(Object.prototype.hasOwnProperty.call(saved,String(cam.idx+1)))setCameraLabel(cam,saved[String(cam.idx+1)])})}
function saveDummyLabel(number,label){const cam=dummyCameras[number-1];if(!cam)return;const clean=setCameraLabel(cam,label),saved=parse(LABELS);saved[String(number)]=clean;localStorage.setItem(LABELS,JSON.stringify(saved))}
function loadConnections(){try{const a=JSON.parse(localStorage.getItem(CONNECTIONS)||'[]');connected=new Set(Array.isArray(a)?a.map(Number).filter(n=>n>=1&&n<=dummyCameras.length):[])}catch{connected=new Set()}}
function saveConnections(){localStorage.setItem(CONNECTIONS,JSON.stringify([...connected].sort((a,b)=>a-b)))}
function enter(){const current=parse(STORE);if(Object.keys(current).length)localStorage.setItem(CFG,JSON.stringify(current));localStorage.setItem(MODE,'1');location.reload()}
function exit(){const current=parse(STORE);if(Object.keys(current).length)localStorage.setItem(CFG,JSON.stringify(current));localStorage.removeItem(MODE);location.reload()}
function registryDetail(){return dummyCameras.map(cam=>{const m=cameraMeta(cam),on=connected.has(m.number);return {idx:cam.idx,number:m.number,label:m.label,name:cam.name,addr:cam.addr,type:cam.type,connected:on,battery:on?69:null,remain:on?2700:null,recording:on?0:null,hot:on?0:null}})}
function publishDemoConnections(){
 const detail=registryDetail();
 document.dispatchEvent(new CustomEvent('fps-demo-camera-connections',{detail:{connected:[...connected],cameras:detail}}));
 document.dispatchEvent(new CustomEvent('fps-camera-registry-update',{detail}));
 const count=connected.size,c=document.getElementById('fpsPreviewCameraCount');
 if(c){for(let i=0;i<=4;i++)if(![...c.options].some(o=>Number(o.value)===i))c.add(new Option(String(i),String(i)));c.value=String(count);c.dispatchEvent(new Event('change',{bubbles:true}))}
 const note=document.getElementById('fpsPreviewNotice');if(note)note.textContent=`Demo cameras connected: ${count}`;
}
function patchDemoRows(){
 if(!demo())return;
 const table=document.querySelector('#camTableWrap .cam-table');if(!table)return;
 const on=multi();
 [...table.querySelectorAll('tbody tr')].forEach((row,i)=>{
  const number=i+1,locked=i>0&&!on,isOn=connected.has(number);
  row.classList.toggle('fps-demo-multi-locked',locked);row.classList.toggle('fps-demo-connected',isOn);
  row.querySelectorAll('input,select,button').forEach(el=>{if(!el.closest('.cam-actions')||el.textContent.trim()!=='Disconnect')el.disabled=locked});
  row.title=locked?'Enable Multi Cam to use additional demo cameras.':'';
  const actions=row.querySelector('.cam-actions');if(actions){const b=[...actions.querySelectorAll('button')].find(x=>(x.getAttribute('onclick')||'').includes('camConnect')||x.dataset.fpsDemoConnect==='1');if(b){b.dataset.fpsDemoConnect='1';b.textContent=isOn?'Disconnect':'Connect';b.disabled=locked}}
  const badge=row.querySelector('.fps-cam-live-badge');if(badge){badge.classList.toggle('connected',isOn);badge.textContent=isOn?'connected':'offline'}
  const batt=row.querySelector('.fps-cam-live-battery .fps-cam-live-value');if(batt)batt.textContent=isOn?'69%':'—';
  const rem=row.querySelector('.fps-cam-live-remain .fps-cam-live-value');if(rem)rem.textContent=isOn?'45:00':'—';
  const state=row.querySelector('.fps-cam-live-state .fps-cam-live-value');if(state)state.textContent=isOn?'RDY':'—';
 });
}
function patchDemoPreview(){
 if(!demo())return;
 const n=connected.size;
 const badge=document.getElementById('fpsPreviewState');
 if(badge&&n>1){const base=badge.textContent.replace(/\s*\(\d+\)$/,'');badge.textContent=`${base} (${n})`}
 document.querySelectorAll('#fpsPreviewRows .fps-preview-line').forEach(line=>{
  let t=line.textContent;
  if(n>1)t=t.replace(/\((\d+)\)/g,`(${n})`).replace(/\bREC\s+\d+\/\d+\b/g,`REC ${n}/${n}`);
  line.textContent=t;
 });
 patchDemoRows();
 requestAnimationFrame(patchDemoPreview);
}
function applyDummyCameraState(){
 if(!demo())return;
 if(!multi()){for(const n of [...connected])if(n>1)connected.delete(n);saveConnections()}
 patchDemoRows();publishDemoConnections();
}
function renderDummyCameras(){if(!demo()||typeof window.renderCameraTable!=='function')return false;window.renderCameraTable(dummyCameras.map(c=>({...c})));setTimeout(applyDummyCameraState,60);return true}
function hookDummyCameras(){
 if(!demo()||typeof window.renderCameraTable!=='function')return false;
 if(window.renderCameraTable.__fpsDemoDummy)return true;
 const original=window.renderCameraTable;
 const wrapped=function(){return original.call(this,dummyCameras.map(c=>({...c})))};
 wrapped.__fpsDemoDummy=true;window.renderCameraTable=wrapped;renderDummyCameras();return true;
}
function rowIndexForButton(button){const row=button.closest('tbody tr'),table=button.closest('.cam-table');if(!row||!table)return-1;return [...table.querySelectorAll('tbody tr')].indexOf(row)}
function handleDemoConnect(e){
 if(!demo())return;
 const button=e.target?.closest?.('.cam-actions button');if(!button)return;
 const isConnect=(button.getAttribute('onclick')||'').includes('camConnect')||button.dataset.fpsDemoConnect==='1';if(!isConnect)return;
 const idx=rowIndexForButton(button);if(idx<0||idx>0&&!multi())return;
 e.preventDefault();e.stopImmediatePropagation();
 const number=idx+1;if(connected.has(number))connected.delete(number);else connected.add(number);saveConnections();patchDemoRows();publishDemoConnections();
}
function handleDemoLabelSave(e){
 if(!demo())return;
 const button=e.target?.closest?.('.fps-cam-label-edit button');if(!button)return;
 const row=button.closest('tbody tr'),table=button.closest('.cam-table');if(!row||!table)return;
 const rows=[...table.querySelectorAll('tbody tr')],idx=rows.indexOf(row);if(idx<0||idx>0&&!multi())return;
 const input=row.querySelector('.fps-cam-label-edit input');if(!input)return;
 e.preventDefault();e.stopImmediatePropagation();
 const number=idx+1,clean=cleanLabel(input.value);input.value=clean;saveDummyLabel(number,clean);button.textContent='Saved ✓';setTimeout(()=>{renderDummyCameras();publishDemoConnections()},220);
}
function init(){
 const connect=document.getElementById('connectBtn');if(!connect)return;
 const b=document.createElement('button');b.type='button';b.id='fpsDemoModeBtn';b.className='fps-demo-v1';b.textContent=demo()?'Exit Demo Mode':'Demo Mode';b.title=demo()?'Return to real ESP32 connection mode':'Try all configurator settings without a connected board';b.onclick=demo()?exit:enter;connect.insertAdjacentElement('afterend',b);
 if(!demo())return;
 loadDummyLabels();loadConnections();
 document.documentElement.classList.add('fps-demo-active');document.body.classList.add('fps-demo-active');
 const frame=document.createElement('div');frame.className='fps-demo-frame';frame.setAttribute('aria-hidden','true');document.body.appendChild(frame);
 const badge=document.createElement('div');badge.className='fps-demo-v1-banner';badge.innerHTML='<strong>DEMO MODE</strong><span>Virtual C3 — settings are saved locally. No commands are sent to hardware.</span>';const panel=document.getElementById('panel-config');if(panel)panel.prepend(badge);
 panel?.querySelectorAll('input,select,button').forEach(e=>{if(e.id!=='connectBtn')e.disabled=false});
 ['applyBtn','auxApplyBtn','osdApplyBtn','bf45ApplyBtn'].forEach(id=>{const e=document.getElementById(id);if(e){e.disabled=false;e.textContent='Saved in Demo';e.onclick=ev=>{ev.preventDefault();ev.stopImmediatePropagation();localStorage.setItem(CFG,localStorage.getItem(STORE)||'{}');const old=e.textContent;e.textContent='Saved ✓';setTimeout(()=>e.textContent=old,900)}}});
 let tries=0;const hook=setInterval(()=>{if(hookDummyCameras()||++tries>40)clearInterval(hook)},100);
 document.addEventListener('click',handleDemoConnect,true);document.addEventListener('click',handleDemoLabelSave,true);
 document.addEventListener('change',e=>{if(e.target?.id==='fpsMultiCamSync'||e.target?.id==='fpsExperimentalMaster')setTimeout(()=>{renderDummyCameras();applyDummyCameraState()},30)},true);
 const wrap=document.getElementById('camTableWrap');if(wrap)new MutationObserver(()=>setTimeout(patchDemoRows,0)).observe(wrap,{childList:true,subtree:true});
 setTimeout(()=>{applyDummyCameraState();requestAnimationFrame(patchDemoPreview)},500);
}
const css=document.createElement('style');css.textContent=`.fps-demo-v1{margin-left:8px;border-color:#7c3aed!important}.fps-demo-active .fps-demo-v1{background:#7c3aed!important;color:#fff!important}.fps-demo-v1-banner{margin:0 0 16px;padding:12px 16px;border:1px solid #7c3aed;border-radius:10px;background:rgba(124,58,237,.13);display:flex;align-items:center;gap:12px}.fps-demo-v1-banner strong{color:#c4b5fd;white-space:nowrap}.fps-demo-v1-banner span{font-size:12px;color:var(--muted,#aaa)}.fps-demo-frame{position:fixed;inset:0;z-index:99998;border:4px solid #7c3aed;pointer-events:none;box-sizing:border-box;box-shadow:inset 0 0 22px rgba(124,58,237,.12)}html.fps-demo-active{background-image:linear-gradient(rgba(124,58,237,.035),rgba(124,58,237,.035))}html.fps-demo-active::before{content:'DEMO MODE  •  VIRTUAL C3';position:fixed;z-index:99999;top:0;left:50%;transform:translateX(-50%);padding:3px 14px 5px;background:#7c3aed;color:#fff;border-radius:0 0 9px 9px;font:700 11px/1.2 system-ui,sans-serif;letter-spacing:.08em;pointer-events:none;box-shadow:0 2px 12px rgba(124,58,237,.4)}.fps-demo-active #camTableWrap .fps-demo-multi-locked{opacity:.34;filter:grayscale(1)}.fps-demo-active #camTableWrap .fps-demo-multi-locked td{background:#f3f4f6!important}.fps-demo-active #camTableWrap .fps-demo-multi-locked:hover td{background:#f3f4f6!important}.fps-demo-active #camTableWrap .fps-demo-connected td{background:rgba(34,197,94,.045)}@media(max-width:640px){.fps-demo-frame{border-width:3px}.fps-demo-v1-banner{align-items:flex-start;flex-direction:column;gap:4px}}`;document.head.appendChild(css);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,160));else setTimeout(init,160);
})();
