(()=>{
'use strict';
const $=id=>document.getElementById(id);
let readTimer=null,readRequested=false;
function legacyMode(){return $('fpsBfMode')?.value==='legacy'}
function connected(){return !!$('connectBtn')?.disabled}
function stripTarget(value){const s=String(value||'');const m=s.match(/^@([1-4]):(.*)$/s);return m?{target:parseInt(m[1],10),text:m[2]}:{target:1,text:s}}
function fire(id,type='change'){const e=$(id);if(e)e.dispatchEvent(new Event(type,{bubbles:true}))}
function setSelectTarget(id,target){const s=$(id);if(!s)return;const value=legacyMode()?(target===2?'craft':'pilot'):String(target);if([...s.options].some(o=>o.value===value)){s.value=value;fire(id)}}
function syncMetadataFromDevice(){
 if(!readRequested||!connected())return;
 window.fpsBoardSyncing=true;
 readRequested=false;
 const bf=$('bf45Compat');
 if($('fpsBfMode')&&bf){$('fpsBfMode').value=bf.checked?'legacy':'current';fire('fpsBfMode')}
 if($('fpsPilotMaster')&&$('pilotEn')){$('fpsPilotMaster').checked=$('pilotEn').checked;fire('fpsPilotMaster')}
 if($('fpsCraftMaster')&&$('craftEn')){$('fpsCraftMaster').checked=$('craftEn').checked;fire('fpsCraftMaster')}
 // Existing firmware persists the Full Edition OSD master in fpv_state_mode.
 if($('fpsOsdMaster')){$('fpsOsdMaster').checked=!!$('fpvStateMode')?.checked;fire('fpsOsdMaster')}
 for(let n=1;n<=4;n++){
  const input=$('osd'+n),master=$('fpsMsg'+n);if(!input||!master)continue;
  const wire=String(input.value||'').trim(),off=!wire||wire==='{off}';
  master.checked=!off;
  if(off&&!String(input.dataset.fpsLastTemplate||'').trim()) input.dataset.fpsLastTemplate=['','{batt}','{state} {recdur}','{mode} {res} {fps} {eis}','{rectf} {rcap}'][n];
  if(!off) input.dataset.fpsLastTemplate=wire;
  if(off) input.value=input.dataset.fpsLastTemplate||'';
  fire('osd'+n,'input');fire('fpsMsg'+n);
 }
 const recordMeta=String($('fpvRecordText')?.value||'');
 if($('fpsRecOnly')){$('fpsRecOnly').checked=recordMeta.startsWith('@ARM:');fire('fpsRecOnly')}
 // fpv_prearm is ONLY the "before first arm" behaviour. Presence of text is
 // the Temporary Message master, so the two controls round-trip independently.
 const p=stripTarget($('fpvPreArmText')?.value);
 if($('fpvPreArmText')){$('fpvPreArmText').value=p.text||'CLEAN LENS';fire('fpvPreArmText','input')}
 if($('fpsTempMaster')){$('fpsTempMaster').checked=!!p.text;fire('fpsTempMaster')}
 setSelectTarget('fpsTempDest',p.target);
 if($('fpvCustomDurationSec')&&$('fpvPreArmShow')){const ms=Math.max(100,parseInt($('fpvPreArmShow').value,10)||1000);$('fpvCustomDurationSec').value=(ms/1000).toFixed(ms%1000===0?0:1);fire('fpvCustomDurationSec','input')}
 const w=stripTarget($('fpvLowText')?.value);
 if($('fpsWarnMaster')){$('fpsWarnMaster').checked=!!$('fpvLowBatt')?.checked||!!$('fpvRecLow')?.checked||!!$('fpvHot')?.checked;fire('fpsWarnMaster')}
 setSelectTarget('fpsWarnDest',w.target);
 if($('fpsAuxMaster')&&$('auxChannel')){$('fpsAuxMaster').checked=parseInt($('auxChannel').value,10)>0;fire('fpsAuxMaster')}
 fire('fpvFlash');fire('fpvPreArm');fire('fpvLowPct','input');fire('fpvRecLowMin','input');
 window.fpsBoardSyncing=false;
 document.dispatchEvent(new CustomEvent('fps-config-read-complete'));
}
function scheduleReadSync(){if(!readRequested)return;clearTimeout(readTimer);readTimer=setTimeout(syncMetadataFromDevice,220)}
function installReadCompletionHook(){if(typeof parseConfigLine!=='function'||parseConfigLine.__fpsWrapped)return;const original=parseConfigLine;const wrapped=function(line){original(line);if(readRequested&&/^\[cfg\]\s+\w+\s*=/.test(String(line||'')))scheduleReadSync()};wrapped.__fpsWrapped=true;parseConfigLine=wrapped}
function requestRead(){readRequested=true;clearTimeout(readTimer)}
window.fpsRequestBoardRead=requestRead;
function init(){installReadCompletionHook();$('readBtn')?.addEventListener('click',requestRead,true)}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,450));else setTimeout(init,450);
})();
