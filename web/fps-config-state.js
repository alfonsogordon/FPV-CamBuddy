(()=>{
'use strict';
const MODE='freeclinkerDemoMode', CFG='freeclinkerDemoConfig', C3='freeclinkerC3Config', FRESH='fpsDefaults20260912Final2';
const $=id=>document.getElementById(id), isDemo=()=>localStorage.getItem(MODE)==='1';
const DEFAULTS={osd1:'{batt}',osd2:'{state} {recdur}',osd3:'{mode} {res} {fps} {eis}',osd4:'{rectf} {rcap}',pilotTpl:'{stateonly} {batt} {rectf}',craftTpl:'',pilotEn:true,craftEn:false,fpvFlash:true,fpvPreArm:true,fpvPreArmText:'CLEAN LENS',fpvCustomDurationSec:'1.0',fpvLowPct:'10',fpvRecLowMin:'5',cameraMatch:'0',wakeGuard:true,stopOnDisarm:true,disarmDelay:'5',debugBle:false,lowPower:true,wifiApEnabled:false,bf45Compat:false};
const IDS=['osd1','osd2','osd3','osd4','pilotTpl','craftTpl','pilotEn','craftEn','fpvFlash','fpvPreArm','fpvPreArmText','fpvCustomDurationSec','fpvLowPct','fpvRecLowMin','cameraMatch','wakeGuard','stopOnDisarm','disarmDelay','debugBle','lowPower','wifiApEnabled','bf45Compat','fpsCustomEnable','fpvLowBatt','auxChannel'];
function read(k){try{return JSON.parse(localStorage.getItem(k)||'{}')}catch{return{}}}function write(k,v){localStorage.setItem(k,JSON.stringify(v))}function put(id,v){const e=$(id);if(e)e.type==='checkbox'?e.checked=!!v:e.value=v}function get(id){const e=$(id);return !e?undefined:(e.type==='checkbox'?e.checked:e.value)}
function defaults(){Object.entries(DEFAULTS).forEach(([k,v])=>put(k,v));put('fpsOsdEnable',false);put('fpsCustomEnable',false);put('fpvLowBatt',false);put('fpsAuxEnable',false);localStorage.setItem('fpsOsdEnabled','0');localStorage.setItem('fpsOsdMessageCount','4');localStorage.setItem('fpsWarningDestination','1');localStorage.setItem('fpsLegacyTempDestination','pilot')}
function capture(){const d={};IDS.forEach(id=>{const v=get(id);if(v!==undefined)d[id]=v});d.fpsOsdEnabled=!!$('fpsOsdEnable')?.checked;d.fpsAuxEnable=!!$('fpsAuxEnable')?.checked;d.fpsOsdMessageCount=Number(localStorage.getItem('fpsOsdMessageCount')||4);d.fpsWarningDestination=localStorage.getItem('fpsWarningDestination')||'1';d.fpsLegacyTempDestination=localStorage.getItem('fpsLegacyTempDestination')||'pilot';return d}
function restore(d){if(!d||!Object.keys(d).length)return false;Object.entries(d).forEach(([k,v])=>{if(IDS.includes(k))put(k,v)});if(d.fpsOsdEnabled!==undefined)put('fpsOsdEnable',d.fpsOsdEnabled);if(d.fpsAuxEnable!==undefined)put('fpsAuxEnable',d.fpsAuxEnable);if(d.fpsOsdMessageCount)localStorage.setItem('fpsOsdMessageCount',String(d.fpsOsdMessageCount));if(d.fpsWarningDestination)localStorage.setItem('fpsWarningDestination',d.fpsWarningDestination);if(d.fpsLegacyTempDestination)localStorage.setItem('fpsLegacyTempDestination',d.fpsLegacyTempDestination);return true}
function mirror(d){write(C3,{...read(C3),...d,bf45_compat:d.bf45Compat?'true':'false',pilot_tpl:d.pilotTpl||'',craft_tpl:d.craftTpl||'',fpv_flash:d.fpvFlash?'true':'false',fpv_prearm:d.fpvPreArm?'true':'false',fpv_prearm_text:d.fpvPreArmText||'',fpv_prearm_show:String(Math.round((Number(d.fpvCustomDurationSec)||1)*1000)),fpv_low_batt:d.fpvLowBatt?'true':'false',fpv_low_pct:String(d.fpvLowPct||10),fpv_rect_min:String(d.fpvRecLowMin||5)})}function save(){if(isDemo()){const d=capture();write(CFG,d);mirror(d)}}
function builder(id){return $(id)?.closest('.fps-builder')}
function featureBody(toggle){const e=$(toggle);if(!e)return[];const head=e.closest('.fps-feature-head');const feature=head?.parentElement;if(!feature)return[];return [...feature.children].filter(x=>x!==head)}
function showFeatureBody(toggle,on){featureBody(toggle).forEach(x=>x.style.display=on?'':'none')}
function refresh(){const legacy=!!$('bf45Compat')?.checked,on=!!$('fpsOsdEnable')?.checked;
 // Only hide whole OSD builders by BF mode/master. Never hide their internal element pickers.
 ['osd1','osd2','osd3','osd4'].forEach(id=>{const b=builder(id);if(b)b.style.display=on&&!legacy?'':'none'});
 for(const[id,en]of[['pilotTpl','pilotEn'],['craftTpl','craftEn']]){const b=builder(id);if(b)b.style.display=on&&legacy&&!!$(en)?.checked?'':'none'}
 const method=$('fpsOsdMethod');if(method)method.value=legacy?'legacy':'current';
 // Shared features exist in BOTH BF modes. Keep their header/toggle visible and collapse only their body.
 showFeatureBody('fpsCustomEnable',!!$('fpsCustomEnable')?.checked);
 showFeatureBody('fpvLowBatt',!!$('fpvLowBatt')?.checked);
 showFeatureBody('fpsAuxEnable',!!$('fpsAuxEnable')?.checked);
 // Explicitly undo stale inline hiding on OSD picker/chip controls from earlier helpers.
 if(on&&!legacy)document.querySelectorAll('.fps-builder').forEach(b=>{const inp=b.querySelector('input[id^="osd"]');if(inp&&/^osd[1-4]$/.test(inp.id)){b.querySelectorAll('.fps-elements,.fps-chip-row,.fps-chip-list,.fps-builder-elements,.fps-element-options').forEach(x=>x.style.display='')}});
}
function init(){if(!$('panel-config'))return;if(!localStorage.getItem(FRESH)){defaults();localStorage.setItem(FRESH,'1');if(isDemo()){write(CFG,capture());mirror(capture())}}if(isDemo()){const d=read(CFG);Object.keys(d).length?restore(d):(defaults(),write(CFG,capture()))}
 // Do not dispatch synthetic change events: older handlers were hiding whole feature containers.
 ['osd1','osd2','osd3','osd4','pilotTpl','craftTpl'].forEach(id=>$(id)?.dispatchEvent(new Event('input',{bubbles:false})));refresh();setTimeout(()=>{if(isDemo())restore(read(CFG));refresh()},80);
 document.addEventListener('input',e=>{if(e.target.closest?.('#panel-config'))setTimeout(()=>{refresh();save()},0)},true);document.addEventListener('change',e=>{if(e.target.closest?.('#panel-config'))setTimeout(()=>{refresh();save()},0)},true);document.addEventListener('click',e=>{if(e.target.closest?.('a')&&isDemo())save()},true);window.addEventListener('pagehide',save);window.addEventListener('beforeunload',save);
 document.querySelectorAll('#panel-config .cfg-field-desc').forEach(e=>{if((e.textContent||'').trim()==='Default: battery percentage')e.style.display='none'});
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,650));else setTimeout(init,650);
})();