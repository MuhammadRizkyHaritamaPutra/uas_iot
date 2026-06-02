#include <WiFi.h>
#include <PubSubClient.h>
#include <AntaresESPMQTT.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <time.h>
#include <sys/time.h>

// ================= ANTARES =================

#define ACCESSKEY ""

#define WIFISSID "2309106082"
#define PASSWORD "ibnuibnu2004"

#define projectName "SmartTandon"
#define deviceName "TandonAir"

AntaresESPMQTT antares(ACCESSKEY);

// ================= TELEGRAM UNIVERSAL BOT =================
const char* BOT_TOKEN = "8048484142:AAHdxK967uwALedOFJbIJwBK56sXHMRLqoI";
const char* CHAT_ID = "-5288176995";

WiFiClientSecure telegramClient;
UniversalTelegramBot bot(BOT_TOKEN, telegramClient);

String lastTelegramAlertStatus = "";

int telegramHttpCode = 0;

unsigned long telegramLatencyMs = 0;

// ================= EMQX CONTROL =================

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

const char* topic_buzzer = "iot7/tandon/buzzer";

WiFiClient emqxWifiClient;
PubSubClient mqttControl(emqxWifiClient);

// ================= PIN =================

#define TRIG_PIN 5
#define ECHO_PIN 18

#define LED_MERAH 27
#define LED_KUNING 26
#define LED_HIJAU 25

#define BUZZER 13

// ================= TANDON =================

const int tinggiTandon = 18;

// ================= DATA SENSOR =================

float jarakRaw = 0;

// ================= PENGUJIAN =================

unsigned long sensorLatencyMs = 0;
unsigned long antaresPublishLatencyMs = 0;

int emqxReconnectCount = 0;

String wifiStatusText = "DISCONNECTED";
String emqxStatusText = "DISCONNECTED";

// ================= BUZZER CONTROL =================

bool manualOverride = false;
bool manualBuzzer = false;

String lastStatusAir = "";

// ================= TIME / NTP =================

uint64_t getUnixMillis() {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return ((uint64_t)tv.tv_sec * 1000ULL) + (tv.tv_usec / 1000);
}

void setupTime() {
  configTime(8 * 3600, 0, "pool.ntp.org", "time.google.com");

  Serial.print("Sinkronisasi waktu NTP");

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
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long durasi = pulseIn(ECHO_PIN, HIGH, 30000);

  if (durasi == 0) {
    Serial.println("Sensor gagal membaca");
    jarakRaw = 0;
    return -1;
  }

  float jarak = durasi * 0.0343 / 2.0;

  jarakRaw = jarak;

  return jarak;
}

// ================= TELEGRAM =================

void sendTelegramMessage(String message) {

  if (WiFi.status() != WL_CONNECTED) {

    telegramHttpCode = 0;
    telegramLatencyMs = 0;

    return;
  }

  unsigned long telegramStart = millis();

  bool sent =
    bot.sendMessage(String(CHAT_ID), message, "");

  telegramLatencyMs =
    millis() - telegramStart;

  telegramHttpCode = sent ? 200 : 0;
}

void checkTelegramNotification(
  int waterLevel,
  float jarak
) {

  String currentAlertStatus;

  if (waterLevel == 100) {
    currentAlertStatus = "FULL_100";
  }
  else if (waterLevel > 70) {
    currentAlertStatus = "HIGH";
  }
  else if (waterLevel < 35) {
    currentAlertStatus = "LOW";
  }
  else {
    currentAlertStatus = "NORMAL";
  }

  if (currentAlertStatus ==
      lastTelegramAlertStatus) {
    return;
  }

  lastTelegramAlertStatus =
    currentAlertStatus;

  if (currentAlertStatus == "LOW") {

    sendTelegramMessage(
      "PERINGATAN SMART TANDON\n\n"
      "Status: Air rendah\n"
      "Level air: " + String(waterLevel) + "%\n"
      "Jarak sensor: " + String(jarak, 2) + " cm"
    );
  }

  else if (currentAlertStatus == "HIGH") {

    sendTelegramMessage(
      "PERINGATAN SMART TANDON\n\n"
      "Status: Air tinggi\n"
      "Level air: " + String(waterLevel) + "%\n"
      "Jarak sensor: " + String(jarak, 2) + " cm"
    );
  }

  else if (currentAlertStatus == "FULL_100") {

    sendTelegramMessage(
      "SMART TANDON PENUH\n\n"
      "Level air: 100%"
    );
  }
}

// ================= CALLBACK MQTT =================

void callback(
  char* topic,
  byte* payload,
  unsigned int length
) {

  String message = "";

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == topic_buzzer) {

    manualOverride = true;

    if (
      message.indexOf("true") != -1 ||
      message.indexOf("1") != -1
    ) {
      manualBuzzer = true;
    }

    else if (
      message.indexOf("false") != -1 ||
      message.indexOf("0") != -1
    ) {
      manualBuzzer = false;
    }
  }
}

// ================= RECONNECT MQTT =================

