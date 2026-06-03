#include <WiFi.h>
#include <AntaresESPMQTT.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
// ================= WIFI =================

#define WIFISSID "2309106082"
#define PASSWORD "ibnuibnu2004"

// ================= ANTARES =================

#define ACCESSKEY ""
#define projectName "SmartTandon"
#define deviceName "TandonAir"

AntaresESPMQTT antares(ACCESSKEY);

// ================= TELEGRAM =================

#define BOT_TOKEN  "8048484142:AAHdxK967uwALedOFJbIJwBK56sXHMRLqoI"
#define CHAT_ID "-5288176995"

WiFiClientSecure telegramClient;

UniversalTelegramBot bot(
  BOT_TOKEN,
  telegramClient);


// ================= MQTT EMQX =================

const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;

const char* topicBuzzer =
  "iot7/tandon/buzzer";

WiFiClient mqttWifiClient;

PubSubClient mqttClient(mqttWifiClient);

// ================= STATUS =================

bool notifLowSent = false;
bool notifFullSent = false;
bool notif100Sent = false;

bool buzzerManual = true;

// TAMBAHAN
bool buzzerMuted = false;

// ================= TELEGRAM =================

void kirimTelegram(String pesan) {

  bot.sendMessage(
    CHAT_ID,
    pesan,
    "");

  Serial.println(
    "NOTIF TELEGRAM TERKIRIM");
}

// ================= MQTT CALLBACK =================

void callback(
  char* topic,
  byte* payload,
  unsigned int length) {

  String message = "";

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println("====================");
  Serial.println("MQTT DATA DITERIMA");
  Serial.println(message);

  DynamicJsonDocument doc(128);

  DeserializationError error =
    deserializeJson(doc, message);

  if (error) {

    Serial.println("JSON ERROR");

    return;
  }

  buzzerManual = doc["buzzer"];

  // jika user OFF
  if (!buzzerManual) {

    buzzerMuted = true;

  } else {

    buzzerMuted = false;
  }

  Serial.print("BUZZER MANUAL: ");
  Serial.println(buzzerManual);
}

// ================= MQTT CONNECT =================

void connectMQTT() {

  while (!mqttClient.connected()) {

    Serial.println(
      "CONNECT MQTT EMQX...");

    String clientId =
      "ESP32_" + String(random(1000, 9999));

    if (
      mqttClient.connect(
        clientId.c_str())) {

      Serial.println(
        "MQTT CONNECTED");

      mqttClient.subscribe(
        topicBuzzer);

      Serial.print(
        "SUBSCRIBE: ");

      Serial.println(
        topicBuzzer);

    } else {

      Serial.print(
        "FAILED rc=");

      Serial.println(
        mqttClient.state());

      delay(2000);
    }
  }
}

// ================= PIN =================

#define TRIG_PIN 5
#define ECHO_PIN 18

#define LED_MERAH 27
#define LED_KUNING 26
#define LED_HIJAU 25

#define BUZZER 13

// ================= TANDON =================

const float tinggiTandon = 15.0;

// ================= INTERVAL =================

const unsigned long publishIntervalMs = 2000;
unsigned long lastPublish = 0;

// ================= TIME =================

uint64_t getUnixMillis() {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return ((uint64_t)tv.tv_sec * 1000ULL) + (tv.tv_usec / 1000);
}

// Untuk tampilan jam lokal di Serial Monitor.
// sent_at_ms tetap UTC epoch millis.
String getTimeText(uint64_t unixMs) {
  time_t rawTime = (unixMs / 1000) + (8 * 3600);
  int ms = unixMs % 1000;

  struct tm timeInfo;
  gmtime_r(&rawTime, &timeInfo);

  char buffer[32];

  snprintf(
    buffer,
    sizeof(buffer),
    "%02d:%02d:%02d.%03d",
    timeInfo.tm_hour,
    timeInfo.tm_min,
    timeInfo.tm_sec,
    ms);

  return String(buffer);
}

void setupTime() {
  configTime(0, 0, "pool.ntp.org", "time.google.com");

  Serial.print("Sinkronisasi NTP");

  int retry = 0;

  while (time(nullptr) < 100000 && retry < 20) {
    Serial.print(".");
    delay(500);
    retry++;
  }

  Serial.println();

  if (time(nullptr) > 100000) {
    Serial.println("NTP READY");
  } else {
    Serial.println("NTP GAGAL");
  }
}

// ================= SENSOR =================

float bacaJarak() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long durasi = pulseIn(ECHO_PIN, HIGH, 40000);

  if (durasi == 0) {
    return -1;
  }

  float jarak = durasi * 0.0343 / 2.0;

  return jarak;
}

// ================= LOGIC =================

String getStatusAir(int waterLevel) {
  if (waterLevel <= 34) {
    return "LOW";
  } else if (waterLevel <= 70) {
    return "MEDIUM";
  } else {
    return "FULL";
  }
}

