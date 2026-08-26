/*
 * H9 — IR Blaster + Replay
 * ESP32 IR signal capture, protocol decode, and remote replay
 *
 * Features:
 *   - IR signal capture via interrupt-driven receiver
 *   - NEC, Samsung, RC5 protocol auto-detection
 *   - Signal replay with configurable repeat count
 *   - Built-in common remote codes (TV, AC)
 *   - Serial command interface
 *
 * Hardware: ESP32 NodeMCU + IR receiver (TSOP38238) + IR LED
 * Pin D4 (GPIO4)  — IR receiver data
 * Pin D2 (GPIO2)  — IR LED (via NPN transistor)
 */

#include <Arduino.h>

// ── Pin Definitions ──────────────────────────────────────────────
#define IR_RX_PIN   4    // TSOP38238 DATA
#define IR_TX_PIN   2    // IR LED via NPN transistor

// ── Protocol Timing Constants (microseconds) ─────────────────────
// NEC
#define NEC_HDR_MARK    9000
#define NEC_HDR_SPACE   4500
#define NEC_BIT_MARK    560
#define NEC_ONE_SPACE   1690
#define NEC_ZERO_SPACE  560
#define NEC_RPT_SPACE   2250
#define NEC_RPT_HDR     9000
#define NEC_RPT_HDR_S   2250

// Samsung
#define SAM_HDR_MARK    4500
#define SAM_HDR_SPACE   4500
#define SAM_BIT_MARK    560
#define SAM_ONE_SPACE   1690
#define SAM_ZERO_SPACE  560

// RC5
#define RC5_HALF_BIT    889
#define RC5_TICK        (RC5_HALF_BIT * 2)

// Timing tolerance
#define TOLERANCE       30
#define MATCH(raw, expected) \
  ((raw) >= ((expected) - TOLERANCE) && (raw) <= ((expected) + TOLERANCE))

// ── Protocol IDs ─────────────────────────────────────────────────
enum Protocol { PROTO_UNKNOWN = 0, PROTO_NEC, PROTO_SAMSUNG, PROTO_RC5 };

const char* protoName[] = { "Unknown", "NEC", "Samsung", "RC5" };

// ── Signal Storage ───────────────────────────────────────────────
#define MAX_RAW_LEN 256
uint16_t rawBuffer[MAX_RAW_LEN];
volatile uint16_t rawIndex = 0;
volatile bool signalReady = false;
volatile Protocol detectedProto = PROTO_UNKNOWN;
volatile uint32_t irData = 0;

// Timing measurement
volatile uint32_t lastTime = 0;

// ── IR Receiver ISR ──────────────────────────────────────────────
void IRAM_ATTR irReceiveISR() {
  uint32_t now = micros();
  uint32_t duration = now - lastTime;
  lastTime = now;

  if (rawIndex < MAX_RAW_LEN) {
    rawBuffer[rawIndex++] = (uint16_t)duration;
  }

  // Reset after long space (end of transmission)
  if (duration > 50000) {
    rawIndex = 0;
  }
}

// ── Protocol Detection ───────────────────────────────────────────
Protocol detectProtocol() {
  if (rawIndex < 20) return PROTO_UNKNOWN;

  // Check for NEC: 9000 mark, 4500 space, then data
  if (MATCH(rawBuffer[0], NEC_HDR_MARK) && MATCH(rawBuffer[1], NEC_HDR_SPACE)) {
    if (rawIndex >= 34) return PROTO_NEC;
  }

  // Check for Samsung: 4500 mark, 4500 space
  if (MATCH(rawBuffer[0], SAM_HDR_MARK) && MATCH(rawBuffer[1], SAM_HDR_SPACE)) {
    if (rawIndex >= 34) return PROTO_SAMSUNG;
  }

  // Check for RC5: Manchester encoded, ~889us half-bits
  // RC5 starts with two start bits (1,1) then toggle bit
  // Look for consistent timing around 889us
  if (rawIndex >= 14) {
    uint16_t idx = rawIndex;
    uint8_t limit = (idx < 20) ? idx : 20;
    uint8_t consistent = 0;
    for (uint8_t i = 0; i < limit; i++) {
      if (MATCH(rawBuffer[i], RC5_HALF_BIT) || MATCH(rawBuffer[i], RC5_TICK)) {
        consistent++;
      }
    }
    if (consistent > 10) return PROTO_RC5;
  }

  return PROTO_UNKNOWN;
}

