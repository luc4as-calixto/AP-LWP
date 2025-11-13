# LWP Soluções

**Nomes:**  
- Lucas Calixto de Souza  
- Wellyson Rudnick  
- Pedro Henrique Bartsch Da Silva  

**Turma:** TDESI 2024 1/V1  
**Data de entrega:** ??/??/????  

---

## Descrição dos Componentes Utilizados

**Hardware**
| **Componente**           | **Função**                                                                                 | **Justificativa Técnica**                                                                                            |
| ------------------------ | ------------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------------- |
| **ESP32-C6**             | Microcontrolador principal responsável pela coleta de dados e comunicação MQTT.            | Oferece conectividade Wi-Fi integrada, baixo consumo e compatibilidade com sensores, display e bibliotecas modernas. |
| **HC-SR04**              | Sensor ultrassônico utilizado para detecção de presença e cálculo da ocupação do ambiente. | Mede distâncias de forma precisa e confiável, ideal para contagem de pessoas em tempo real.                          |
| **DHT11**                | Sensor digital de temperatura e umidade.                                                   | Fácil integração via biblioteca DHT, com precisão suficiente para monitoramento ambiental simples.                   |
| **Display OLED (0.96")** | Exibe localmente informações como temperatura, umidade, ocupação e status do sistema.      | Tela compacta, baixo consumo de energia e excelente contraste visual.                                                |
| **LED RGB**              | Sinaliza o estado da sala (livre, ocupada, alerta) conforme os dados processados.          | Permite feedback visual intuitivo através de cores distintas.                                                        |
---
**Softwares e Bibliotecas**
| **Software / Biblioteca**  | **Função**                                                                                    | **Justificativa Técnica**                                                                               |
| -------------------------- | --------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| **Arduino IDE**            | Ambiente de desenvolvimento utilizado para programação e upload dos códigos nas placas ESP32. | Plataforma de fácil uso, compatível com bibliotecas IoT e hardware ESP32.                               |
| **WiFi.h**                 | Gerencia a conexão sem fio do ESP32 à rede local.                                             | Necessária para a comunicação entre os módulos via MQTT.                                                |
| **WiFiClientSecure.h**     | Garante comunicação segura (SSL/TLS) entre o ESP32 e o broker MQTT.                           | Adiciona camada de segurança, protegendo os dados transmitidos na rede contra interceptações.           |
| **PubSubClient**           | Implementa o protocolo MQTT para publicação e assinatura de tópicos.                          | Leve, estável e amplamente usada em sistemas IoT baseados em ESP.                                       |
| **Ultrasonic.h**           | Controla o sensor HC-SR04, simplificando medições de distância.                               | Facilita leituras de distância sem necessidade de cálculos manuais de tempo de eco.                     |
| **DHT.h**                  | Leitura do sensor DHT11 de temperatura e umidade.                                             | Permite integração rápida com medições confiáveis e simples.                                            |
| **Wire.h**                 | Comunicação I2C entre o ESP32 e o display OLED.                                               | Fundamental para a comunicação com dispositivos que utilizam barramento I2C.                            |
| **Adafruit GFX / SSD1306** | Controle e renderização gráfica do display OLED.                                              | Compatível com o display 0.96” e fornece recursos gráficos personalizáveis.                             |
| **ArduinoJson.h**          | Criação e leitura de mensagens em formato JSON.                                               | Facilita a estruturação dos dados enviados e recebidos via MQTT, mantendo compatibilidade com Node-RED. |
| **Node-RED**               | Orquestra a lógica do sistema e exibe o painel de controle (Dashboard).                       | Permite criação visual de fluxos e dashboards integrados com o broker MQTT.                             |
| **HiveMQ MQTT Broker**     | Gerencia as mensagens publicadas e assinadas pelos dispositivos.                              | Broker privado, seguro e de alta performance, garantindo comunicação rápida e confiável entre os nós.   |

---

## Estrutura dos Tópicos MQTT e Payloads
| **Tópico**                    | **Função**                                                       | **Exemplo de Payload (JSON)**                                                           | **QoS** |
| ----------------------------- | ---------------------------------------------------------------- | --------------------------------------------------------------------------------------- | ------- |
| `placa1/ocupacao/LWP`         | Publicação de entrada/saída de pessoas pelo NCA                  | `{ "evento": "entrada", "timestamp": 1730000123 }`                                      | 2       |
| `placa2/ambiente/LWP`         | Publicação de dados ambientais (temperatura e umidade) pela UMAF | `{ "temperatura": 24.1, "umidade": 56.2 }`                                              | 1       |
| `placa1/ocupacao/consolidado` | Estado completo da sala consolidado pelo Orquestrador            | `{ "ocupacao": 3, "limite_op": 5, "status_area": "Atenção", "sinalizacao": "amarelo" }` | 1       |
| `placa1/config/limite`        | Configuração do limite de ocupação via Dashboard                 | `{ "limite": 7 }`                                                                       | 1       |

---

## Registro de Testes
| **Teste**                       | **Ação Realizada**                   | **Resultado Esperado**                             | **Resultado Obtido** | **Status** |
| ------------------------------- | ------------------------------------ | -------------------------------------------------- | -------------------- | ---------- |
| Entrada de pessoa simulada      | Passagem diante dos sensores HC-SR04 | Incremento de +1 na ocupação                       | +1                   | OK         |
| Saída de pessoa simulada        | Passagem de saída                    | Decremento de -1 na ocupação                       | -1                   | OK         |
| Placa1/Placa2 desconectada      | Desconexão de uma das Placas         | Status “offline” exibido no dashboard via LWT      | “offline”            | OK         |
| Alteração do limite de ocupação | Mudança pelo dashboard               | Limite atualizado no tópico `placa1/config/limite` | Limite atualizado    | OK         |
| LED RGB de sinalização          | Ocupação acima do limite             | LED acende vermelho                                | LED acendeu vermelho | OK         |