void reconnectEMQX() {

  while (!mqttControl.connected()) {

    String clientId = "esp32_tandon_";
    clientId += String(random(0xffff), HEX);

    if (mqttControl.connect(clientId.c_str())) {

      emqxReconnectCount++;

      mqttControl.subscribe(topic_buzzer);

    } else {

      delay(3000);
    }
  }
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

  digitalWrite(BUZZER, LOW);

  antares.setDebug(true);

  antares.wifiConnection(
    WIFISSID,
    PASSWORD
  );

  setupTime();

  telegramClient.setInsecure();

  antares.setMqttServer();

  mqttControl.setServer(
    mqtt_server,
    mqtt_port
  );

  mqttControl.setCallback(callback);

  Serial.println("SMART TANDON READY");
}

// ================= LOOP =================

void loop() {

  antares.checkMqttConnection();

  if (!mqttControl.connected()) {
    reconnectEMQX();
  }

  mqttControl.loop();

  // ================= SENSOR =================

  unsigned long sensorStart = millis();

  float jarak = bacaJarak();

  sensorLatencyMs =
    millis() - sensorStart;

  if (jarak < 0) {
    jarak = tinggiTandon;
  }

  int waterLevel =
    round(
      ((tinggiTandon - jarak)
      / tinggiTandon) * 100.0
    );

  waterLevel =
    constrain(waterLevel, 0, 100);

  // ================= TELEGRAM =================

  checkTelegramNotification(
    waterLevel,
    jarak
  );

  String statusAir;

  int ledMerah = 0;
  int ledKuning = 0;
  int ledHijau = 0;

  int autoBuzzer = 0;

  // ================= LOGIC =================

  if (waterLevel < 35) {

    statusAir = "LOW";

    ledMerah = 1;
  }

  else if (waterLevel <= 70) {

    statusAir = "MEDIUM";

    ledKuning = 1;
  }

  else {

    statusAir = "FULL";

    ledHijau = 1;

    autoBuzzer = 1;
  }

  // ================= RESET MANUAL =================

  if (
    lastStatusAir != "FULL" &&
    statusAir == "FULL"
  ) {

    manualOverride = false;
    manualBuzzer = false;
  }

  lastStatusAir = statusAir;

  // ================= OUTPUT =================

  digitalWrite(LED_MERAH, ledMerah);
  digitalWrite(LED_KUNING, ledKuning);
  digitalWrite(LED_HIJAU, ledHijau);

  int buzzerStatus;

  if (manualOverride) {
    buzzerStatus =
      manualBuzzer ? 1 : 0;
  } else {
    buzzerStatus = autoBuzzer;
  }

  digitalWrite(BUZZER, buzzerStatus);

  // ================= STATUS =================

  wifiStatusText =
    WiFi.status() == WL_CONNECTED
    ? "CONNECTED"
    : "DISCONNECTED";

  emqxStatusText =
    mqttControl.connected()
    ? "CONNECTED"
    : "DISCONNECTED";

  // ================= TIMESTAMP =================

  uint64_t sentAtMs = getUnixMillis();

  char sentAtBuffer[24];

  snprintf(
    sentAtBuffer,
    sizeof(sentAtBuffer),
    "%llu",
    sentAtMs
  );

  // ================= ANTARES =================

  antares.add("water_level", waterLevel);
  antares.add("status_air", statusAir);

  antares.add("led_merah", ledMerah);
  antares.add("led_kuning", ledKuning);
  antares.add("led_hijau", ledHijau);

  antares.add("buzzer", buzzerStatus);

  antares.add("auto_buzzer", autoBuzzer);

  antares.add(
    "manual_override",
    manualOverride ? 1 : 0
  );

  antares.add(
    "manual_buzzer",
    manualBuzzer ? 1 : 0
  );

  antares.add("jarak_air_cm", jarak);

  // ================= SENSOR TEST =================

  antares.add("jarak_raw_cm", jarakRaw);

  antares.add(
    "sensor_latency_ms",
    (int)sensorLatencyMs
  );

  // ================= LATENCY TEST =================

  antares.add("sent_at_ms", sentAtBuffer);

  antares.add(
    "antares_publish_latency_ms",
    (int)antaresPublishLatencyMs
  );

  antares.add(
    "telegram_latency_ms",
    (int)telegramLatencyMs
  );

  // ================= CONNECTIVITY =================

  antares.add(
    "wifi_status",
    wifiStatusText
  );

  antares.add(
    "wifi_rssi",
    WiFi.RSSI()
  );

  antares.add(
    "emqx_status",
    emqxStatusText
  );

  antares.add(
    "emqx_reconnect_count",
    emqxReconnectCount
  );

  antares.add(
    "telegram_http_code",
    telegramHttpCode
  );

  // ================= PUBLISH =================

  unsigned long publishStart = millis();

  antares.publish(
    projectName,
    deviceName
  );

  antaresPublishLatencyMs =
    millis() - publishStart;

  // ================= SERIAL =================

  Serial.println("====================");

  Serial.print("Water Level: ");
  Serial.print(waterLevel);
  Serial.println("%");

  Serial.print("Status Air: ");
  Serial.println(statusAir);

  Serial.print("Sensor Latency: ");
  Serial.print(sensorLatencyMs);
  Serial.println(" ms");

  Serial.print("Antares Publish: ");
  Serial.print(antaresPublishLatencyMs);
  Serial.println(" ms");

  Serial.print("Telegram Latency: ");
  Serial.print(telegramLatencyMs);
  Serial.println(" ms");

  delay(5000);
}