// ── Decode NEC ───────────────────────────────────────────────────
uint32_t decodeNEC() {
  uint32_t data = 0;
  uint8_t bits = 0;

  for (uint16_t i = 2; i + 1 < rawIndex && bits < 32; i += 2) {
    uint16_t mark = rawBuffer[i];
    uint16_t space = rawBuffer[i + 1];

    if (MATCH(mark, NEC_BIT_MARK)) {
      if (MATCH(space, NEC_ONE_SPACE)) {
        data |= (1UL << bits);
      } else if (!MATCH(space, NEC_ZERO_SPACE)) {
        break;
      }
      bits++;
    }
  }
  return data;
}

// ── Decode Samsung ───────────────────────────────────────────────
uint32_t decodeSamsung() {
  uint32_t data = 0;
  uint8_t bits = 0;

  for (uint16_t i = 2; i + 1 < rawIndex && bits < 32; i += 2) {
    uint16_t mark = rawBuffer[i];
    uint16_t space = rawBuffer[i + 1];

    if (MATCH(mark, SAM_BIT_MARK)) {
      if (MATCH(space, SAM_ONE_SPACE)) {
        data |= (1UL << bits);
      } else if (!MATCH(space, SAM_ZERO_SPACE)) {
        break;
      }
      bits++;
    }
  }
  return data;
}

// ── Decode RC5 ───────────────────────────────────────────────────
uint32_t decodeRC5() {
  uint32_t data = 0;
  uint8_t bits = 0;
  uint8_t level = 1;  // RC5 starts high

  for (uint16_t i = 0; i < rawIndex && bits < 14; i++) {
    uint16_t duration = rawBuffer[i];
    uint8_t ticks = (duration + RC5_HALF_BIT / 2) / RC5_HALF_BIT;

    for (uint8_t t = 0; t < ticks && bits < 14; t++) {
      if (bits == 0 || bits == 1) {
        // Start bits, always 1
        level = !level;
      } else {
        // Data bits: Manchester — transition = 1, no transition = 0
        data = (data << 1) | (level ? 1 : 0);
        level = !level;
      }
      bits++;
    }
  }
  return data;
}

// ── Store Signal for Replay ──────────────────────────────────────
struct IRSignal {
  Protocol proto;
  uint32_t data;
  uint16_t raw[MAX_RAW_LEN];
  uint16_t rawLen;
};

IRSignal storedSignal;
bool hasStoredSignal = false;

void storeSignal(Protocol proto, uint32_t data) {
  storedSignal.proto = proto;
  storedSignal.data = data;
  storedSignal.rawLen = rawIndex;
  memcpy(storedSignal.raw, rawBuffer, rawIndex * sizeof(uint16_t));
  hasStoredSignal = true;
}

// ── Transmit Raw ─────────────────────────────────────────────────
void txRaw(uint16_t *buffer, uint16_t len, uint8_t repeat) {
  for (uint8_t r = 0; r <= repeat; r++) {
    for (uint16_t i = 0; i < len; i++) {
      if (i % 2 == 0) {
        // Mark: carrier on
        unsigned long start = micros();
        while (micros() - start < buffer[i]) {
          digitalWrite(IR_TX_PIN, HIGH);
          delayMicroseconds(10);
          digitalWrite(IR_TX_PIN, LOW);
          delayMicroseconds(10);
        }
      } else {
        // Space: carrier off
        digitalWrite(IR_TX_PIN, LOW);
        delayMicroseconds(buffer[i]);
      }
    }
  }
  digitalWrite(IR_TX_PIN, LOW);
}

