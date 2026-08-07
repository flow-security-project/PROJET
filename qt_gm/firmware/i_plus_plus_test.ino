/* ============================================================================
 *  i++ v4.0 — Firmware ESP32 de TEST (capture pure + protocole MQTT)
 *  ============================================================================
 *  BUT DU PROGRAMME :
 *    - Valider chaque capteur sur le matériel réel (self-test au boot +
 *      commandes série en cours d'exécution).
 *    - Publier les trames BRUTES sur MQTT (topics spec Plan_Projet.md).
 *    - Recevoir et appliquer les commandes du dashboard Qt
 *      (config/set, led/set, lcd/set, test, evacuation/force, alerte/reset).
 *
 *  PRINCIPE D'ARCHITECTURE (Plan_Projet.md) :
 *    - ESP32 = capture UNIQUEMENT. Aucun traitement embarqué, sauf
 *      pré-traitement AUDIO minimal (RMS 1 s + FFT 256 pts) pour éviter de
 *      saturer le réseau (~256 kbit/s en brut).
 *    - Tout le processing est côté application Qt (système A-B, densité,
 *      fusion, alertes...). Le firmware N'INTERPRÈTE RIEN.
 *
 *  CÂBLAGE PAR DÉFAUT (ESP32-S3) :
 *    VL53L0X  : SDA=GPIO8,  SCL=GPIO9        (I2C, addr 0x29)
 *    SHT4x    : SDA=GPIO8,  SCL=GPIO9        (I2C, addr 0x44)
 *    LCD 16x2 : SDA=GPIO8,  SCL=GPIO9        (I2C, addr 0x27, PCF8574)
 *    HC-SR04  : TRIG=GPIO10, ECHO=GPIO11     (GPIO)
 *    INMP441  : SCK=GPIO4,  WS=GPIO5, SD=GPIO6 (I2S, L/R -> GND = canal gauche)
 *    WS2812B  : DATA=GPIO12                  (ring 12 px)
 *
 *    !!! HC-SR04 : ECHO délivre 5 V -> pont diviseur de tension
 *        (2 x 1 kOhm) vers GPIO11 3.3 V obligatoire, sinon ESP32 endommagé.
 *
 *  BIBLIOTHÈQUES REQUISES (Gestionnaire de bibliothèques Arduino) :
 *    Adafruit VL53L0X, Adafruit SHT4x, LiquidCrystal_I2C,
 *    Adafruit NeoPixel, arduinoFFT (v2.x), PubSubClient
 *
 *  TOPICS MQTT (spec Plan_Projet.md) :
 *    PUBLIE :
 *      salle/{id}/raw/tof       {"d_mm","status","t_ms"}          5 Hz
 *      salle/{id}/raw/ultrason  {"d_cm","event","t_ms"}           événementiel
 *      salle/{id}/audio         {"rms","band_cris","p99","t_ms"}  1 Hz
 *      salle/{id}/raw/env       {"t_c","hr","t_ms"}               0.5 Hz
 *      salle/{id}/led/etat      feedback LED confirmé             sur changement
 *      salle/{id}/lcd/etat      feedback LCD confirmé             sur changement
 *      salle/{id}/config/confirm config appliquée (round-trip)    sur config
 *      salle/{id}/test/result   {"composant","resultat","latence_ms"}
 *      salle/{id}/heartbeat     {"etat":"online","uptime_s"}      10 s
 *    SOUSCRIT :
 *      salle/{id}/config/set       (nom, capacité, horaires)
 *      salle/{id}/led/set          (couleur progressive pilotée par Qt)
 *      salle/{id}/lcd/set          (lignes LCD pilotées par Qt)
 *      salle/{id}/test             (test LED / LCD maintenance)
 *      salle/{id}/evacuation/force (stroboscope + LCD EVACUATION)
 *      salle/{id}/alerte/reset     (retour état normal)
 *
 *  DIAGNOSTIC SÉRIE (115200 bauds) :
 *    h = aide | t = ToF | u = ultrason | a = audio | e = env
 *    l = cycle LED | c = test LCD | m = test publication MQTT
 *
 *  NOTE : le timestamp t_ms est un temps de fonctionnement (millis()).
 *  L'heure absolue (epoch) sera fournie par le backend Qt côté serveur.
 * ============================================================================ */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#if defined(USE_TLS) && USE_TLS
#include <WiFiClientSecure.h>
#endif
#include <PubSubClient.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_SHT4x.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <arduinoFFT.h>

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#define CORE_I2S_NEW_API
#include <driver/i2s_std.h>
#else
#include <driver/i2s.h>
#endif

/* ============================================================================
 *  1. CONFIGURATION — À COMPLÉTER AVANT FLASHAGE  (##REMPLIR##)
 * ============================================================================ */
