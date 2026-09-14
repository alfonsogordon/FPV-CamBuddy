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
.fps-v102-badge{display:inline-block;margin-left:7px;padding:2px 6px;border-radius:5px;background:#f5ff00;color:#111;font-size:9px;font-weight:950;letter-spacing:.08em;vertical-align:middle}
.fps-v102-grid{display:grid;grid-template-columns:minmax(0,1fr) minmax(125px,165px);gap:10px 14px;align-items:center}.fps-v102-grid label{font-size:12px}.fps-v102-grid select,.fps-v102-grid input[type=number]{width:100%;min-width:0}.fps-v102-note{grid-column:1/-1;color:#786f32;font-size:11px;line-height:1.45}.fps-v102-sub{grid-column:1/-1;border-top:1px solid rgba(180,185,0,.24);padding-top:10px;margin-top:2px;font-weight:800;color:#665f00}
@media(max-width:620px){.fps-v102-grid{grid-template-columns:1fr}.fps-v102-note,.fps-v102-sub{grid-column:1}}
`;document.head.appendChild(s)}
function makeToggle(id){return `<label class="switch"><input type="checkbox" id="${id}"><span class="track"><span class="thumb"></span></span></label>`}
function build(){if($('fpsV102Experimental'))return;const old=$('fpsDynamicPowerWrap');if(!old)return;
 old.style.display='none';
 const sec=document.createElement('section');sec.id='fpsV102Experimental';sec.className='fps-section';sec.innerHTML=`
 <div class="fps-section-head"><div><strong>Multi Cam + BLE Power <span class="fps-v102-badge">EXPERIMENTAL V1.0.2</span></strong><div class="fps-section-desc">Test-only controls for coordinating multiple supported cameras and changing BLE TX power around arm/disarm transitions.</div></div></div>
 <div class="fps-section-body" style="display:grid">
  <div id="fpsMultiCamSyncWrap" class="fps-row fps-toggle-row"><div><strong>Multi Cam coordinator</strong><div class="fps-inline-desc">Discover and coordinate all supported ready camera families. Reboot required after changing.</div></div>${makeToggle('fpsMultiCamSync')}</div>
  <div id="fpsDynamicPowerV102Wrap" class="fps-row fps-toggle-row"><div><strong>Advanced BLE power profile</strong><div class="fps-inline-desc">Idle → arm boost → armed → disarm boost → idle. Boost timings of 0 ms preserve the earlier two-state behaviour.</div></div>${makeToggle('fpsDynamicPowerV102')}</div>
  <div class="fps-v102-grid" id="fpsPowerChildren">
   <label for="fpsIdlePower">Idle / disarmed power</label><select id="fpsIdlePower">${optionHtml()}</select>
   <label for="fpsArmBoostPower">Arm-boost power</label><select id="fpsArmBoostPower">${optionHtml()}</select>
   <label for="fpsArmBoostMs">Arm-boost duration (ms)</label><input id="fpsArmBoostMs" type="number" min="0" max="60000" step="100" value="0">
   <label for="fpsArmedPowerV102">Armed power</label><select id="fpsArmedPowerV102">${optionHtml()}</select>
   <label for="fpsDisBoostPower">Disarm-boost power</label><select id="fpsDisBoostPower">${optionHtml()}</select>
   <label for="fpsDisBoostMs">Disarm-boost duration (ms)</label><input id="fpsDisBoostMs" type="number" min="0" max="60000" step="100" value="0">
   <div class="fps-v102-sub">Latch behaviour</div>
   <div><strong>Only after multiple cameras detected</strong><div class="fps-inline-desc">Do not override normal Low Power mode until the coordinator has observed more than one connected camera. Once triggered, the latch stays active until reboot even if a camera disconnects.</div></div>${makeToggle('fpsPowerMultiOnly')}
   <div class="fps-v102-note">Power levels use the ESP32 BLE steps from -12 to +9 dBm. This is experimental RF behaviour: bench-test camera reacquisition and RC link quality before flight.</div>
  </div>
 </div>`;
 old.insertAdjacentElement('afterend',sec);
 setVal('fpsMultiCamSync',DEFAULTS.multiCam);setVal('fpsDynamicPowerV102',$('fpsDynamicPower')?.checked||false);setVal('fpsIdlePower',$('fpsDisarmedPower')?.value||'9');setVal('fpsArmedPowerV102',$('fpsArmedPower')?.value||'-12');setVal('fpsArmBoostPower',DEFAULTS.armBoostPower);setVal('fpsArmBoostMs',DEFAULTS.armBoostMs);setVal('fpsDisBoostPower',DEFAULTS.disBoostPower);setVal('fpsDisBoostMs',DEFAULTS.disBoostMs);setVal('fpsPowerMultiOnly',DEFAULTS.multiOnly);
 const mirror=()=>{if($('fpsDynamicPower'))$('fpsDynamicPower').checked=$('fpsDynamicPowerV102').checked;if($('fpsDisarmedPower'))$('fpsDisarmedPower').value=$('fpsIdlePower').value;if($('fpsArmedPower'))$('fpsArmedPower').value=$('fpsArmedPowerV102').value;const en=$('fpsDynamicPowerV102').checked;['fpsIdlePower','fpsArmBoostPower','fpsArmBoostMs','fpsArmedPowerV102','fpsDisBoostPower','fpsDisBoostMs','fpsPowerMultiOnly'].forEach(id=>{$(id).disabled=!en})};
 sec.addEventListener('change',mirror);sec.addEventListener('input',mirror);mirror();
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
 const master=$('fpsDynamicPowerV102');if(master)master.dispatchEvent(new Event('change'));
}
function hookParser(){if(typeof parseConfigLine!=='function'||parseConfigLine.__fpsV102Wrapped)return false;const original=parseConfigLine;const wrapped=function(line){receive(line);return original(line)};wrapped.__fpsV102Wrapped=true;parseConfigLine=wrapped;return true}
function init(){style();let tries=0;const t=setInterval(()=>{build();hookParser();if($('fpsV102Experimental')&&typeof parseConfigLine==='function'){clearInterval(t);window.fpsFirmwareVersion?.applyFeatureGates?.()}else if(++tries>40)clearInterval(t)},100)}
window.fpsV102Experimental={receive};
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();
