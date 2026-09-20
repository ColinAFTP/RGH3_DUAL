// Tests refused requests and the At Target handshake over Modbus TCP.
// Usage: node tools/modbus_refuse_test.js
//  1. Selects pattern 1 (20 mm gaps, valid) and measures how fast At Target drops after the write.
//  2. Selects pattern 2 with 50 mm gaps (targets out of range): expects At Target to STAY off, Move Refused on, reason 6 (bad targets).
//  3. Selects pattern 7 (invalid selection): expects Move Refused on, reason 4 (invalid pattern).
//  4. Selects pattern 0 (home): expects Move Refused to clear and the gripper to go home.
// CPU1 accepts one Modbus client at a time: disconnect other Modbus masters first.
const net = require('net');
const HOST = '192.168.2.51', PORT = 502, UNIT = 0xFF;
const ts = () => { const d = new Date(); return d.toTimeString().slice(0, 8) + '.' + String(d.getMilliseconds()).padStart(3, '0'); };
const log = (m) => console.log(ts() + '  ' + m);
const sleep = (ms) => new Promise(r => setTimeout(r, ms));

const sock = net.connect(PORT, HOST);
let tid = 0, pending = null;
sock.on('data', (buf) => { if (pending) { const p = pending; pending = null; p(buf); } });
sock.on('error', (e) => { console.error('socket error', e.message); process.exit(1); });
setTimeout(() => { console.log(ts() + '  WATCHDOG: exiting'); process.exit(2); }, 90000);

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
  return r[9] | (r[10] << 8);
}
async function readHolding(addr) {
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(1, 2);
  const r = await req(3, d);
  if (r[7] & 0x80) throw new Error('Modbus exception reading holding');
  return r.readUInt16BE(9);
}
const bits = (di) => `Home=${di & 1} AtTarget=${(di >> 1) & 1} Fault=${(di >> 3) & 1} Refused=${(di >> 4) & 1} Cpu2Online=${(di >> 5) & 1}`;
async function show(label) {
  const di = await readDI(117, 6);
  const reason = await readHolding(110);
  log(`${label}: ${bits(di)} RefusedReason(110)=${reason}`);
}

// Write a pattern and report how long At Target takes to drop (polling as fast as possible)
async function selectAndWatchAtTarget(pattern) {
  const t0 = Date.now();
  await writeRegs(104, [pattern]);
  let droppedAt = -1, polls = 0;
  while (Date.now() - t0 < 600) {
    const di = await readDI(117, 6);
    polls++;
    if (droppedAt < 0 && !((di >> 1) & 1)) droppedAt = Date.now() - t0;
    await sleep(5);
  }
  log(`pattern ${pattern} written: At Target ${droppedAt >= 0 ? 'dropped after ' + droppedAt + ' ms' : 'did NOT drop'} (${polls} polls in 600 ms)`);
}

sock.on('connect', async () => {
  try {
    log('connected');
    await show('baseline');
    await writeRegs(105, [3000]);
    await writeRegs(111, Array(9).fill(20));          // pattern 1 = block 0 = registers 111-119
    await writeRegs(121, Array(9).fill(50));          // pattern 2 = block 1: 50 mm gaps, out of range on purpose

    log('--- 1. valid pattern 1 ---');
    await selectAndWatchAtTarget(1);
    await sleep(3000);
    await show('after pattern 1 move');

    log('--- 2. pattern 2 with out-of-range targets: must be refused (reason 6) ---');
    await selectAndWatchAtTarget(2);
    await show('after refused pattern 2');

    log('--- 3. invalid pattern selection 7: must be refused by CPU1 (reason 4) ---');
    await writeRegs(104, [7]);
    await sleep(300);
    await show('after selection 7');

    log('--- 4. home: refusal must clear ---');
    await selectAndWatchAtTarget(0);
    await sleep(4000);
    await show('after home');
    log('done');
  } catch (e) { console.error('ERROR', e.message); }
  sock.end(); process.exit(0);
});
