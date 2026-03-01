// ============================================================================
// ESP32-C3 OLED — HUB TEST (SBUS -> RcBusRx decode -> SBUS re-pack -> ESPNOW)
// Entrée : SBUS sur Serial1 (GPIO20), 100000 8E2, inversion
// Sortie : ESPNOW paquet EsnSbusPkt contenant une trame SBUS 25 bytes reconstruite
// + AJOUT: peers UNICAST configurables (max 5) + sauvegarde NVS
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <RcBusRx.h>

#include <Preferences.h>

static Preferences prefs;

static const uint8_t S3_MAC[6]     = { 0x20, 0x6E, 0xF1, 0xB1, 0xAD, 0xD0 };
static const uint8_t BRIDGE_MAC[6] = { 0xDC, 0xB4, 0xD9, 0x9C, 0x42, 0x08 };

// ─────────────────────────────────────────────
// CONFIG
// ─────────────────────────────────────────────
static constexpr int SBUS_RX_PIN   = 20;
static constexpr int SBUS_TX_PIN   = -1;
static constexpr bool SBUS_INVERT  = true;
static constexpr uint8_t ESPNOW_CH = 1;

// X8R ~100 Hz => 10 ms
static constexpr uint32_t SEND_PERIOD_MS = 10;

// UART RC
static HardwareSerial& RC = Serial1;

enum RcProto : uint8_t {
  RC_PROTO_SBUS = 0,
  RC_PROTO_IBUS,
  RC_PROTO_SRXL,
  RC_PROTO_SUMD,
  RC_PROTO_JETI,
};

static RcProto gRcProto = RC_PROTO_SBUS;  // pour l’instant: SBUS only

// ─────────────────────────────────────────────
// SAVE LOAD (proto)
// ─────────────────────────────────────────────
static void saveProtoToNvs() {
  prefs.begin("hub", false);          // namespace "hub"
  prefs.putUChar("rcproto", (uint8_t)gRcProto);
  prefs.end();

  Serial.println("NVS: rcproto saved");
}

static void loadProtoFromNvs() {
  prefs.begin("hub", true);           // read-only
  uint8_t p = prefs.getUChar("rcproto", (uint8_t)RC_PROTO_SBUS);
  prefs.end();

  if (p <= RC_PROTO_JETI) {
    gRcProto = (RcProto)p;
  } else {
    gRcProto = RC_PROTO_SBUS;
  }

  Serial.printf("NVS: rcproto loaded = %u\n", (unsigned)gRcProto);
}

// ─────────────────────────────────────────────
// ESPNOW UNICAST peer list (NVS) — max 5
// ─────────────────────────────────────────────
static constexpr uint8_t MAX_PEERS = 5;

struct PeerSlot {
  bool    used;
  uint8_t mac[6];
};

static PeerSlot gPeers[MAX_PEERS] = {};

static void print_mac(const uint8_t m[6]) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}

static bool parse_hex_byte(const char* s, uint8_t& out) {
  auto hex = [](char c)->int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
  };
  int hi = hex(s[0]), lo = hex(s[1]);
  if (hi < 0 || lo < 0) return false;
  out = (uint8_t)((hi << 4) | lo);
  return true;
}

static bool parse_mac(const char* str, uint8_t mac[6]) {
  // "AA:BB:CC:DD:EE:FF"
  if (!str) return false;
  if (strlen(str) < 17) return false;
  for (int i = 0; i < 6; i++) {
    if (!parse_hex_byte(str + (i * 3), mac[i])) return false;
    if (i < 5 && str[i * 3 + 2] != ':') return false;
  }
  return true;
}

static int find_peer_slot(const uint8_t mac[6]) {
  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    if (gPeers[i].used && memcmp(gPeers[i].mac, mac, 6) == 0) return (int)i;
  }
  return -1;
}

static int find_free_slot() {
  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    if (!gPeers[i].used) return (int)i;
  }
  return -1;
}

static uint8_t peers_count_used() {
  uint8_t n = 0;
  for (uint8_t i = 0; i < MAX_PEERS; i++) if (gPeers[i].used) n++;
  return n;
}

static void peers_list() {
  Serial.println("PEERS:");
  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    Serial.printf("  [%u] ", (unsigned)i);
    if (!gPeers[i].used) {
      Serial.println("(empty)");
    } else {
      print_mac(gPeers[i].mac);
      Serial.println();
    }
  }
}