// ── Transmit NEC ─────────────────────────────────────────────────
void txNEC(uint32_t code, uint8_t repeat) {
  uint16_t buffer[68];
  uint8_t idx = 0;

  // Header
  buffer[idx++] = NEC_HDR_MARK;
  buffer[idx++] = NEC_HDR_SPACE;

  // 32 bits
  for (int8_t i = 0; i < 32; i++) {
    buffer[idx++] = NEC_BIT_MARK;
    buffer[idx++] = (code & (1UL << i)) ? NEC_ONE_SPACE : NEC_ZERO_SPACE;
  }

  // Stop bit
  buffer[idx++] = NEC_BIT_MARK;
  buffer[idx++] = 100;

  txRaw(buffer, idx, repeat);
}

// ── Transmit Samsung ─────────────────────────────────────────────
void txSamsung(uint32_t code, uint8_t repeat) {
  uint16_t buffer[68];
  uint8_t idx = 0;

  buffer[idx++] = SAM_HDR_MARK;
  buffer[idx++] = SAM_HDR_SPACE;

  for (int8_t i = 0; i < 32; i++) {
    buffer[idx++] = SAM_BIT_MARK;
    buffer[idx++] = (code & (1UL << i)) ? SAM_ONE_SPACE : SAM_ZERO_SPACE;
  }

  buffer[idx++] = SAM_BIT_MARK;
  buffer[idx++] = 100;

  txRaw(buffer, idx, repeat);
}

// ── Transmit RC5 ─────────────────────────────────────────────────
void txRC5(uint32_t code, uint8_t repeat) {
  uint16_t buffer[30];
  uint8_t idx = 0;
  uint8_t level = 1;

  for (int8_t i = 13; i >= 0; i--) {
    uint8_t bit = (code >> i) & 1;
    if (bit == level) {
      buffer[idx++] = RC5_TICK;
    } else {
      buffer[idx++] = RC5_HALF_BIT;
      buffer[idx++] = RC5_HALF_BIT;
    }
    level = !level;
  }

  txRaw(buffer, idx, repeat);
}

// ── Rebuild Raw from Protocol Data ───────────────────────────────
void replayStoredSignal() {
  if (!hasStoredSignal) {
    Serial.println(F("[!] No signal stored. Capture one first."));
    return;
  }

  Serial.print(F("[*] Replaying "));
  Serial.print(protoName[storedSignal.proto]);
  Serial.print(F(" code: 0x"));
  Serial.println(storedSignal.data, HEX);

  // Replay from raw buffer (most accurate)
  txRaw(storedSignal.raw, storedSignal.rawLen, 2);

  Serial.println(F("[+] Replay complete."));
}

// ── Capture & Analyze ────────────────────────────────────────────
void cmdCapture() {
  rawIndex = 0;
  signalReady = false;
  lastTime = micros();

  Serial.println(F("[*] Waiting for IR signal..."));
  Serial.println(F("    Point remote at receiver (GPIO4)"));

  uint32_t timeout = millis();
  while (!signalReady && (millis() - timeout < 10000)) {
    delay(10);
    if (rawIndex > 2) {
      // Check if signal ended (long space)
      uint32_t elapsed = micros() - lastTime;
      if (elapsed > 50000 && rawIndex > 10) {
        signalReady = true;
      }
    }
  }

  if (!signalReady) {
    Serial.println(F("[!] Timeout — no signal received"));
    return;
  }

  Protocol proto = detectProtocol();
  uint32_t data = 0;

  Serial.print(F("[+] Captured "));
  Serial.print(rawIndex);
  Serial.println(F(" samples"));

  switch (proto) {
    case PROTO_NEC:
      data = decodeNEC();
      break;
    case PROTO_SAMSUNG:
      data = decodeSamsung();
      break;
    case PROTO_RC5:
      data = decodeRC5();
      break;
    default:
      Serial.println(F("[!] Unknown protocol"));
      break;
  }

  if (proto != PROTO_UNKNOWN) {
    Serial.print(F("[+] Protocol:  "));
    Serial.println(protoName[proto]);
    Serial.print(F("[+] Raw code: 0x"));
    Serial.println(data, HEX);
    Serial.print(F("[+] Binary:   "));
    for (int8_t i = 31; i >= 0; i--) {
      Serial.print((data >> i) & 1);
      if (i % 8 == 0) Serial.print(" ");
    }
    Serial.println();

    // Decode common fields
    if (proto == PROTO_NEC || proto == PROTO_SAMSUNG) {
      uint8_t addr = data & 0xFF;
      uint8_t addrInv = (data >> 8) & 0xFF;
      uint8_t cmd = (data >> 16) & 0xFF;
      uint8_t cmdInv = (data >> 24) & 0xFF;
      Serial.print(F("[+] Address:  0x"));
      if (addr < 0x10) Serial.print("0");
      Serial.println(addr, HEX);
      Serial.print(F("[+] Command:  0x"));
      if (cmd < 0x10) Serial.print("0");
      Serial.println(cmd, HEX);

      if (proto == PROTO_NEC && (addr ^ addrInv) == 0xFF && (cmd ^ cmdInv) == 0xFF) {
        Serial.println(F("[+] NEC checksum OK"));
      } else if (proto == PROTO_SAMSUNG) {
        Serial.println(F("[+] Samsung protocol"));
      }
    } else if (proto == PROTO_RC5) {
      uint8_t toggle = (data >> 11) & 1;
      uint8_t addr = (data >> 6) & 0x1F;
      uint8_t cmd = data & 0x3F;
      Serial.print(F("[+] Toggle:   "));
      Serial.println(toggle);
      Serial.print(F("[+] Address:  0x"));
      if (addr < 0x10) Serial.print("0");
      Serial.println(addr, HEX);
      Serial.print(F("[+] Command:  0x"));
      if (cmd < 0x10) Serial.print("0");
      Serial.println(cmd, HEX);
    }

    storeSignal(proto, data);
  }
}

