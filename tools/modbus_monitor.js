// Watches the controller's status bits and registers over Modbus TCP and prints every change with a timestamp.
// Usage: node tools/modbus_monitor.js [seconds]      (default 60)
// Shows: Home(117) AtTarget(118) Fault(120) Refused(121) Cpu2Online(122), FaultSpreaders(108) FaultType(109) RefusedReason(110).
// Option "soft": node tools/modbus_monitor.js 40 soft  holds the PLC manual request (coil 104) on for the whole run and clears it at the end.
// Option "slow": node tools/modbus_monitor.js 45 slow  starts a slow pattern 1 move (speed 250) so a manual DIP switch flip can be tested mid-move.
// Option "home": node tools/modbus_monitor.js 40 home  first selects pattern 1 (speed 3000), then after 4 s pattern 0 (home), all on the same connection.
// CPU1 accepts one Modbus client at a time: disconnect other Modbus masters first.
const net = require('net');
const HOST = '192.168.2.51', PORT = 502, UNIT = 0xFF;
const seconds = parseInt(process.argv[2] || '60', 10);
const ts = () => { const d = new Date(); return d.toTimeString().slice(0, 8) + '.' + String(d.getMilliseconds()).padStart(3, '0'); };
const sleep = (ms) => new Promise(r => setTimeout(r, ms));

const sock = net.connect(PORT, HOST);
let tid = 0, pending = null;
sock.on('data', (buf) => { if (pending) { const p = pending; pending = null; p(buf); } });
sock.on('error', (e) => { console.error('socket error', e.message); process.exit(1); });
setTimeout(() => { console.log(ts() + '  time is up'); process.exit(0); }, (seconds + 5) * 1000);

function req(func, data) {
  return new Promise((resolve, reject) => {
    const pdu = Buffer.concat([Buffer.from([func]), data]);
    const h = Buffer.alloc(7);
    h.writeUInt16BE(++tid & 0xFFFF, 0); h.writeUInt16BE(0, 2); h.writeUInt16BE(pdu.length + 1, 4); h.writeUInt8(UNIT, 6);
    const timer = setTimeout(() => { if (pending === done) { pending = null; reject(new Error('timeout func ' + func)); } }, 2000);
    const done = (buf) => { clearTimeout(timer); resolve(buf); };
    pending = done;
    sock.write(Buffer.concat([h, pdu]));
  });
}
async function readDI(addr, qty) {
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(qty, 2);
  const r = await req(2, d);
  if (r[7] & 0x80) throw new Error('Modbus exception reading DI');
  return r[9] | (r[10] << 8);
}
async function writeRegs(addr, values) {
  const d = Buffer.alloc(5 + values.length * 2);
  d.writeUInt16BE(addr, 0); d.writeUInt16BE(values.length, 2); d.writeUInt8(values.length * 2, 4);
  values.forEach((v, i) => d.writeUInt16BE(v, 5 + i * 2));
  const r = await req(16, d);
  if (r[7] & 0x80) throw new Error("Modbus exception writing " + addr);
}
async function writeCoil(addr, on) {
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(on ? 0xFF00 : 0, 2);
  await req(5, d);
}
async function readHolding(addr, qty) {
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(qty, 2);
  const r = await req(3, d);
  if (r[7] & 0x80) throw new Error('Modbus exception reading holding');
  const out = [];
  for (let i = 0; i < qty; i++) out.push(r.readUInt16BE(9 + i * 2));
  return out;
}

sock.on('connect', async () => {
  let last = '';
  let step = 0;
  const t0 = Date.now();
  const end = Date.now() + seconds * 1000;
  console.log(ts() + '  connected, watching for ' + seconds + ' s');
  try {
    while (Date.now() < end) {
      if (process.argv[3] === "soft" && step === 0) { step = 1; console.log(ts() + "  >> PLC manual request ON (coil 104) for the whole run"); await writeCoil(104, true); }
      if (process.argv[3] === "slow" && step === 0) { step = 1; console.log(ts() + "  >> slow pattern 1 move (speed 250 steps/s, about 30 s)"); await writeRegs(111, Array(9).fill(20)); await writeRegs(105, [250]); await writeRegs(104, [1]); }
      if (process.argv[3] === "home") {
        if (step === 0) { step = 1; console.log(ts() + "  >> select pattern 1"); await writeRegs(105, [3000]); await writeRegs(104, [1]); }
        if (step === 1 && Date.now() - t0 > 4000) { step = 2; console.log(ts() + "  >> select pattern 0 (home)"); await writeRegs(104, [0]); }
      }
      const di = await readDI(117, 6);                  // 117..122
      const hr = await readHolding(108, 3);             // 108, 109, 110
      const s = `Home=${di & 1} AtTarget=${(di >> 1) & 1} Fault=${(di >> 3) & 1} Refused=${(di >> 4) & 1} Cpu2Online=${(di >> 5) & 1}  ` +
                `FaultSpreaders(108)=0x${hr[0].toString(16)} FaultType(109)=${hr[1]} RefusedReason(110)=${hr[2]}`;
      if (s !== last) { console.log(ts() + '  ' + s); last = s; }
      await sleep(20);
    }
  } catch (e) { console.error("ERROR", e.message); }
  if (process.argv[3] === "soft") { try { await writeCoil(104, false); console.log(ts() + "  >> PLC manual request OFF"); } catch (e) {} }
  sock.end(); process.exit(0);
});
