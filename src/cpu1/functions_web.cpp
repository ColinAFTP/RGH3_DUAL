#include <Arduino.h>
#include <stdarg.h>
#include <string.h>

#include "constants.h"
#include "functions_web.h"
#include "variables_cpu1.h"
#include "web_page.h"

// Read only diagnostics web server. The Modbus polling in the main loop must never be held up, so:
//  - only one connection is served at a time,
//  - a request is read a few bytes at a time and the answer is sent in small chunks (never waiting for the network),
//  - every answer closes the connection.
// The page polls /status.json, /gaps.json and /log.json?since=N.

namespace {

// ---------------------------------------------------------------- event log

constexpr int LOG_SIZE = 48;
constexpr int LOG_TEXT = 72;
struct LogEntry {
  uint32_t seq;                         // 1, 2, 3, ... never repeats
  uint32_t ms;                          // millis() when the event happened
  char text[LOG_TEXT];
};
LogEntry logBuf[LOG_SIZE];
uint32_t logSeq = 0;                    // Sequence number of the newest entry

// ---------------------------------------------------------------- CPU2 status

StatusPacket cpu2Status;
uint32_t cpu2StatusMs = 0;              // millis() when the last status packet arrived (0 = never)
uint32_t cpu2RxCount = 0;

// ---------------------------------------------------------------- loop time

uint32_t loopLastUs = 0;
uint32_t loopMaxUs = 0;
uint32_t loopSumUs = 0;
uint32_t loopCount = 0;
uint32_t loopWindowStart = 0;
uint32_t publishedMaxUs = 0;
uint32_t publishedAvgUs = 0;

// ---------------------------------------------------------------- HTTP server

EthernetServer webServer(80);
EthernetClient webClient;
enum WebState { WEB_IDLE, WEB_READ, WEB_SEND };
WebState webState = WEB_IDLE;
uint32_t webStart = 0;                  // millis() when the current connection was accepted

char reqLine[120];                      // First line of the request, for example: GET /status.json HTTP/1.1
int reqLen = 0;
bool reqLineDone = false;
uint32_t lastFour = 0;                  // Last four request bytes, to find the blank line that ends the headers

char bodyBuf[4096];                     // JSON body
size_t bodyLen = 0;
const char* body = nullptr;             // Points at bodyBuf or at the page in flash
char hdrBuf[160];
size_t hdrLen = 0;
size_t sentTotal = 0;                   // Bytes of header + body sent so far

// Append formatted text to the JSON body
void jp(const char* fmt, ...) {
  if (bodyLen >= sizeof(bodyBuf) - 1) return;
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(bodyBuf + bodyLen, sizeof(bodyBuf) - bodyLen, fmt, ap);
  va_end(ap);
  if (n > 0) {
    size_t room = sizeof(bodyBuf) - bodyLen - 1;
    bodyLen += ((size_t)n < room) ? (size_t)n : room;
  }
}

void buildStatus() {
  bodyLen = 0;
  uint32_t now = millis();
  bool cpu2Ok = cpu2StatusMs != 0 && (now - cpu2StatusMs) < 1500;
  jp("{\"up\":%lu,\"loopMax\":%lu,\"loopAvg\":%lu,\"plc\":%d,\"home\":%d,\"target\":%d,\"fault\":%d,\"mask\":%u,"
     "\"pattern\":%d,\"speed\":%d,\"inputs\":%u,\"relays\":%u,",
     (unsigned long)now, (unsigned long)publishedMaxUs, (unsigned long)publishedAvgUs,
     ethernetClient.connected() ? 1 : 0, statusHome ? 1 : 0, statusAtTarget ? 1 : 0, statusFault ? 1 : 0,
     (unsigned)faultMaskRx, patternSelection, stepperSpeed, (unsigned)inputData, (unsigned)relayData);
  jp("\"cpu2\":{\"ok\":%d,\"rx\":%lu,\"state\":%u,\"known\":%u,\"mask\":%u,\"pos\":[",
     cpu2Ok ? 1 : 0, (unsigned long)cpu2RxCount, (unsigned)cpu2Status.state, (unsigned)(cpu2Status.flags & 1),
     (unsigned)cpu2Status.faultMask);
  for (int i = 0; i < NUM_GAPS; i++) {
    jp("%s%d", i ? "," : "", (int)cpu2Status.positions[i]);
  }
  jp("]}}");
}

// Gaps in tenths of a millimetre
void buildGaps() {
  bodyLen = 0;
  jp("{\"gaps\":[");
  for (int p = 0; p < NUM_PATTERNS; p++) {
    jp("%s[", p ? "," : "");
    for (int g = 0; g < NUM_GAPS; g++) {
      jp("%s%ld", g ? "," : "", lroundf(gapArrays[p][g] * 10.0f));
    }
    jp("]");
  }
  jp("]}");
}

// All log entries newer than 'since', oldest first
void buildLog(uint32_t since) {
  bodyLen = 0;
  jp("{\"last\":%lu,\"ev\":[", (unsigned long)logSeq);
  uint32_t first = logSeq > LOG_SIZE ? logSeq - LOG_SIZE + 1 : 1;
  if (since + 1 > first) first = since + 1;
  bool comma = false;
  for (uint32_t s = first; s <= logSeq; s++) {
    const LogEntry& e = logBuf[s % LOG_SIZE];
    if (e.seq != s) continue;
    jp("%s[%lu,%lu,\"", comma ? "," : "", (unsigned long)e.seq, (unsigned long)e.ms);
    for (const char* c = e.text; *c && bodyLen < sizeof(bodyBuf) - 8; c++) {
      if (*c == '"' || *c == '\\') continue;              // Keep the JSON valid
      bodyBuf[bodyLen++] = *c;
    }
    jp("\"]");
    comma = true;
  }
  jp("]}");
}

void buildHeader(const char* status, const char* type, size_t length) {
  hdrLen = snprintf(hdrBuf, sizeof(hdrBuf),
                    "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %u\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n",
                    status, type, (unsigned)length);
}

// Decide what to answer from the request line
void route() {
  sentTotal = 0;
  if (strncmp(reqLine, "GET ", 4) != 0) {
    bodyLen = 0;
    body = bodyBuf;
    buildHeader("405 Method Not Allowed", "text/plain", 0);
    return;
  }
  const char* path = reqLine + 4;
  if (strncmp(path, "/status.json", 12) == 0) {
    buildStatus();
    body = bodyBuf;
    buildHeader("200 OK", "application/json", bodyLen);
  } else if (strncmp(path, "/gaps.json", 10) == 0) {
    buildGaps();
    body = bodyBuf;
    buildHeader("200 OK", "application/json", bodyLen);
  } else if (strncmp(path, "/log.json", 9) == 0) {
    const char* q = strstr(path, "since=");
    uint32_t since = q ? strtoul(q + 6, nullptr, 10) : 0;
    buildLog(since);
    body = bodyBuf;
    buildHeader("200 OK", "application/json", bodyLen);
  } else if (path[0] == '/' && (path[1] == ' ' || path[1] == '?')) {
    body = WEB_PAGE;
    bodyLen = strlen(WEB_PAGE);
    buildHeader("200 OK", "text/html; charset=utf-8", bodyLen);
  } else {
    bodyLen = 0;
    body = bodyBuf;
    buildHeader("404 Not Found", "text/plain", 0);
  }
}

void webClose() {
  webClient.stop();
  webState = WEB_IDLE;
}

const char* reasonText(int reason) {
  switch (reason) {
    case EVT_REASON_BUSY: return "already moving or homing";
    case EVT_REASON_FAULT: return "homing fault is active";
    case EVT_REASON_UNKNOWN_POS: return "positions unknown, home first";
    case EVT_REASON_BAD_PATTERN: return "invalid pattern";
    case EVT_REASON_NO_GAPS: return "could not refresh gap data";
    case EVT_REASON_BAD_TARGETS: return "targets outside the rack travel";
    case EVT_REASON_HOME_FAILED: return "home request failed";
    default: return "unknown reason";
  }
}

// Turn one CPU2 event into an event log line
void logCpu2Event(const StatusEvent& e) {
  switch (e.code) {
    case EVT_BOOT: logEvent("CPU2 started"); break;
    case EVT_POWERUP_HOME: logEvent("CPU2 power up: all home sensors on, gripper is home"); break;
    case EVT_POWERUP_SEARCH: logEvent("CPU2 power up: not home, search home started"); break;
    case EVT_TRIGGER: logEvent("CPU2 start move trigger, pattern %d", e.arg); break;
    case EVT_MOVE_START: logEvent("CPU2 move started, pattern %d", e.arg); break;
    case EVT_MOVE_DONE: logEvent("CPU2 move finished"); break;
    case EVT_HOME_START:
      logEvent("CPU2 homing started (%s)", e.arg == 0 ? "TeensyStep only" : e.arg == 1 ? "approach, then pulses" : "search, pulses only");
      break;
    case EVT_HOME_PULSES: logEvent("CPU2 homing: direct pulse stage started"); break;
    case EVT_HOME_DONE: logEvent("CPU2 homing complete, all spreaders home"); break;
    case EVT_HOME_FAULT: {
      char list[40] = "";
      for (int b = 0; b < 10; b++) {
        if (e.arg & (1 << b)) {
          char one[8];
          snprintf(one, sizeof(one), " S%d", b + 1);
          strncat(list, one, sizeof(list) - strlen(list) - 1);
        }
      }
      logEvent("HOMING FAULT, failed spreaders:%s", list);
      break;
    }
    case EVT_FAULT_RESET: logEvent("CPU2 fault reset, search home started"); break;
    case EVT_REFUSED: logEvent("CPU2 refused request: %s", reasonText(e.arg)); break;
    case EVT_IO_FAIL: logEvent("CPU2 cannot read the home sensors over I2C, pulses stopped"); break;
    default: logEvent("CPU2 event %d (%d)", e.code, e.arg); break;
  }
}

}  // namespace

