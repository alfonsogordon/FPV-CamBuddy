(()=>{
'use strict';
const $=id=>document.getElementById(id), REV='fpsUiPolish20260912v1', STORE='fpsUiV2State', DEMO='freeclinkerDemoMode', DEMOCFG='freeclinkerDemoConfig';
const defaults={osdEnabled:false,tempEnabled:false,warnEnabled:false,auxEnabled:false,bfMode:'current',msgEnabled:[true,true,true,true],osd1:'{batt}',osd2:'{state} {recdur}',osd3:'{mode} {res} {fps} {eis}',osd4:'{rectf} {rcap}',pilotEn:true,pilotTpl:'{stateonly} {batt} {rectf}',craftEn:false,craftTpl:'',fpvFlash:true,fpvPreArm:true,fpvPreArmText:'CLEAN LENS',fpvCustomDurationSec:'1.0',fpvLowPct:'10',fpvRecLowMin:'5',tempDest:'1',warnDest:'1',auxChannel:'0',cameraMatch:'0',wakeGuard:true,stopOnDisarm:true,disarmDelay:'5',debugBle:false,lowPower:true,wifiApEnabled:false};
function parse(k){try{return JSON.parse(localStorage.getItem(k)||'{}')}catch{return{}}}
function seedOnce(){if(localStorage.getItem(REV)==='1')return;const current=parse(STORE);const next={...current,...defaults};localStorage.setItem(STORE,JSON.stringify(next));if(localStorage.getItem(DEMO)==='1')localStorage.setItem(DEMOCFG,JSON.stringify(next));localStorage.setItem(REV,'1');location.reload()}
function init(){seedOnce();const master=$('fpsOsdMaster'),top=document.querySelector('.fps-top');if(!master||!top)return;
 const versionLabel=top.querySelector(':scope > label');
 function collapsed(){const on=master.checked;if(versionLabel)versionLabel.style.display=on?'grid':'none';document.querySelectorAll('.fps-section').forEach(s=>{const id=s.querySelector('.fps-section-head input')?.id;if(['fpsTempMaster','fpsWarnMaster'].includes(id))s.style.display=on?'':'none'});const apply=$('fpsApplyAll')?.closest('.fps-apply');if(apply)apply.style.display=on?'flex':'none'}
 master.addEventListener('change',()=>setTimeout(collapsed,0));collapsed();
 const aux=$('fpsAuxMaster');if(aux){const section=aux.closest('.fps-section'),body=section?.querySelector('.fps-section-body');if(body&&!body.querySelector('.fps-aux-note')){const note=document.createElement('div');note.className='fps-note fps-aux-note';note.textContent='Leave this off if you only want arming/disarming the quad to start and stop recording.';body.prepend(note)}}
 // Keep the global OSD features visually grouped with OSD and cleanly collapsed.
 document.querySelectorAll('.fps-section').forEach(s=>{const id=s.querySelector('.fps-section-head input')?.id;if(['fpsTempMaster','fpsWarnMaster','fpsAuxMaster'].includes(id)){const b=s.querySelector('.fps-section-body');if(b)b.hidden=!s.querySelector('.fps-section-head input').checked}});
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(init,260));else setTimeout(init,260);
})();