#define WIFI_SSID        "##REMPLIR##"      // nom du réseau WiFi
#define WIFI_PASS        "##REMPLIR##"      // mot de passe WiFi

#define MQTT_HOST        "10.57.197.137"    // IP du broker (machine PC actuelle)
#define MQTT_PORT        1884               // 8883 TLS / 1883-1884 sans TLS
#define MQTT_USER        ""                 // identifiant MQTT ("" si aucun)
#define MQTT_PASS        ""                 // mot de passe MQTT ("" si aucun)

#define SALLE_ID         "TEST-01"          // identifiant de ce nœud (ex: "B204")
#define SERIAL_BAUD      115200

// TLS : 1 = activé (certificat CA), 0 = désactivé (débug réseau)
#define USE_TLS          0
#if defined(USE_TLS) && USE_TLS
// Certificat CA du broker (PEM). Si la valeur reste "##REMPLIR...##",
// le firmware bascule automatiquement en setInsecure() (test uniquement).
static const char CA_CERT[] PROGMEM = R"EOF(##REMPLIR_CERTIFICAT_CA_PEM##)EOF";
#endif

/* ============================================================================
 *  2. PINS & ADRESSES — câblage par défaut (ajuster si différent)
 * ============================================================================ */
#define PIN_SDA           8
#define PIN_SCL           9
#define PIN_TRIG          10
#define PIN_ECHO          11
#define PIN_LED_RGB       12
#define PIN_I2S_SCK       4
#define PIN_I2S_WS        5
#define PIN_I2S_SD        6

#define I2C_ADDR_VL53L0X  0x29
#define I2C_ADDR_SHT4X    0x44
#define I2C_ADDR_LCD      0x27

#define NB_LED            12
#define OBJET_CM_LO       100.0f   // seuil présence (< 100 cm = objet détecté)
#define OBJET_CM_HI       130.0f   // seuil départ (> 130 cm = libre)

/* ============================================================================
 *  3. CADENCES D'ÉCHANTILLONNAGE (spec Donnee_Capteur / Plan_Projet)
 * ============================================================================ */
#define PERIOD_TOF_MS     200      // 5 Hz
#define PERIOD_ULTRASON_MS 200     // 5 Hz de mesure, publication événementielle
#define PERIOD_ENV_MS     2000     // 0.5 Hz
#define PERIOD_HEARTBEAT_MS 10000  // signe de vie
#define AUDIO_SAMPLE_RATE 16000    // Hz, INMP441
#define AUDIO_CHUNK       512      // échantillons lus par drain DMA
#define FFT_N             256      // points FFT
#define RMS_HISTORY_SIZE  120      // historique RMS (2 min à 1 Hz) pour P99

/* ============================================================================
 *  4. OBJETS GLOBAUX
 * ============================================================================ */
Adafruit_VL53L0X  vl53;
Adafruit_SHT4x    sht4;
LiquidCrystal_I2C lcd(I2C_ADDR_LCD, 16, 2);
Adafruit_NeoPixel strip(NB_LED, PIN_LED_RGB, NEO_GRB + NEO_KHZ800);

#if defined(USE_TLS) && USE_TLS
WiFiClientSecure wifiClient;
#else
WiFiClient wifiClient;
#endif
PubSubClient mqtt(wifiClient);

/* --- Configuration salle (RAM, reçue via MQTT, aucun calcul) --- */
typedef struct {
  char     nom[32];
  uint16_t capacite;
  float    hauteurPorte;
  char     horaireDebut[6];
  char     horaireFin[6];
  bool     configuree;
} SalleConfig;
SalleConfig cfg = { "TEST-01", 30, "07:00", "22:00", false };

/* --- État LED/LCD local (appliqué, jamais calculé) --- */
enum LedMode { LED_NORMAL, LED_STROBE };
LedMode   ledMode = LED_NORMAL;
uint32_t  ledCouleur = 0xFF0000;      // RGB24 (rouge par défaut pour test)
uint8_t   ledLuminosite = 80;
bool      ledStrobeOn = false;
char      lcdLigne1[17];
char      lcdLigne2[17];
bool      evacuationForcee = false;

/* --- Audio --- */
float      rmsHistory[RMS_HISTORY_SIZE];
int        rmsIdx = 0, rmsCount = 0;
float      lastRms = 0.0f;
double     sumSq = 0.0;
uint32_t   audioSamples = 0;
float      bandFrac = 0.0f;
bool       bandCris = false;
arduinoFFT fft;
arduinoFFT::Complex fftBuf[FFT_N];
int        fftIdx = 0;

#if defined(CORE_I2S_NEW_API)
i2s_chan_handle_t i2sChan = NULL;
#else
#define I2S_NUM  I2S_NUM_0
#endif

