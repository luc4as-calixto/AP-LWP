// placa1 para sensoriamento (ocupação, temperatura e umidade);
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==========- Variaveis -==========
unsigned long ultimoTempoDist1 = 0;
const long intervaloDist1 = 1000;
unsigned long ultimoTempoDist2 = 0;
const long intervaloDist2 = 1000;
int valordist1 = 0;
int valordist2 = 0;

// ========- Fim Variaveis -========

// ============- pinos -============

// Ultrassonico1
// MUDAR OS PINOS DE ENTRADA
const byte echo_pin1 = 1;
const byte trigg_pin1 = 2;
// Ultrassonico2
const byte echo_pin2 = 3;
const byte trigg_pin2 = 4;

// ==========- Fim pinos -==========

const byte pot_pin = 4;
const byte echo_pin = 18;
const byte trigg_pin = 9;

// ======- Config WiFi e MQTT -=====
const String SSID = "";
const String PSWD = "";
const String brokerUrl = "test.mosquitto.org";
const int port = 1883;

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
void callback(char* topic, byte* payload, unsigned long length);
int lerDistanciaUltrassonico1();
int lerDistanciaUltrassonico2();

// =============- JSON -============
JsonDocument doc;

void setup() {
  Serial.begin(115200);

  // Ultrassonico1
  pinMode(trigg_pin1, OUTPUT);
  pinMode(pot_pin1, INPUT);
  // Ultrassonico2
  pinMode(trigg_pin2, OUTPUT);
  pinMode(pot_pin2, INPUT);

  connectToWiFi();
  connectToBroker();
}

void loop() {

  // Verifica conexão WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nConexão WiFi perdida");
    connectToWiFi();
  }

  // Verifica conexão com broker
  if (!mqttClient.connected()) {
    Serial.println("\nConexão MQTT perdida");
    connectToBroker();
  }


  agora = millis();


  // Leitura ultrassonico1
  if (agora - ultimoTempoDis1t >= intervaloDist1) {
    ultimoTempoDist1 = agora;
    valordist1 = lerDistanciaUltrassonico1();
    Serial.println("");
    Serial.printf("Distância: %d cm\n", valordist1);
    

  }

  // Leitura ultrassonico2
  if (agora - ultimoTempoDis12 >= intervaloDist2) {
    ultimoTempoDis2t = agora;
    valordist2 = lerDistanciaUltrassonico2();
    Serial.println("");
    Serial.printf("Distância: %d cm\n", valordist2);
    

  }

  // Envia para o broker
  // DEPOIS APENAS COLOCAR QUE ENVIE DPS DE UM CERTO TEMPO
  if (Serial.available() > 0) {
    String info = "";

    // PARA ENVIAR TIPO JSON
    doc["nome"] = variavel;
    serializeJson(doc, info);

    message = Serial.readStringUntil('\n');
    //{ "NOME": <VALOR>, "distancia": <valor> }

    // envia mensagem
    mqttClient.publish("duplaLWP/acesso/alerta", info.c_str());
    delay(1000);
  }

  mqttClient.loop();
}

// Funções

// Conexão WiFi
void connectToWiFi() {
  Serial.println("\nIniciando conexão com rede WiFi");
  WiFi.begin(SSID, PSWD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }
  Serial.print("\nWi-Fi Conectado!");
}

// Conexão Broker
void connectToBroker() {
  // Verifica a conexão WiFi
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  Serial.println("\nConectando ao broker");
  mqttClient.setServer(brokerUrl.c_str(), port);
  // USER
  String userId = "ESP-LWP";
  userId += String(random(0xfff), HEX);
  while (!mqttClient.connected()) {
    mqttClient.connect(userId.c_str());
    Serial.print(".");
    delay(500);

    // MUDAR TOPICO
    mqttClient.subscribe("duplaLWP/acesso/alerta");
    mqttClient.setCallback(callback);
  }
  Serial.println("\nConectado com sucesso!");
}

// recebe resposta
void callback(char* topic, byte* payload, unsigned long length) {
  String resposta = "";
  for (int i = 0; i < length; i++) {
    resposta += (char)payload[i];
  }

  Serial.println(resposta);
}


// Le a dist do ultrassonico1
int lerDistanciaUltrassonico1() {
  digitalWrite(trigg_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigg_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigg_pin, LOW);

  unsigned long duracao1 = pulseIn(echo_pin, HIGH, 20000);
  if (duracao1 == 0) return -1;
  int distancia1 = duracao1 * 0.0343 / 2;
  return distancia1;
}

// Le a dist do ultrassonico2
int lerDistanciaUltrassonico2() {
  digitalWrite(trigg_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigg_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigg_pin, LOW);

  unsigned long duracao2 = pulseIn(echo_pin, HIGH, 20000);
  if (duracao2 == 0) return -1;
  int distancia2 = duracao2 * 0.0343 / 2;
  return distancia2;
}