static void peers_save_to_nvs() {
  prefs.begin("hub", false);

  // petit "magic" pour savoir si initialisé
  prefs.putUChar("peers_v", 1);

  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    char keyU[10]; char keyM[10];
    snprintf(keyU, sizeof(keyU), "p%u_u", (unsigned)i);
    snprintf(keyM, sizeof(keyM), "p%u_m", (unsigned)i);

    prefs.putBool(keyU, gPeers[i].used);
    prefs.putBytes(keyM, gPeers[i].mac, 6);
  }

  prefs.end();
  Serial.println("NVS: peers saved");
}

static void peers_load_from_nvs_or_defaults() {
  prefs.begin("hub", true);
  uint8_t v = prefs.getUChar("peers_v", 0);
  prefs.end();

  // Si pas initialisé => defaults (tes 2 MAC actuelles) + save
  if (v != 1) {
    memset(gPeers, 0, sizeof(gPeers));

    gPeers[0].used = true; memcpy(gPeers[0].mac, BRIDGE_MAC, 6);
    gPeers[1].used = true; memcpy(gPeers[1].mac, S3_MAC, 6);

    peers_save_to_nvs();
    Serial.println("NVS: peers init defaults (BRIDGE + S3)");
    return;
  }

  // Load normal
  prefs.begin("hub", true);
  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    char keyU[10]; char keyM[10];
    snprintf(keyU, sizeof(keyU), "p%u_u", (unsigned)i);
    snprintf(keyM, sizeof(keyM), "p%u_m", (unsigned)i);

    gPeers[i].used = prefs.getBool(keyU, false);
    size_t n = prefs.getBytesLength(keyM);
    if (n == 6) prefs.getBytes(keyM, gPeers[i].mac, 6);
    else memset(gPeers[i].mac, 0, 6);

    // robustesse: si MAC vide => slot off
    uint8_t z[6] = {0};
    if (gPeers[i].used && memcmp(gPeers[i].mac, z, 6) == 0) gPeers[i].used = false;
  }
  prefs.end();

  Serial.printf("NVS: peers loaded (used=%u)\n", (unsigned)peers_count_used());
}

// ─────────────────────────────────────────────
// ESPNOW packet
// ─────────────────────────────────────────────
#pragma pack(push, 1)
struct EsnSbusPkt {
  uint16_t magic;   // 0x5342 = 'SB'
  uint8_t  seq;
  uint32_t ms;
  uint8_t  sbus[25];
};
#pragma pack(pop)

static constexpr uint16_t ESN_MAGIC_SB = 0x5342;
static uint8_t s_seq = 0;

// ─────────────────────────────────────────────
// SBUS encode helpers
// ─────────────────────────────────────────────
static inline uint16_t clamp_u16(int v, int lo, int hi) {
  if (v < lo) return (uint16_t)lo;
  if (v > hi) return (uint16_t)hi;
  return (uint16_t)v;
}

// 1000..2000 µs -> 172..1811 (profil "FrSky-like"), clamp 0..2047
static inline uint16_t us_to_sbus11(uint16_t us) {
  int v = 172 + (int)(((int)us - 1000) * 1639 / 1000);
  return clamp_u16(v, 0, 2047);
}

// Pack 16 canaux 11-bit -> trame SBUS 25 bytes
static void sbus_pack_25(uint8_t out[25], const uint16_t ch11[16], bool frameLost, bool failsafe) {
  memset(out, 0, 25);
  out[0] = 0x0F;

  uint32_t bitpos = 0;
  for (int i = 0; i < 16; i++) {
    uint16_t v = (uint16_t)(ch11[i] & 0x07FF);
    for (int b = 0; b < 11; b++) {
      if (v & (1u << b)) {
        uint32_t p = bitpos + (uint32_t)b;
        out[1 + (p >> 3)] |= (uint8_t)(1u << (p & 7));
      }
    }
    bitpos += 11;
  }

  uint8_t flags = 0;
  if (frameLost) flags |= (1u << 2);
  if (failsafe)  flags |= (1u << 3);
  out[23] = flags;
  out[24] = 0x00;
}

// ─────────────────────────────────────────────
// Peer runtime (anti-bug si peer absent)
// ─────────────────────────────────────────────
struct PeerRt {
  uint16_t ok = 0;
  uint16_t fail = 0;
  uint8_t  failStreak = 0;
  uint32_t mutedUntilMs = 0;
  uint32_t lastOkMs = 0;
};

static PeerRt gPeerRt[MAX_PEERS] = {};

static int find_peer_slot_by_mac(const uint8_t mac[6]) {
  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    if (gPeers[i].used && memcmp(gPeers[i].mac, mac, 6) == 0) return (int)i;
  }
  return -1;
}