/* --- Horloges scheduler --- */
uint32_t lastToF = 0, lastUltra = 0, lastEnv = 0, lastHb = 0;
uint32_t lastStrobe = 0, lastSerial = 0;
bool     objetDetecte = false;

/* ============================================================================
 *  5. HELPERS JSON MINIMAUX (sans dépendance ArduinoJson)
 * ============================================================================ */
static const char* jsonFind(const char* json, const char* key) {
  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  const char* p = strstr(json, pattern);
  if (!p) return NULL;
  p += strlen(pattern);
  while (*p && (*p == ' ' || *p == '\t')) p++;
  if (*p != ':') return NULL;
  p++;
  while (*p && (*p == ' ' || *p == '\t')) p++;
  return p;
}

static bool jsonGetStr(const char* json, const char* key, char* out, size_t maxLen) {
  const char* p = jsonFind(json, key);
  if (!p) return false;
  if (*p == '"') {
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < maxLen - 1) out[i++] = *p++;
    out[i] = 0;
    return true;
  }
  size_t i = 0;
  while (*p && *p != ',' && *p != '}' && i < maxLen - 1) out[i++] = *p++;
  out[i] = 0;
  return true;
}

static bool jsonGetInt(const char* json, const char* key, long* out) {
  char tmp[16];
  if (!jsonGetStr(json, key, tmp, sizeof(tmp))) return false;
  *out = atol(tmp);
  return true;
}

static bool jsonGetBool(const char* json, const char* key, bool* out) {
  char tmp[8];
  if (!jsonGetStr(json, key, tmp, sizeof(tmp))) return false;
  *out = (strcmp(tmp, "true") == 0 || strcmp(tmp, "1") == 0);
  return true;
}

/* ============================================================================
 *  6. DRIVERS — LECTURES BRUTES
 * ============================================================================ */
static bool ultrasonMesure(float* cm) {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long us = pulseIn(PIN_ECHO, HIGH, 30000UL);   // timeout 30 ms (~5 m)
  if (us == 0) return false;
  *cm = us * 0.0343f / 2.0f;
  return true;
}

static bool readToF(uint16_t* d_mm, uint8_t* status) {
  VL53L0X_RangingMeasurementData_t m;
  vl53.rangingTest(&m, false);                   // false = pas de message série
  if (m.RangeStatus == 4) return false;          // 4 = "out of range"
  *d_mm   = m.RangeMilliMeter;
  *status = m.RangeStatus;
  return true;
}

static bool readEnv(float* t_c, float* hr) {
  sensors_event_t temp, hum;
  if (!sht4.getEvent(&hum, &temp)) return false;
  *t_c = temp.temperature;
  *hr  = hum.relative_humidity;
  return true;
}

/* ============================================================================
 *  7. AUDIO MINIMAL — I2S + RMS 1 s + FFT 256 pts + bande cris + P99
 * ============================================================================ */
static bool audioInit() {
#if defined(CORE_I2S_NEW_API)
  i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  if (i2s_new_channel(&chanCfg, &i2sChan, NULL) != ESP_OK) return false;

  i2s_std_config_t stdCfg = {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                    I2S_SLOT_MODE_MONO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = PIN_I2S_SCK,
      .ws   = PIN_I2S_WS,
      .dout = I2S_GPIO_UNUSED,
      .din  = PIN_I2S_SD,
      .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
    },
  };
  if (i2s_channel_init_std_mode(i2sChan, &stdCfg) != ESP_OK) return false;
  return (i2s_channel_enable(i2sChan) == ESP_OK);
#else
  i2s_config_t i2sCfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len  = 256,
  };
  if (i2s_driver_install(I2S_NUM, &i2sCfg, 0, NULL) != ESP_OK) return false;
  i2s_pin_config_t pins = {
    .bck_io_num   = PIN_I2S_SCK,
    .ws_io_num    = PIN_I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = PIN_I2S_SD,
  };
  return (i2s_set_pin(I2S_NUM, &pins) == ESP_OK);
#endif
}

static size_t audioRead(int16_t* buf, size_t maxSamples) {
#if defined(CORE_I2S_NEW_API)
  size_t n = 0;
  i2s_channel_read(i2sChan, buf, maxSamples * 2, &n, portMAX_DELAY);
  return n / 2;
#else
  size_t n = 0;
  i2s_read(I2S_NUM, buf, maxSamples * 2, &n, portMAX_DELAY);
  return n / 2;
#endif
}

static void audioPushRms(float rms01) {
  rmsHistory[rmsIdx] = rms01;
  rmsIdx = (rmsIdx + 1) % RMS_HISTORY_SIZE;
  if (rmsCount < RMS_HISTORY_SIZE) rmsCount++;
}

