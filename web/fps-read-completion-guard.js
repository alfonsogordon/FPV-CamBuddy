(()=>{
'use strict';
let timer=0,hookTimer=0,lastConnected=false;
const $=id=>document.getElementById(id);
function connected(){return !!$('connectBtn')?.disabled}
function scheduleComplete(){
 clearTimeout(timer);
 timer=setTimeout(()=>{
  if(!connected())return;
  document.dispatchEvent(new CustomEvent('fps-config-read-complete'));
 },320);
}
function hook(){
 if(typeof parseConfigLine!=='function'||parseConfigLine.__fpsReadGuardWrapped)return false;
 const original=parseConfigLine;
 const wrapped=function(line){
  const result=original(line);
  if(/^\[cfg\]\s+/.test(String(line||'')))scheduleComplete();
  return result;
 };
 wrapped.__fpsReadGuardWrapped=true;
 parseConfigLine=wrapped;
 return true;
}
function ensureHook(){
 if(hook())return;
 clearInterval(hookTimer);
 let tries=0;
 hookTimer=setInterval(()=>{if(hook()||++tries>80)clearInterval(hookTimer)},100);
}
function watchConnection(){
 const c=$('connectBtn');if(!c)return;
 const check=()=>{
  const now=connected();
  if(now&&!lastConnected){
   ensureHook();
   setTimeout(()=>{
    if(!connected())return;
    window.fpsRequestBoardRead?.();
    if(typeof sendCommand==='function')sendCommand('show');
   },650);
  }
  lastConnected=now;
 };
 new MutationObserver(check).observe(c,{attributes:true,attributeFilter:['disabled']});
 check();
}
function init(){ensureHook();watchConnection()}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();