static bool peer_is_muted(uint8_t idx, uint32_t now) {
  return (idx < MAX_PEERS) && (gPeerRt[idx].mutedUntilMs != 0) && ((int32_t)(now - gPeerRt[idx].mutedUntilMs) < 0);
}

static void peer_mute(uint8_t idx, uint32_t now, uint32_t durMs) {
  if (idx >= MAX_PEERS) return;
  gPeerRt[idx].mutedUntilMs = now + durMs;
  gPeerRt[idx].failStreak = 0; // reset streak quand on mute
}

static void on_espnow_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status) {
  if (!mac_addr) return;

  int idx = find_peer_slot_by_mac(mac_addr);
  if (idx < 0) return;

  const uint32_t now = millis();

  if (status == ESP_NOW_SEND_SUCCESS) {
    gPeerRt[idx].ok++;
    gPeerRt[idx].failStreak = 0;
    gPeerRt[idx].lastOkMs = now;
    gPeerRt[idx].mutedUntilMs = 0;
  } else {
    gPeerRt[idx].fail++;
    if (gPeerRt[idx].failStreak < 255) gPeerRt[idx].failStreak++;

    // Circuit breaker: si 6 fails d'affilée -> mute 3s (à ajuster)
    if (gPeerRt[idx].failStreak >= 6) {
      peer_mute((uint8_t)idx, now, 3000);
    }
  }
}

// ─────────────────────────────────────────────
// ESPNOW init (ton init, conservé) + peers configurables
// ─────────────────────────────────────────────
static void espnow_add_peer(const uint8_t mac[6], const char* name) {
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, mac, 6);
  p.channel = ESPNOW_CH;
  p.encrypt = false;
  p.ifidx   = WIFI_IF_STA;

  if (esp_now_is_peer_exist(mac)) esp_now_del_peer(mac);
  esp_err_t e = esp_now_add_peer(&p);
  Serial.printf("addPeer %-6s -> %d\n", name, (int)e);
}

static void espnow_del_peer(const uint8_t mac[6], const char* name) {
  if (!esp_now_is_peer_exist(mac)) return;
  esp_err_t e = esp_now_del_peer(mac);
  Serial.printf("delPeer %-6s -> %d\n", name, (int)e);
}

static void espnow_sync_peers_from_table() {
  // Ajoute tous les peers "used" (UNICAST)
  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    if (!gPeers[i].used) continue;
    espnow_add_peer(gPeers[i].mac, "PEER");
  }
}

static void espnow_init() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);

  // Chez toi : obligatoire
  WiFi.begin();

  // IMPORTANT: NE PAS disconnect() ici
  WiFi.setSleep(false);
  delay(200);

  // Fix canal (avant esp_now_init)
  esp_err_t ec = esp_wifi_set_channel(ESPNOW_CH, WIFI_SECOND_CHAN_NONE);
  Serial.printf("set_channel(%u) -> %d\n", (unsigned)ESPNOW_CH, (int)ec);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init FAIL");
    while (true) delay(1000);
  }

  // callback TX
  esp_now_register_send_cb(on_espnow_send_cb);

  // Peers configurables
  peers_load_from_nvs_or_defaults();
  memset(gPeerRt, 0, sizeof(gPeerRt));
  espnow_sync_peers_from_table();

  uint8_t primary; wifi_second_chan_t second;
  esp_wifi_get_channel(&primary, &second);
  Serial.printf("ESP-NOW READY | MAC=%s | CH=%u\n", WiFi.macAddress().c_str(), (unsigned)primary);

  peers_list();
}

static volatile uint32_t g_tx_ok = 0;
static volatile uint32_t g_tx_ko = 0;