static float audioPercentile99() {
  if (rmsCount == 0) return 0.0f;
  float tmp[RMS_HISTORY_SIZE];
  memcpy(tmp, rmsHistory, sizeof(float) * rmsCount);
  for (int i = 1; i < rmsCount; i++) {           // tri insertion (120 el., 1 Hz : OK)
    float key = tmp[i];
    int j = i - 1;
    while (j >= 0 && tmp[j] > key) { tmp[j + 1] = tmp[j]; j--; }
    tmp[j + 1] = key;
  }
  int idx = (int)(0.99f * (float)(rmsCount - 1));
  return tmp[idx];
}

static void audioComputeFft() {
  fft.windowing(fftBuf, FFT_N, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  fft.compute(fftBuf, FFT_N, FFT_FORWARD);
  fft.complexToMagnitude(fftBuf, FFT_N);
  // Résolution : 16000/256 = 62.5 Hz/bin.
  // Bande utile 200-4000 Hz  -> bins 4..64
  // Bande cris   800-3000 Hz -> bins 13..48
  float total = 0.0f, band = 0.0f;
  for (int i = 4; i <= 64 && i < FFT_N / 2; i++) {
    float m = fftBuf[i].re;
    total += m * m;
    if (i >= 13 && i <= 48) band += m * m;
  }
  bandFrac = (total > 0.0f) ? band / total : 0.0f;
  bandCris = (bandFrac > 0.35f) && (lastRms > 0.08f);
}

static void audioDrain() {
  int16_t buf[AUDIO_CHUNK];
  size_t n = audioRead(buf, AUDIO_CHUNK);
  for (size_t i = 0; i < n; i++) {
    float s = buf[i] / 32768.0f;
    sumSq += (double)(s * s);
    audioSamples++;
    fftBuf[fftIdx].re = buf[i];
    fftBuf[fftIdx].im = 0.0f;
    fftIdx++;
    if (fftIdx >= FFT_N) { audioComputeFft(); fftIdx = 0; }
  }
  if (audioSamples >= AUDIO_SAMPLE_RATE) {      // 1 s écoulée -> publication
    float rms01 = sqrt((float)(sumSq / (double)audioSamples));
    lastRms = rms01;
    audioPushRms(rms01);
    audioSamples = 0;
    sumSq = 0.0;
    publishAudio();
  }
}

/* ============================================================================
 *  8. LED & LCD — APPLICATION DES ÉTATS + FEEDBACK CONFIRMÉ
 * ============================================================================ */
static const char* ledColorName() {
  // Approximation par composante dominante pour le feedback
  if      (ledCouleur == 0xFF0000) return "rouge";
  else if (ledCouleur == 0x00FF00) return "vert";
  else if (ledCouleur == 0xFFA500) return "orange";
  else if (ledCouleur == 0xFFFF00) return "jaune";
  else if (ledCouleur == 0x1565C0) return "bleu";
  else if (ledCouleur == 0x000000) return "eteint";
  else return "personnalise";
}

static uint32_t colorFromName(const char* nom) {
  if      (strcmp(nom, "rouge") == 0) return 0xFF0000;
  else if (strcmp(nom, "vert")  == 0) return 0x00FF00;
  else if (strcmp(nom, "orange")== 0) return 0xFFA500;
  else if (strcmp(nom, "jaune") == 0) return 0xFFFF00;
  else if (strcmp(nom, "bleu")  == 0) return 0x1565C0;
  else if (strcmp(nom, "eteint")== 0) return 0x000000;
  return 0xFF0000;
}

static void ledApply() {
  strip.setBrightness(ledLuminosite);
  if (ledMode == LED_STROBE) {
    strip.fill(ledStrobeOn ? ledCouleur : strip.Color(0, 0, 0));
  } else {
    strip.fill(ledCouleur);
  }
  strip.show();
}

static void publishLedFeedback() {
  char payload[96];
  snprintf(payload, sizeof(payload),
           "{\"couleur\":\"%s\",\"mode\":\"%s\",\"luminosite\":%u}",
           ledColorName(),
           (ledMode == LED_STROBE) ? "stroboscope" : "normal",
           ledLuminosite);
  pubTopic("led/etat", payload);
}

static void publishLcdFeedback() {
  char payload[96];
  snprintf(payload, sizeof(payload), "{\"ligne1\":\"%s\",\"ligne2\":\"%s\"}",
           lcdLigne1, lcdLigne2);
  pubTopic("lcd/etat", payload);
}

static void lcdRender() {
  char l1[17], l2[17];
  snprintf(l1, sizeof(l1), "%-16s", lcdLigne1);
  snprintf(l2, sizeof(l2), "%-16s", lcdLigne2);
  lcd.setCursor(0, 0); lcd.print(l1);
  lcd.setCursor(0, 1); lcd.print(l2);
  publishLcdFeedback();
}

static void lcdDefault() {
  snprintf(lcdLigne1, sizeof(lcdLigne1), "i++ v4.0 | %s", cfg.nom);
  snprintf(lcdLigne2, sizeof(lcdLigne2), "Cap: %u  EN LIGNE", cfg.capacite);
  lcdRender();
}

/* ============================================================================
 *  9. MQTT — CONNEXION, PUBLICATION, COMMANDES
 * ============================================================================ */
static bool pubTopic(const char* suffix, const char* payload) {
  char topic[96];
  snprintf(topic, sizeof(topic), "salle/%s/%s", SALLE_ID, suffix);
  return mqtt.publish(topic, payload);
}

static void publishToF() {
  uint16_t d; uint8_t st;
  char payload[96];
  if (readToF(&d, &st)) {
    snprintf(payload, sizeof(payload),
             "{\"d_mm\":%u,\"status\":%u,\"t_ms\":%lu}", d, st, (unsigned long)millis());
  } else {
    snprintf(payload, sizeof(payload), "{\"d_mm\":-1,\"status\":4,\"t_ms\":%lu}",
             (unsigned long)millis());
  }
  pubTopic("raw/tof", payload);
}

static void publishUltrason(bool presence) {
  float cm;
  char payload[96];
  if (ultrasonMesure(&cm)) {
    snprintf(payload, sizeof(payload),
             "{\"d_cm\":%.1f,\"event\":\"%s\",\"t_ms\":%lu}",
             cm, presence ? "presence" : "depart", (unsigned long)millis());
  } else {
    snprintf(payload, sizeof(payload),
             "{\"d_cm\":-1,\"event\":\"%s\",\"t_ms\":%lu}",
             presence ? "presence" : "depart", (unsigned long)millis());
  }
  pubTopic("raw/ultrason", payload);
}

static void publishEnv() {
  float t, h;
  char payload[96];
  if (readEnv(&t, &h)) {
    snprintf(payload, sizeof(payload),
             "{\"t_c\":%.2f,\"hr\":%.1f,\"t_ms\":%lu}", t, h, (unsigned long)millis());
  } else {
    snprintf(payload, sizeof(payload), "{\"t_c\":-99,\"hr\":-1,\"t_ms\":%lu}",
             (unsigned long)millis());
  }
  pubTopic("raw/env", payload);
}

static void publishAudio() {
  char payload[96];
  float p99 = audioPercentile99();
  snprintf(payload, sizeof(payload),
           "{\"rms\":%.3f,\"band_cris\":%s,\"p99\":%.3f,\"band_frac\":%.2f,\"t_ms\":%lu}",
           lastRms, bandCris ? "true" : "false", p99, bandFrac,
           (unsigned long)millis());
  pubTopic("audio", payload);
}

static void publishHeartbeat() {
  char payload[64];
  snprintf(payload, sizeof(payload),
           "{\"etat\":\"online\",\"uptime_s\":%lu}", (unsigned long)(millis() / 1000));
  pubTopic("heartbeat", payload);
}

static void publishConfigConfirm() {
  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"nom\":\"%s\",\"capacite\":%u,\"horaires\":{\"debut\":\"%s\",\"fin\":\"%s\"}}",
           cfg.nom, cfg.capacite, cfg.horaireDebut, cfg.horaireFin);
  pubTopic("config/confirm", payload);
}

