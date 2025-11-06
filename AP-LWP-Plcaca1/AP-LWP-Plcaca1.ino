#include <WiFi.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>

// ======= CONFIGURAÇÕES DE REDE =======
const char* ssid = "Redmi 8 do lucas";
const char* password = "12345678";

// ======= CONFIGURAÇÕES MQTT =======
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_topic = "porta/status";

WiFiClient espClient;
PubSubClient client(espClient);

// ======= CONFIGURAÇÃO DOS SENSORES =======
#define TRIG1 18
#define ECHO1 19
#define TRIG2 23
#define ECHO2 22

Ultrasonic sensor1(TRIG1, ECHO1);
Ultrasonic sensor2(TRIG2, ECHO2);

// ======= VARIÁVEIS DE CONTROLE =======
const int LIMIAR_BASE = 85;   // cm (limite padrão)
float limiarDinamico1 = LIMIAR_BASE;
float limiarDinamico2 = LIMIAR_BASE;

bool detectando = false;
bool bloqueado = false;  // indica obstrução contínua
unsigned long tempoObstrucao = 0;
const unsigned long TEMPO_OBSTRUCAO = 1500; // 2 segundos

String ordem[2] = {"", ""};
unsigned long tempoInicial;
const unsigned long TIMEOUT = 1500;

// ======= FUNÇÕES DE CONEXÃO =======
void conectarWiFi() {
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

void conectarMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ESP32C6_Portas")) {
      Serial.println("✅ Conectado!");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 2s");
      delay(2000);
    }
  }
}

// ======= CONFIGURAÇÃO INICIAL =======
void setup() {
  Serial.begin(115200);
  conectarWiFi();
  client.setServer(mqtt_server, mqtt_port);
}

// ======= LOOP PRINCIPAL =======
void loop() {
  if (!client.connected()) conectarMQTT();
  client.loop();

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

    // Se ficar obstruído por mais de 2s → define novo limiar dinâmico
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
  } 
  else {
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
    if (ordem[0] == "sensor1" && ordem[1] == "sensor2") {
      Serial.println("➡️ Entrada detectada!");
      client.publish(mqtt_topic, "entrada");
      delay(1000);
    } else if (ordem[0] == "sensor2" && ordem[1] == "sensor1") {
      Serial.println("⬅️ Saída detectada!");
      client.publish(mqtt_topic, "saida");
      delay(1000);
    } else {
      Serial.println("⚠️ Detecção inválida ou incompleta");
    }

    detectando = false;
    ordem[0] = "";
    ordem[1] = "";
    delay(800);
  }

  delay(50);
}