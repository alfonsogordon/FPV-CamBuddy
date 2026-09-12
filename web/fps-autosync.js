(()=>{
'use strict';
const $=id=>document.getElementById(id);
const DEMO='freeclinkerDemoMode';
const BASE=new Set(['cameraType','cameraMatch','wakeGuard','stopOnDisarm','disarmDelay','debugBle','lowPower','wifiApEnabled','wifiApDelay']);
const AUX=new Set(['fpsAuxMaster','auxChannel','auxMode']);
let timer=null;
let suppressUntil=0;

function connected(){return !!$('connectBtn')?.disabled}
function demo(){return localStorage.getItem(DEMO)==='1'}
function suppress(ms=1800){suppressUntil=Math.max(suppressUntil,Date.now()+ms)}
function suppressed(){return Date.now()<suppressUntil}
function click(id){const b=$(id);if(b&&!b.disabled)b.click()}

function saveFor(target){
  if(!target||!connected()||demo()||suppressed())return;
  const id=target.id||'';
  if(BASE.has(id)){click('applyBtn');return}
  if(AUX.has(id)){click('auxApplyBtn');return}
  // Everything owned by the FPSteVe OSD UI is translated by the single
  // Stage-3 Apply path. This includes its native hidden template controls.
  if(id.startsWith('fps')||/^osd[1-4]$/.test(id)||['pilotTpl','craftTpl','fpvFlash','fpvPreArm','fpvPreArmText','fpvCustomDurationSec','fpvLowPct','fpvRecLowMin'].includes(id)){
    click('fpsApplyAll');
  }
}

function queue(target,fast=false){
  if(!connected()||demo()||suppressed())return;
  clearTimeout(timer);
  timer=setTimeout(()=>saveFor(target),fast?120:450);
}

function autoRead(){
  if(!connected()||demo())return;
  suppress(2200);
  // The native connect path already issues `show`; using Read here gives the
  // Stage-3 metadata layer its existing, tested completion/sync path as well.
  setTimeout(()=>click('readBtn'),350);
}

function init(){
  const connect=$('connectBtn');
  if(connect){
    new MutationObserver(()=>{if(connect.disabled)autoRead()}).observe(connect,{attributes:true,attributeFilter:['disabled']});
    if(connect.disabled)autoRead();
  }
  $('readBtn')?.addEventListener('click',()=>suppress(1800),true);
  document.addEventListener('change',e=>queue(e.target,true));
  document.addEventListener('input',e=>queue(e.target,false));
}

if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,700));
else setTimeout(init,700);
})();
