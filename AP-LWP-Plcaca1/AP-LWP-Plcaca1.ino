// placa1 para sensoriamento (ocupação);
// =========- bibliotecas -=========
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>
#include <ArduinoJson.h>

// ==========- Variaveis -==========
const int LIMIAR_BASE = 85;  // cm (limite padrão)
float limiarDinamico1 = LIMIAR_BASE;
float limiarDinamico2 = LIMIAR_BASE;

bool detectando = false;
bool bloqueado = false;  // indica obstrução contínua
unsigned long tempoObstrucao = 0;
const unsigned long TEMPO_OBSTRUCAO = 1000;  // 1 segundos

String ordem[2] = { "", "" };
unsigned long tempoInicial;
const unsigned long TIMEOUT = 1500;

// ========- Fim Variaveis -========

// ============- pinos -============
// Ultrassonico1
#define TRIG1 19
#define ECHO1 18

// Ultrassonico2
#define TRIG2 23
#define ECHO2 22


Ultrasonic sensor1(TRIG1, ECHO1);
Ultrasonic sensor2(TRIG2, ECHO2);

// ==========- Fim pinos -==========

// ======- Config WiFi e MQTT -=====
const char* ssid = "Redmi 8 do lucas";
const char* password = "12345678";

const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
// Topic: envia a deteccao
const char* mqtt_topic = "placa1/deteccao/LWP";

// =========- Config LWT -==========
// Status placa1
const char* LWTTopic = "Placa1/status";
const char* LWTMessage = "offline";
const int LWTQoS = 1;
const bool LWTRetain = true;

// =====- Topico da placa LWT -=====
const String placa1Topic = "placa1/status";

// ===========- Funções -===========

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void connectToWiFi();
void connectToBroker();