static void espnow_send_sbus25(const uint8_t sbus25[25]) {
  EsnSbusPkt p = {};
  p.magic = ESN_MAGIC_SB;
  p.seq   = s_seq++;
  p.ms    = millis();
  memcpy(p.sbus, sbus25, 25);

  const uint32_t now = millis();

  bool anyQueued = false;
  bool anyHardErr = false;

  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    if (!gPeers[i].used) continue;
    if (peer_is_muted(i, now)) continue;

    esp_err_t e = esp_now_send(gPeers[i].mac, (const uint8_t*)&p, sizeof(p));

    if (e == ESP_OK) {
      anyQueued = true;
      continue;
    }

    // Log utile (1 ligne) pour diagnostiquer
    Serial.printf("ESP-NOW send err=%d on peer[%u]\n", (int)e, (unsigned)i);

    // On ne mute QUE si la pile est pleine (NO_MEM)
    if (e == ESP_ERR_ESPNOW_NO_MEM) {
      peer_mute(i, now, 800);
    } else {
      anyHardErr = true; // wifi pas prêt / espnow pas prêt / peer absent / etc.
    }
  }

  if (anyQueued && !anyHardErr) g_tx_ok++;
  else                          g_tx_ko++;
}
// ─────────────────────────────────────────────
// RC init+ attach RcBusRx
// ─────────────────────────────────────────────
static void reinitRcSerial() {
  RC.end();
  delay(10);
  RC.setRxBufferSize(4096);

  switch (gRcProto) {
    case RC_PROTO_SBUS:
      RC.begin(100000, SERIAL_8E2, SBUS_RX_PIN, SBUS_TX_PIN, true /*inv forced*/);
      RcBusRx.setProto(RC_BUS_RX_SBUS);
      RcBusRx.serialAttach(&RC);
      Serial.printf("RC IN: SBUS | RX=%d inv=FORCED\n", SBUS_RX_PIN);
      break;

    case RC_PROTO_IBUS:
      RC.begin(115200, SERIAL_8N1, SBUS_RX_PIN, SBUS_TX_PIN, false);
      RcBusRx.setProto(RC_BUS_RX_IBUS);
      RcBusRx.serialAttach(&RC);
      Serial.printf("RC IN: IBUS | RX=%d\n", SBUS_RX_PIN);
      break;

    case RC_PROTO_SRXL:
      RC.begin(115200, SERIAL_8N1, SBUS_RX_PIN, SBUS_TX_PIN, false);
      RcBusRx.setProto(RC_BUS_RX_SRXL);
      RcBusRx.serialAttach(&RC);
      Serial.printf("RC IN: SRXL | RX=%d\n", SBUS_RX_PIN);
      break;

    case RC_PROTO_SUMD:
      RC.begin(115200, SERIAL_8N1, SBUS_RX_PIN, SBUS_TX_PIN, false);
      RcBusRx.setProto(RC_BUS_RX_SUMD);
      RcBusRx.serialAttach(&RC);
      Serial.printf("RC IN: SUMD | RX=%d\n", SBUS_RX_PIN);
      break;

    case RC_PROTO_JETI:
      RC.begin(125000, SERIAL_8N1, SBUS_RX_PIN, SBUS_TX_PIN, false);
      RcBusRx.setProto(RC_BUS_RX_JETI);
      RcBusRx.serialAttach(&RC);
      Serial.printf("RC IN: JETI | RX=%d\n", SBUS_RX_PIN);
      break;

    default:
      Serial.println("RC IN: proto unknown");
      break;
  }
}

// ─────────────────────────────────────────────
// Console série (ligne) : garde 0..4 + ajoute PEER ...
// ─────────────────────────────────────────────
static bool read_line(char* out, size_t cap) {
  static size_t n = 0;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      out[n] = 0;
      n = 0;
      return true;
    }
    if (n + 1 < cap) out[n++] = c;
  }
  return false;
}

static void handle_peer_cmd(const char* line) {
  // PEER LIST
  // PEER ADD AA:BB:CC:DD:EE:FF
  // PEER DEL AA:BB:CC:DD:EE:FF
  // PEER CLEAR
  if (strncmp(line, "PEER ", 5) != 0) return;
  const char* p = line + 5;

  if (strcmp(p, "LIST") == 0) {
    peers_list();
    return;
  }

  if (strcmp(p, "CLEAR") == 0) {
    for (uint8_t i = 0; i < MAX_PEERS; i++) {
      if (gPeers[i].used) espnow_del_peer(gPeers[i].mac, "PEER");
      gPeers[i].used = false;
      memset(gPeers[i].mac, 0, 6);
    }
    peers_save_to_nvs();
    Serial.println("OK: peers cleared");
    return;
  }

  if (strncmp(p, "ADD ", 4) == 0) {
    uint8_t mac[6];
    if (!parse_mac(p + 4, mac)) {
      Serial.println("ERR: bad MAC");
      return;
    }

    int ex = find_peer_slot(mac);
    if (ex >= 0) {
      Serial.println("OK: already present");
      return;
    }

    int free = find_free_slot();
    if (free < 0) {
      Serial.println("ERR: peer list full");
      return;
    }

    gPeers[free].used = true;
    memcpy(gPeers[free].mac, mac, 6);

    espnow_add_peer(mac, "PEER");
    peers_save_to_nvs();

    Serial.print("OK: added ["); Serial.print(free); Serial.print("] ");
    print_mac(mac); Serial.println();
    return;
  }

  if (strncmp(p, "DEL ", 4) == 0) {
    uint8_t mac[6];
    if (!parse_mac(p + 4, mac)) {
      Serial.println("ERR: bad MAC");
      return;
    }

    int idx = find_peer_slot(mac);
    if (idx < 0) {
      Serial.println("ERR: not found");
      return;
    }

    espnow_del_peer(mac, "PEER");
    gPeers[idx].used = false;
    memset(gPeers[idx].mac, 0, 6);

    peers_save_to_nvs();

    Serial.print("OK: deleted ["); Serial.print(idx); Serial.print("] ");
    print_mac(mac); Serial.println();
    return;
  }

  Serial.println("USAGE: PEER LIST | PEER ADD AA:.. | PEER DEL AA:.. | PEER CLEAR");
}

