(()=>{
'use strict';
const $=id=>document.getElementById(id);
let readTimer=null,readRequested=false;
const CRAFT_DEFAULT='{res} {fps}',CURRENT_DEFAULTS=['','{batt}','{state} {recdur}','{mode} {res} {fps} {eis}','{rectf} {rcap}'];
function legacyMode(){return $('fpsBfMode')?.value==='legacy'}
function connected(){return !!$('connectBtn')?.disabled}
function stripTarget(value){const s=String(value||'');const m=s.match(/^@([1-4]):(.*)$/s);return m?{target:parseInt(m[1],10),text:m[2]}:{target:1,text:s}}
function fire(id,type='change'){const e=$(id);if(e)e.dispatchEvent(new Event(type,{bubbles:true}))}
function setSelectTarget(id,target){const s=$(id);if(!s)return;const value=legacyMode()?(target===2?'craft':'pilot'):String(target);if([...s.options].some(o=>o.value===value)){s.value=value;fire(id)}}
function craftTemplateIsOff(){const v=String($('craftTpl')?.value||'').trim().toLowerCase();return !v||v==='off'||v==='{off}'}
function healCraftTemplate(){if(!$('fpsCraftMaster')?.checked||!$('craftTpl')||!craftTemplateIsOff())return false;$('craftTpl').value=CRAFT_DEFAULT;fire('craftTpl','input');return true}
function syncGoProNote(){const n=$('fpsGoProConnectionNote');if(!n)return;n.style.display=$('wakeGuard')?.checked?'':'none'}
function syncMetadataFromDevice(){
 if(!readRequested||!connected())return;
 window.fpsBoardSyncing=true;readRequested=false;
 const bf=$('bf45Compat');if($('fpsBfMode')&&bf){$('fpsBfMode').value=bf.checked?'legacy':'current';fire('fpsBfMode')}
 if($('fpsPilotMaster')&&$('pilotEn')){$('fpsPilotMaster').checked=$('pilotEn').checked;fire('fpsPilotMaster')}
 if($('fpsCraftMaster')&&$('craftEn')){$('fpsCraftMaster').checked=$('craftEn').checked;healCraftTemplate();fire('fpsCraftMaster')}
 const osdOn=!!$('fpvStateMode')?.checked;if($('fpsOsdMaster')){$('fpsOsdMaster').checked=osdOn;fire('fpsOsdMaster')}
 const rawMsgs=[1,2,3,4].map(n=>String($('osd'+n)?.value||'').trim());
 const freshEmpty=!osdOn&&rawMsgs.every(v=>!v||v==='{off}');
 for(let n=1;n<=4;n++){
  const input=$('osd'+n),master=$('fpsMsg'+n);if(!input||!master)continue;
  const wire=String(input.value||'').trim(),off=!wire||wire==='{off}';
  // A factory-fresh board intentionally stores no active OSD transport while
  // the OSD master is OFF. Rehydrate the agreed V1 templates as latent UI
  // defaults so enabling OSD produces a useful four-message layout.
  master.checked=freshEmpty?true:!off;
  if((off||freshEmpty)&&!String(input.dataset.fpsLastTemplate||'').trim())input.dataset.fpsLastTemplate=CURRENT_DEFAULTS[n];
  if(!off)input.dataset.fpsLastTemplate=wire;
  if(off)input.value=input.dataset.fpsLastTemplate||CURRENT_DEFAULTS[n];
  fire('osd'+n,'input');fire('fpsMsg'+n);
 }
 const recordMeta=String($('fpvRecordText')?.value||'');if($('fpsRecOnly')){$('fpsRecOnly').checked=recordMeta.startsWith('@ARM:');fire('fpsRecOnly')}
 const p=stripTarget($('fpvPreArmText')?.value);if($('fpvPreArmText')){$('fpvPreArmText').value=p.text||'CLEAN LENS';fire('fpvPreArmText','input')}if($('fpsTempMaster')){$('fpsTempMaster').checked=!!p.text;fire('fpsTempMaster')}setSelectTarget('fpsTempDest',p.target);
 if($('fpvCustomDurationSec')&&$('fpvPreArmShow')){const ms=Math.max(100,parseInt($('fpvPreArmShow').value,10)||1000);$('fpvCustomDurationSec').value=(ms/1000).toFixed(ms%1000===0?0:1);fire('fpvCustomDurationSec','input')}
 const w=stripTarget($('fpvLowText')?.value);if($('fpsWarnMaster')){$('fpsWarnMaster').checked=!!$('fpvLowBatt')?.checked||!!$('fpvRecLow')?.checked||!!$('fpvHot')?.checked;fire('fpsWarnMaster')}setSelectTarget('fpsWarnDest',w.target);
 if($('fpsAuxMaster')&&$('auxChannel')){$('fpsAuxMaster').checked=parseInt($('auxChannel').value,10)>0;fire('fpsAuxMaster')}
 fire('fpvFlash');fire('fpvPreArm');fire('fpvLowPct','input');fire('fpvRecLowMin','input');syncGoProNote();window.fpsBoardSyncing=false;document.dispatchEvent(new CustomEvent('fps-config-read-complete'));
}
function scheduleReadSync(){if(!readRequested)return;clearTimeout(readTimer);readTimer=setTimeout(syncMetadataFromDevice,220)}
function installReadCompletionHook(){if(typeof parseConfigLine!=='function'||parseConfigLine.__fpsWrapped)return;const original=parseConfigLine;const wrapped=function(line){original(line);if(readRequested&&/^\[cfg\]\s+\w+\s*=/.test(String(line||'')))scheduleReadSync()};wrapped.__fpsWrapped=true;parseConfigLine=wrapped}
function requestRead(){readRequested=true;clearTimeout(readTimer)}window.fpsRequestBoardRead=requestRead;
function init(){installReadCompletionHook();$('readBtn')?.addEventListener('click',requestRead,true);$('fpsCraftMaster')?.addEventListener('change',()=>{if(healCraftTemplate())fire('fpsCraftMaster')});$('wakeGuard')?.addEventListener('change',syncGoProNote);syncGoProNote()}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,450));else setTimeout(init,450);
})();
