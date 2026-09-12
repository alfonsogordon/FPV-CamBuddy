(()=>{
'use strict';
const MODE='freeclinkerDemoMode', CFG='freeclinkerDemoConfig', C3='freeclinkerC3Config', FRESH='fpsDefaults20260912Final1';
const $=id=>document.getElementById(id), isDemo=()=>localStorage.getItem(MODE)==='1';
const DEFAULTS={
 osd1:'{batt}',osd2:'{state} {recdur}',osd3:'{mode} {res} {fps} {eis}',osd4:'{rectf} {rcap}',
 pilotTpl:'{stateonly} {batt} {rectf}',craftTpl:'',pilotEn:true,craftEn:false,fpvFlash:true,
 fpvPreArm:true,fpvPreArmText:'CLEAN LENS',fpvCustomDurationSec:'1.0',fpvLowPct:'10',fpvRecLowMin:'5',
 cameraMatch:'0',wakeGuard:true,stopOnDisarm:true,disarmDelay:'5',debugBle:false,lowPower:true,wifiApEnabled:false,
 bf45Compat:false,fpvOsdEnabled:false,fpsCustomEnable:false,fpvLowBatt:false,fpsAuxEnable:false,auxChannel:'0'
};
const IDS=['osd1','osd2','osd3','osd4','pilotTpl','craftTpl','pilotEn','craftEn','fpvFlash','fpvPreArm','fpvPreArmText','fpvCustomDurationSec','fpvLowPct','fpvRecLowMin','cameraMatch','wakeGuard','stopOnDisarm','disarmDelay','debugBle','lowPower','wifiApEnabled','bf45Compat','fpsCustomEnable','fpvLowBatt','auxChannel'];
function read(k){try{return JSON.parse(localStorage.getItem(k)||'{}')}catch{return{}}} function write(k,v){localStorage.setItem(k,JSON.stringify(v))}
function put(id,v){const e=$(id);if(!e)return;e.type==='checkbox'?e.checked=!!v:e.value=v}
function get(id){const e=$(id);return !e?undefined:(e.type==='checkbox'?e.checked:e.value)}
function applyDefaults(){for(const [k,v] of Object.entries(DEFAULTS))if(k!=='fpvOsdEnabled'&&k!=='fpsAuxEnable')put(k,v);if($('fpsOsdEnable'))$('fpsOsdEnable').checked=false;if($('fpsCustomEnable'))$('fpsCustomEnable').checked=false;if($('fpvLowBatt'))$('fpvLowBatt').checked=false;if($('fpsAuxEnable'))$('fpsAuxEnable').checked=false;localStorage.setItem('fpsOsdEnabled','0');localStorage.setItem('fpsOsdMessageCount','4');localStorage.setItem('fpsWarningDestination','1');localStorage.setItem('fpsLegacyTempDestination','pilot')}
function capture(){const d={};IDS.forEach(id=>{const v=get(id);if(v!==undefined)d[id]=v});d.fpsOsdEnabled=!!$('fpsOsdEnable')?.checked;d.fpsAuxEnable=!!$('fpsAuxEnable')?.checked;d.fpsOsdMessageCount=Number(localStorage.getItem('fpsOsdMessageCount')||4);d.fpsWarningDestination=localStorage.getItem('fpsWarningDestination')||'1';d.fpsLegacyTempDestination=localStorage.getItem('fpsLegacyTempDestination')||'pilot';return d}
function restore(d){if(!d||!Object.keys(d).length)return false;for(const[k,v]of Object.entries(d))if(IDS.includes(k))put(k,v);if($('fpsOsdEnable')&&d.fpsOsdEnabled!==undefined)$('fpsOsdEnable').checked=!!d.fpsOsdEnabled;if($('fpsAuxEnable')&&d.fpsAuxEnable!==undefined)$('fpsAuxEnable').checked=!!d.fpsAuxEnable;if(d.fpsOsdMessageCount)localStorage.setItem('fpsOsdMessageCount',String(d.fpsOsdMessageCount));if(d.fpsWarningDestination)localStorage.setItem('fpsWarningDestination',d.fpsWarningDestination);if(d.fpsLegacyTempDestination)localStorage.setItem('fpsLegacyTempDestination',d.fpsLegacyTempDestination);return true}
function mirror(d){const old=read(C3);write(C3,{...old,...d,bf45_compat:d.bf45Compat?'true':'false',pilot_tpl:d.pilotTpl||'',craft_tpl:d.craftTpl||'',fpv_flash:d.fpvFlash?'true':'false',fpv_prearm:d.fpvPreArm?'true':'false',fpv_prearm_text:d.fpvPreArmText||'',fpv_prearm_show:String(Math.round((Number(d.fpvCustomDurationSec)||1)*1000)),fpv_low_batt:d.fpvLowBatt?'true':'false',fpv_low_pct:String(d.fpvLowPct||10),fpv_rect_min:String(d.fpvRecLowMin||5)})}
function save(){if(!isDemo())return;const d=capture();write(CFG,d);mirror(d)}
function refresh(){const legacy=!!$('bf45Compat')?.checked,on=!!$('fpsOsdEnable')?.checked;const osdCard=$('osd1')?.closest('.config-card');if(osdCard)osdCard.querySelectorAll('.fps-builder').forEach(b=>{const id=b.dataset.input||b.querySelector('input[id]')?.id;const current=['osd1','osd2','osd3','osd4'].includes(id),leg=['pilotTpl','craftTpl'].includes(id);let show=on&&((!legacy&&current)||(legacy&&leg));if(id==='pilotTpl')show=show&&!!$('pilotEn')?.checked;if(id==='craftTpl')show=show&&!!$('craftEn')?.checked;b.style.display=show?'':'none'});
 for(const[id,en]of[['pilotTpl','pilotEn'],['craftTpl','craftEn']]){const b=$(id)?.closest('.fps-builder');if(b)b.style.display=on&&legacy&&!!$(en)?.checked?'':'none'}
 const method=$('fpsOsdMethod');if(method)method.value=legacy?'legacy':'current';
 const custom=$('fpsCustomEnable')?.checked;[$('fpvPreArmText'),$('fpvPreArm'),$('fpvCustomDurationSec')].forEach(e=>{const r=e?.closest('.cfg-field')||e?.parentElement;if(r)r.style.display=custom?'':'none';if(e)e.disabled=!custom});
 const warn=$('fpvLowBatt')?.checked;[$('fpvLowPct'),$('fpvRecLowMin')].forEach(e=>{const r=e?.closest('.cfg-field')||e?.parentElement;if(r)r.style.display=warn?'':'none'});
}
function fireVisuals(){['bf45Compat','pilotEn','craftEn','fpsOsdEnable','fpsCustomEnable','fpvLowBatt','fpsAuxEnable'].forEach(id=>$(id)?.dispatchEvent(new Event('change',{bubbles:true})));['osd1','osd2','osd3','osd4','pilotTpl','craftTpl'].forEach(id=>$(id)?.dispatchEvent(new Event('input',{bubbles:false})));refresh()}
function init(){if(!$('panel-config'))return;
 // Fresh browser: seed every requested value, but leave optional feature masters OFF.
 if(!localStorage.getItem(FRESH)){applyDefaults();localStorage.setItem(FRESH,'1');if(isDemo()){write(CFG,capture());mirror(capture())}}
 // In Demo Mode the saved virtual-C3 snapshot is authoritative on every page load.
 if(isDemo()){const d=read(CFG);if(Object.keys(d).length)restore(d);else{applyDefaults();write(CFG,capture())}}
 fireVisuals();
 // Some older UI helpers mutate values during their own change handlers; restore once more after they finish.
 setTimeout(()=>{if(isDemo())restore(read(CFG));refresh()},50);
 document.addEventListener('input',e=>{if(e.target.closest?.('#panel-config'))setTimeout(()=>{refresh();save()},0)},true);
 document.addEventListener('change',e=>{if(e.target.closest?.('#panel-config'))setTimeout(()=>{refresh();save()},0)},true);
 document.addEventListener('click',e=>{const a=e.target.closest?.('a');if(a&&isDemo())save()},true);
 window.addEventListener('pagehide',save);window.addEventListener('beforeunload',save);
 // Remove stale helper copy that contradicts the actual four-message defaults.
 document.querySelectorAll('#panel-config .cfg-field-desc').forEach(e=>{if((e.textContent||'').trim()==='Default: battery percentage')e.style.display='none'});
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,650));else setTimeout(init,650);
})();