(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
const $=id=>document.getElementById(id);
function multiOn(){return !!$('fpsMultiCamSync')?.checked}
function repair(){
 const panel=$('fpsAdvancedMultiOsdPanel');
 const master=$('fpsOsdMaster');
 const top=master?.closest('.fps-top');
 if(panel&&top&&panel.previousElementSibling!==top)top.insertAdjacentElement('afterend',panel);
 if(panel)panel.style.display=multiOn()?'':'none';
}
let pending=false;
function queue(){if(pending)return;pending=true;requestAnimationFrame(()=>{pending=false;repair()})}
document.addEventListener('change',e=>{if(e.target?.id==='fpsMultiCamSync'||e.target?.id==='fpsExperimentalMaster')queue()},true);
new MutationObserver(queue).observe(document.documentElement,{childList:true,subtree:true});
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(repair,500));else setTimeout(repair,500);
})();