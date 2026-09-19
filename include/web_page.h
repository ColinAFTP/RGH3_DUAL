#ifndef WEB_PAGE_H
#define WEB_PAGE_H

// The diagnostics web page served by CPU1 (read only). It polls /status.json, /gaps.json and /log.json.
// Included only by functions_web.cpp.

static const char WEB_PAGE[] = R"HTML(<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Gripper Controller</title>
<style>
:root{--bg:#f4f5f7;--card:#fff;--fg:#1c2330;--mut:#667085;--line:#dfe3ea;--on:#1a9c4b;--off:#c9ced8;--bad:#d92d20;--warn:#d98a00}
@media(prefers-color-scheme:dark){:root{--bg:#12161d;--card:#1b212b;--fg:#e6e9ef;--mut:#96a0b2;--line:#2c3441;--off:#3a4352}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:14px system-ui,sans-serif}
header{display:flex;flex-wrap:wrap;gap:8px 18px;align-items:center;padding:10px 16px;border-bottom:1px solid var(--line);background:var(--card)}
h1{font-size:17px;margin:0 8px 0 0}h2{font-size:14px;margin:0 0 8px;color:var(--mut);font-weight:600}
main{display:grid;grid-template-columns:1fr;gap:12px;padding:12px 16px}
@media(min-width:760px){main{grid-template-columns:repeat(2,1fr)}}
@media(min-width:1300px){main{grid-template-columns:repeat(4,1fr)}}
.card{background:var(--card);border:1px solid var(--line);border-radius:8px;padding:12px}
.wide{grid-column:1/-1}
.chip{display:inline-block;padding:3px 10px;border-radius:12px;background:var(--off);color:#fff;font-weight:600;font-size:12px}
.chip.on{background:var(--on)}.chip.bad{background:var(--bad)}.chip.warn{background:var(--warn)}
.mut{color:var(--mut)}table{border-collapse:collapse;width:100%}th,td{padding:4px 6px;text-align:right;border-bottom:1px solid var(--line)}
th:first-child,td:first-child{text-align:left}th{color:var(--mut);font-weight:500;font-size:12px}
.led{display:inline-block;width:11px;height:11px;border-radius:50%;background:var(--off);vertical-align:middle}.led.on{background:var(--on)}.led.bad{background:var(--bad)}
.bits{display:grid;grid-template-columns:repeat(8,1fr);gap:6px 4px;text-align:center;font-size:11px;color:var(--mut)}
tr.act td{background:rgba(26,156,75,.13)}
#log{max-height:300px;overflow:auto;font:12px ui-monospace,Consolas,monospace}
#log div{padding:2px 0;border-bottom:1px solid var(--line)}#log .t{color:var(--mut);margin-right:8px}#log .f{color:var(--bad);font-weight:600}
#off{background:var(--bad);color:#fff;padding:6px 16px;text-align:center}
</style></head><body>
<div id="off" hidden>No answer from the controller</div>
<header><h1>Gripper Controller</h1>
<span>Up <b id="up">-</b></span><span>PLC <span class="chip" id="plc">-</span></span><span>CPU2 <span class="chip" id="c2">-</span></span>
<span class="mut">CPU1 Loop <b id="loop">-</b></span></header>
<main>
<section class="card"><h2>Status</h2>
<p><span class="chip" id="home">Home</span> <span class="chip" id="target">At Target</span> <span class="chip" id="fault">Fault</span></p>
<table><tr><td>CPU2 State</td><td id="state">-</td></tr><tr><td>PLC Pattern</td><td id="pat">-</td></tr><tr><td>PLC Speed</td><td id="spd">-</td></tr><tr><td>Ticker</td><td id="ticker">-</td></tr><tr><td>Input Glitches</td><td id="gl">-</td></tr><tr><td>Fault Type</td><td id="ft">-</td></tr>
<tr><td>Failed Spreaders (Reg 108)</td><td id="mask">-</td></tr></table></section>
<section class="card"><h2>Spreaders</h2><table id="sp"><tr><th>Spreader</th><th>Home Sensor</th><th>Position mm</th><th>Fault</th></tr></table></section>
<section class="card"><h2>Inputs (Proxies)</h2><div class="bits" id="in"></div></section>
<section class="card"><h2>Relays</h2><div class="bits" id="rl"></div></section>
<section class="card wide"><h2>Gap Patterns (mm, Spreader 1 to 10 Left to Right)</h2><table id="gp"></table></section>
<section class="card wide"><h2>Event Log</h2><div id="log"></div></section>
</main>
<script>
const $=id=>document.getElementById(id),SP=[1,2,3,4,6,7,8,9,10],FTYPE=['None','Homing Failed','Over Travel'],STATE=['Idle','Moving','Homing: Approach','Homing: Pulses','Fault','Idle, Positions Unknown'];
const inLbl=['P1 OT L','P2 S1','P3 S2','P4 S3','P5 S4','P6 S6','P7 S7','P8 S8','P9 S9','P10 S10','P11 OT R','In11','In12','In13','In14','In15'];
const rlLbl=['R1 Home','R2 Target','R3','R4','R5','R6','R7','R8','R9','R10','R11','R12','R13','R14','R15','R16'];
const chip=(id,on,cls)=>{$(id).className='chip'+(on?' '+(cls||'on'):'')};
const led=(on,cls)=>'<span class="led'+(on?' '+(cls||'on'):'')+'"></span>';
function fmt(s){const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return(d?d+'d ':'')+h+'h '+m+'m '+s%60+'s'}
function bits(el,v,lbl){el.innerHTML=lbl.map((l,i)=>'<div>'+led(v>>i&1)+'<br>'+l+'</div>').join('')}
function render(s){
 $('up').textContent=fmt(Math.floor(s.up/1000));$('loop').textContent=(s.loopAvg/1000).toFixed(2)+' ms avg, '+(s.loopMax/1000).toFixed(2)+' ms max';
 $('plc').textContent=s.plc?'Connected':'No Client';chip('plc',s.plc);
 const c=s.cpu2;$('c2').textContent=c.ok?'Online':'No Data';chip('c2',c.ok,c.ok?'on':'bad');
 chip('home',s.home);chip('target',s.target);chip('fault',s.fault,'bad');
 $('state').textContent=c.ok?STATE[c.state]||c.state:'-';$('pat').textContent=s.pattern+(s.pattern?'':' (home)');$('spd').textContent=s.speed;$('ticker').textContent=s.tick;$('gl').textContent=s.glitches;$('ft').textContent=FTYPE[s.ftype]||s.ftype;
 $('mask').textContent='0x'+s.mask.toString(16).toUpperCase();
 let h='<tr><th>Spreader</th><th>Home Sensor</th><th>Position mm</th><th>Fault</th></tr>';
 SP.forEach((n,i)=>{h+='<tr><td>S'+n+'</td><td>'+led(s.inputs>>(i+1)&1)+'</td><td>'+(c.ok?(c.pos[i]/10).toFixed(1):'-')+'</td><td>'+led(s.mask>>(n-1)&1,'bad')+'</td></tr>'});
 $('sp').innerHTML=h;bits($('in'),s.inputs,inLbl);bits($('rl'),s.relays,rlLbl);window.act=s.pattern}
async function getj(u){const r=await fetch(u,{cache:'no-store'});if(!r.ok)throw 0;return r.json()}
async function tick(){try{render(await getj('/status.json'));$('off').hidden=true}catch(e){$('off').hidden=false}setTimeout(tick,500)}
async function gaps(){try{const g=await getj('/gaps.json');let h='<tr><th>Pattern</th>';for(let i=0;i<9;i++)h+='<th>Gap '+(i+1)+'</th>';h+='</tr>';
 g.gaps.forEach((r,p)=>{h+='<tr'+(window.act==p+1?' class="act"':'')+'><td>'+(p+1)+'</td>'+r.map(v=>'<td>'+(v/10).toFixed(1)+'</td>').join('')+'</tr>'});$('gp').innerHTML=h}catch(e){}setTimeout(gaps,3000)}
let last=0;
async function log(){try{const j=await getj('/log.json?since='+last),L=$('log');
 j.ev.forEach(e=>{const d=document.createElement('div');d.innerHTML='<span class="t">'+fmt(Math.floor(e[1]/1000))+'</span>';const t=document.createElement('span');t.textContent=e[2];if(/FAULT/i.test(e[2]))t.className='f';d.appendChild(t);L.insertBefore(d,L.firstChild)});
 while(L.children.length>200)L.removeChild(L.lastChild);last=j.last}catch(e){}setTimeout(log,1000)}
tick();gaps();log();
</script></body></html>)HTML";

#endif
