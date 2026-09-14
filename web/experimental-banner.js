(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const YELLOW='#f5ff00';
const DEMO_KEY='freeclinkerDemoMode',EXP_KEY='fpsExperimentalFeatures',MULTI_KEY='fpsExperimentalMultiCamSync';
const demoMode=()=>localStorage.getItem(DEMO_KEY)==='1';
const s=document.createElement('style');
s.textContent=`
.fps-exp-banner{position:sticky;top:0;z-index:99999;background:${YELLOW};color:#101318;text-align:center;font:950 13px/1.25 system-ui,sans-serif;padding:8px 12px;letter-spacing:.055em;border-bottom:2px solid #a8b000;box-shadow:0 0 16px rgba(245,255,0,.34)}
.fps-exp-banner strong{font-weight:1000}.fps-test-label{margin-left:7px;padding:2px 6px;border-radius:4px;background:${YELLOW};color:#101318;font-size:10px;font-weight:950;letter-spacing:.07em;white-space:nowrap}.fps-test-label::before{content:'EXPERIMENTAL'}
#fpsRebootOverlay{position:fixed;inset:0;z-index:2147483646;display:none;place-items:center;padding:24px;background:rgba(5,8,15,.94);backdrop-filter:blur(8px);font-family:system-ui,sans-serif}
#fpsRebootOverlay.show{display:grid}.fps-reboot-card{width:min(680px,100%);padding:34px 28px;border:2px solid ${YELLOW};border-radius:18px;background:#101318;color:#f8fafc;text-align:center;box-shadow:0 0 0 1px rgba(245,255,0,.14),0 0 50px rgba(245,255,0,.18)}.fps-reboot-icon{font-size:42px;line-height:1;margin-bottom:14px}.fps-reboot-card h2{margin:0 0 12px;font-size:27px;line-height:1.1;color:${YELLOW}}.fps-reboot-card p{margin:0 auto 22px;max-width:540px;color:#d1d5db;font-size:15px;line-height:1.55}.fps-reboot-card .fps-reboot-save{display:block;width:100%;padding:16px 20px;border-radius:10px;border:2px solid #60a5fa;background:#2563eb;color:#fff;font-size:17px;font-weight:900;letter-spacing:.035em}.fps-reboot-card .fps-reboot-save:hover:not(:disabled){background:#1d4ed8}.fps-reboot-card .fps-reboot-later{margin-top:12px;border:0;background:transparent;color:#94a3b8;text-decoration:underline}.fps-reboot-card .fps-reboot-status{min-height:18px;margin-top:12px;color:#fca5a5;font-size:12px;font-weight:700}
@media(max-width:650px){.fps-test-label{font-size:8px;margin-left:4px}.fps-exp-banner{font-size:11px}.fps-reboot-card{padding:28px 20px}.fps-reboot-card h2{font-size:23px}}
`;
document.head.appendChild(s);
function dedupeBanner(){
 const banners=[...document.querySelectorAll('.fps-exp-banner')];
 let keep=banners.find(b=>b.dataset.fpsCanonical==='1')||banners[0];
 if(!keep){keep=document.createElement('div');keep.className='fps-exp-banner';document.body.prepend(keep)}
 keep.dataset.fpsCanonical='1';
 keep.innerHTML='<strong>⚠ EXPERIMENTAL V1.0.2 TEST ENVIRONMENT</strong> — development firmware and controls; bench test before flight';
 banners.forEach(b=>{if(b!==keep)b.remove()});
}
function ensureRebootOverlay(){
 if(demoMode()){
  document.getElementById('fpsRebootOverlay')?.remove();
  return null;
 }
 let overlay=document.getElementById('fpsRebootOverlay');
 if(overlay)return overlay;
 overlay=document.createElement('div');overlay.id='fpsRebootOverlay';
 overlay.innerHTML='<div class="fps-reboot-card" role="dialog" aria-modal="true" aria-labelledby="fpsRebootTitle"><div class="fps-reboot-icon">⚠️</div><h2 id="fpsRebootTitle">POWER CYCLE REQUIRED</h2><p>You need to power cycle the C3 for this change to apply. Save the settings first, then disconnect power and reconnect the board.</p><button type="button" class="fps-reboot-save" id="fpsRebootSave">SAVE SETTINGS</button><button type="button" class="fps-reboot-later" id="fpsRebootLater">Keep editing</button><div class="fps-reboot-status" id="fpsRebootStatus"></div></div>';
 document.body.appendChild(overlay);
 document.getElementById('fpsRebootLater').onclick=()=>overlay.classList.remove('show');
 document.getElementById('fpsRebootSave').onclick=()=>{
  const status=document.getElementById('fpsRebootStatus');
  const source=overlay.dataset.source||'';
  const button=source==='caddx'?document.getElementById('caddxApplyBtn'):document.getElementById('fpsTopSave');
  if(!button||button.disabled){status.textContent='Connect to the C3 and read the settings first, then save again.';return}
  status.textContent='';overlay.classList.remove('show');button.click();
 };
 return overlay;
}
function rebootRequired(el){
 if(!el)return false;
 if(el.id==='fpsMultiCamSync')return true;
 const row=el.closest?.('.cfg-field,.fps-row,#caddxWifiCard');
 return !!row&&/reboot required/i.test(row.textContent||'');
}
function showRebootWarning(el){
 if(demoMode())return;
 const overlay=ensureRebootOverlay();
 if(!overlay)return;
 overlay.dataset.source=el.closest?.('#caddxWifiCard')?'caddx':'global';
 const status=document.getElementById('fpsRebootStatus');if(status)status.textContent='';
 overlay.classList.add('show');
}
function syncDemoMultiFlags(){
 if(!demoMode())return;
 const exp=document.getElementById('fpsExperimentalMaster');
 const multi=document.getElementById('fpsMultiCamSync');
 if(exp)localStorage.setItem(EXP_KEY,exp.checked?'1':'0');
 if(multi)localStorage.setItem(MULTI_KEY,multi.checked?'1':'0');
}
function syncDemoPreview(detail){
 if(!demoMode())return;
 syncDemoMultiFlags();
 const connected=Array.isArray(detail?.connected)?detail.connected:[];
 const cameras=Array.isArray(detail?.cameras)?detail.cameras:[];
 const count=connected.length;
 const countSelect=document.getElementById('fpsPreviewCameraCount');
 if(countSelect){
  for(let i=0;i<=4;i++)if(![...countSelect.options].some(o=>Number(o.value)===i))countSelect.add(new Option(String(i),String(i)));
  countSelect.value=String(count);
  countSelect.dispatchEvent(new Event('change',{bubbles:true}));
 }
 const source=document.getElementById('fpsPreviewWarningSource');
 if(source){
  const previous=Number(source.value)||1;
  source.innerHTML='';
  for(const cam of cameras){
   const n=Number(cam.number)||((Number(cam.idx)||0)+1);
   const label=String(cam.label||'').trim();
   const option=new Option(label?`Cam ${n} · ${label}`:`Cam ${n} · CAM${n}`,String(n));
   option.disabled=!cam.connected;
   source.add(option);
  }
  const wanted=[...source.options].find(o=>Number(o.value)===previous&&!o.disabled)||[...source.options].find(o=>!o.disabled)||source.options[0];
  if(wanted){source.value=wanted.value;source.dispatchEvent(new Event('change',{bubbles:true}))}
 }
}
dedupeBanner();
const observer=new MutationObserver(()=>dedupeBanner());
observer.observe(document.body,{childList:true,subtree:false});
window.addEventListener('load',()=>{dedupeBanner();if(!demoMode())ensureRebootOverlay();else document.getElementById('fpsRebootOverlay')?.remove();setTimeout(()=>{dedupeBanner();observer.disconnect()},1500)});
document.addEventListener('change',e=>{if(e.isTrusted&&!demoMode()&&rebootRequired(e.target))showRebootWarning(e.target);if(e.target?.id==='fpsExperimentalMaster'||e.target?.id==='fpsMultiCamSync')setTimeout(syncDemoMultiFlags,0)},true);
document.addEventListener('fps-demo-camera-connections',e=>syncDemoPreview(e.detail));
if(demoMode()){
 document.getElementById('fpsRebootOverlay')?.remove();
 let tries=0;const t=setInterval(()=>{syncDemoMultiFlags();if(document.getElementById('fpsMultiCamSync')||++tries>20)clearInterval(t)},150);
}
const brand=document.querySelector('.brand');
if(brand&&!brand.querySelector('.fps-test-label')){const t=document.createElement('span');t.className='fps-test-label';brand.appendChild(t)}
})();