static void publishTestResult(const char* composant, bool ok, uint32_t latenceMs) {
  char payload[96];
  snprintf(payload, sizeof(payload),
           "{\"composant\":\"%s\",\"resultat\":\"%s\",\"latence_ms\":%lu}",
           composant, ok ? "OK" : "ECHEC", (unsigned long)latenceMs);
  pubTopic("test/result", payload);
}

/* --- Commandes reçues --- */
static void cmdConfigSet(const char* json) {
  long v;
  if (jsonGetStr(json, "nom", cfg.nom, sizeof(cfg.nom))) cfg.configuree = true;
  if (jsonGetInt(json, "capacite", &v) && v > 0) cfg.capacite = (uint16_t)v;
  jsonGetStr(json, "debut",  cfg.horaireDebut, sizeof(cfg.horaireDebut));
  jsonGetStr(json, "fin",    cfg.horaireFin,   sizeof(cfg.horaireFin));
  lcdDefault();
  publishConfigConfirm();
  Serial.printf("[CMD] config appliquee : %s, cap=%u, %s-%s\n",
                cfg.nom, cfg.capacite, cfg.horaireDebut, cfg.horaireFin);
}

static void cmdLedSet(const char* json) {
  char couleur[16];
  long lum = 80;
  if (jsonGetStr(json, "couleur", couleur, sizeof(couleur)))
    ledCouleur = colorFromName(couleur);
  jsonGetInt(json, "luminosite", &lum);
  ledLuminosite = constrain(lum, 0, 255);
  ledMode = LED_NORMAL;
  ledApply();
  publishLedFeedback();
  Serial.printf("[CMD] LED -> %s (lum %u)\n", ledColorName(), ledLuminosite);
}

