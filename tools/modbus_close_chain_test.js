// Bench test of the closing chain in manual mode: closing a spreader pushes its inner neighbours along until all are against the static spreader.
// Usage: node tools/modbus_close_chain_test.js
// Steps: manual mode on (coil 104); spreader 6 is opened so the right group is out; then YOU simulate gaps by switching proxy 6 and proxy 7 OFF;
// spreader 7 is jogged closed. Expected: (1) spreader 7 moves alone; when you switch proxy 7 ON, spreader 6 moves as well;
// when you switch proxy 6 ON, both stop. Manual mode is left with coil 104 at the end (automatic home).
const net=require("net"),http=require("http");const s=net.connect(502,"192.168.2.51");let tid=0,p=null;
const ts=()=>{const d=new Date();return d.toTimeString().slice(0,8)+"."+String(d.getMilliseconds()).padStart(3,"0")};
const log=m=>console.log(ts()+"  "+m);const sleep=ms=>new Promise(r=>setTimeout(r,ms));
s.on("data",b=>{if(p){const f=p;p=null;f(b)}});
function req(func,data){return new Promise(r=>{const pdu=Buffer.concat([Buffer.from([func]),data]);const h=Buffer.alloc(7);h.writeUInt16BE(++tid,0);h.writeUInt16BE(0,2);h.writeUInt16BE(pdu.length+1,4);h.writeUInt8(255,6);p=r;s.write(Buffer.concat([h,pdu]));});}
async function wr(a,v){const d=Buffer.alloc(7);d.writeUInt16BE(a,0);d.writeUInt16BE(1,2);d.writeUInt8(2,4);d.writeUInt16BE(v,5);await req(16,d);}
async function coil(a,on){const d=Buffer.alloc(4);d.writeUInt16BE(a,0);d.writeUInt16BE(on?0xFF00:0,2);await req(5,d);}
async function di(){const d=Buffer.alloc(4);d.writeUInt16BE(117,0);d.writeUInt16BE(6,2);return (await req(2,d))[9];}
function web(){return new Promise(res=>{http.get({host:"192.168.2.51",port:80,path:"/status.json",timeout:2000,agent:false},r=>{let b="";r.on("data",c=>b+=c);r.on("end",()=>{try{res(JSON.parse(b))}catch(e){res(null)}})}).on("error",()=>res(null))});}
const P6=j=>(j.inputs>>5)&1, P7=j=>(j.inputs>>6)&1;          // proxy 6 = spreader 6 (bit 5), proxy 7 = spreader 7 (bit 6)
const S6=j=>(j.cpu2.pos[4]/10).toFixed(1), S7=j=>(j.cpu2.pos[5]/10).toFixed(1);
setTimeout(()=>{console.log(ts()+"  time is up");process.exit(0)},900000);
s.on("connect",async()=>{
  await coil(104,true);for(let i=0;i<40;i++){if((await di()>>2)&1)break;await sleep(50);}
  let j=await web();log("manual mode on; start: S6="+S6(j)+" S7="+S7(j)+" mm");
  await wr(107,6);await coil(105,true);const o0=Date.now();while(Date.now()-o0<2000){await di();await sleep(80);}await coil(105,false);await sleep(700);
  j=await web();log("after opening spreader 6 (the right group moves out): S6="+S6(j)+" S7="+S7(j)+" mm");
  log(">>> NOW switch proxy 6 AND proxy 7 OFF (simulated gaps). Then, about 5 s later, switch proxy 7 ON. About 5 s after that, switch proxy 6 ON.");
  const w0=Date.now();while(Date.now()-w0<600000){await di();j=await web();if(j&&P6(j)===0&&P7(j)===0)break;await sleep(300);}   // keep talking Modbus so this client never looks silent
  if(!j||P6(j)!==0||P7(j)!==0){log("proxies 6 and 7 never both went off: giving up");await coil(104,false);process.exit(0);}
  log("proxy 6 = OFF, proxy 7 = OFF: jogging spreader 7 CLOSED");
  await wr(107,7);
  let last="",lastPrint=0,done=false;const t0=Date.now();
  while(Date.now()-t0<70000&&!done){
    await coil(106,true);const b0=Date.now();
    while(Date.now()-b0<8000){
      await di();j=await web();if(!j)continue;
      const st="P6="+(P6(j)?"ON ":"off")+" P7="+(P7(j)?"ON ":"off");
      if(st!==last||Date.now()-lastPrint>1000){log(st+"   S6="+S6(j)+" S7="+S7(j)+" mm");last=st;lastPrint=Date.now();}
      if(P6(j)&&P7(j)){done=true;break;}
      await sleep(60);}
    if(done)break;await coil(106,false);await sleep(400);
  }
  if(done){const a=await web();await sleep(1200);for(let i=0;i<8;i++){await di();await sleep(100);}const b=await web();
    log("both proxies on. S6 "+S6(a)+" -> "+S6(b)+", S7 "+S7(a)+" -> "+S7(b)+" mm over the next second (must not change)");}
  await coil(106,false);await coil(104,false);log("coil 106 released, manual mode off (automatic home)");await sleep(6000);
  s.end();process.exit(0);});
