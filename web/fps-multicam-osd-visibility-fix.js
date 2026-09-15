(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const $=id=>document.getElementById(id);
function checkedOrStored(id,key){const e=$(id);if(e)return !!e.checked;return localStorage.getItem(key)==='1'}
function expOn(){return checkedOrStored('fpsExperimentalMaster','fpsExperimentalFeatures')}
function multiOn(){return expOn()&&checkedOrStored('fpsMultiCamSync','fpsExperimentalMultiCamSync')}
function osdOn(){return !!$('fpsOsdMaster')?.checked}
function advancedOn(){return multiOn()&&osdOn()&&!!$('fpsAdvancedMultiOsd')?.checked}
function repair(){
 let panel=$('fpsAdvancedMultiOsdPanel');
 const master=$('fpsOsdMaster'),top=master?.closest('.fps-top');
 if(!panel&&multiOn()&&osdOn()){window.fpsMultiCamOsd?.refresh?.();panel=$('fpsAdvancedMultiOsdPanel')}
 if(panel&&top&&panel.previousElementSibling!==top)top.insertAdjacentElement('afterend',panel);
 if(panel){panel.style.display=multiOn()&&osdOn()?'':'none';panel.classList.add('fps-exp-yellow')}
 document.querySelectorAll('.fps-multiosd-builder-tools').forEach(x=>x.style.display=advancedOn()?'grid':'none');
 const sim=$('fpsMultiOsdSimulator');if(sim)sim.style.display=advancedOn()?'grid':'none';
}
function addStyle(){if($('fpsAdvancedMultiOsdYellowStyle'))return;const s=document.createElement('style');s.id='fpsAdvancedMultiOsdYellowStyle';s.textContent=`
#fpsAdvancedMultiOsdPanel.fps-exp-yellow{border:2px solid #e5ef00!important;box-shadow:0 0 0 1px rgba(245,255,0,.12),0 0 22px rgba(245,255,0,.09);background:#1a201b!important}
#fpsAdvancedMultiOsdPanel.fps-exp-yellow>.fps-section-head{background:linear-gradient(90deg,#303922,#252d22)!important;border-bottom:1px solid rgba(245,255,0,.18)}
#fpsAdvancedMultiOsdPanel.fps-exp-yellow>.fps-section-head strong{color:#fbff65!important}
#fpsAdvancedMultiOsdPanel.fps-exp-yellow>.fps-section-head .fps-section-desc{color:#f0f2df!important;opacity:1!important}
#fpsAdvancedMultiOsdPanel.fps-exp-yellow>.fps-section-body{background:#1d241f!important}
#fpsAdvancedMultiOsdPanel .fps-multiosd-exp{display:inline-block;margin-left:7px;padding:2px 6px;border-radius:5px;background:#f5ff00!important;color:#111!important;font-size:9px;font-weight:950;letter-spacing:.08em}
`;document.head.appendChild(s)}
let pending=false;function queue(){if(pending)return;pending=true;requestAnimationFrame(()=>{pending=false;repair()})}
document.addEventListener('change',e=>{if(['fpsMultiCamSync','fpsExperimentalMaster','fpsOsdMaster','fpsAdvancedMultiOsd'].includes(e.target?.id))setTimeout(queue,0)},true);
new MutationObserver(queue).observe(document.documentElement,{childList:true,subtree:true});
addStyle();
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>{setTimeout(repair,500);setTimeout(repair,1000)});else{setTimeout(repair,500);setTimeout(repair,1000)}
})();