(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const $=id=>document.getElementById(id);
function checkedOrStored(id,key){const e=$(id);if(e)return !!e.checked;return localStorage.getItem(key)==='1'}
function expOn(){return checkedOrStored('fpsExperimentalMaster','fpsExperimentalFeatures')}
function multiOn(){return expOn()&&!!$('fpsMultiCamSync')?.checked}
function osdOn(){return !!$('fpsOsdMaster')?.checked}
function advancedOn(){return multiOn()&&osdOn()&&!!$('fpsAdvancedMultiOsd')?.checked}
function dedupePreviewIds(){
 if(!advancedOn())return;
 document.querySelectorAll('#fpsPreviewRows .fps-preview-line').forEach(line=>{
  const raw=String(line.textContent||'');
  const matches=[...raw.matchAll(/-([A-Z0-9][A-Z0-9 _-]{0,6})(?=\s|$)/g)];
  if(matches.length<2)return;
  const ids=matches.map(m=>m[1]);
  if(new Set(ids).size!==1)return;
  const id=ids[0];
  let seen=false;
  const esc=id.replace(/[.*+?^${}()|[\]\\]/g,'\\$&');
  const next=raw.replace(new RegExp('-'+esc+'(?=\\s|$)','g'),m=>{if(!seen){seen=true;return m}return''}).replace(/\s+/g,' ').trim();
  if(next!==raw)line.textContent=next;
 });
}
function repair(){
 let panel=$('fpsAdvancedMultiOsdPanel');
 const master=$('fpsOsdMaster'),top=master?.closest('.fps-top');
 if(!panel&&multiOn()&&osdOn()){window.fpsMultiCamOsd?.refresh?.();panel=$('fpsAdvancedMultiOsdPanel')}
 if(panel&&top&&panel.previousElementSibling!==top)top.insertAdjacentElement('afterend',panel);
 if(panel){const visible=multiOn()&&osdOn();panel.classList.add('fps-exp-yellow');panel.classList.toggle('fps-multiosd-visible',visible);panel.style.display=visible?'block':'none'}
 document.querySelectorAll('.fps-multiosd-builder-tools').forEach(x=>x.style.display=advancedOn()?'grid':'none');
 const sim=$('fpsMultiOsdSimulator');if(sim)sim.style.display=advancedOn()?'grid':'none';
 dedupePreviewIds();
}
function addStyle(){if($('fpsAdvancedMultiOsdYellowStyle'))return;const s=document.createElement('style');s.id='fpsAdvancedMultiOsdYellowStyle';s.textContent=`
#fpsAdvancedMultiOsdPanel.fps-exp-yellow.fps-multiosd-visible{display:block!important}
#fpsAdvancedMultiOsdPanel.fps-exp-yellow:not(.fps-multiosd-visible){display:none!important}
#fpsAdvancedMultiOsdPanel.fps-exp-yellow{border:2px solid #e5ef00!important;box-shadow:0 0 0 1px rgba(245,255,0,.12),0 0 22px rgba(245,255,0,.09);background:#1a201b!important}
#fpsAdvancedMultiOsdPanel.fps-exp-yellow>.fps-section-head{background:linear-gradient(90deg,#303922,#252d22)!important;border-bottom:1px solid rgba(245,255,0,.18)}
#fpsAdvancedMultiOsdPanel.fps-exp-yellow>.fps-section-head strong{color:#fbff65!important}
#fpsAdvancedMultiOsdPanel.fps-exp-yellow>.fps-section-head .fps-section-desc{color:#f0f2df!important;opacity:1!important}
#fpsAdvancedMultiOsdPanel.fps-exp-yellow>.fps-section-body{background:#1d241f!important}
#fpsAdvancedMultiOsdPanel .fps-multiosd-exp{display:inline-block;margin-left:7px;padding:2px 6px;border-radius:5px;background:#f5ff00!important;color:#111!important;font-size:9px;font-weight:950;letter-spacing:.08em}
`;document.head.appendChild(s)}
let pending=false;function queue(){if(pending)return;pending=true;requestAnimationFrame(()=>{pending=false;repair()})}
document.addEventListener('change',e=>{if(['fpsMultiCamSync','fpsExperimentalMaster','fpsOsdMaster','fpsAdvancedMultiOsd'].includes(e.target?.id))setTimeout(queue,0)},true);
new MutationObserver(queue).observe(document.documentElement,{childList:true,subtree:true,characterData:true});
addStyle();
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>{setTimeout(repair,500);setTimeout(repair,1000)});else{setTimeout(repair,500);setTimeout(repair,1000)}
})();