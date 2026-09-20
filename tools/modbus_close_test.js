// Bench test: jog spreader 1 closed in manual mode and switch proxy 2 ON by hand while it runs: the jog must stop within milliseconds ("touching its neighbour").
// Usage: node tools/modbus_close_test.js   (manual mode must be on, for example with the manual DIP switch; it waits for proxy 2 to be switched OFF first)
const net=require("net"),http=require("http");const s=net.connect(502,"192.168.2.51");let tid=0,p=null;
const ts=()=>{const d=new Date();return d.toTimeString().slice(0,8)+"."+String(d.getMilliseconds()).padStart(3,"0")};
const log=m=>console.log(ts()+"  "+m);const sleep=ms=>new Promise(r=>setTimeout(r,ms));
s.on("data",b=>{if(p){const f=p;p=null;f(b)}});
function req(func,data){return new Promise(r=>{const pdu=Buffer.concat([Buffer.from([func]),data]);const h=Buffer.alloc(7);h.writeUInt16BE(++tid,0);h.writeUInt16BE(0,2);h.writeUInt16BE(pdu.length+1,4);h.writeUInt8(255,6);p=r;s.write(Buffer.concat([h,pdu]));});}
async function wr(a,v){const d=Buffer.alloc(7);d.writeUInt16BE(a,0);d.writeUInt16BE(1,2);d.writeUInt8(2,4);d.writeUInt16BE(v,5);await req(16,d);}
async function coil(a,on){const d=Buffer.alloc(4);d.writeUInt16BE(a,0);d.writeUInt16BE(on?0xFF00:0,2);await req(5,d);}
async function di(){const d=Buffer.alloc(4);d.writeUInt16BE(117,0);d.writeUInt16BE(6,2);return (await req(2,d))[9];}
function web(){return new Promise(res=>{http.get({host:"192.168.2.51",port:80,path:"/status.json",timeout:2000,agent:false},r=>{let b="";r.on("data",c=>b+=c);r.on("end",()=>{try{res(JSON.parse(b))}catch(e){res(null)}})}).on("error",()=>res(null))});}
const p2=j=>j?((j.inputs>>1)&1):-1;
setTimeout(()=>{console.log(ts()+"  time is up");process.exit(0)},110000);
s.on("connect",async()=>{
  log("waiting for proxy 2 to be switched OFF (up to 60 s)...");
  let j;const w0=Date.now();
  while(Date.now()-w0<60000){j=await web();if(p2(j)===0)break;await sleep(300);}
  if(p2(j)!==0){log("proxy 2 never went off: giving up");process.exit(0);}
  log("proxy 2 is OFF: jogging spreader 1 closed (bursts of 8 s); now switch proxy 2 ON whenever you like");
  await wr(107,1);
  const t0=Date.now();let stoppedAt=null;
  while(Date.now()-t0<80000&&!stoppedAt){
    await coil(106,true);const b0=Date.now();
    while(Date.now()-b0<8000){await di();j=await web();
      if(p2(j)===1){stoppedAt=Date.now();log("proxy 2 switched ON (S1 = "+(j.cpu2.pos[0]/10).toFixed(1)+" mm)");break;}
      await sleep(60);}
    if(stoppedAt)break;
    await coil(106,false);await sleep(400);
  }
  if(stoppedAt){for(let i=0;i<6;i++){await di();await sleep(150);}j=await web();log("0.9 s later: S1 = "+(j.cpu2.pos[0]/10).toFixed(1)+" mm (must not have moved further)");}
  await coil(106,false);log("coil 106 released");s.end();process.exit(0);});
