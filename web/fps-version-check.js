(()=>{
'use strict';
const EXPECTED='1.0.1';
const LEGACY={'0.0.11':'1.0.0'};
let boardRaw='',boardDisplay='',pending=false,warned=false,timer=0,poll=0,deadline=0;
const $=id=>document.getElementById(id);
function normalise(v){return LEGACY[v]||v}
function style(){if($('fpsVersionStyle'))return;const s=document.createElement('style');s.id='fpsVersionStyle';s.textContent=`
#fpsVersionWarning{display:none;position:relative;z-index:20;padding:7px 14px;background:#fff7cc;border-bottom:1px solid #e7c94d;color:#655400;font:11px/1.4 'Courier New',monospace;text-align:center}#fpsVersionWarning.show{display:block}#fpsVersionWarning a{color:#8a6f00;font-weight:700;text-decoration:underline}
.fps-ver-backdrop{position:fixed;inset:0;z-index:11000;display:grid;place-items:center;padding:20px;background:rgba(0,0,0,.66);backdrop-filter:blur(3px)}.fps-ver-dialog{width:min(470px,100%);padding:20px;border:1px solid #e7c94d;border-radius:12px;background:#fff;color:#1f2937;box-shadow:0 18px 60px #0007}.fps-ver-dialog h3{margin:0 0 9px;color:#8a6f00}.fps-ver-dialog p{margin:0 0 10px;font-size:13px;line-height:1.5}.fps-ver-actions{display:flex;justify-content:flex-end;gap:8px;margin-top:16px}
`;document.head.appendChild(s)}
function bar(){let b=$('fpsVersionWarning');if(b)return b;b=document.createElement('div');b.id='fpsVersionWarning';const header=document.querySelector('header');header?.insertAdjacentElement('afterend',b);return b}
function stopPoll(){clearInterval(poll);poll=0;deadline=0}
function clear(){boardRaw='';boardDisplay='';pending=false;warned=false;clearTimeout(timer);stopPoll();const b=$('fpsVersionWarning');if(b){b.classList.remove('show');b.innerHTML=''}$('fpsVersionDialog')?.remove()}
function mismatch(){return boardDisplay&&boardDisplay!==EXPECTED}
function render(){const b=bar();if(!b)return;if(!mismatch()){b.classList.remove('show');b.innerHTML='';return}b.innerHTML=`Firmware update recommended — this board is running <strong>V${boardDisplay}</strong>; this configurator targets <strong>V${EXPECTED}</strong>. Some features may not work. <a href="flash.html">Update firmware</a>`;b.classList.add('show')}
function popup(){if(warned||!mismatch())return;warned=true;const bg=document.createElement('div');bg.id='fpsVersionDialog';bg.className='fps-ver-backdrop';bg.innerHTML=`<div class="fps-ver-dialog" role="dialog" aria-modal="true"><h3>Firmware update recommended</h3><p>Your board is running <strong>V${boardDisplay}</strong>, while this configurator is for <strong>V${EXPECTED}</strong>.</p><p>The board can stay connected, but newer features may be unavailable or may not work correctly until the firmware is updated.</p><div class="fps-ver-actions"><button type="button" id="fpsVerDismiss">Continue anyway</button><button type="button" class="primary" id="fpsVerFlash">Go to Flash</button></div></div>`;document.body.appendChild(bg);$('fpsVerDismiss').onclick=()=>bg.remove();$('fpsVerFlash').onclick=()=>location.href='flash.html'}
function receive(line){const m=String(line||'').match(/^\[cfg\]\s+FreeCLinker firmware v([^\s]+)/i);if(!m)return false;boardRaw=m[1].trim();boardDisplay=normalise(boardRaw);pending=false;clearTimeout(timer);stopPoll();render();popup();console.info(`[version] board=${boardRaw} display=${boardDisplay} expected=${EXPECTED}`);return true}
function query(){if(!$('connectBtn')?.disabled||pending)return;pending=true;sendCommand('version');clearTimeout(timer);timer=setTimeout(()=>{pending=false},1800)}
function waitForConnection(){stopPoll();deadline=Date.now()+15000;const tick=()=>{hook();if($('connectBtn')?.disabled){query();if(boardDisplay)stopPoll();return}if(Date.now()>=deadline)stopPoll()};tick();if(!boardDisplay)poll=setInterval(tick,250)}
function hook(){if(typeof parseConfigLine!=='function'||parseConfigLine.__fpsVersionWrapped)return;const original=parseConfigLine;const wrapped=function(line){receive(line);return original(line)};wrapped.__fpsVersionWrapped=true;parseConfigLine=wrapped}
function init(){style();bar();hook();const connect=$('connectBtn'),disconnect=$('disconnectBtn');connect?.addEventListener('click',()=>{warned=false;boardRaw='';boardDisplay='';waitForConnection()});disconnect?.addEventListener('click',()=>setTimeout(clear,50));window.fpsFirmwareVersion={expected:EXPECTED,legacy:LEGACY,receive,query,clear}}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();