// ── Replay by Code ───────────────────────────────────────────────
void cmdReplay(const String& arg) {
  if (!hasStoredSignal && arg.length() == 0) {
    Serial.println(F("[!] No signal stored and no code provided."));
    Serial.println(F("    Usage: replay <hex_code>"));
    Serial.println(F("    Or:    replay       (replays last captured)"));
    return;
  }

  if (arg.length() > 0) {
    uint32_t code = strtoul(arg.c_str(), NULL, 16);
    Serial.print(F("[*] Replaying code: 0x"));
    Serial.println(code, HEX);

    switch (storedSignal.proto) {
      case PROTO_NEC:     txNEC(code, 2);     break;
      case PROTO_SAMSUNG: txSamsung(code, 2);  break;
      case PROTO_RC5:     txRC5(code, 2);      break;
      default:
        Serial.println(F("[!] No protocol stored. Capture first."));
        return;
    }
  } else {
    replayStoredSignal();
  }
}

// ── Built-in Codes ───────────────────────────────────────────────
void cmdCodes() {
  Serial.println(F("╔══════════════════════════════════════════╗"));
  Serial.println(F("║       H9 — Built-in IR Codes            ║"));
  Serial.println(F("╠══════════════════════════════════════════╣"));
  Serial.println(F("║  NEC codes (most common remotes):       ║"));
  Serial.println(F("║    TV Power:     0x00FF                ║"));
  Serial.println(F("║    Volume Up:    0x00FF40BF            ║"));
  Serial.println(F("║    Volume Down:  0x00FFC837            ║"));
  Serial.println(F("║    Channel Up:   0x00FF48B7            ║"));
  Serial.println(F("║    Channel Down: 0x00FF08F7            ║"));
  Serial.println(F("║    Mute:         0x00FFC03F            ║"));
  Serial.println(F("║    Input/Source: 0x00FF58A7            ║"));
  Serial.println(F("║                                       ║"));
  Serial.println(F("║  Samsung TV:                           ║"));
  Serial.println(F("║    Power:        0xE0E040BF            ║"));
  Serial.println(F("║    Volume Up:    0xE0E0E015            ║"));
  Serial.println(F("║    Volume Down:  0xE0E0D02F            ║"));
  Serial.println(F("║                                       ║"));
  Serial.println(F("║  Usage: send <code> [proto]            ║"));
  Serial.println(F("║    send 0x00FF          (last proto)   ║"));
  Serial.println(F("║    send 0x00FF nec      (explicit)     ║"));
  Serial.println(F("╚══════════════════════════════════════════╝"));
}

