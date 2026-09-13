(()=>{
'use strict';
const $=id=>document.getElementById(id);
let readTimer=null,readRequested=false;
const CRAFT_DEFAULT='{res} {fps}',CURRENT_DEFAULTS=['','{batt}','{state} {recdur}','{mode} {res} {fps} {eis}','{rectf} {rcap}'];
const EXP_KEY='fpsExperimentalFeatures',MULTI_KEY='fpsExperimentalMultiCamSync';
function legacyMode(){return $('fpsBfMode')?.value==='legacy'}
function connected(){return !!$('connectBtn')?.disabled}
function stripTarget(value){const s=String(value||'');const m=s.match(/^@([1-4]):(.*)$/s);return m?{target:parseInt(m[1],10),text:m[2]}:{target:1,text:s}}
function fire(id,type='change'){const e=$(id);if(e)e.dispatchEvent(new Event(type,{bubbles:true}))}
function setSelectTarget(id,target){const s=$(id);if(!s)return;const value=legacyMode()?(target===2?'craft':'pilot'):String(target);if([...s.options].some(o=>o.value===value)){s.value=value;fire(id)}}
function craftTemplateIsOff(){const v=String($('craftTpl')?.value||'').trim().toLowerCase();return !v||v==='off'||v==='{off}'}
function healCraftTemplate(){if(!$('fpsCraftMaster')?.checked||!$('craftTpl')||!craftTemplateIsOff())return false;$('craftTpl').value=CRAFT_DEFAULT;fire('craftTpl','input');return true}
function syncGoProNote(){const n=$('fpsGoProConnectionNote');if(!n)return;n.style.display=$('wakeGuard')?.checked?'':'none'}
function installExperimentalUi(){
 if($('fpsExperimentalMaster'))return;
 const panel=$('panel-config');if(!panel)return;
 const style=document.createElement('style');style.textContent=`
  :root{--fps-exp:#facc15;--fps-exp-bg:rgba(250,204,21,.08);--fps-exp-border:rgba(250,204,21,.55)}
  .fps-exp-master{display:flex;align-items:center;justify-content:space-between;gap:16px;margin:0 0 14px;padding:12px 14px;border:1px solid #3b4250;border-radius:9px;background:#0f151f;transition:.2s}
  .fps-exp-master strong{display:block}.fps-exp-master .fps-exp-desc{font-size:11px;color:#8d98a8;margin-top:3px}
  .fps-exp-master.is-on{border-color:var(--fps-exp-border);background:var(--fps-exp-bg);box-shadow:0 0 0 1px rgba(250,204,21,.08) inset}
  .fps-exp-master.is-on strong,.fps-exp-master.is-on .fps-exp-desc{color:var(--fps-exp)}
  .fps-exp-feature{display:none;margin-top:10px;border:1px solid var(--fps-exp-border)!important;background:var(--fps-exp-bg)!important;box-shadow:0 0 0 1px rgba(250,204,21,.05) inset}
  .fps-exp-feature *{border-color:rgba(250,204,21,.35)}
  .fps-exp-feature .fps-exp-title,.fps-exp-feature strong{color:var(--fps-exp)!important}
  .fps-exp-badge{display:inline-block;margin-left:7px;padding:1px 6px;border:1px solid var(--fps-exp-border);border-radius:999px;color:var(--fps-exp);font-size:9px;font-weight:700;letter-spacing:.04em;text-transform:uppercase}
  .fps-exp-feature .fps-inline-desc,.fps-exp-feature .fps-exp-help{color:#cbbd77!important}
  .fps-exp-feature.is-visible{display:block}
 `;document.head.appendChild(style);
 const master=document.createElement('div');master.id='fpsExperimentalMasterWrap';master.className='fps-exp-master';master.innerHTML=`<div><strong>Experimental Features</strong><div class="fps-exp-desc">Off = normal V1 feature set. Turn on to reveal experimental options throughout the configurator.</div></div><label class="switch"><input type="checkbox" id="fpsExperimentalMaster"><span class="track"><span class="thumb"></span></span></label>`;
 const firstCard=panel.querySelector('.config-card');if(firstCard)panel.insertBefore(master,firstCard);else panel.prepend(master);
 const wake=$('wakeGuard'),cameraCard=wake?.closest('.config-card');if(cameraCard){
  const box=document.createElement('div');box.id='fpsMultiCamSyncWrap';box.className='cfg-field fps-exp-feature';box.innerHTML=`<div><div class="fps-exp-title"><strong>Multi Cam Sync</strong><span class="fps-exp-badge">Experimental</span></div><div class="fps-inline-desc fps-exp-help">Connect and control two GoPros from one C3 so arm/disarm recording commands can be kept in sync. Initial test target: 2 cameras.</div></div><label class="switch"><input type="checkbox" id="fpsMultiCamSync"><span class="track"><span class="thumb"></span></span></label>`;
  const note=$('fpsGoProConnectionNote');if(note)note.insertAdjacentElement('afterend',box);else cameraCard.appendChild(box);
 }
 const masterInput=$('fpsExperimentalMaster'),multi=$('fpsMultiCamSync');
 masterInput.checked=localStorage.getItem(EXP_KEY)==='1';if(multi)multi.checked=localStorage.getItem(MULTI_KEY)==='1';
 function syncExp(){const on=masterInput.checked;localStorage.setItem(EXP_KEY,on?'1':'0');master.classList.toggle('is-on',on);document.querySelectorAll('.fps-exp-feature').forEach(x=>x.classList.toggle('is-visible',on));if(!on&&multi){multi.checked=false;localStorage.setItem(MULTI_KEY,'0')}}
 masterInput.addEventListener('change',syncExp);multi?.addEventListener('change',()=>localStorage.setItem(MULTI_KEY,multi.checked?'1':'0'));syncExp();
}
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
function init(){installReadCompletionHook();installExperimentalUi();$('readBtn')?.addEventListener('click',requestRead,true);$('fpsCraftMaster')?.addEventListener('change',()=>{if(healCraftTemplate())fire('fpsCraftMaster')});$('wakeGuard')?.addEventListener('change',syncGoProNote);syncGoProNote()}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,450));else setTimeout(init,450);
})();