static void cmdLcdSet(const char* json) {
  char buf[17];
  if (jsonGetStr(json, "ligne1", buf, sizeof(buf))) {
    snprintf(lcdLigne1, sizeof(lcdLigne1), "%-16.16s", buf);
  }
  if (jsonGetStr(json, "ligne2", buf, sizeof(buf))) {
    snprintf(lcdLigne2, sizeof(lcdLigne2), "%-16.16s", buf);
  }
  lcdRender();
  Serial.printf("[CMD] LCD -> [%s]\n         [%s]\n", lcdLigne1, lcdLigne2);
}

static void cmdTest(const char* json) {
  uint32_t t0 = millis();
  char composant[16], valeur[24];
  bool ok = false;
  jsonGetStr(json, "composant", composant, sizeof(composant));
  jsonGetStr(json, "valeur",    valeur,    sizeof(valeur));

  if (strcmp(composant, "led") == 0) {
    ledCouleur = colorFromName(valeur);
    ledMode = LED_NORMAL;
    ledApply();
    publishLedFeedback();
    ok = true;
  } else if (strcmp(composant, "lcd") == 0) {
    if (jsonGetStr(json, "ligne1", lcdLigne1, sizeof(lcdLigne1)) &&
        jsonGetStr(json, "ligne2", lcdLigne2, sizeof(lcdLigne2))) {
      lcdRender();
      ok = true;
    }
  } else {
    snprintf(composant, sizeof(composant), "inconnu");
  }
  publishTestResult(composant, ok, millis() - t0);
  Serial.printf("[CMD] test %s -> %s (%lu ms)\n",
                composant, ok ? "OK" : "ECHEC", (unsigned long)(millis() - t0));
}

static void cmdEvacuationForce(const char* json) {
  bool actif;
  jsonGetBool(json, "active", &actif);
  evacuationForcee = actif;
  if (actif) {
    ledMode = LED_STROBE;
    ledCouleur = 0xFF0000;
    snprintf(lcdLigne1, sizeof(lcdLigne1), "EVACUATION ->");
    snprintf(lcdLigne2, sizeof(lcdLigne2), "SORTEZ PAR LA PORTE");
    Serial.println("[CMD] EVACUATION FORCEE ACTIVE");
  } else {
    ledMode = LED_NORMAL;
    lcdDefault();
    Serial.println("[CMD] evacuation forcee desactivee");
  }
  lcdRender();
  ledApply();
  publishLedFeedback();
}

static void cmdAlerteReset(const char* json) {
  char type[24];
  jsonGetStr(json, "type", type, sizeof(type));
  evacuationForcee = false;
  ledMode = LED_NORMAL;
  lcdDefault();
  ledApply();
  publishLedFeedback();
  Serial.printf("[CMD] alerte reset (%s)\n", type);
}

static void mqttCallback(char* topic, byte* payload, unsigned int len) {
  char json[256];
  size_t n = min(len, sizeof(json) - 1);
  memcpy(json, payload, n);
  json[n] = 0;

  Serial.printf("[MQTT] recu : %s -> %s\n", topic, json);

  if (strstr(topic, "config/set"))      cmdConfigSet(json);
  else if (strstr(topic, "led/set"))    cmdLedSet(json);
  else if (strstr(topic, "lcd/set"))    cmdLcdSet(json);
  else if (strstr(topic, "test"))       cmdTest(json);
  else if (strstr(topic, "evacuation/force")) cmdEvacuationForce(json);
  else if (strstr(topic, "alerte/reset"))     cmdAlerteReset(json);
}

static bool mqttConnect() {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (mqtt.connected()) return true;

  char clientId[32];
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(clientId, sizeof(clientId), "iplusplus-%02X%02X%02X",
           mac[3], mac[4], mac[5]);

  char willTopic[64];
  snprintf(willTopic, sizeof(willTopic), "salle/%s/heartbeat", SALLE_ID);
  mqtt.setWill(willTopic, "{\"etat\":\"offline\"}", true, 0);

  Serial.printf("[MQTT] connexion %s:%d (client %s)...\n",
                MQTT_HOST, MQTT_PORT, clientId);
  if (mqtt.connect(clientId, MQTT_USER, MQTT_PASS)) {
    char t[64];
    const char* topics[] = { "config/set", "led/set", "lcd/set",
                             "test", "evacuation/force", "alerte/reset" };
    for (unsigned int i = 0; i < sizeof(topics) / sizeof(topics[0]); i++) {
      snprintf(t, sizeof(t), "salle/%s/%s", SALLE_ID, topics[i]);
      mqtt.subscribe(t);
    }
    Serial.println("[MQTT] connecte + abonnements OK");
    publishHeartbeat();
    return true;
  }
  Serial.printf("[MQTT] echec (rc=%d)\n", mqtt.state());
  return false;
}

