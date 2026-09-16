(()=>{
'use strict';
if(!location.pathname.includes('/experimental/'))return;
function update(){
 const sec=document.getElementById('fpsV102Experimental');if(!sec)return false;
 const details=[...sec.querySelectorAll('.fps-v102-info')].find(x=>/first-time Multi Cam setup/i.test(x.querySelector('summary')?.textContent||''));
 const body=details?.querySelector('.fps-v102-info-body');if(!body||body.dataset.fpsSetupInfoV2==='1')return !!body;
 body.dataset.fpsSetupInfoV2='1';
 body.innerHTML=`
  <p><strong>For a completely new camera, register it in Single Cam mode first using the correct Camera Type.</strong> The current Single Cam path still uses Camera Type to choose the camera backend. Multi Cam then uses the multi-brand coordinator with the cameras the C3 has learned/saved.</p>
  <ol>
   <li>Leave <strong>Multi Cam OFF</strong> and select the correct <strong>Camera Type</strong> for the new camera.</li>
   <li>Power on only that new camera and let FPV CamBuddy discover, connect and save it.</li>
   <li><strong>Power-cycle the C3</strong> before registering another new camera.</li>
   <li>Repeat with the correct Camera Type for each additional new camera.</li>
   <li>Once the cameras are learned, enable <strong>Multi Cam</strong>. Saved cameras can then be powered together or one by one and the coordinator will reconnect whichever are available.</li>
  </ol>
  <p><strong>Mixed-brand example:</strong> for a new GoPro + Insta360 setup, register the GoPro in Single Cam with Camera Type = GoPro, power-cycle the C3, register the Insta360 with Camera Type = Insta360, then enable Multi Cam.</p>
  <p>For multiple new GoPros the same one-at-a-time rule applies. A GoPro that has never been paired with FPV CamBuddy before may also need its <strong>Pair</strong> menu opened for the initial connection.</p>
  <p><strong>Planned:</strong> an experimental Automatic Camera Detection switch will remove this manual first-time Camera Type step once it has been hardware-tested across the supported camera families.</p>`;
 return true;
}
function init(){let tries=0;const t=setInterval(()=>{if(update()||++tries>40)clearInterval(t)},100);new MutationObserver(update).observe(document.documentElement,{childList:true,subtree:true})}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();