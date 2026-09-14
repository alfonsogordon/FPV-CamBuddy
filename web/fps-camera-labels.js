(()=>{
'use strict';
const MAX_LABEL=7;
const MIN_FW='1.0.2';

function cleanLabel(v){
 return String(v||'').replace(/[\[\]\x00-\x1f\x7f]/g,'').trim().replace(/\s+/g,' ').slice(0,MAX_LABEL);
}
function parseMeta(tags,idx){
 const s=String(tags||'');
 const n=s.match(/\[cam=(\d+)\]/);
 const l=s.match(/\[label=([^\]]*)\]/);
 return {number:n?parseInt(n[1],10):idx+1,label:l?l[1]:''};
}
function supportsLabels(){
 const api=window.fpsFirmwareVersion;
 return !!(api&&typeof api.supports==='function'&&api.supports(MIN_FW));
}
function refreshSupportState(){
 const ok=supportsLabels();
 document.querySelectorAll('.fps-cam-label-edit input,.fps-cam-label-edit button').forEach(el=>{
  el.disabled=!ok;
  el.title=ok?'':'Camera labels require V1.0.2 experimental firmware or newer.';
 });
 window.fpsFirmwareVersion?.applyFeatureGates?.();
}
function addStyle(){
 if(document.getElementById('fpsCameraLabelStyle'))return;
 const s=document.createElement('style');s.id='fpsCameraLabelStyle';s.textContent=`
 #fpsCameraLabelsInfo{width:100%;max-width:560px;margin:0 0 10px;padding:11px 13px;border:1px solid #d7c900;border-radius:7px;background:rgba(245,255,0,.07);font-size:11px;line-height:1.45;color:#6f6817}
 #fpsCameraLabelsInfo strong{color:#514b00}.fps-cam-number{font-weight:900;white-space:nowrap}.fps-cam-label-edit{display:flex;align-items:center;gap:5px;min-width:150px}.fps-cam-label-edit input{width:82px;padding:4px 6px;border:1px solid #d1d5db;border-radius:4px;font:12px 'Courier New',monospace;text-transform:none}.fps-cam-label-edit input:disabled{opacity:.38}.fps-cam-label-edit button{padding:4px 7px}.fps-cam-label-preview{display:block;margin-top:3px;color:#8a831e;font-size:9px;white-space:nowrap}
 @media(max-width:700px){.cam-table{font-size:11px}.cam-table th,.cam-table td{padding:8px 7px}.fps-cam-label-edit{min-width:125px}.fps-cam-label-edit input{width:70px}}
 `;document.head.appendChild(s);
}
function ensureInfo(){
 const wrap=document.getElementById('camTableWrap');if(!wrap)return;
 let info=document.getElementById('fpsCameraLabelsInfo');
 if(!info){
  info=document.createElement('div');info.id='fpsCameraLabelsInfo';
  info.innerHTML='<strong>V1.0.2 Camera IDs / OSD labels</strong><br>Each saved hardware ID keeps a stable Cam number. Add an optional label of up to <strong>7 characters</strong> (for example QUAD, CHEST or 360CAM). Seven characters is the maximum that still fits the longest current camera warning: <code>BATT LOW + label</code> inside the 16-character OSD limit.';
  wrap.parentNode.insertBefore(info,wrap);
 }
 window.fpsFirmwareVersion?.applyFeatureGates?.();
}
async function saveLabel(idx,input,button){
 const label=cleanLabel(input.value);input.value=label;
 button.disabled=true;input.disabled=true;
 await window.sendCommand?.(`cameras label ${idx} ${label||'-'}`);
 setTimeout(()=>{window.startCamCapture?.();window.sendCommand?.('cameras list')},180);
}
function enhance(cameras){
 addStyle();ensureInfo();
 const table=document.querySelector('#camTableWrap .cam-table');if(!table)return;
 const head=table.querySelector('thead tr');if(!head||head.dataset.fpsLabels==='1')return;
 head.dataset.fpsLabels='1';
 const hs=[...head.children];
 const camTh=document.createElement('th');camTh.textContent='Cam';head.insertBefore(camTh,hs[1]);
 const labelTh=document.createElement('th');labelTh.textContent='OSD Label';head.insertBefore(labelTh,head.children[3]);
 const ok=supportsLabels();
 const rows=[...table.querySelectorAll('tbody tr')];
 rows.forEach((row,i)=>{
  const c=cameras[i];if(!c)return;
  const meta=parseMeta(c.tags,c.idx);
  const numberTd=document.createElement('td');numberTd.className='fps-cam-number';numberTd.textContent='C'+meta.number;
  row.insertBefore(numberTd,row.children[1]);
  const labelTd=document.createElement('td');
  const edit=document.createElement('div');edit.className='fps-cam-label-edit';
  const input=document.createElement('input');input.type='text';input.maxLength=MAX_LABEL;input.value=meta.label;input.placeholder='optional';input.disabled=!ok;input.setAttribute('aria-label',`Camera ${meta.number} OSD label`);
  const save=document.createElement('button');save.type='button';save.textContent='Set';save.disabled=!ok;
  const preview=document.createElement('span');preview.className='fps-cam-label-preview';
  const draw=()=>{const v=cleanLabel(input.value);preview.textContent=`BATT LOW ${v||('C'+meta.number)}`.slice(0,16)};
  input.addEventListener('input',draw);input.addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();save.click()}});
  save.addEventListener('click',()=>saveLabel(c.idx,input,save));
  edit.append(input,save);labelTd.append(edit,preview);draw();
  row.insertBefore(labelTd,row.children[3]);
 });
 refreshSupportState();
}
function hook(){
 if(typeof window.renderCameraTable!=='function'||window.renderCameraTable.__fpsLabels)return false;
 const original=window.renderCameraTable;
 const wrapped=function(cameras){const r=original.apply(this,arguments);setTimeout(()=>enhance(cameras||[]),0);return r};
 wrapped.__fpsLabels=true;window.renderCameraTable=wrapped;return true;
}
function init(){addStyle();ensureInfo();let tries=0;const t=setInterval(()=>{if(hook()||++tries>40)clearInterval(t)},100);document.addEventListener('fps-firmware-version',refreshSupportState,false)}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();