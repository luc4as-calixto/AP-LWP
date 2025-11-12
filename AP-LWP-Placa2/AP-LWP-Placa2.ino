#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// ========================
// CONFIGURAÇÕES WIFI/MQTT
// ========================
const char* SSID = "Redmi 8 do lucas";
const char* PWSD = "12345678";
const char* brokerUrl = "test.mosquitto.org";
const int port = 1883;

// ===============
// TOPICOS MQTT
// ===============
const char* TOPIC_DADOS        = "placa2/ambiente/dado";
const char* TOPIC_OCUPACAO     = "placa1/ocupacao";
const char* TOPIC_LWT          = "placa2/ambiente/LWP";

const int LwtQos = 1;
const bool LwtRetain = true;
const char* LwtMsg = "offline";

// ==================
// DHT SENSOR CONIFG
// ==================
#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// ===============
// DISPLAY CONFIG
// ===============
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define I2C_SCK 6
#define I2C_SDA 5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===============
//LED RGB (NÃO SEI SE PRECISA)
// ===============
#define LED_PIN 8
#define NUMPIXELS 1

Adafruit_NeoPixel rgb(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// =====================
// OBJETOS E VARIAVEIS
// =====================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastPublish = 0;
const int publishInterval = 5000;
int dhtErrorCount = 0;
const int maxDhtErrors = 3;

unsigned long lastDisplayUpdate = 0;
const unsigned long displayInterval = 10000; // 10 segundos
String lastStatus = "";
int lastOcupacao = -1;


// ===============
// SETUP
// ===============
void setup() {
  Serial.begin(115200);

  
  Wire.begin(I2C_SDA, I2C_SCK);
  rgb.begin();
  rgb.clear();
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) {
    Serial.println("Falha no OLED");
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Iniciando UMAF...");
  display.display();

  dht.begin();
  delay(2000);

  
  connectWifi();
  connectBroker();
}
// ===============
// LOOP
// ===============
void loop() {
  if (!mqttClient.connected()) connectBroker();
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastPublish > publishInterval) {
    lastPublish = now;
    publishSensorData();
  }
  
}
// ===============
// FUNÇÕES
// ===============
void connectWifi() {
  Serial.println("\n [WiFi] Conectando a rede...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PWSD);         // WiFi.begin(Nome da rede [WiFi], Senha da rede)

  unsigned long conexaoRede = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - conexaoRede < 15000){
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n[WiFi] Conectado");
  } else {
    Serial.println("\n[WiFi] Falha ao conectar! Reiniciando...");
    delay(3000);
    ESP.restart();
  }
}

void connectBroker() {
  Serial.println("\n[MQTT] Conectando ao broker...");
  Serial.println(brokerUrl);
   
  mqttClient.setServer(brokerUrl, port);      // setServer(broker conectado, porta de entrada)
  mqttClient.setCallback(callbackMsg);

  // ID
  String clientId = "UMAF-";
  clientId = String(random(0xffff), HEX);

  if (mqttClient.connect(
    clientId.c_str(),       // ID da placa
    "",                     // usuario (broker)
    "",                     // senha   (broker)
    TOPIC_LWT,              // Topico (LWT) ?
    LwtQos,                 // QoS (Nível 0/1/2)
    LwtRetain,              // Retenção das mensagens no broker (true/false)
    LwtMsg)){               // Mensagem LWT
        
    Serial.println("[MQTT] Conectado com sucesso!");
    mqttClient.publish(TOPIC_LWT," online",true);
    mqttClient.subscribe(TOPIC_OCUPACAO);
    } else{
      Serial.print("[MQTT] Falha. Código: ");
      Serial.println(mqttClient.state());
      delay(2000);
    }  
}

void publishSensorData() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Verifica se os valores estão dentro de faixas realistas
  if (isnan(temp) || isnan(hum) || temp < -40.0 || temp > 80.0 || hum < 0.0 || hum > 100.0) {
    dhtErrorCount++;
    Serial.println("[DHT] ERRO: Leitura inválida do sensor");
    Serial.printf("Tentativa %d de %d\n", dhtErrorCount, maxDhtErrors);
    
    // Se muitos erros consecutivos, tenta reinicializar o sensor
    if (dhtErrorCount >= maxDhtErrors) {
      Serial.println("[DHT] Muitos erros. Reiniciando sensor...");
      dhtErrorCount = 0;
      dht.begin();
      delay(1000);
    }
    return;
  }
  
  // Reset do contador de erros se a leitura for bem-sucedida
  dhtErrorCount = 0;

  Serial.printf("[DHT] Temp: %.1f°C, Hum: %.1f%%\n", temp, hum);

  // Monta JSON
  DynamicJsonDocument doc(256);
  doc["temperatura"] = temp;
  doc["umidade"] = hum;

  String payload;
  serializeJson(doc, payload);

  // Publica se conectado
  if (mqttClient.connected()) {
    bool ok = mqttClient.publish(TOPIC_DADOS, payload.c_str());
    Serial.println("[MQTT] Dado publicado: " + payload);
  } else {
    Serial.println("[MQTT] Não conectado, não publicou");
  }
}

void callbackMsg(char* topic, byte* payload, unsigned int length){
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("[MQTT] Mensagem recebida: " + msg);

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, msg);
  
  if (error) {
    Serial.println("[JSON] Erro no parse: " + String(error.f_str()));
    return;
  }

  String status = doc["texto_pessoas_na_sala"] | "Indefinido";
  int ocupacao = doc["pessoas_na_sala"] | 0;

  updateDisplay(status, ocupacao);
  setColor(status);
}

void updateDisplay(String status, int ocupacao) {
  static bool mostrarAmbiente = true;
  static unsigned long lastToggle = 0;
  const unsigned long toggleInterval = 7000;


  unsigned long now = millis();
  // Alterna entre telas
  if (now - lastToggle >= toggleInterval){
    mostrarAmbiente = !mostrarAmbiente;
    lastToggle = now;
  }
  // Atualiza o display se passou o tempo minimo OU status mudou
  if ((now - lastDisplayUpdate < displayInterval) && 
      (status == lastStatus && ocupacao == lastOcupacao)) {
    return;
  }

  lastDisplayUpdate = now;
  lastStatus = status;
  lastOcupacao = ocupacao;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  if(mostrarAmbiente) {
  // Tela ambiente
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  display.println("==============");
  display.println("==== UMAF ====");
  display.println("Temperatura:");
  display.printf("%1.f C\n", temp);
  display.println("Humidade:");
  display.printf("%1.f %%\n", hum);
  } else {
    // Tela ocupacao
    display.println("==============");
    display.println("==== UMAF ====");
    display.println("Ocupacao atual:");
    display.println(String(ocupacao) + " pessoas");
    display.println("Status:");
    display.println(status);
  }
  display.display();
}

void setColor(String status){
  if (status == "Ocupacao ok") 
    rgb.setPixelColor(0, rgb.Color(0,255,0));     // Verde
  else if (status == "Ocupacao media") 
    rgb.setPixelColor(0, rgb.Color(255,255,0)); // Amarelo
  else if (status == "Ocupacao maxima") 
    rgb.setPixelColor(0, rgb.Color(255,0,0));   // Vermelho
  else if (status == "Ocupação vazia")
    rgb.setPixelColor(0, rgb.Color(0,0,255));   // Azul 
  
  rgb.show();
}