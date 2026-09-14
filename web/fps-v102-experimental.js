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
#fpsV102Experimental{border:2px solid #e5ef00!important;box-shadow:0 0 0 1px rgba(245,255,0,.12),0 0 22px rgba(245,255,0,.09)}
#fpsV102Experimental>.fps-section-head{background:linear-gradient(90deg,rgba(245,255,0,.16),rgba(245,255,0,.04))}
#fpsV102Experimental.fps-exp-feature:not(.is-visible){display:none!important}
#fpsV102Experimental.fps-exp-feature.is-visible{display:block}
.fps-v102-badge{display:inline-block;margin-left:7px;padding:2px 6px;border-radius:5px;background:#f5ff00;color:#111;font-size:9px;font-weight:950;letter-spacing:.08em;vertical-align:middle}
.fps-v102-grid{display:grid;grid-template-columns:minmax(0,1fr) minmax(125px,165px);gap:10px 14px;align-items:center}.fps-v102-grid label{font-size:12px}.fps-v102-grid select,.fps-v102-grid input[type=number]{width:100%;min-width:0}.fps-v102-note{grid-column:1/-1;color:#786f32;font-size:11px;line-height:1.45}.fps-v102-sub{grid-column:1/-1;border-top:1px solid rgba(180,185,0,.24);padding-top:10px;margin-top:2px;font-weight:800;color:#665f00}.fps-v102-group{grid-column:1/-1;font-size:11px;font-weight:900;letter-spacing:.05em;text-transform:uppercase;color:#756d13;margin-top:2px}
.fps-v102-info{grid-column:1/-1;margin:3px 0 2px;border:1px solid rgba(211,219,43,.35);border-radius:7px;background:rgba(245,255,0,.045);overflow:hidden}.fps-v102-info summary{cursor:pointer;list-style:none;padding:9px 11px;font-size:11px;font-weight:900;color:#8b831c;user-select:none}.fps-v102-info summary::-webkit-details-marker{display:none}.fps-v102-info summary::before{content:'ⓘ ';color:#a59b12}.fps-v102-info[open] summary{border-bottom:1px solid rgba(211,219,43,.22)}.fps-v102-info-body{padding:10px 12px 12px;color:#756f37;font-size:11px;line-height:1.55}.fps-v102-info-body p{margin:0 0 8px}.fps-v102-info-body p:last-child{margin-bottom:0}.fps-v102-info-body strong{color:#675f00}.fps-v102-flow{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:6px;margin:9px 0}.fps-v102-step{padding:7px 6px;border:1px solid rgba(211,219,43,.25);border-radius:5px;text-align:center;font-size:10px;line-height:1.35}.fps-v102-step strong{display:block;margin-bottom:2px}
.fps-v102-hidden{display:none!important}
.fps-lowpower-locked{opacity:.42;filter:grayscale(.8);transition:opacity .15s ease,filter .15s ease}.fps-lowpower-locked .switch{cursor:not-allowed}.fps-lowpower-locked .fps-inline-desc,.fps-lowpower-locked .cfg-hint{opacity:.8}
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
function refreshVisibility(){
 const sec=$('fpsV102Experimental');
 const exp=$('fpsExperimentalMaster');
 const expOn=!!exp?.checked;
 if(sec)sec.classList.toggle('is-visible',expOn);
 const multi=!!$('fpsMultiCamSync')?.checked;
 const dynWrap=$('fpsDynamicPowerV102Wrap');
 const children=$('fpsPowerChildren');
 if(dynWrap)dynWrap.classList.toggle('fps-v102-hidden',!multi);
 if(children)children.classList.toggle('fps-v102-hidden',!multi);
 const en=multi&&!!$('fpsDynamicPowerV102')?.checked;
 ['fpsIdlePower','fpsArmBoostPower','fpsArmBoostMs','fpsArmedPowerV102','fpsDisBoostPower','fpsDisBoostMs','fpsPowerMultiOnly'].forEach(id=>{const e=$(id);if(e)e.disabled=!en});
 refreshLegacyLowPower();
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
 <div class="fps-section-head"><div><strong>Experimental Multi Cam Settings <span class="fps-v102-badge">V1.0.2</span></strong><div class="fps-section-desc">Everything specific to the V1.0.2 Multi Cam test build is grouped here: multi-camera coordination and the BLE power profile used around arm/disarm transitions.</div></div></div>
 <div class="fps-section-body" style="display:grid">
  <div id="fpsMultiCamSyncWrap" class="fps-row fps-toggle-row"><div><strong>Enable Multi Cam coordinator</strong><div class="fps-inline-desc">Discover and coordinate all supported ready camera families. Reboot required after changing.</div></div>${makeToggle('fpsMultiCamSync')}</div>
  <div id="fpsDynamicPowerV102Wrap" class="fps-row fps-toggle-row"><div><strong>Enable Multi Cam BLE power profile</strong><div class="fps-inline-desc">Idle → arm boost → armed → disarm boost → idle. Boost timings of 0 ms preserve the earlier two-state behaviour.</div></div>${makeToggle('fpsDynamicPowerV102')}</div>
  <div class="fps-v102-grid" id="fpsPowerChildren">
   <details class="fps-v102-info">
    <summary>More info — why use several BLE power levels?</summary>
    <div class="fps-v102-info-body">
     <p>The aim is to use <strong>high BLE power only when it is most useful</strong>, then reduce it during normal flight. Around ARM and DISARM the C3 may need to send important start/stop commands, recover a camera that is reconnecting, or bring multiple cameras into the same state. A short high-power boost gives those transitions the best chance of succeeding.</p>
     <div class="fps-v102-flow"><div class="fps-v102-step"><strong>Idle</strong>Reliable nearby connection while preparing the quad.</div><div class="fps-v102-step"><strong>Arm boost</strong>Brief strong signal for START commands and reconciliation.</div><div class="fps-v102-step"><strong>Armed</strong>Lower power during flight to reduce unnecessary 2.4 GHz RF near the receiver and flight electronics.</div><div class="fps-v102-step"><strong>Disarm boost</strong>Brief strong signal for STOP commands and camera reacquisition.</div></div>
     <p>After the disarm boost expires, the C3 returns to the configured idle power. Setting either boost duration to <strong>0 ms</strong> skips that boost, giving you a simpler two-level armed/disarmed setup.</p>
     <p><strong>Only after multiple cameras detected</strong> is useful if you want the normal Low Power setting to remain in charge for ordinary single-camera use. The staged profile takes over only after the C3 has actually seen more than one connected camera, then stays latched until reboot so the RF behaviour does not keep changing as a camera briefly drops in and out.</p>
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
   <div><strong>Only after multiple cameras detected</strong><div class="fps-inline-desc">Do not override normal Low Power mode until the coordinator has observed more than one connected camera. Once triggered, the latch stays active until reboot even if a camera disconnects.</div></div>${makeToggle('fpsPowerMultiOnly')}
   <div class="fps-v102-note">Power levels use the ESP32 BLE steps from -12 to +9 dBm. This is experimental RF behaviour: bench-test camera reacquisition and RC link quality before flight.</div>
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
 setTimeout(reconcileExperimentalUi,550);
 setTimeout(reconcileExperimentalUi,900);
 setTimeout(()=>window.fpsFirmwareVersion?.applyFeatureGates?.(),100);
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
function init(){style();let tries=0;const t=setInterval(()=>{build();hookParser();reconcileExperimentalUi();if($('fpsV102Experimental')&&typeof parseConfigLine==='function'){clearInterval(t);refreshVisibility();window.fpsFirmwareVersion?.applyFeatureGates?.()}else if(++tries>40)clearInterval(t)},100);document.addEventListener('change',e=>{if(e.target?.id==='fpsExperimentalMaster')setTimeout(reconcileExperimentalUi,0)},true)}
window.fpsV102Experimental={receive,refreshVisibility,reconcileExperimentalUi,refreshLegacyLowPower};
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();