(()=>{
'use strict';
const $=id=>document.getElementById(id);
const POWER_VALUES=['-12','-9','-6','-3','0','3','6','9'];
const DEFAULTS={multiCam:false,multiOnly:false,armBoostPower:'9',armBoostMs:'0',disBoostPower:'9',disBoostMs:'0'};
function optionHtml(){return POWER_VALUES.map(v=>`<option value="${v}">${Number(v)>0?'+':''}${v} dBm</option>`).join('')}
function setVal(id,v){const e=$(id);if(!e)return;if(e.type==='checkbox')e.checked=!!v;else e.value=String(v)}
function parseBool(v){return String(v).trim()==='true'||String(v).trim()==='1'}
function valueText(line,key){const r=new RegExp(`^\\[cfg\\]\\s+${key}\\s*=\\s*([^\\s]+)`,'i').exec(String(line||''));return r?r[1]:null}
function style(){if($('fpsV102Style'))return;const s=document.createElement('style');s.id='fpsV102Style';s.textContent=`
#fpsV102Experimental{border:2px solid #e5ef00!important;box-shadow:0 0 0 1px rgba(245,255,0,.12),0 0 22px rgba(245,255,0,.09);background:#1a201b!important}
#fpsV102Experimental>.fps-section-head{background:linear-gradient(90deg,#303922,#252d22)!important;border-bottom:1px solid rgba(245,255,0,.18)}
#fpsV102Experimental>.fps-section-head strong{color:#fbff65!important;text-shadow:none!important}
#fpsV102Experimental>.fps-section-head .fps-section-desc{color:#f0f2df!important;opacity:1!important}
#fpsV102Experimental .fps-section-body{background:#1d241f!important}
#fpsV102Experimental .fps-row strong{color:#fffed0!important}
#fpsV102Experimental .fps-inline-desc{color:#ecefdc!important;opacity:1!important}
#fpsV102Experimental.fps-exp-feature:not(.is-visible){display:none!important}
#fpsV102Experimental.fps-exp-feature.is-visible{display:block}
.fps-v102-badge{display:inline-block;margin-left:7px;padding:2px 6px;border-radius:5px;background:#f5ff00;color:#111;font-size:9px;font-weight:950;letter-spacing:.08em;vertical-align:middle}
.fps-v102-grid{display:grid;grid-template-columns:minmax(0,1fr) minmax(125px,165px);gap:10px 14px;align-items:center}.fps-v102-grid label{font-size:12px;color:#f4f4df!important}.fps-v102-grid select,.fps-v102-grid input[type=number]{width:100%;min-width:0}.fps-v102-note{grid-column:1/-1;color:#edf0bd;font-size:11px;line-height:1.48}.fps-v102-sub{grid-column:1/-1;border-top:1px solid rgba(225,235,90,.34);padding-top:10px;margin-top:2px;font-weight:800;color:#f3f69f}.fps-v102-group{grid-column:1/-1;font-size:11px;font-weight:900;letter-spacing:.05em;text-transform:uppercase;color:#eef38e;margin-top:2px}
.fps-v102-info{grid-column:1/-1;margin:3px 0 2px;border:1px solid rgba(225,235,90,.52);border-radius:7px;background:#252d23;overflow:hidden}.fps-v102-info summary{cursor:pointer;list-style:none;padding:9px 11px;font-size:11px;font-weight:900;color:#fbfcae;user-select:none}.fps-v102-info summary::-webkit-details-marker{display:none}.fps-v102-info summary::before{content:'ⓘ ';color:#f5ff00}.fps-v102-info[open] summary{border-bottom:1px solid rgba(225,235,90,.34)}.fps-v102-info-body{padding:10px 12px 12px;color:#f1f2dc;font-size:11px;line-height:1.6}.fps-v102-info-body p{margin:0 0 8px}.fps-v102-info-body p:last-child{margin-bottom:0}.fps-v102-info-body strong{color:#ffffc7}.fps-v102-info-body ol{margin:7px 0 9px;padding-left:21px}.fps-v102-info-body li{margin:5px 0}.fps-v102-flow{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:6px;margin:9px 0}.fps-v102-step{padding:7px 6px;border:1px solid rgba(225,235,90,.34);border-radius:5px;text-align:center;font-size:10px;line-height:1.35}.fps-v102-step strong{display:block;margin-bottom:2px;color:#ffffba}
.fps-v102-hidden{display:none!important}
.fps-lowpower-locked{opacity:.52;filter:grayscale(.65);transition:opacity .15s ease,filter .15s ease}.fps-lowpower-locked .switch{cursor:not-allowed}.fps-lowpower-locked .fps-inline-desc,.fps-lowpower-locked .cfg-hint{opacity:.9}
#fpsMultiCamMatchInfo{margin:6px 0 10px}.fps-match-multicam-disabled{opacity:.55}
@media(max-width:620px){.fps-v102-grid{grid-template-columns:1fr}.fps-v102-note,.fps-v102-sub,.fps-v102-group,.fps-v102-info{grid-column:1}.fps-v102-flow{grid-template-columns:1fr 1fr}}
`;document.head.appendChild(s)}
function makeToggle(id){return `<label class="switch"><input type="checkbox" id="${id}"><span class="track"><span class="thumb"></span></span></label>`}
function refreshLegacyLowPower(){
 const lp=$('lowPower');if(!lp)return;
 const advanced=!!$('fpsDynamicPowerV102')?.checked;
 const multiOnly=!!$('fpsPowerMultiOnly')?.checked;
 const lock=advanced&&!multiOnly;
 lp.disabled=lock;
 const row=lp.closest('.cfg-field,.fps-row');
 if(row)row.classList.toggle('fps-lowpower-locked',lock);
 lp.setAttribute('aria-disabled',lock?'true':'false');
 lp.title=lock?'Controlled by the Advanced Multi Cam BLE power profile. Enable “Only after multiple cameras detected” to use normal Low Power before the multi-camera latch triggers.':'';
}
function refreshCameraMatching(){
 const match=$('cameraMatch');if(!match)return;
 const multi=!!$('fpsMultiCamSync')?.checked;
 const row=match.closest('.cfg-field,.fps-row');
 match.disabled=multi || (!$('connectBtn')?.disabled && localStorage.getItem('freeclinkerDemoMode')!=='1');
 row?.classList.toggle('fps-match-multicam-disabled',multi);
 let info=$('fpsMultiCamMatchInfo');
 if(!info){info=document.createElement('details');info.id='fpsMultiCamMatchInfo';info.className='fps-v102-info';info.innerHTML='<summary>More info — Camera matching and Multi Cam</summary><div class="fps-v102-info-body"><p><strong>Camera matching is a Single Cam setting.</strong> Fallback, Strict and Strongest Signal decide which one camera FPV CamBuddy should select during a normal single-camera scan.</p><p>When Multi Cam is enabled, FPV CamBuddy is intentionally trying to reconnect multiple saved cameras, so this selector is disabled and does not choose C1 versus C2. Saved camera identities remain persistent for Multi Cam and OSD source selection.</p><p>Turn Multi Cam off to use Camera matching again.</p></div>';row?.insertAdjacentElement('afterend',info)}
 if(info)info.style.display=multi?'':'none';
}
function refreshVisibility(){
 const sec=$('fpsV102Experimental');
 const exp=$('fpsExperimentalMaster');
 const expOn=!!exp?.checked;
 if(sec)sec.classList.toggle('is-visible',expOn);
 const multi=!!$('fpsMultiCamSync')?.checked;
 const dynWrap=$('fpsDynamicPowerV102Wrap');
 const children=$('fpsPowerChildren');
 if(dynWrap)dynWrap.classList.toggle('fps-v102-hidden',!multi);
 const en=multi&&!!$('fpsDynamicPowerV102')?.checked;
 if(children)children.classList.toggle('fps-v102-hidden',!en);
 ['fpsIdlePower','fpsArmBoostPower','fpsArmBoostMs','fpsArmedPowerV102','fpsDisBoostPower','fpsDisBoostMs','fpsPowerMultiOnly'].forEach(id=>{const e=$(id);if(e)e.disabled=!en});
 refreshLegacyLowPower();refreshCameraMatching();
}
function reconcileExperimentalUi(){
 const sec=$('fpsV102Experimental');if(!sec)return;
 sec.classList.add('fps-exp-feature');
 document.querySelectorAll('#fpsMultiCamSyncWrap.fps-exp-feature').forEach(x=>{if(!sec.contains(x))x.remove()});
 const master=$('fpsExperimentalMaster');
 if(master){
  sec.classList.toggle('is-visible',master.checked);
  if(!master.checked){const multi=$('fpsMultiCamSync');if(multi?.checked){multi.checked=false;multi.dispatchEvent(new Event('change',{bubbles:true}))}}
 }
 refreshVisibility();
}
function build(){if($('fpsV102Experimental'))return;const old=$('fpsDynamicPowerWrap');if(!old)return;
 old.style.display='none';
 const sec=document.createElement('section');sec.id='fpsV102Experimental';sec.className='fps-section fps-exp-feature';sec.innerHTML=`
 <div class="fps-section-head"><div><strong>Experimental Multi Cam Settings <span class="fps-v102-badge">V1.0.2</span></strong><div class="fps-section-desc">Multi-camera coordination and optional BLE power controls.</div></div></div>
 <div class="fps-section-body" style="display:grid">
  <div id="fpsMultiCamSyncWrap" class="fps-row fps-toggle-row"><div><strong>Enable Multi Cam coordinator</strong><div class="fps-inline-desc">Connect and control multiple saved cameras together. Reboot required after changing.</div></div>${makeToggle('fpsMultiCamSync')}</div>
  <details class="fps-v102-info">
   <summary>More info — first-time Multi Cam setup</summary>
   <div class="fps-v102-info-body">
    <p><strong>New cameras must be learned one at a time, with a C3 power cycle between each new camera.</strong></p>
    <ol><li>Power on only the first new camera and let FPV CamBuddy discover, connect and save it.</li><li>Power-cycle the C3.</li><li>Power on only the next new camera and let FPV CamBuddy save it.</li><li>Power-cycle the C3 again before adding another new camera.</li></ol>
    <p>If a GoPro has never been paired with FPV CamBuddy before, open that camera's <strong>Pair</strong> menu for the initial connection if required.</p>
    <p>Once all cameras have been learned, normal use is easy: power them on <strong>all together or one by one</strong>. Multi Cam will reconnect to whichever saved cameras are available.</p>
   </div>
  </details>
  <div id="fpsDynamicPowerV102Wrap" class="fps-row fps-toggle-row"><div><strong>Advanced power settings</strong><div class="fps-inline-desc">Optional staged BLE power around arm/disarm transitions.</div></div>${makeToggle('fpsDynamicPowerV102')}</div>
  <div class="fps-v102-grid" id="fpsPowerChildren">
   <details class="fps-v102-info">
    <summary>More info — staged BLE power</summary>
    <div class="fps-v102-info-body">
     <p>Use higher BLE power only around important transitions, then lower it during normal flight.</p>
     <div class="fps-v102-flow"><div class="fps-v102-step"><strong>Idle</strong>Nearby connection before flight.</div><div class="fps-v102-step"><strong>Arm boost</strong>Short boost for START/reconnect.</div><div class="fps-v102-step"><strong>Armed</strong>Lower in-flight BLE power.</div><div class="fps-v102-step"><strong>Disarm boost</strong>Short boost for STOP/reconnect.</div></div>
     <p>A boost duration of <strong>0 ms</strong> skips that boost. <strong>Only after multiple cameras detected</strong> keeps the normal Low Power setting in charge until more than one camera has actually connected; after that the staged profile stays latched until reboot.</p>
   </div>
   </details>
   <div class="fps-v102-group">BLE power stages</div>
   <label for="fpsIdlePower">Idle / disarmed power</label><select id="fpsIdlePower">${optionHtml()}</select>
   <label for="fpsArmBoostPower">Arm-boost power</label><select id="fpsArmBoostPower">${optionHtml()}</select>
   <label for="fpsArmBoostMs">Arm-boost duration (ms)</label><input id="fpsArmBoostMs" type="number" min="0" max="60000" step="100" value="0">
   <label for="fpsArmedPowerV102">Armed power</label><select id="fpsArmedPowerV102">${optionHtml()}</select>
   <label for="fpsDisBoostPower">Disarm-boost power</label><select id="fpsDisBoostPower">${optionHtml()}</select>
   <label for="fpsDisBoostMs">Disarm-boost duration (ms)</label><input id="fpsDisBoostMs" type="number" min="0" max="60000" step="100" value="0">
   <div class="fps-v102-sub">Multi-camera activation</div>
   <div><strong>Only after multiple cameras detected</strong><div class="fps-inline-desc">Keep normal Low Power mode until more than one camera has connected. Once triggered, stays active until reboot.</div></div>${makeToggle('fpsPowerMultiOnly')}
   <div class="fps-v102-note">Experimental RF behaviour: bench-test camera reacquisition and RC link quality before flight.</div>
  </div>
 </div>`;
 old.insertAdjacentElement('afterend',sec);
 setVal('fpsMultiCamSync',DEFAULTS.multiCam);setVal('fpsDynamicPowerV102',$('fpsDynamicPower')?.checked||false);setVal('fpsIdlePower',$('fpsDisarmedPower')?.value||'9');setVal('fpsArmedPowerV102',$('fpsArmedPower')?.value||'-12');setVal('fpsArmBoostPower',DEFAULTS.armBoostPower);setVal('fpsArmBoostMs',DEFAULTS.armBoostMs);setVal('fpsDisBoostPower',DEFAULTS.disBoostPower);setVal('fpsDisBoostMs',DEFAULTS.disBoostMs);setVal('fpsPowerMultiOnly',DEFAULTS.multiOnly);
 const mirror=()=>{
  if($('fpsDynamicPower'))$('fpsDynamicPower').checked=$('fpsDynamicPowerV102').checked;
  if($('fpsDisarmedPower'))$('fpsDisarmedPower').value=$('fpsIdlePower').value;
  if($('fpsArmedPower'))$('fpsArmedPower').value=$('fpsArmedPowerV102').value;
  refreshVisibility();
 };
 sec.addEventListener('change',mirror);sec.addEventListener('input',mirror);mirror();
 setTimeout(reconcileExperimentalUi,550);setTimeout(reconcileExperimentalUi,900);setTimeout(()=>window.fpsFirmwareVersion?.applyFeatureGates?.(),100);
}
function receive(line){let v;
 if((v=valueText(line,'multi_cam'))!==null)setVal('fpsMultiCamSync',parseBool(v));
 else if((v=valueText(line,'dyn_power'))!==null){setVal('fpsDynamicPowerV102',parseBool(v));setVal('fpsDynamicPower',parseBool(v))}
 else if((v=valueText(line,'arm_dbm'))!==null){setVal('fpsArmedPowerV102',parseInt(v,10));setVal('fpsArmedPower',parseInt(v,10))}
 else if((v=valueText(line,'dis_dbm'))!==null){setVal('fpsIdlePower',parseInt(v,10));setVal('fpsDisarmedPower',parseInt(v,10))}
 else if((v=valueText(line,'arm_boost_dbm'))!==null)setVal('fpsArmBoostPower',parseInt(v,10));
 else if((v=valueText(line,'arm_boost_ms'))!==null)setVal('fpsArmBoostMs',parseInt(v,10)||0);
 else if((v=valueText(line,'dis_boost_dbm'))!==null)setVal('fpsDisBoostPower',parseInt(v,10));
 else if((v=valueText(line,'dis_boost_ms'))!==null)setVal('fpsDisBoostMs',parseInt(v,10)||0);
 else if((v=valueText(line,'power_multi_only'))!==null)setVal('fpsPowerMultiOnly',parseBool(v));
 refreshVisibility();
}
function hookParser(){if(typeof parseConfigLine!=='function'||parseConfigLine.__fpsV102Wrapped)return false;const original=parseConfigLine;const wrapped=function(line){receive(line);return original(line)};wrapped.__fpsV102Wrapped=true;parseConfigLine=wrapped;return true}
function init(){style();let tries=0;const t=setInterval(()=>{build();hookParser();reconcileExperimentalUi();if($('fpsV102Experimental')&&typeof parseConfigLine==='function'){clearInterval(t);refreshVisibility();window.fpsFirmwareVersion?.applyFeatureGates?.()}else if(++tries>40)clearInterval(t)},100);document.addEventListener('change',e=>{if(e.target?.id==='fpsExperimentalMaster'||e.target?.id==='fpsMultiCamSync')setTimeout(reconcileExperimentalUi,0)},true)}
window.fpsV102Experimental={receive,refreshVisibility,reconcileExperimentalUi,refreshLegacyLowPower,refreshCameraMatching};
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();