static void handle_proto_digit_line(const char* line) {
  // Compat: "0".."4" (ancien mode char)
  if (!line || !line[0] || line[1] != 0) return;

  char c = line[0];
  bool changed = false;

  if (c == '0') {
    gRcProto = RC_PROTO_SBUS;
    changed = true;
  }
  if (c == '1') {
    gRcProto = RC_PROTO_IBUS;
    changed = true;
  }
  if (c == '2') {
    gRcProto = RC_PROTO_SRXL;
    changed = true;
  }
  if (c == '3') {
    gRcProto = RC_PROTO_SUMD;
    changed = true;
  }
  if (c == '4') {
    gRcProto = RC_PROTO_JETI;
    changed = true;
  }

  if (!changed) return;

  reinitRcSerial();
  saveProtoToNvs();

  const char* name = "UNKNOWN";
  switch (gRcProto) {
    case RC_PROTO_SBUS: name = "SBUS"; break;
    case RC_PROTO_IBUS: name = "IBUS"; break;
    case RC_PROTO_SRXL: name = "SRXL"; break;
    case RC_PROTO_SUMD: name = "SUMD"; break;
    case RC_PROTO_JETI: name = "JETI"; break;
  }

  Serial.printf("\n>>> RC PROTO SWITCHED TO: %s <<<\n\n", name);
}

// ─────────────────────────────────────────────
// Arduino
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("=== C3 HUB: SBUS decode (RcBusRx) -> SBUS repack -> ESPNOW ===");

  loadProtoFromNvs();

  espnow_init();
  reinitRcSerial();

  // (optionnel) affiche la liste au boot
  peers_list();
}

void loop() {
  // Console série (ligne)
  {
    static char line[96];
    if (read_line(line, sizeof(line))) {
      handle_proto_digit_line(line); // "0".."4"
      handle_peer_cmd(line);         // "PEER ..."
    }
  }

  // 1) Décode SBUS entrant
  RcBusRx.process();

  // 2) Envoi à 100 Hz (10 ms)
  static uint32_t lastSend = 0;
  const uint32_t now = millis();

  if ((uint32_t)(now - lastSend) >= SEND_PERIOD_MS) {
    lastSend = now;

    uint16_t ch11[16];
    for (int i = 0; i < 16; i++) {
      uint16_t us = (uint16_t)RcBusRx.width_us((uint8_t)(i + 1));  // 1..16
      if (us < 800)  us = 1000;
      if (us > 2200) us = 2000;
      ch11[i] = us_to_sbus11(us);
    }

    bool frameLost = false;
    bool failsafe  = false;

    if (gRcProto == RC_PROTO_SBUS) {
     
      frameLost = (RcBusRx.flags(SBUS_RX_FRAME_LOST) != 0);
      failsafe  = (RcBusRx.flags(SBUS_RX_FAILSAFE) != 0);
    }

    uint8_t sbus25[25];
    sbus_pack_25(sbus25, ch11, frameLost, failsafe);

    espnow_send_sbus25(sbus25);
  }

 /*   // 3) Log 1 Hz (anti-spam)
    static uint32_t lastLog = 0;
    if ((uint32_t)(now - lastLog) >= 1000) {
      lastLog = now;

      Serial.printf("[HUB] seq=%u tx_ok=%lu tx_ko=%lu peers=%u | IN us: %u %u %u %u\n",
                    (unsigned)(uint8_t)(s_seq - 1),
                    (unsigned long)g_tx_ok,
                    (unsigned long)g_tx_ko,
                    (unsigned)peers_count_used(),
                    (unsigned)RcBusRx.width_us(1),
                    (unsigned)RcBusRx.width_us(2),
                    (unsigned)RcBusRx.width_us(3),
                    (unsigned)RcBusRx.width_us(4));
    }*/

  delay(1);
}
