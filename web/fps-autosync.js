(()=>{
'use strict';
const $=id=>document.getElementById(id);
const DEMO='freeclinkerDemoMode';
const BASE=new Set(['cameraType','cameraMatch','wakeGuard','stopOnDisarm','disarmDelay','debugBle','lowPower','wifiApEnabled','wifiApDelay']);
const AUX=new Set(['fpsAuxMaster','auxChannel','auxMode']);
const OSD_NATIVE=new Set(['pilotTpl','craftTpl','fpvFlash','fpvPreArm','fpvPreArmText','fpvCustomDurationSec','fpvLowPct','fpvRecLowMin']);
let needsRead=false,dirty=false,reading=false,saving=false;
function connected(){return !!$('connectBtn')?.disabled}
function demo(){return localStorage.getItem(DEMO)==='1'}
function managed(id){return BASE.has(id)||AUX.has(id)||id.startsWith('fps')||/^osd[1-4]$/.test(id)||OSD_NATIVE.has(id)}
function click(id){const b=$(id);if(b&&!b.disabled)b.click()}
function style(){const s=document.createElement('style');s.id='fpsExplicitSaveStyle';s.textContent='#fpsDeviceBar{display:none;align-items:center;gap:10px;padding:9px 16px;background:#111827;color:#f9fafb;border-bottom:1px solid #374151;flex-shrink:0;box-shadow:0 2px 8px rgba(0,0,0,.18);z-index:20}#fpsDeviceBar.connected{display:flex}#fpsDeviceBar .lbl{font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:#9ca3af;font-weight:700}#fpsDeviceState{font-size:12px;margin-right:auto;color:#d1d5db;font-weight:700}#fpsDeviceBar button{min-width:150px;padding:7px 14px;font-weight:800;border-width:2px}#fpsTopRead{background:#374151;color:#fff;border-color:#6b7280}#fpsTopSave{background:#2563eb;color:#fff;border-color:#60a5fa}@keyframes readPulse{0%,100%{background:#7f1d1d;box-shadow:0 0 0 0 rgba(239,68,68,.2)}50%{background:#dc2626;box-shadow:0 0 0 8px rgba(239,68,68,.35)}}@keyframes savePulse{0%,100%{box-shadow:0 0 0 0 rgba(59,130,246,.2)}50%{box-shadow:0 0 0 8px rgba(59,130,246,.3)}}#fpsTopRead.need{animation:readPulse .8s ease-in-out infinite;border-color:#fecaca}#fpsTopSave.need{animation:savePulse .9s ease-in-out infinite;border-color:#bfdbfe}';document.head.appendChild(s)}
function refresh(){const b=$('fpsDeviceBar'),r=$('fpsTopRead'),s=$('fpsTopSave'),t=$('fpsDeviceState'),isConnected=connected();if(b)b.classList.toggle('connected',isConnected);if(r){r.disabled=!isConnected||reading||saving;r.classList.toggle('need',isConnected&&needsRead&&!reading)}if(s){s.disabled=!isConnected||needsRead||reading||saving;s.classList.toggle('need',isConnected&&!needsRead&&dirty&&!saving)}if(t&&isConnected){if(reading)t.textContent='Reading all settings from C3…';else if(saving)t.textContent='Saving all settings to C3…';else if(needsRead)t.textContent='New device connected — READ SETTINGS before editing';else if(dirty)t.textContent='Unsaved changes — SAVE / APPLY before disconnecting';else if(!t.textContent.includes('✓'))t.textContent='Settings ready'}}
function mark(e){if(e&&managed(e.id||'')&&!needsRead&&!reading&&!saving){dirty=true;refresh()}}
function prepareFriendlyValuesForSave(){
  // AUX master is a friendly UI control; on the board, channel 0 is the
  // authoritative persisted representation of AUX controls being disabled.
  const auxMaster=$('fpsAuxMaster'),aux=$('auxChannel');
  if(auxMaster&&aux&&!auxMaster.checked)aux.value='0';
}
function bar(){const h=document.querySelector('header');if(!h||$('fpsDeviceBar'))return;const b=document.createElement('div');b.id='fpsDeviceBar';b.innerHTML='<span class="lbl">Device settings</span><span id="fpsDeviceState"></span><button id="fpsTopRead" type="button" disabled>READ SETTINGS</button><button id="fpsTopSave" type="button" disabled>SAVE / APPLY SETTINGS</button>';h.insertAdjacentElement('afterend',b);
$('fpsTopRead').onclick=()=>{if(!connected()||demo()||reading||saving)return;reading=true;refresh();click('readBtn')};
$('fpsTopSave').onclick=()=>{if(!connected()||demo()||reading||saving)return;saving=true;prepareFriendlyValuesForSave();refresh();click('applyBtn');setTimeout(()=>click('auxApplyBtn'),120);setTimeout(()=>click('fpsApplyAll'),240)};
}
function init(){style();bar();const r=$('readBtn');if(r)r.textContent='Read Settings';const a=$('fpsApplyAll');if(a)a.textContent='Save / Apply OSD Settings';const c=$('connectBtn');if(c){new MutationObserver(()=>{if(connected()){needsRead=true;dirty=false;reading=false;saving=false}else{needsRead=false;dirty=false;reading=false;saving=false}refresh()}).observe(c,{attributes:true,attributeFilter:['disabled']});if(connected())needsRead=true}

document.addEventListener('fps-config-read-complete',()=>{if(!connected())return;reading=false;needsRead=false;dirty=false;const t=$('fpsDeviceState');if(t)t.textContent='All settings read from C3 ✓';refresh()});
document.addEventListener('fps-settings-applied',()=>{if(!connected())return;saving=false;dirty=false;const t=$('fpsDeviceState');if(t)t.textContent='All settings saved to C3 ✓';refresh()});

document.addEventListener('change',e=>{if(e.isTrusted)mark(e.target)});document.addEventListener('input',e=>{if(e.isTrusted)mark(e.target)});document.addEventListener('click',e=>{if(!e.isTrusted)return;const btn=e.target?.closest?.('.fps-menu button,.fps-chips button');if(!btn)return;const input=btn.closest('.fps-builder')?.querySelector?.('input.fps-hidden-template, input[id^="osd"], input#pilotTpl, input#craftTpl');if(input)setTimeout(()=>mark(input),0)});refresh()}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,700));else setTimeout(init,700);
})();
