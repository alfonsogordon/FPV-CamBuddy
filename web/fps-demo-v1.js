(()=>{
'use strict';
const MODE='freeclinkerDemoMode', CFG='freeclinkerDemoConfig', STORE='fpsUiV2State';
const demo=()=>localStorage.getItem(MODE)==='1';
const multi=()=>!!document.getElementById('fpsMultiCamSync')?.checked||localStorage.getItem('fpsExperimentalMultiCamSync')==='1';
const dummyCameras=[
 {idx:0,name:'GoPro HERO11 Black Mini',addr:'DE:MO:00:00:00:01',type:'GoPro',tags:'[cam=1][label=FRONT]'},
 {idx:1,name:'GoPro MAX2',addr:'DE:MO:00:00:00:02',type:'GoPro',tags:'[cam=2][label=REAR]'},
 {idx:2,name:'DJI Osmo Action 5 Pro',addr:'DE:MO:00:00:00:03',type:'DJI',tags:'[cam=3][label=]'},
 {idx:3,name:'Insta360 X5',addr:'DE:MO:00:00:00:04',type:'Insta360',tags:'[cam=4][label=360CAM]'}
];
function parse(k){try{return JSON.parse(localStorage.getItem(k)||'{}')}catch{return{}}}
function enter(){const current=parse(STORE);if(Object.keys(current).length)localStorage.setItem(CFG,JSON.stringify(current));localStorage.setItem(MODE,'1');location.reload()}
function exit(){const current=parse(STORE);if(Object.keys(current).length)localStorage.setItem(CFG,JSON.stringify(current));localStorage.removeItem(MODE);location.reload()}
function applyDummyCameraState(){
 if(!demo())return;
 const table=document.querySelector('#camTableWrap .cam-table');if(!table)return;
 const on=multi();
 [...table.querySelectorAll('tbody tr')].forEach((row,i)=>{
  const locked=i>0&&!on;
  row.classList.toggle('fps-demo-multi-locked',locked);
  row.querySelectorAll('input,select,button').forEach(el=>{el.disabled=locked});
  row.title=locked?'Enable Multi Cam to use additional demo cameras.':'';
 });
}
function renderDummyCameras(){
 if(!demo()||typeof window.renderCameraTable!=='function')return false;
 window.renderCameraTable(dummyCameras.map(c=>({...c})));
 setTimeout(applyDummyCameraState,50);
 return true;
}
function hookDummyCameras(){
 if(!demo()||typeof window.renderCameraTable!=='function')return false;
 if(window.renderCameraTable.__fpsDemoDummy)return true;
 const original=window.renderCameraTable;
 const wrapped=function(list){return original.call(this,dummyCameras.map(c=>({...c})))};
 wrapped.__fpsDemoDummy=true;
 window.renderCameraTable=wrapped;
 renderDummyCameras();
 return true;
}
function init(){
 const connect=document.getElementById('connectBtn');if(!connect)return;
 const b=document.createElement('button');b.type='button';b.id='fpsDemoModeBtn';b.className='fps-demo-v1';b.textContent=demo()?'Exit Demo Mode':'Demo Mode';b.title=demo()?'Return to real ESP32 connection mode':'Try all configurator settings without a connected board';b.onclick=demo()?exit:enter;connect.insertAdjacentElement('afterend',b);
 if(!demo())return;
 document.documentElement.classList.add('fps-demo-active');document.body.classList.add('fps-demo-active');
 const frame=document.createElement('div');frame.className='fps-demo-frame';frame.setAttribute('aria-hidden','true');document.body.appendChild(frame);
 const badge=document.createElement('div');badge.className='fps-demo-v1-banner';badge.innerHTML='<strong>DEMO MODE</strong><span>Virtual C3 — settings are saved locally. No commands are sent to hardware.</span>';const panel=document.getElementById('panel-config');if(panel)panel.prepend(badge);
 panel?.querySelectorAll('input,select,button').forEach(e=>{if(e.id!=='connectBtn')e.disabled=false});
 ['applyBtn','auxApplyBtn','osdApplyBtn','bf45ApplyBtn'].forEach(id=>{const e=document.getElementById(id);if(e){e.disabled=false;e.textContent='Saved in Demo';e.onclick=ev=>{ev.preventDefault();ev.stopImmediatePropagation();localStorage.setItem(CFG,localStorage.getItem(STORE)||'{}');const old=e.textContent;e.textContent='Saved ✓';setTimeout(()=>e.textContent=old,900)}}});
 let tries=0;const hook=setInterval(()=>{if(hookDummyCameras()||++tries>40)clearInterval(hook)},100);
 document.addEventListener('change',e=>{if(e.target?.id==='fpsMultiCamSync'||e.target?.id==='fpsExperimentalMaster')setTimeout(()=>{renderDummyCameras();applyDummyCameraState()},20)},true);
 const wrap=document.getElementById('camTableWrap');if(wrap)new MutationObserver(()=>setTimeout(applyDummyCameraState,0)).observe(wrap,{childList:true,subtree:true});
}
const css=document.createElement('style');css.textContent=`.fps-demo-v1{margin-left:8px;border-color:#7c3aed!important}.fps-demo-active .fps-demo-v1{background:#7c3aed!important;color:#fff!important}.fps-demo-v1-banner{margin:0 0 16px;padding:12px 16px;border:1px solid #7c3aed;border-radius:10px;background:rgba(124,58,237,.13);display:flex;align-items:center;gap:12px}.fps-demo-v1-banner strong{color:#c4b5fd;white-space:nowrap}.fps-demo-v1-banner span{font-size:12px;color:var(--muted,#aaa)}.fps-demo-frame{position:fixed;inset:0;z-index:99998;border:4px solid #7c3aed;pointer-events:none;box-sizing:border-box;box-shadow:inset 0 0 22px rgba(124,58,237,.12)}html.fps-demo-active{background-image:linear-gradient(rgba(124,58,237,.035),rgba(124,58,237,.035))}html.fps-demo-active::before{content:'DEMO MODE  •  VIRTUAL C3';position:fixed;z-index:99999;top:0;left:50%;transform:translateX(-50%);padding:3px 14px 5px;background:#7c3aed;color:#fff;border-radius:0 0 9px 9px;font:700 11px/1.2 system-ui,sans-serif;letter-spacing:.08em;pointer-events:none;box-shadow:0 2px 12px rgba(124,58,237,.4)}.fps-demo-active #camTableWrap .fps-demo-multi-locked{opacity:.34;filter:grayscale(1)}.fps-demo-active #camTableWrap .fps-demo-multi-locked td{background:#f3f4f6!important}.fps-demo-active #camTableWrap .fps-demo-multi-locked:hover td{background:#f3f4f6!important}@media(max-width:640px){.fps-demo-frame{border-width:3px}.fps-demo-v1-banner{align-items:flex-start;flex-direction:column;gap:4px}}`;document.head.appendChild(css);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,160));else setTimeout(init,160);
})();
