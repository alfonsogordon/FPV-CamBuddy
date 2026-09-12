(()=>{
'use strict';
const MODE='freeclinkerDemoMode', CFG='freeclinkerDemoConfig', STORE='fpsUiV1State';
const demo=()=>localStorage.getItem(MODE)==='1';
function parse(k){try{return JSON.parse(localStorage.getItem(k)||'{}')}catch{return{}}}
function enter(){const current=parse(STORE);if(Object.keys(current).length)localStorage.setItem(CFG,JSON.stringify(current));localStorage.setItem(MODE,'1');location.reload()}
function exit(){const current=parse(STORE);if(Object.keys(current).length)localStorage.setItem(CFG,JSON.stringify(current));localStorage.removeItem(MODE);location.reload()}
function init(){
 const connect=document.getElementById('connectBtn');if(!connect)return;
 const b=document.createElement('button');b.type='button';b.id='fpsDemoModeBtn';b.className='fps-demo-v1';b.textContent=demo()?'Exit Demo Mode':'Demo Mode';b.title=demo()?'Return to real ESP32 connection mode':'Try all configurator settings without a connected board';b.onclick=demo()?exit:enter;connect.insertAdjacentElement('afterend',b);
 if(!demo())return;
 document.documentElement.classList.add('fps-demo-active');
 const badge=document.createElement('div');badge.className='fps-demo-v1-banner';badge.innerHTML='<strong>DEMO MODE</strong><span>Virtual C3 — settings are saved locally. No commands are sent to hardware.</span>';const panel=document.getElementById('panel-config');if(panel)panel.prepend(badge);
 // The stock configurator disables board-backed fields while disconnected. In Demo Mode they are virtual controls.
 panel?.querySelectorAll('input,select,button').forEach(e=>{if(e.id!=='connectBtn')e.disabled=false});
 // Prevent stock Apply buttons from attempting serial writes in Demo Mode. The rebuilt UI persists changes immediately.
 ['applyBtn','auxApplyBtn','osdApplyBtn','bf45ApplyBtn'].forEach(id=>{const e=document.getElementById(id);if(e){e.disabled=false;e.textContent='Saved in Demo';e.onclick=ev=>{ev.preventDefault();ev.stopImmediatePropagation();localStorage.setItem(CFG,localStorage.getItem(STORE)||'{}');const old=e.textContent;e.textContent='Saved ✓';setTimeout(()=>e.textContent=old,900)}}});
}
const css=document.createElement('style');css.textContent='.fps-demo-v1{margin-left:8px;border-color:#7c3aed!important}.fps-demo-active .fps-demo-v1{background:#7c3aed!important;color:#fff!important}.fps-demo-v1-banner{margin:0 0 16px;padding:12px 16px;border:1px solid #7c3aed;border-radius:10px;background:rgba(124,58,237,.13);display:flex;align-items:center;gap:12px}.fps-demo-v1-banner strong{color:#c4b5fd;white-space:nowrap}.fps-demo-v1-banner span{font-size:12px;color:var(--muted,#aaa)}';document.head.appendChild(css);
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,160));else setTimeout(init,160);
})();