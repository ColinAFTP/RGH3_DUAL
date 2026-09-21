// Bench test of the relays over Modbus TCP.
// Usage: node tools/modbus_relay_test.js
// 1. Home and At Target must NOT switch relays 1 and 2 any more: with register 106 = 0 all relays stay off.
// 2. Register 106 controls the relays directly (bit 0 = relay 1 ... bit 15 = relay 16).
// 3. Relay test (coil 103): the relays switch on one at a time, 1 to 16, round and round; the register bits are ignored meanwhile.
// 4. Clearing the coil puts the relays back to the register value.
// The relay outputs are read from the dashboard status (field "relays"). Watch and listen to the real relays on the board as well.
const net = require('net');
const http = require('http');
const HOST = '192.168.2.51', PORT = 502, UNIT = 0xFF;
const ts = () => { const d = new Date(); return d.toTimeString().slice(0, 8) + '.' + String(d.getMilliseconds()).padStart(3, '0'); };
const log = (m) => console.log(ts() + '  ' + m);
const sleep = (ms) => new Promise(r => setTimeout(r, ms));

const sock = net.connect(PORT, HOST);
let tid = 0, pending = null;
sock.on('data', (buf) => { if (pending) { const p = pending; pending = null; p(buf); } });
sock.on('error', (e) => { console.error('socket error', e.message); process.exit(1); });
setTimeout(() => { console.log(ts() + '  WATCHDOG: exiting'); process.exit(2); }, 120000);

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
async function writeReg(addr, value) {
  const d = Buffer.alloc(7);
  d.writeUInt16BE(addr, 0); d.writeUInt16BE(1, 2); d.writeUInt8(2, 4); d.writeUInt16BE(value, 5);
  await req(16, d);
}
async function writeCoil(addr, on) {
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(on ? 0xFF00 : 0, 2);
  await req(5, d);
}
async function readDI(addr, qty) {
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(qty, 2);
  const r = await req(2, d);
  return r[9] | (r[10] << 8);
}
function web() {
  return new Promise((resolve) => {
    http.get({ host: HOST, port: 80, path: '/status.json', timeout: 2000, agent: false }, (res) => {
      let s = ''; res.on('data', (c) => s += c); res.on('end', () => { try { resolve(JSON.parse(s)); } catch (e) { resolve(null); } });
    }).on('error', () => resolve(null)).on('timeout', function () { this.destroy(); resolve(null); });
  });
}
const relayList = (v) => { const on = []; for (let i = 0; i < 16; i++) if ((v >> i) & 1) on.push(i + 1); return on.length ? 'relay ' + on.join(', ') : 'none'; };

sock.on('connect', async () => {
  try {
    log('connected');
    await writeCoil(103, false); await writeReg(106, 0); await sleep(400);
    let j = await web(); const di = await readDI(117, 6);
    log(`1. register 106 = 0, Home=${di & 1} AtTarget=${(di >> 1) & 1}: relays on = ${relayList(j.relays)}   (must be none)`);

    await writeReg(106, 0x8005); await sleep(400); j = await web();
    log(`2. register 106 = 0x8005: relays on = ${relayList(j.relays)}   (must be relay 1, 3, 16)`);

    log('3. relay test on (coil 103). Sampling the relays for 10 s:');
    await writeCoil(103, true);
    const t0 = Date.now(); let last = -1; const seen = [];
    while (Date.now() - t0 < 10000) {
      await readDI(117, 6);                       // keep talking Modbus
      j = await web();
      if (j && j.relays !== last) { last = j.relays; seen.push(relayList(j.relays)); log('   ' + relayList(j.relays) + `   (test running: ${j.rtest})`); }
      await sleep(100);
    }
    log(`   ${seen.length} different relay patterns seen in 10 s (expect about 20 at 500 ms per relay: 16 relays = 8 s per round)`);

    await writeCoil(103, false); await sleep(500); j = await web();
    log(`4. relay test off: relays on = ${relayList(j.relays)}   (must be relay 1, 3, 16 again), test running: ${j.rtest}`);

    await writeReg(106, 0); await sleep(400); j = await web();
    log(`   register 106 = 0: relays on = ${relayList(j.relays)}`);
    log('done');
  } catch (e) { console.error('ERROR', e.message); }
  try { await writeCoil(103, false); await writeReg(106, 0); } catch (e) { }
  sock.end(); process.exit(0);
});