// ======= FUNÇÕES DE CONEXÃO =======
// Conexão WiFi
void connectToWiFi() {
  Serial.print("Conectando-se ao WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Conectado ao WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Conexão Broker
void connectToBroker() {
  if (WiFi.status() != WL_CONNECTED) return;

  mqttClient.setServer(mqtt_server, mqtt_port);

  // ID único da placa
  String userId = "ESP-PLACA1-";
  userId += String(random(0xFFF), HEX);

  Serial.println("\nConectando ao broker MQTT...");

  while (!mqttClient.connected()) {
    Serial.print(".");

    // Tenta conectar com LWT (Last Will and Testament)
    if (mqttClient.connect(
          userId.c_str(),
          nullptr, nullptr,
          LWTTopic,
          LWTQoS,
          LWTRetain,
          LWTMessage)) {
      Serial.println("\n✅ Conectado ao broker MQTT!");

      // Publica o status online
      mqttClient.publish(LWTTopic, "online", LWTRetain);

      // Inscreve no tópico da própria placa
      mqttClient.subscribe(mqtt_topic);

      // Define o callback
      mqttClient.setCallback(callback);
    } else {
      Serial.print("❌ Falha (rc=");
      Serial.print(mqttClient.state());
      Serial.println(") — Tentando novamente em 2s...");
      delay(2000);
    }
  }
}


// recebe resposta
void callback(char* topic, byte* payload, unsigned long length) {
  String resposta = "";
  for (unsigned int i = 0; i < length; i++) {
    resposta += (char)payload[i];
  }

  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(resposta);
}

// ======= CONFIGURAÇÃO INICIAL =======
void setup() {
  Serial.begin(115200);

  connectToWiFi();

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
}


// ======= LOOP PRINCIPAL =======
void loop() {
  if (!mqttClient.connected()) connectToBroker();
  mqttClient.loop();

  StaticJsonDocument<200> doc;
  doc["evento"] = "livre";
  String info;
  serializeJson(doc, info);
  mqttClient.publish(mqtt_topic, info.c_str());

  // leitura dos sensores
  float d1 = sensor1.read();
  float d2 = sensor2.read();

  if (d1 == 0) d1 = 400;
  if (d2 == 0) d2 = 400;

  Serial.print("Distância 1: ");
  Serial.print(d1);
  Serial.print(" cm | Distância 2: ");
  Serial.print(d2);
  Serial.print(" cm | Limiar1: ");
  Serial.print(limiarDinamico1);
  Serial.print(" | Limiar2: ");
  Serial.println(limiarDinamico2);

  // ======= DETECÇÃO DE OBSTRUÇÃO CONTÍNUA =======
  if (d1 < LIMIAR_BASE || d2 < LIMIAR_BASE) {
    if (tempoObstrucao == 0) tempoObstrucao = millis();

    // Se ficar obstruído por mais de 1s → define novo limiar dinâmico
    if (millis() - tempoObstrucao > TEMPO_OBSTRUCAO && !bloqueado) {
      bloqueado = true;
      limiarDinamico1 = (d1 < LIMIAR_BASE) ? (d1 - 10) : LIMIAR_BASE;
      limiarDinamico2 = (d2 < LIMIAR_BASE) ? (d2 - 10) : LIMIAR_BASE;

      // Impede limiar negativo
      if (limiarDinamico1 < 10) limiarDinamico1 = 10;
      if (limiarDinamico2 < 10) limiarDinamico2 = 10;

      Serial.println("🚫 Caminho bloqueado — limite dinâmico ajustado");
      Serial.print("Novo limite: ");
      Serial.print(limiarDinamico1);
      Serial.print(" / ");
      Serial.println(limiarDinamico2);
    }
  } else {
    tempoObstrucao = 0;

    // Se estava bloqueado e agora liberou → restaura padrão
    if (bloqueado && d1 > LIMIAR_BASE && d2 > LIMIAR_BASE) {
      bloqueado = false;
      limiarDinamico1 = LIMIAR_BASE;
      limiarDinamico2 = LIMIAR_BASE;
      Serial.println("✅ Caminho liberado — limiar padrão restaurado");
    }
  }

  // ===== DETECÇÃO DE PASSAGEM =====
  if ((d1 < limiarDinamico1 || d2 < limiarDinamico2) && !detectando) {
    detectando = true;
    tempoInicial = millis();
    ordem[0] = (d1 < limiarDinamico1) ? "sensor1" : "sensor2";
    Serial.println("Detecção inicial: " + ordem[0]);
  }

  if (detectando) {
    if (d1 < limiarDinamico1 && ordem[0] != "sensor1" && ordem[1] == "") {
      ordem[1] = "sensor1";
      Serial.println("Segundo sensor: sensor1");
    }
    if (d2 < limiarDinamico2 && ordem[0] != "sensor2" && ordem[1] == "") {
      ordem[1] = "sensor2";
      Serial.println("Segundo sensor: sensor2");
    }
  }

  // ===== AVALIAÇÃO FINAL =====
  if (detectando && (ordem[1] != "" || millis() - tempoInicial > TIMEOUT)) {
    StaticJsonDocument<200> doc;
    String info;

    if (ordem[0] == "sensor1" && ordem[1] == "sensor2") {
      Serial.println("➡️ Entrada detectada!");
      doc["evento"] = "entrada";
    } else if (ordem[0] == "sensor2" && ordem[1] == "sensor1") {
      Serial.println("⬅️ Saída detectada!");
      doc["evento"] = "saida";
    } else {
      Serial.println("⚠️ Detecção inválida ou incompleta");
      detectando = false;
      ordem[0] = "";
      ordem[1] = "";
      delay(800);
      return;  // sai sem enviar nada
    }

    doc["timestamp"] = millis();

    // converte JSON para string
    serializeJson(doc, info);

    // publica no MQTT
    mqttClient.publish(mqtt_topic, info.c_str());

    // debug no serial
    Serial.print("📤 JSON enviado: ");
    Serial.println(info);

    // limpa estado
    detectando = false;
    ordem[0] = "";
    ordem[1] = "";
    delay(800);
    delay(500);
  }
}