void logEvent(const char* fmt, ...) {
  LogEntry& e = logBuf[(logSeq + 1) % LOG_SIZE];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(e.text, LOG_TEXT, fmt, ap);
  va_end(ap);
  e.ms = millis();
  logSeq++;
  e.seq = logSeq;
}

void webSetup() {
  webServer.begin();
  logEvent("CPU1 started");
}

void webLoopTick() {
  uint32_t nowUs = micros();
  uint32_t d = nowUs - loopLastUs;
  loopLastUs = nowUs;
  if (loopWindowStart == 0) {           // First pass: nothing to measure yet
    loopWindowStart = millis();
    return;
  }
  if (d > loopMaxUs) loopMaxUs = d;
  loopSumUs += d;
  loopCount++;
  if (millis() - loopWindowStart >= 1000) {
    publishedMaxUs = loopMaxUs;
    publishedAvgUs = loopCount ? loopSumUs / loopCount : 0;
    loopMaxUs = 0;
    loopSumUs = 0;
    loopCount = 0;
    loopWindowStart = millis();
  }
}

void cpu2StatusService() {
  while (statusRingTail != statusRingHead) {
    uint8_t tail = statusRingTail;
    StatusPacket p;
    memcpy(&p, (const void*)&statusRing[tail], sizeof(p));
    statusRingTail = (tail + 1) % STATUS_RING_SIZE;

    cpu2Status = p;
    cpu2StatusMs = millis();
    cpu2RxCount++;
    int n = p.numEvents < STATUS_MAX_EVENTS ? p.numEvents : STATUS_MAX_EVENTS;
    for (int i = 0; i < n; i++) {
      logCpu2Event(p.events[i]);
    }
  }
}