void updateOutput(
  String statusAir,
  int& ledMerah,
  int& ledKuning,
  int& ledHijau,
  int& buzzerStatus) {

  ledMerah = 0;
  ledKuning = 0;
  ledHijau = 0;
  buzzerStatus = 0;

  // ================= LED =================

  if (statusAir == "LOW") {

    ledMerah = 1;

  } else if (statusAir == "MEDIUM") {

    ledKuning = 1;

  } else if (statusAir == "FULL") {

    ledHijau = 1;
  }

  // ================= RESET MUTE =================
  // jika air turun dari FULL
  // mute otomatis direset

  if (statusAir != "FULL") {

    buzzerMuted = false;
  }

  // ================= BUZZER =================

  if (statusAir == "FULL" && !buzzerMuted) {

    buzzerStatus = 1;
  }

  digitalWrite(LED_MERAH, ledMerah);
  digitalWrite(LED_KUNING, ledKuning);
  digitalWrite(LED_HIJAU, ledHijau);

  digitalWrite(BUZZER, buzzerStatus);
}

// ================= PUBLISH ANTARES =================

void publishData() {
  float jarakAirCm = bacaJarak();

  if (jarakAirCm < 0) {
    Serial.println("Sensor gagal membaca. Data tidak dikirim.");
    return;
  }

  float ketinggianAirCm = tinggiTandon - jarakAirCm;
  ketinggianAirCm = constrain(ketinggianAirCm, 0, tinggiTandon);

  int waterLevel = round((ketinggianAirCm / tinggiTandon) * 100.0);
  waterLevel = constrain(waterLevel, 0, 100);

  String statusAir = getStatusAir(waterLevel);

  // ================= TELEGRAM =================

  // AIR HAMPIR HABIS

  if (waterLevel <= 10) {

    if (!notifLowSent) {

      kirimTelegram(
        "⚠️ AIR HAMPIR HABIS\n"
        "Level: "
        + String(waterLevel)
        + "%");

      notifLowSent = true;
    }

  } else {

    notifLowSent = false;
  }

  // FULL

  if (statusAir == "FULL") {

    if (!notifFullSent) {

      kirimTelegram(
        "✅ TANDON FULL\n"
        "Level: "
        + String(waterLevel)
        + "%");

      notifFullSent = true;
    }

  } else {

    notifFullSent = false;
  }

  // 100%

  if (waterLevel >= 100) {

    if (!notif100Sent) {

      kirimTelegram(
        "🚰 AIR SUDAH 100%");

      notif100Sent = true;
    }

  } else {

    notif100Sent = false;
  }

  int ledMerah = 0;
  int ledKuning = 0;
  int ledHijau = 0;
  int buzzerStatus = 0;

  updateOutput(
    statusAir,
    ledMerah,
    ledKuning,
    ledHijau,
    buzzerStatus);

  uint64_t sentAtMs = getUnixMillis();
  String waktuKirimData = getTimeText(sentAtMs);

  char sentAtBuffer[24];

  snprintf(
    sentAtBuffer,
    sizeof(sentAtBuffer),
    "%llu",
    (unsigned long long)sentAtMs);

  antares.checkMqttConnection();

  // ================= DATA UNTUK FLUTTER =================

  antares.add("water_level", waterLevel);
  antares.add("status_air", statusAir);

  antares.add("led_merah", ledMerah);
  antares.add("led_kuning", ledKuning);
  antares.add("led_hijau", ledHijau);

  antares.add("buzzer", buzzerStatus);

  // Dipakai Flutter untuk hitung latensi
  antares.add("sent_at_ms", sentAtBuffer);

  // Tambahan biar mudah dibaca
  antares.add("waktu_kirim_data", waktuKirimData);
  antares.add("jarak_air_cm", jarakAirCm);
  antares.add(
    "buzzer_manual",
    buzzerManual);

  antares.publish(projectName, deviceName);

  // ================= SERIAL MONITOR SIMPLE =================

  Serial.println("====================================");
  Serial.print("Water Level     : ");
  Serial.print(waterLevel);
  Serial.println(" %");

  Serial.print("Status Air      : ");
  Serial.println(statusAir);

  Serial.print("Jarak Air       : ");
  Serial.print(jarakAirCm, 2);
  Serial.println(" cm");

  Serial.print("LED Merah       : ");
  Serial.println(ledMerah);

  Serial.print("LED Kuning      : ");
  Serial.println(ledKuning);

  Serial.print("LED Hijau       : ");
  Serial.println(ledHijau);

  Serial.print("Buzzer          : ");
  Serial.println(buzzerStatus);

  Serial.print("Waktu Kirim     : ");
  Serial.println(waktuKirimData);

  Serial.print("sent_at_ms      : ");
  Serial.println(sentAtBuffer);
}

// ================= SETUP =================

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_KUNING, LOW);
  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(BUZZER, LOW);

  antares.setDebug(false);

  antares.wifiConnection(
    WIFISSID,
    PASSWORD);

  setupTime();

  antares.setMqttServer();

  // ================= TELEGRAM =================

  telegramClient.setInsecure();

  // ================= MQTT =================

  mqttClient.setServer(
    mqttServer,
    mqttPort);

  mqttClient.setCallback(
    callback);

  connectMQTT();

  Serial.println();
  Serial.println("SMART TANDON READY");
  Serial.println("Payload Antares dibuat simple.");
  Serial.println("Data: TandonModel + waktu_kirim_data + jarak_air_cm");
  Serial.println();
}

// ================= LOOP =================

void loop() {

  if (WiFi.status() != WL_CONNECTED) {

    WiFi.reconnect();
  }

  if (!mqttClient.connected()) {

    connectMQTT();
  }

  mqttClient.loop();

  unsigned long now = millis();

  if (
    now - lastPublish
    >= publishIntervalMs) {

    lastPublish = now;

    publishData();
  }

  delay(10);
}
