(()=>{
'use strict';
function check(){
 const craft=document.getElementById('craftTpl');
 const craftMaster=document.getElementById('fpsCraftMaster');
 const wake=document.getElementById('wakeGuard');
 const note=document.getElementById('fpsGoProConnectionNote');
 if(!craft||!craftMaster||!wake||!note)return;
 // This file is a lightweight runtime guard only; the actual behaviour lives in fps-stage3-parity.js.
 // It deliberately does not own state or write settings.
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',()=>setTimeout(check,500));else setTimeout(check,500);
})();
