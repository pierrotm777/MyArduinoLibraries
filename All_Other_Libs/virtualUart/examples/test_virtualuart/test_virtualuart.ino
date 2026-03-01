#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <RcBusRx.h>
#include "VirtualUart.h"

static constexpr uint8_t ESPNOW_CH = 1;

static VirtualUart vUart(2048, 0);

#pragma pack(push, 1)
struct EsnSbusPkt {
  uint16_t magic;
  uint8_t  seq;
  uint32_t ms;
  uint8_t  sbus[25];
};
#pragma pack(pop)

static constexpr uint16_t ESN_MAGIC_SB = 0x5342;

// ---------------- Helpers ----------------
static void printMac(const uint8_t* mac) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2],
                mac[3], mac[4], mac[5]);
}

// ---------------- ESPNOW RX ----------------
static void on_recv_cb(const esp_now_recv_info_t*,
                       const uint8_t* data,
                       int len)
{
  if (!data || len != sizeof(EsnSbusPkt)) return;

  const EsnSbusPkt* p = (const EsnSbusPkt*)data;
  if (p->magic != ESN_MAGIC_SB) return;

  vUart.pushRx(p->sbus, 25);
}

// ---------------- WiFi / ESPNOW ----------------
static void espnow_init() {

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  delay(100);

  esp_wifi_set_channel(ESPNOW_CH, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW FAIL");
    while(true) delay(1000);
  }

  esp_now_register_recv_cb(on_recv_cb);

  // ----- Affichage MAC + canal -----
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);

  uint8_t ch; wifi_second_chan_t sc;
  esp_wifi_get_channel(&ch, &sc);

  Serial.print("MAC STA = ");
  printMac(mac);
  Serial.printf(" | CH = %u\n", (unsigned)ch);

  Serial.println("ESP-NOW READY");
}

// ---------------- Arduino ----------------
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("=== RX SBUS ESPNOW (minimal) ===");

  espnow_init();

  RcBusRx.setProto(RC_BUS_RX_SBUS);
  RcBusRx.serialAttach(&vUart);
}

void loop() {

  RcBusRx.process();

  static uint32_t last = 0;
  uint32_t now = millis();

  if (now - last >= 200) {
    last = now;

    Serial.printf("CH1=%u  CH2=%u  CH3=%u  CH4=%u\n",
      (unsigned)RcBusRx.width_us(1),
      (unsigned)RcBusRx.width_us(2),
      (unsigned)RcBusRx.width_us(3),
      (unsigned)RcBusRx.width_us(4)
    );
  }

  delay(1);
}
