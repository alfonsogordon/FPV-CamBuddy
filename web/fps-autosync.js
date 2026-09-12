(()=>{
'use strict';
const $=id=>document.getElementById(id);
const DEMO='freeclinkerDemoMode';
const BASE=new Set(['cameraType','cameraMatch','wakeGuard','stopOnDisarm','disarmDelay','debugBle','lowPower','wifiApEnabled','wifiApDelay']);
const AUX=new Set(['fpsAuxMaster','auxChannel','auxMode']);
const OSD_NATIVE=new Set(['pilotTpl','craftTpl','fpvFlash','fpvPreArm','fpvPreArmText','fpvCustomDurationSec','fpvLowPct','fpvRecLowMin']);
let needsRead=false;
let dirty=false;

function connected(){return !!$('connectBtn')?.disabled}
function demo(){return localStorage.getItem(DEMO)==='1'}
function isManaged(id){return BASE.has(id)||AUX.has(id)||id.startsWith('fps')||/^osd[1-4]$/.test(id)||OSD_NATIVE.has(id)}
function setPulse(el,on,kind){
  if(!el)return;
  el.classList.toggle('fps-attention',!!on);
  el.classList.toggle('fps-attention-read',!!on&&kind==='read');
  el.classList.toggle('fps-attention-save',!!on&&kind==='save');
}
function refresh(){
  setPulse($('readBtn'),connected()&&needsRead,'read');
  setPulse($('fpsApplyAll'),connected()&&dirty,'save');
}
function markDirty(target){
  if(!target||!isManaged(target.id||''))return;
  dirty=true;
  refresh();
}
function injectStyle(){
  if($('fpsExplicitSaveStyle'))return;
  const s=document.createElement('style');
  s.id='fpsExplicitSaveStyle';
  s.textContent=`
@keyframes fpsReadPulse{0%,100%{box-shadow:0 0 0 0 rgba(220,38,38,.15);background:#fff;color:#b91c1c;border-color:#ef4444}50%{box-shadow:0 0 0 6px rgba(220,38,38,.22);background:#fee2e2;color:#991b1b;border-color:#dc2626}}
@keyframes fpsSavePulse{0%,100%{box-shadow:0 0 0 0 rgba(37,99,235,.18)}50%{box-shadow:0 0 0 6px rgba(37,99,235,.25)}}
.fps-attention-read{animation:fpsReadPulse .9s ease-in-out infinite!important;font-weight:700!important}
.fps-attention-save{animation:fpsSavePulse .9s ease-in-out infinite!important;font-weight:700!important}
`;
  document.head.appendChild(s);
}
function renameButtons(){
  const r=$('readBtn'); if(r)r.textContent='Read Settings';
  const a=$('fpsApplyAll'); if(a)a.textContent='Save / Apply Settings';
}
function init(){
  injectStyle();renameButtons();
  const connect=$('connectBtn');
  if(connect){
    new MutationObserver(()=>{
      if(connected()){
        needsRead=true;
        dirty=false;
      }else{
        needsRead=false;
        dirty=false;
      }
      refresh();
    }).observe(connect,{attributes:true,attributeFilter:['disabled']});
    if(connected())needsRead=true;
  }
  $('readBtn')?.addEventListener('click',()=>{
    if(!connected()||demo())return;
    needsRead=false;
    dirty=false;
    refresh();
  });
  $('fpsApplyAll')?.addEventListener('click',()=>{
    if(!connected()||demo())return;
    dirty=false;
    refresh();
  });
  document.addEventListener('change',e=>{if(e.isTrusted)markDirty(e.target)});
  document.addEventListener('input',e=>{if(e.isTrusted)markDirty(e.target)});
  document.addEventListener('click',e=>{
    if(!e.isTrusted)return;
    const btn=e.target?.closest?.('.fps-menu button,.fps-chips button');
    if(!btn)return;
    const b=btn.closest('.fps-builder');
    const input=b?.querySelector?.('input.fps-hidden-template, input[id^="osd"], input#pilotTpl, input#craftTpl');
    if(input)setTimeout(()=>markDirty(input),0);
  });
  refresh();
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,700));
else setTimeout(init,700);
})();
