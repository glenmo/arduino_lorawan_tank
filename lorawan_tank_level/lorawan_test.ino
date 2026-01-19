/*
  Arduino UNO R4 WiFi + SX1276 (Duinotech LoRa Shield) + ChirpStack (AU915)

  - Uses RadioLib LoRaWAN stack (Class A)
  - OTAA join
  - AU915 sub-band 2 (channels 8-15 + 65) for typical 8-channel AU915 gateway configs

  Libraries:
  - RadioLib (Library Manager)

  Notes:
  - Replace JOIN_EUI / DEV_EUI / APP_KEY with values from ChirpStack Device (OTAA).
  - The ChirpStack server IP and Tenant ID are NOT used in this sketch; they’re used in ChirpStack UI/API.
*/

#include <RadioLib.h>
#include <LoRaWAN.h>

// -------------------------
// Pin mapping (edit if needed)
// -------------------------
// Common SX1276 LoRa Shield style:
// NSS/CS = D10, DIO0 = D2, DIO1 = D3, RESET = A0 (sometimes D9)
static constexpr int PIN_LORA_CS   = 10;
static constexpr int PIN_LORA_DIO0 = 2;
static constexpr int PIN_LORA_DIO1 = 3;

// Try A0 first (very common on shield layouts). If your shield uses D9, change to 9.
static constexpr int PIN_LORA_RST  = A0;

// Create the radio module instance for SX1276
SX1276 radio = new Module(PIN_LORA_CS, PIN_LORA_DIO0, PIN_LORA_RST, PIN_LORA_DIO1);

// AU915 with sub-band selection.
// Sub-band is "starting from 1", so sub-band 2 is passed as 2.
// This restricts uplinks to channels 8-15 + 65 (common 8-channel AU915 config).
LoRaWANNode node(&radio, &AU915, 2);

// -------------------------
// Device credentials (fill these from ChirpStack OTAA device)
// -------------------------
// Enter as big-endian hex (same as shown in ChirpStack UI).
// Example placeholders only - replace them.
static constexpr uint64_t JOIN_EUI = 0x0000000000000000ULL;  // 8 bytes
static constexpr uint64_t DEV_EUI  = 0x0000000000000000ULL;  // 8 bytes

// 16-byte AES key (AppKey). Replace with your key from ChirpStack.
static const uint8_t APP_KEY[16] = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// If you are using LoRaWAN 1.0.x style OTAA, set NwkKey = AppKey.
// If you are using LoRaWAN 1.1 with distinct keys, put the correct NwkKey here.
static const uint8_t NWK_KEY[16] = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Uplink settings
static constexpr uint8_t  FPORT = 1;
static constexpr uint32_t UPLINK_INTERVAL_MS = 60UL * 1000UL;

uint32_t counter = 0;

void die(const __FlashStringHelper* msg, int16_t state) {
  Serial.print(msg);
  Serial.print(F(" (state="));
  Serial.print(state);
  Serial.println(F(")"));
  while (true) {
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Serial.println(F("\nUNO R4 WiFi - SX1276 - LoRaWAN OTAA - AU915 (sub-band 2)"));

  // Initialise the radio (LoRa PHY).
  int16_t state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    die(F("Radio init failed"), state);
  }
  Serial.println(F("Radio init OK"));

  // Optional: set maximum power (your module/shield may limit this).
  // AU915 typical max is often 20 dBm on many modules; adjust if needed.
  // radio.setOutputPower(20);

  // Configure OTAA credentials.
  state = node.beginOTAA(JOIN_EUI, DEV_EUI, NWK_KEY, APP_KEY);
  if (state != RADIOLIB_ERR_NONE) {
    die(F("node.beginOTAA failed"), state);
  }

  Serial.println(F("Joining (OTAA)..."));
  state = node.activateOTAA();
  if (state != RADIOLIB_ERR_NONE) {
    die(F("OTAA join failed"), state);
  }

  Serial.print(F("Joined. DevAddr=0x"));
  Serial.println(node.getDevAddr(), HEX);
}

void loop() {
  // Build a small binary payload: 4-byte counter (big-endian)
  uint8_t payload[4];
  payload[0] = (counter >> 24) & 0xFF;
  payload[1] = (counter >> 16) & 0xFF;
  payload[2] = (counter >>  8) & 0xFF;
  payload[3] = (counter >>  0) & 0xFF;

  Serial.print(F("Uplink counter="));
  Serial.println(counter);

  // Send uplink and wait for any downlink (RX1/RX2).
  // Confirmed uplink = false by default.
  int16_t state = node.sendReceive(payload, sizeof(payload), FPORT);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("Uplink OK"));
  } else {
    Serial.print(F("Uplink failed (state="));
    Serial.print(state);
    Serial.println(F(")"));
    // If you see repeated RX2 timeouts, it is usually:
    // - wrong sub-band/channels
    // - wrong keys/EUIs
    // - gateway frequency plan mismatch
    // - downlink path blocked (less common on local ChirpStack)
  }

  counter++;
  delay(UPLINK_INTERVAL_MS);
}

