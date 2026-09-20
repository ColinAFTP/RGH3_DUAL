// Bench test of manual mode over Modbus TCP, reading the live positions from the web page.
// Usage: node tools/modbus_manual_test.js
// Needs: the gripper at home with all home sensors (proxy 2-10) on, no other Modbus master connected, the manual DIP switch OFF.
// Steps: enter manual mode with coil 104; jog spreader 8 open (spreaders 9 and 10 must be pushed along, 6 and 7 must not move); close with all
// sensors on (must not move); dead man (coil left on while this script goes silent: movement must stop after ~1 s); invalid spreader 5;
// a pattern request during manual mode (must be refused, reason 8); leave manual mode (automatic home, coil 101 = homing in progress).
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
  const d = Buffer.alloc(5 + 2);
  d.writeUInt16BE(addr, 0); d.writeUInt16BE(1, 2); d.writeUInt8(2, 4); d.writeUInt16BE(value, 5);
  const r = await req(16, d);
  if (r[7] & 0x80) throw new Error('Modbus exception writing ' + addr);
}
async function writeCoil(addr, on) {
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(on ? 0xFF00 : 0, 2);
  const r = await req(5, d);
  if (r[7] & 0x80) throw new Error('Modbus exception writing coil ' + addr);
}
async function readCoil(addr) {
  const d = Buffer.alloc(4); d.writeUInt16BE(addr, 0); d.writeUInt16BE(1, 2);
  const r = await req(1, d);
  if (r[7] & 0x80) throw new Error('Modbus exception reading coil');
  return r[9] & 1;
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
function webStatus() {
  return new Promise((resolve) => {
    http.get({ host: HOST, port: 80, path: '/status.json', timeout: 2000, agent: false }, (res) => {
      let s = ''; res.on('data', (c) => s += c); res.on('end', () => { try { resolve(JSON.parse(s)); } catch (e) { resolve(null); } });
    }).on('error', () => resolve(null)).on('timeout', function () { this.destroy(); resolve(null); });
  });
}
const SP = [1, 2, 3, 4, 6, 7, 8, 9, 10];
async function positions() {
  const s = await webStatus();
  if (!s) return null;
  const out = {};
  SP.forEach((n, i) => out['S' + n] = (s.cpu2.pos[i] / 10).toFixed(1));
  return { p: out, s };
}
const fmt = (p) => `S6=${p.S6} S7=${p.S7} S8=${p.S8} S9=${p.S9} S10=${p.S10} mm`;
async function flags(label) {
  const di = await readDI(117, 6);
  const homing = await readCoil(101);
  const reason = await readHolding(110);
  log(`${label}: Home=${di & 1} AtTarget=${(di >> 1) & 1} Fault=${(di >> 3) & 1} Refused=${(di >> 4) & 1} Cpu2Online=${(di >> 5) & 1} ManualMode(119)=${(di >> 2) & 1} HomingBusy(coil 101)=${homing} RefusedReason(110)=${reason}`);
}
async function jog(coil, ms, label) {
  await writeCoil(coil, true);
  const t0 = Date.now();
  while (Date.now() - t0 < ms) { await readDI(117, 6); await sleep(80); }   // keep talking so the dead man stays quiet
  await writeCoil(coil, false);
  await sleep(600);
  const r = await positions();
  log(`${label}: ${r ? fmt(r.p) : 'no web answer'}`);
  return r;
}

sock.on('connect', async () => {
  try {
    log('connected');
    await flags('start');
    let r = await positions();
    log('start positions: ' + (r ? fmt(r.p) : '?'));

    log('--- 1. enter manual mode (coil 104) ---');
    await writeCoil(104, true);
    for (let i = 0; i < 40; i++) { if ((await readDI(117, 6) >> 2) & 1) break; await sleep(50); }
    await flags('manual requested');

    log('--- 2. jog spreader 8 OPEN for 2 s: spreaders 9 and 10 must move too, 6 and 7 must not ---');
    await writeReg(107, 8);
    await jog(105, 2000, 'after opening spreader 8');

    log('--- 3. jog spreader 8 CLOSE with all home sensors on: must not move (touching) ---');
    const before = await positions();
    await jog(106, 1000, 'after close attempt');
    log('    (before the close attempt: ' + fmt(before.p) + ')');

    log('--- 4. dead man: coil 105 on, then this script goes silent for 3 s: movement must stop after ~1 s ---');
    await writeCoil(105, true);
    const d0 = Date.now();
    const samples = [];
    while (Date.now() - d0 < 3200) { const q = await positions(); if (q) samples.push(`${((Date.now() - d0) / 1000).toFixed(1)}s S8=${q.p.S8}`); await sleep(350); }
    log('    S8 over time while silent: ' + samples.join('  '));
    await writeCoil(105, false);
    await sleep(300);

    log('--- 5. invalid spreader 5: must not move ---');
    await writeReg(107, 5);
    await jog(105, 700, 'after jogging spreader 5');

    log('--- 6. a pattern request during manual mode must be refused (reason 8) ---');
    await writeReg(104, 1);
    await sleep(500);
    await flags('after pattern 1 request');
    await writeReg(104, 0);
    await sleep(300);

    log('--- 7. leave manual mode: automatic home, coil 101 = homing in progress ---');
    await writeCoil(104, false);
    const h0 = Date.now();
    let sawBusy = false, last = '';
    while (Date.now() - h0 < 9000) {
      const busy = await readCoil(101);
      const di = await readDI(117, 6);
      if (busy) sawBusy = true;
      const s = `HomingBusy=${busy} ManualMode=${(di >> 2) & 1} Home=${di & 1} AtTarget=${(di >> 1) & 1}`;
      if (s !== last) { log('    ' + s); last = s; }
      await sleep(80);
    }
    r = await positions();
    log('final positions: ' + (r ? fmt(r.p) : '?') + `   (coil 101 was ${sawBusy ? 'on' : 'NEVER on'} during the automatic home)`);
    await flags('end');
    log('done');
  } catch (e) { console.error('ERROR', e.message); }
  try { await writeCoil(105, false); await writeCoil(106, false); await writeCoil(104, false); } catch (e) { }
  sock.end(); process.exit(0);
});
