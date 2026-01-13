#include <Arduino.h>
#include <stdarg.h>

// LoRaWAN (MCCI Arduino LoRaWAN library)
#define ARDUINO_LMIC_PROJECT_CONFIG_H /Users/glen/Arduino/lorawan_tank_level/lorawan_tank_level/lmic_project_config.h
#define ARDUINO_LMIC_CFG_SUBBAND 1
#include <Arduino_LoRaWAN_network.h>
#include <arduino_lmic.h>

// Modbus RTU (DFRobot RS485 shield)
#include <ModbusMaster.h>

// -----------------------------
// LoRaWAN configuration
// -----------------------------
// Fill these with your OTAA credentials (LSB for DevEUI/AppEUI).
static const uint8_t kDevEui[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t kAppEui[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t kAppKey[16] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t kAppPort = 1;
static const unsigned long kUplinkIntervalMs = 60UL * 1000UL;
static const int kStatusLedPin = LED_BUILTIN;

// TODO: confirm these pins against the Duinotech XC4392 shield wiring.
const Arduino_LMIC::HalPinmap_t lmic_pins = {
  .nss = 10,
  .rxtx = Arduino_LMIC::HalPinmap_t::LMIC_UNUSED_PIN,
  .rst = 9,
  .dio = { 3, 4, 5 },
  .rxtx_rx_active = 0,
  .rssi_cal = 0,
  .spi_freq = LMIC_SPI_FREQ,
  .pConfig = nullptr
};

// -----------------------------
// RS485 / Modbus configuration
// -----------------------------
// Adjust pins to match the DFRobot RS485 Shield wiring on the UNO Q.
static const int kRs485DeRePin = 2;  // Driver enable / receiver enable (often tied together).
static const uint32_t kRs485Baud = 9600;
static const uint8_t kModbusSlaveId = 1;
static const uint16_t kModbusRegDepthMm = 0x0000;  // TBC: update once the sensor register map is confirmed.

class cMyLoRaWAN : public Arduino_LoRaWAN_network {
public:
  cMyLoRaWAN() {}
  using Super = Arduino_LoRaWAN_network;
  void setup();

protected:
  bool GetOtaaProvisioningInfo(OtaaProvisioningInfo *pInfo) override;
  void NetSaveSessionInfo(const SessionInfo &Info, const uint8_t *pExtraInfo, size_t nExtraInfo) override;
  void NetSaveSessionState(const SessionState &State) override;
  bool NetGetSessionState(SessionState &State) override;
};

static ModbusMaster gModbus;
static cMyLoRaWAN gLoRaWAN;

static unsigned long gLastUplinkMs = 0;
static volatile bool gJoinBlinkPending = false;
static volatile bool gTxBlinkPending = false;

static void blinkStatusLed(uint8_t times, unsigned int onMs, unsigned int offMs) {
  for (uint8_t i = 0; i < times; ++i) {
    digitalWrite(kStatusLedPin, HIGH);
    delay(onMs);
    digitalWrite(kStatusLedPin, LOW);
    delay(offMs);
  }
}

static void preTransmission() {
  digitalWrite(kRs485DeRePin, HIGH);
}

static void postTransmission() {
  digitalWrite(kRs485DeRePin, LOW);
}

static void setupModbus() {
  pinMode(kRs485DeRePin, OUTPUT);
  digitalWrite(kRs485DeRePin, LOW);

  Serial1.begin(kRs485Baud);
  gModbus.begin(kModbusSlaveId, Serial1);
  gModbus.preTransmission(preTransmission);
  gModbus.postTransmission(postTransmission);
}

// -----------------------------
// LoRaWAN helpers
// -----------------------------
static void setupLoRaWAN() {
  gLoRaWAN.setup();
}

static bool readDepthMm(uint16_t *depthMm) {
  uint8_t result = gModbus.readHoldingRegisters(kModbusRegDepthMm, 1);
  if (result != gModbus.ku8MBSuccess) {
    return false;
  }
  *depthMm = gModbus.getResponseBuffer(0);
  return true;
}

static void sendUplink(uint16_t depthMm) {
  uint8_t payload[2];
  payload[0] = static_cast<uint8_t>((depthMm >> 8) & 0xFF);
  payload[1] = static_cast<uint8_t>(depthMm & 0xFF);

  if (!gLoRaWAN.GetTxReady()) {
    return;
  }

  gLoRaWAN.SendBuffer(payload, sizeof(payload), nullptr, nullptr, false, kAppPort);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // Wait for serial console in dev.
  }

  pinMode(kStatusLedPin, OUTPUT);
  digitalWrite(kStatusLedPin, LOW);

  setupModbus();
  setupLoRaWAN();
}

void loop() {
  const unsigned long now = millis();
  gLoRaWAN.loop();

  if (gJoinBlinkPending) {
    gJoinBlinkPending = false;
    blinkStatusLed(3, 75, 75);
  }

  if (gTxBlinkPending) {
    gTxBlinkPending = false;
    blinkStatusLed(2, 50, 100);
  }

  if (now - gLastUplinkMs < kUplinkIntervalMs) {
    return;
  }

  gLastUplinkMs = now;

  uint16_t depthMm = 0;
  if (readDepthMm(&depthMm)) {
    sendUplink(depthMm);
  }
}

void cMyLoRaWAN::setup() {
  this->Super::begin(&lmic_pins);
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  this->RegisterListener(
    [](void *pClientInfo, uint32_t event) -> void {
      (void)pClientInfo;
      if (event == EV_JOINED) {
        gJoinBlinkPending = true;
      } else if (event == EV_TXCOMPLETE) {
        gTxBlinkPending = true;
      }
    },
    (void *)this
  );
}

// Zephyr toolchains can omit vsnprintf; provide a weak fallback for LogPrintf().
extern "C" int vsnprintf(char *str, size_t size, const char *format, va_list ap) __attribute__((weak));
extern "C" int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  (void)format;
  (void)ap;
  if (size > 0) {
    str[0] = '\0';
  }
  return 0;
}

bool cMyLoRaWAN::GetOtaaProvisioningInfo(OtaaProvisioningInfo *pInfo) {
  if (!pInfo) {
    return true;
  }

  memcpy(pInfo->DevEUI, kDevEui, sizeof(pInfo->DevEUI));
  memcpy(pInfo->AppEUI, kAppEui, sizeof(pInfo->AppEUI));
  memcpy(pInfo->AppKey, kAppKey, sizeof(pInfo->AppKey));
  return true;
}

void cMyLoRaWAN::NetSaveSessionInfo(
  const SessionInfo &Info,
  const uint8_t *pExtraInfo,
  size_t nExtraInfo
) {
  (void)Info;
  (void)pExtraInfo;
  (void)nExtraInfo;
}

void cMyLoRaWAN::NetSaveSessionState(const SessionState &State) {
  (void)State;
}

bool cMyLoRaWAN::NetGetSessionState(SessionState &State) {
  (void)State;
  return false;
}
