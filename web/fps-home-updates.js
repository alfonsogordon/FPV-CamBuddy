(()=>{
'use strict';
function init(){
 document.querySelector('.demo-caption')?.remove();
 const hero=document.querySelector('.cta-row');if(hero&&!document.querySelector('.fps-socials')){const s=document.createElement('div');s.className='fps-socials';s.innerHTML='<a href="https://www.youtube.com/@FPSteVe" target="_blank" rel="noopener">YouTube · @FPSteVe ↗</a><a href="https://www.instagram.com/fpvsteve/" target="_blank" rel="noopener">Instagram · @fpvsteve ↗</a>';hero.after(s)}
 const foot=document.querySelector('.foot-links');if(foot&&!foot.querySelector('[href*="youtube.com/@FPSteVe"]'))foot.insertAdjacentHTML('beforeend','<a href="https://www.youtube.com/@FPSteVe" target="_blank" rel="noopener">YouTube</a><a href="https://www.instagram.com/fpvsteve/" target="_blank" rel="noopener">Instagram</a>');
 const intro=document.querySelector('.fps-intro');if(intro)intro.textContent='Current FPV CamBuddy development — from the proven camera bridge through the new Betaflight OSD workflow, browser configurator and bench-test tools.';
 const grid=document.querySelector('.fps-grid');if(grid){grid.innerHTML=`
 <div class="fps-feature"><h3>State-aware Camera OSD <span class="status-badge status-live">IMPLEMENTED</span></h3><p>ERR / RDY / REC state, battery, recording duration, remaining time and camera telemetry with flashing REC behaviour.</p></div>
 <div class="fps-feature"><h3>Betaflight OSD Methods <span class="status-badge status-test">TESTING</span></h3><p>One configurator for BF 4.4–2025.12 Pilot/Craft Name and BF 2026.6+ Custom Messages 1–4.</p></div>
 <div class="fps-feature"><h3>Visual OSD Message Builder <span class="status-badge status-test">TESTING</span></h3><p>Ordered OSD elements, literal custom text, contextual Status controls and an optional raw-template editor.</p></div>
 <div class="fps-feature"><h3>Camera Warnings <span class="status-badge status-test">TESTING</span></h3><p>BATT LOW, REC LOW and CAM HOT temporary overlays, with selectable Custom Message destination on BF 2026.6+.</p></div>
 <div class="fps-feature"><h3>Custom Message Reminder <span class="status-badge status-test">TESTING</span></h3><p>Short custom reminders with configurable display time and optional before-first-arm-only behaviour.</p></div>
 <div class="fps-feature"><h3>Demo + Safe Bench Simulator <span class="status-badge status-test">TESTING</span></h3><p>Hardware-free OSD Demo Mode plus USB C3 ARM/DISARM simulation for camera testing without arming an FC or motors.</p></div>
 <div class="fps-feature"><h3>GoPro HERO11 Mini + MAX2 <span class="status-badge status-live">HARDWARE TESTED</span></h3><p>BLE connection and arm/disarm recording workflow tested on the FPSteVe development hardware.</p></div>
 <div class="fps-feature"><h3>Next Firmware Test Build <span class="status-badge status-plan">IN PIPELINE</span></h3><p>Finish UI/demo validation, commit the new warning routing to firmware, build both targets, then flash the spare ESP32-C3 before the 7-inch installation.</p></div>
 <div class="fps-feature"><h3>AUX Camera Controls — GoPro + DJI <span class="status-badge status-plan">PLANNED</span></h3><p>Useful camera settings controlled from Betaflight AUX channels where each camera protocol supports them.</p></div>
 <div class="fps-feature"><h3>Multi-Camera Sync <span class="status-badge status-plan">PLANNED</span></h3><p>Synchronized recording for GoPro + GoPro, DJI + DJI and mixed GoPro + DJI setups.</p></div>
 <div class="fps-feature"><h3>Expanded Camera Testing <span class="status-badge status-plan">PLANNED</span></h3><p>Community-assisted hardware testing with a clear Tested versus Supported camera matrix.</p></div>`}
 const style=document.createElement('style');style.textContent='.fps-socials{display:flex;justify-content:center;gap:14px;flex-wrap:wrap;margin-top:15px;font-size:12px}.fps-socials a{padding:5px 9px;border-bottom:1px solid #6d3fc0}.fps-socials a:hover{text-decoration:none;color:#d8c4ff}';document.head.appendChild(style);
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();