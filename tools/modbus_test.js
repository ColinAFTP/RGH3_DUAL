// Bench test: stale-data fix + mid-move re-trigger. Talks Modbus TCP directly (no library).
const net = require('net');
const HOST = '192.168.2.51', PORT = 502, UNIT = 0xFF;
const ts = () => { const d = new Date(); return d.toTimeString().slice(0, 8) + '.' + String(d.getMilliseconds()).padStart(3, '0'); };
const log = (m) => console.log(ts() + '  ' + m);
const sleep = (ms) => new Promise(r => setTimeout(r, ms));

const sock = net.connect(PORT, HOST);
let tid = 0, pending = null;
sock.on('data', (buf) => { if (pending) { const p = pending; pending = null; p(buf); } });
sock.on('error', (e) => { console.error('socket error', e.message); process.exit(1); });

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
async function readDI(addr, qty) {
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(qty, 2);
  const r = await req(2, d);
  if (r[7] & 0x80) throw new Error('Modbus exception reading DI');
  return r[9];
}

let last = -1;
async function poll(ms) {
  const end = Date.now() + ms;
  while (Date.now() < end) {
    const b = await readDI(117, 2);
    if (b !== last) { log(`Home(117)=${b & 1}  AtTarget(118)=${(b >> 1) & 1}`); last = b; }
    await sleep(50);
  }
}

setTimeout(() => { console.log(ts() + '  WATCHDOG: exiting'); process.exit(2); }, 20000);

sock.on('connect', async () => {
  try {
    log('connected');
    await poll(300);
    if (process.argv[2] === 'zero') {
      log('Return to zero: select pattern 3 (all-zero gaps)');
      await writeRegs(104, [3]);
      await poll(5000);
      log('done');
      sock.end(); process.exit(0);
    }
    log('TEST 2: write pattern 1 gaps = 20 mm, speed 3000, then select pattern 1 IMMEDIATELY');
    await writeRegs(121, Array(9).fill(20));
    await writeRegs(105, [3000]);
    await writeRegs(104, [1]);
    log('pattern 1 selected');
    await poll(1000);
    log('TEST 3: select pattern 0 while pattern 1 move is running');
    await writeRegs(104, [0]);
    log('pattern 0 selected (mid-move)');
    await poll(5000);
    log('Return to zero: select pattern 3 (all-zero gaps)');
    await writeRegs(104, [3]);
    await poll(5000);
    log('done');
  } catch (e) { console.error('ERROR', e.message); }
  sock.end(); process.exit(0);
});