void webService() {
  switch (webState) {
    case WEB_IDLE: {
      // accept() returns a connected client at once. available() would wait for its first bytes, up to 10 s,
      // and so could freeze the whole loop (including the Modbus polling) for a client that sends nothing.
      EthernetClient c = webServer.accept();
      if (c.connected()) {
        webClient = c;
        webState = WEB_READ;
        webStart = millis();
        reqLen = 0;
        reqLineDone = false;
        lastFour = 0;
      }
      break;
    }

    case WEB_READ: {
      // Read what has arrived. Everything up to the blank line is read (the browser sends several headers), so the
      // connection can be closed without unread data, which would otherwise reset it and cut the answer short.
      bool headersDone = false;
      int guard = 200;
      while (webClient.available() && guard-- > 0) {
        int ch = webClient.read();
        if (ch < 0) break;
        lastFour = (lastFour << 8) | (uint8_t)ch;
        if (!reqLineDone) {
          if (ch == '\n' || reqLen >= (int)sizeof(reqLine) - 1) {
            reqLineDone = true;
          } else if (ch != '\r') {
            reqLine[reqLen++] = (char)ch;
          }
          reqLine[reqLen] = 0;
        }
        if (lastFour == 0x0D0A0D0A) {
          headersDone = true;
          break;
        }
      }
      if (headersDone) {
        route();
        webState = WEB_SEND;
      } else if (millis() - webStart > 1500) {
        webClose();
      }
      break;
    }

    case WEB_SEND: {
      size_t total = hdrLen + bodyLen;
      int room = webClient.availableForWrite();
      if (room > 0 && sentTotal < total) {
        size_t chunk = total - sentTotal;
        if (chunk > (size_t)room) chunk = room;
        if (chunk > 1024) chunk = 1024;
        // The header comes first, then the body
        if (sentTotal < hdrLen) {
          size_t part = hdrLen - sentTotal;
          if (chunk > part) chunk = part;
          webClient.write((const uint8_t*)hdrBuf + sentTotal, chunk);
        } else {
          webClient.write((const uint8_t*)body + (sentTotal - hdrLen), chunk);
        }
        sentTotal += chunk;
      }
      if (sentTotal >= total) {
        webClose();
      } else if (millis() - webStart > 4000 || !webClient.connected()) {
        webClose();
      }
      break;
    }
  }
}
