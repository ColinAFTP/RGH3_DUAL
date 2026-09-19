// Bench test for homing and faults. Usage: node tools/modbus_home_test.js [pattern|reset]
//   (no arg)  : write pattern 1 gaps at 111-119 (20 mm), speed 3000, select pattern 1, wait, select 0 (home), log Home / At Target / Fault
//   reset     : pulse the fault reset coil (107) and log for 20 s
// Talks Modbus TCP directly (no library). CPU1 accepts one client at a time: disconnect other Modbus masters first.
const net = require('net');
const HOST = '192.168.2.51', PORT = 502, UNIT = 0xFF;
const ts = () => { const d = new Date(); return d.toTimeString().slice(0, 8) + '.' + String(d.getMilliseconds()).padStart(3, '0'); };
const log = (m) => console.log(ts() + '  ' + m);
const sleep = (ms) => new Promise(r => setTimeout(r, ms));

const sock = net.connect(PORT, HOST);
let tid = 0, pending = null;
sock.on('data', (buf) => { if (pending) { const p = pending; pending = null; p(buf); } });
sock.on('error', (e) => { console.error('socket error', e.message); process.exit(1); });
setTimeout(() => { console.log(ts() + '  WATCHDOG: exiting'); process.exit(2); }, 60000);

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
async function writeRegs(addr, values) {
  const d = Buffer.alloc(5 + values.length * 2);
  d.writeUInt16BE(addr, 0); d.writeUInt16BE(values.length, 2); d.writeUInt8(values.length * 2, 4);
  values.forEach((v, i) => d.writeUInt16BE(v, 5 + i * 2));
  const r = await req(16, d);
  if (r[7] & 0x80) throw new Error('Modbus exception writing ' + addr);
}
async function writeCoil(addr, on) {
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(on ? 0xFF00 : 0, 2);
  const r = await req(5, d);
  if (r[7] & 0x80) throw new Error('Modbus exception writing coil ' + addr);
}
async function readDI(addr, qty) {          // discrete inputs, returns the first data byte
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(qty, 2);
  const r = await req(2, d);
  if (r[7] & 0x80) throw new Error('Modbus exception reading DI');
  return r[9];
}
async function readHolding(addr) {
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(1, 2);
  const r = await req(3, d);
  if (r[7] & 0x80) throw new Error('Modbus exception reading holding');
  return r.readUInt16BE(9);
}

let last = '';
async function poll(ms) {
  const end = Date.now() + ms;
  while (Date.now() < end) {
    const b = await readDI(117, 4);          // 117 Home, 118 At Target, 119 Manual mode, 120 Homing fault
    const mask = await readHolding(108);
    const s = `Home(117)=${b & 1}  AtTarget(118)=${(b >> 1) & 1}  Fault(120)=${(b >> 3) & 1}  FaultSpreaders(108)=0x${mask.toString(16)}`;
    if (s !== last) { log(s); last = s; }
    await sleep(50);
  }
}

sock.on('connect', async () => {
  try {
    log('connected');
    await poll(300);
    if (process.argv[2] === 'reset') {
      log('Fault reset: coil 107');
      await writeCoil(107, true);
      await poll(20000);
    } else {
      log('Write pattern 1 gaps = 20 mm, speed 3000, select pattern 1');
      await writeRegs(111, Array(9).fill(20));   // pattern 1 = block 0 = registers 111-120
      await writeRegs(105, [3000]);
      await writeRegs(104, [1]);
      await poll(6000);
      log('Select pattern 0 (home)');
      await writeRegs(104, [0]);
      await poll(12000);
    }
    log('done');
  } catch (e) { console.error('ERROR', e.message); }
  sock.end(); process.exit(0);
});
