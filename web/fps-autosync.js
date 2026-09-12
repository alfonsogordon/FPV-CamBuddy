(()=>{
'use strict';
const $=id=>document.getElementById(id);
const DEMO='freeclinkerDemoMode';
const BASE=new Set(['cameraType','cameraMatch','wakeGuard','stopOnDisarm','disarmDelay','debugBle','lowPower','wifiApEnabled','wifiApDelay']);
const AUX=new Set(['fpsAuxMaster','auxChannel','auxMode']);
const OSD_NATIVE=new Set(['pilotTpl','craftTpl','fpvFlash','fpvPreArm','fpvPreArmText','fpvCustomDurationSec','fpvLowPct','fpvRecLowMin']);
let timer=null;

function connected(){return !!$('connectBtn')?.disabled}
function demo(){return localStorage.getItem(DEMO)==='1'}
function click(id){const b=$(id);if(b&&!b.disabled)b.click()}
function isOsdTarget(id){return id.startsWith('fps')||/^osd[1-4]$/.test(id)||OSD_NATIVE.has(id)}

function saveFor(target){
  if(!target||!connected()||demo())return;
  const id=target.id||'';
  if(BASE.has(id)){click('applyBtn');return}
  if(AUX.has(id)){click('auxApplyBtn');return}
  // Keep one canonical OSD translator: Stage-3 Apply converts the friendly UI
  // into the exact persisted CLI values used by the firmware.
  if(isOsdTarget(id))click('fpsApplyAll');
}

function queue(target,fast=false){
  if(!target||!connected()||demo())return;
  clearTimeout(timer);
  timer=setTimeout(()=>saveFor(target),fast?120:450);
}

function autoRead(){
  if(!connected()||demo())return;
  // The native connection already issues `show`. Trigger Read once so the
  // Stage-3 metadata translator runs its existing readback path too. We do NOT
  // suppress trusted user changes: the previous time-window suppression could
  // silently discard a real edit made just after connecting.
  setTimeout(()=>click('readBtn'),250);
}

function builderInputFromButton(target){
  const b=target?.closest?.('.fps-builder');
  return b?.querySelector?.('input.fps-hidden-template, input[id^="osd"], input#pilotTpl, input#craftTpl')||null;
}

function init(){
  const connect=$('connectBtn');
  if(connect){
    new MutationObserver(()=>{if(connect.disabled)autoRead()}).observe(connect,{attributes:true,attributeFilter:['disabled']});
    if(connect.disabled)autoRead();
  }

  document.addEventListener('change',e=>{
    // Stage-3 device readback intentionally fires synthetic change/input events
    // while rebuilding the friendly UI. Those must never write back to the C3.
    // Real browser/user changes are trusted and may autosave immediately.
    if(!e.isTrusted)return;
    queue(e.target,true);
  });

  document.addEventListener('input',e=>{
    if(!e.isTrusted)return;
    queue(e.target,false);
  });

  // OSD token/chip buttons update their hidden template input with a synthetic
  // `input` event. The click itself is trusted, so explicitly queue that owning
  // template after the builder has updated it. This keeps token editing instant
  // in Preview while still persisting it automatically.
  document.addEventListener('click',e=>{
    if(!e.isTrusted)return;
    const btn=e.target?.closest?.('.fps-menu button,.fps-chips button');
    if(!btn)return;
    const input=builderInputFromButton(btn);
    if(input)setTimeout(()=>queue(input,false),0);
  });
}

if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,700));
else setTimeout(init,700);
})();