// ── Send Command ─────────────────────────────────────────────────
void cmdSend(const String& args) {
  int spaceIdx = args.indexOf(' ');
  String hexStr = (spaceIdx > 0) ? args.substring(0, spaceIdx) : args;
  String protoStr = (spaceIdx > 0) ? args.substring(spaceIdx + 1) : "";

  uint32_t code = strtoul(hexStr.c_str(), NULL, 16);
  Protocol proto = storedSignal.proto;

  if (protoStr.equalsIgnoreCase("nec"))         proto = PROTO_NEC;
  else if (protoStr.equalsIgnoreCase("samsung")) proto = PROTO_SAMSUNG;
  else if (protoStr.equalsIgnoreCase("rc5"))     proto = PROTO_RC5;

  if (proto == PROTO_UNKNOWN) {
    Serial.println(F("[!] No protocol specified and none stored."));
    Serial.println(F("    Usage: send <code> nec|samsung|rc5"));
    return;
  }

  Serial.print(F("[*] Sending 0x"));
  Serial.print(code, HEX);
  Serial.print(F(" via "));
  Serial.println(protoName[proto]);

  switch (proto) {
    case PROTO_NEC:     txNEC(code, 2);     break;
    case PROTO_SAMSUNG: txSamsung(code, 2);  break;
    case PROTO_RC5:     txRC5(code, 2);      break;
  }

  Serial.println(F("[+] Sent."));
}

// ── Signal Info ──────────────────────────────────────────────────
void cmdInfo() {
  Serial.print(F("[+] Stored protocol: "));
  Serial.println(protoName[storedSignal.proto]);
  Serial.print(F("[+] Stored code:     0x"));
  Serial.println(storedSignal.data, HEX);
  Serial.print(F("[+] Raw samples:     "));
  Serial.println(storedSignal.rawLen);
  Serial.print(F("[+] Has signal:      "));
  Serial.println(hasStoredSignal ? "YES" : "NO");
}

// ── Menu ─────────────────────────────────────────────────────────
void printMenu() {
  Serial.println();
  Serial.println(F("╔══════════════════════════════════════════╗"));
  Serial.println(F("║       H9 — IR Blaster + Replay          ║"));
  Serial.println(F("╠══════════════════════════════════════════╣"));
  Serial.println(F("║  capture — Record IR signal              ║"));
  Serial.println(F("║  replay  — Replay last / <hex_code>      ║"));
  Serial.println(F("║  send    — Send code <hex> [proto]       ║"));
  Serial.println(F("║  codes   — Show built-in remote codes    ║"));
  Serial.println(F("║  info    — Show stored signal info       ║"));
  Serial.println(F("║  menu    — Show this menu                ║"));
  Serial.println(F("╚══════════════════════════════════════════╝"));
  Serial.println();
}

// ── Setup ────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  pinMode(IR_RX_PIN, INPUT_PULLUP);
  pinMode(IR_TX_PIN, OUTPUT);
  digitalWrite(IR_TX_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(IR_RX_PIN), irReceiveISR, CHANGE);

  Serial.println();
  Serial.println(F("╔══════════════════════════════════════════╗"));
  Serial.println(F("║   H9 — IR Blaster + Replay v1.0         ║"));
  Serial.println(F("║   NEC / Samsung / RC5 support           ║"));
  Serial.println(F("╚══════════════════════════════════════════╝"));
  Serial.print(F("[+] Receiver: GPIO"));
  Serial.print(IR_RX_PIN);
  Serial.print(F("  Transmitter: GPIO"));
  Serial.println(IR_TX_PIN);
  printMenu();
}

// ── Loop ─────────────────────────────────────────────────────────
void loop() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  int spaceIdx = cmd.indexOf(' ');
  String command = (spaceIdx > 0) ? cmd.substring(0, spaceIdx) : cmd;
  String args = (spaceIdx > 0) ? cmd.substring(spaceIdx + 1) : "";
  command.toLowerCase();

  if (command == "capture")       cmdCapture();
  else if (command == "replay")   cmdReplay(args);
  else if (command == "send")     cmdSend(args);
  else if (command == "codes")    cmdCodes();
  else if (command == "info")     cmdInfo();
  else if (command == "menu")     printMenu();
  else {
    Serial.print(F("[?] Unknown: "));
    Serial.println(cmd);
  }
}
