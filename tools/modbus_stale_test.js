// Tests what happens when a Modbus client disappears without closing its connection (PLC power loss, cable pulled) and a new client connects.
// Usage:
//   node tools/modbus_stale_test.js hold [seconds]     Connect, make one request, then stay silent with the socket OPEN (default 120 s). This plays the PLC that vanished.
//   node tools/modbus_stale_test.js probe [seconds]    Play the PLC that comes back: connect fresh and read register 104 every second, report when it gets answers (default 40 s).
// Run "hold", pull and replug the Ethernet cable (or just wait), then run "probe" in another terminal.
const net = require('net');
const HOST = '192.168.2.51', PORT = 502, UNIT = 0xFF;
const mode = process.argv[2];
const seconds = parseInt(process.argv[3] || (mode === 'hold' ? '120' : '40'), 10);
const ts = () => { const d = new Date(); return d.toTimeString().slice(0, 8) + '.' + String(d.getMilliseconds()).padStart(3, '0'); };
const log = (m) => console.log(ts() + '  ' + m);
const sleep = (ms) => new Promise(r => setTimeout(r, ms));

function readReg104(sock, tid) {
  return new Promise((resolve) => {
    const pdu = Buffer.from([3, 0, 104, 0, 1]);
    const h = Buffer.alloc(7);
    h.writeUInt16BE(tid, 0); h.writeUInt16BE(0, 2); h.writeUInt16BE(pdu.length + 1, 4); h.writeUInt8(UNIT, 6);
    const timer = setTimeout(() => { sock.removeListener('data', onData); resolve(null); }, 2500);
    const onData = (b) => { clearTimeout(timer); sock.removeListener('data', onData); resolve(b.length >= 11 ? b.readUInt16BE(9) : -1); };
    sock.on('data', onData);
    sock.write(Buffer.concat([h, pdu]));
  });
}

async function hold() {
  const sock = net.connect(PORT, HOST);
  sock.on('error', (e) => log('socket error (hold): ' + e.message));
  sock.on('close', () => log('socket CLOSED (hold)'));
  await new Promise((r) => sock.on('connect', r));
  const v = await readReg104(sock, 1);
  log(`holding client connected, register 104 = ${v}. Now silent with the socket open for ${seconds} s`);
  await sleep(seconds * 1000);
  log('hold finished');
  sock.destroy();
  process.exit(0);
}

async function probe() {
  const end = Date.now() + seconds * 1000;
  let attempt = 0, ok = false;
  const t0 = Date.now();
  while (Date.now() < end && !ok) {
    attempt++;
    const sock = net.connect(PORT, HOST);
    let err = null;
    sock.on('error', (e) => { err = e.message; });
    const connected = await Promise.race([new Promise((r) => sock.on('connect', () => r(true))), sleep(3000).then(() => false)]);
    if (!connected) {
      log(`attempt ${attempt}: TCP connect failed (${err || 'timeout'})`);
    } else {
      const v = await readReg104(sock, attempt);
      if (v === null) {
        log(`attempt ${attempt}: connected, but NO answer to a read request within 2.5 s`);
      } else {
        log(`attempt ${attempt}: ANSWERED, register 104 = ${v}  (${((Date.now() - t0) / 1000).toFixed(1)} s after the probe started)`);
        ok = true;
      }
    }
    sock.destroy();
    if (!ok) await sleep(1000);
  }
  log(ok ? 'RESULT: a new client can connect and is served' : 'RESULT: a new client was NOT served within ' + seconds + ' s (the old connection is blocking the only Modbus slot)');
  process.exit(ok ? 0 : 1);
}

if (mode === 'hold') hold(); else if (mode === 'probe') probe(); else { console.log('usage: node tools/modbus_stale_test.js hold|probe [seconds]'); process.exit(2); }