/* ============================================================================
 *  10. SELF-TEST AU BOOT + DIAGNOSTIC SÉRIE
 * ============================================================================ */
static void testBoot() {
  Serial.println(F("\n==============================================="));
  Serial.println(F("  i++ v4.0 — Self-test matériel"));
  Serial.println(F("==============================================="));

  Serial.println(F("\n[1] Scan bus I2C (attendu : 0x27, 0x29, 0x44) :"));
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("    - appareil I2C trouve a 0x%02X\n", addr);
    }
  }

  Serial.println(F("\n[2] VL53L0X (ToF) :"));
  if (vl53.begin()) {
    uint16_t d; uint8_t st;
    Serial.printf("    - init OK, mesure : %s\n",
                  readToF(&d, &st) ? "OK" : "ECHEC");
    if (st != 4) Serial.printf("    - distance = %u mm (status %u)\n", d, st);
  } else {
    Serial.println(F("    - ECHEC : capteur introuvable (I2C 0x29)"));
  }

  Serial.println(F("\n[3] SHT4x (T°/HR) :"));
  if (sht4.begin()) {
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
    float t, h;
    if (readEnv(&t, &h))
      Serial.printf("    - init OK : %.2f °C, %.1f %%HR\n", t, h);
    else
      Serial.println(F("    - init OK mais lecture ECHEC"));
  } else {
    Serial.println(F("    - ECHEC : capteur introuvable (I2C 0x44)"));
  }

  Serial.println(F("\n[4] HC-SR04 (ultrason) :"));
  float cm;
  if (ultrasonMesure(&cm))
    Serial.printf("    - distance = %.1f cm\n", cm);
  else
    Serial.println(F("    - ECHEC : pas d'echo (verifier TRIG/ECHO + diviseur)"));

  Serial.println(F("\n[5] INMP441 (audio I2S) :"));
  if (audioInit()) {
    int16_t buf[AUDIO_CHUNK];
    size_t n = audioRead(buf, AUDIO_CHUNK);
    double s = 0;
    for (size_t i = 0; i < n; i++) s += (double)(buf[i] / 32768.0f);
    Serial.printf("    - init OK : %u echantillons, niveau moyen %.4f\n",
                  (unsigned int)n, (float)s);
  } else {
    Serial.println(F("    - ECHEC : init I2S (verifier SCK/WS/SD + L/R->GND)"));
  }

  Serial.println(F("\n[6] WS2812B (LED ring) :"));
  strip.fill(strip.Color(255, 0, 0)); strip.show();
  delay(300);
  strip.fill(strip.Color(0, 255, 0)); strip.show();
  delay(300);
  strip.fill(strip.Color(0, 0, 255)); strip.show();
  delay(300);
  strip.fill(strip.Color(0, 0, 0));   strip.show();
  Serial.println(F("    - sequence RGB OK si vous avez vu R->V->B"));

  Serial.println(F("\n[7] LCD 16x2 :"));
  if (lcd.begin(16, 2)) {
    lcd.backlight();
    lcdDefault();
    Serial.println(F("    - 'i++ v4.0' doit etre affiche"));
  } else {
    Serial.println(F("    - ECHEC : LCD introuvable (I2C 0x27)"));
  }

  Serial.println(F("\n==============================================="));
  Serial.println(F("  Self-test terminé. Commandes serie : h = aide"));
  Serial.println(F("==============================================="));
}

static void aideSerie() {
  Serial.println(F(
    "Commandes serie :\n"
    "  h  = aide\n"
    "  t  = mesure ToF (VL53L0X)\n"
    "  u  = mesure ultrason (HC-SR04)\n"
    "  a  = analyse audio (RMS + bande cris + P99)\n"
    "  e  = environnement (SHT4x)\n"
    "  l  = cycle LED (vert -> jaune -> orange -> rouge -> eteint)\n"
    "  c  = test LCD (ecriture de test)\n"
    "  m  = test publication MQTT (1 trame par topic)\n"
    "  s  = rejouer le self-test"));
}

static void cmdSerie(char ch) {
  switch (ch) {
    case 'h': aideSerie(); break;
    case 't': {
      uint16_t d; uint8_t st;
      Serial.printf("[ToF] %s : ", readToF(&d, &st) ? "OK" : "HORS PORTEE");
      if (st != 4) Serial.printf("%u mm (status %u)\n", d, st);
      else Serial.println("-");
      break;
    }
    case 'u': {
      float cm;
      if (ultrasonMesure(&cm)) Serial.printf("[Ultrason] %.1f cm\n", cm);
      else                     Serial.println("[Ultrason] ECHEC");
      break;
    }
    case 'a': {
      lastRms = 0.5f;
      audioPushRms(0.5f);
      Serial.printf("[Audio] rms=%.3f band_frac=%.2f cris=%s p99=%.3f\n",
                    lastRms, bandFrac, bandCris ? "OUI" : "NON",
                    audioPercentile99());
      break;
    }
    case 'e': {
      float t, h;
      if (readEnv(&t, &h))
        Serial.printf("[Env] %.2f °C | %.1f %%HR\n", t, h);
      else
        Serial.println("[Env] ECHEC");
      break;
    }
    case 'l': {
      static int idx = 0;
      const char* couleurs[] = { "vert", "jaune", "orange", "rouge", "eteint" };
      ledCouleur = colorFromName(couleurs[idx]);
      ledMode = LED_NORMAL;
      ledApply();
      Serial.printf("[LED] %s\n", couleurs[idx]);
      idx = (idx + 1) % 5;
      break;
    }
    case 'c':
      snprintf(lcdLigne1, sizeof(lcdLigne1), "TEST LCD 1 -> OK");
      snprintf(lcdLigne2, sizeof(lcdLigne2), "TEST LCD 2 -> OK");
      lcdRender();
      Serial.println(F("[LCD] lignes de test affichees"));
      break;
    case 'm':
      publishToF();
      publishUltrason(objetDetecte);
      publishAudio();
      publishEnv();
      publishHeartbeat();
      Serial.printf("[MQTT] trames publiees (connecte=%s)\n",
                    mqtt.connected() ? "oui" : "NON");
      break;
    case 's': testBoot(); break;
    default: break;
  }
}

/* ============================================================================
 *  11. SETUP / LOOP — SCHEDULER NON-BLOQUANT
 * ============================================================================ */
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Wire.begin(PIN_SDA, PIN_SCL);
  strip.begin();
  strip.setBrightness(ledLuminosite);
  strip.show();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] connexion a %s ...\n", WIFI_SSID);

#if defined(USE_TLS) && USE_TLS
  if (strstr(CA_CERT, "REMPLIR") != NULL) {
    wifiClient.setInsecure();
    Serial.println(F("[TLS] CA non fourni -> mode setInsecure (TEST UNIQUEMENT)"));
  } else {
    wifiClient.setCACert(CA_CERT);
    Serial.println(F("[TLS] certificat CA charge"));
  }
#endif

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(384);

  testBoot();

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED
                 ? "\n[WiFi] connecte"
                 : "\n[WiFi] ECHEC (verifier SSID/PASS)");

  mqttConnect();
  Serial.println(F("\nPrêt. Envoi de commandes série possible."));
}

void loop() {
  uint32_t now = millis();

  /* --- Drain audio I2S en continu (échantillonnage DMA non bloquant) --- */
  audioDrain();

  /* --- VL53L0X 5 Hz --- */
  if (now - lastToF >= PERIOD_TOF_MS) {
    lastToF = now;
    publishToF();
  }

  /* --- HC-SR04 5 Hz de mesure, publication événementielle --- */
  if (now - lastUltra >= PERIOD_ULTRASON_MS) {
    lastUltra = now;
    float cm;
    if (ultrasonMesure(&cm)) {
      bool present = cm < OBJET_CM_LO;
      bool absent  = cm > OBJET_CM_HI;
      if (present && !objetDetecte) {
        objetDetecte = true;
        publishUltrason(true);
      } else if (absent && objetDetecte) {
        objetDetecte = false;
        publishUltrason(false);
      }
    }
  }

  /* --- SHT4x 0.5 Hz --- */
  if (now - lastEnv >= PERIOD_ENV_MS) {
    lastEnv = now;
    publishEnv();
  }

  /* --- Heartbeat 10 s --- */
  if (now - lastHb >= PERIOD_HEARTBEAT_MS) {
    lastHb = now;
    if (mqtt.connected()) publishHeartbeat();
  }

  /* --- Stroboscope évacuation (250 ms) --- */
  if (ledMode == LED_STROBE && now - lastStrobe >= 250) {
    lastStrobe = now;
    ledStrobeOn = !ledStrobeOn;
    ledApply();
  }

  /* --- MQTT --- */
  if (!mqtt.connected()) {
    if (now - lastHb >= 5000) mqttConnect();   // retente toutes les 5 s
  }
  mqtt.loop();

  /* --- Diagnostic série (lissé, non bloquant) --- */
  if (now - lastSerial >= 50) {
    lastSerial = now;
    if (Serial.available()) cmdSerie((char)Serial.read());
  }
}
