# LWP Soluções

**Nomes:**  
- Lucas Calixto de Souza  
- Wellyson Rudnick  
- Pedro Henrique Bartsch Da Silva  

**Turma:** TDESI 2024 1/V1  
**Data de entrega:** 27/11/2025  

---

## Descrição dos Componentes Utilizados
 *Hardwares e Softwares*

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

## Diagrama do Sistema IoT 
  **Descrição**
O diagrama apresenta a arquitetura geral do sistema IoT, dividida em quatro subsistemas que se comunicam via **MQTT** utilizando **Wi-Fi** e um **broker privado HiveMQ**.

* **Nó de Controle de Acesso (NCA):**
  Realiza medições de ocupação (ultrassônico) e ambiente (DHT11). Envia esses dados ao Orquestrador.

* **Unidade de Monitoramento Ambiental e Feedback (UMAF):**
  Recebe do Orquestrador informações sobre o estado da sala e exibe no display OLED. Também envia dados ambientais complementares.

* **Orquestrador de Processos (Node-RED – Lógica):**
  Processa todas as mensagens recebidas dos dispositivos, consolida informações e envia comandos e feedback.

* **Centro de Controle e Visualização (Node-RED – Dashboard):**
  Exibe em tempo real o estado da sala, medições e alertas para o usuário, recebendo dados do Orquestrador.

O fluxo principal segue:
**NCA → Orquestrador → UMAF & Dashboard**,
garantindo monitoramento contínuo, lógica centralizada e visualização integrada.

<img width="1536" height="1024" alt="image" src="https://github.com/user-attachments/assets/7297cd29-fc7b-4b7e-92d8-e34419b9e898" />


---
## Estrutura dos Tópicos MQTT e Payloads
| **Tópico**                    | **Função**                                                       | **Exemplo de Payload (JSON)**                                                           | **QoS** |
| ----------------------------- | ---------------------------------------------------------------- | --------------------------------------------------------------------------------------- | ------- |
| `placa1/ocupacao/LWP`         | Publicação de entrada/saída de pessoas pelo NCA                  | `{ "evento": "entrada", "timestamp": 1730000123 }`                                      | 2       |
| `placa2/ambiente/LWP`         | Publicação de dados ambientais (temperatura e umidade) pela UMAF | `{ "temperatura": 24.1, "umidade": 56.2 }`                                              | 1       |
| `placa1/ocupacao/consolidado` | Estado completo da sala consolidado pelo Orquestrador            | `{"texto_pessoas_na_sala":"Ocupacao ok","ocupacao_sala":"16/98","contador":16}"`        | 1       |
| `placa1/quantidade/slider`    | Manda a quantidade de pessoas deseja pelo usuário                | `{ "max": 100 }`                                                                        | 0       |
| `placa1/aviso`                | Manda o aviso de ocupação                                        | `{ "aviso": "OK" }`                                                                     | 0       |
| `placa1/gauge`                | Manda o contador para o gauge                                    | `{contador: 17}`                                                                        | 0       |
| `placa1/ocupacao_da_sala`     | Manda a ocupaçao da sala                                         | `{ocupacao_sala: 17/100}`                                                               | 0       |
| `placa1/situacao_sala`        | Manda a situação da sala                                         | `{texto_pessoas_na_sala: vazio}`                                                        | 0       |
| `placa1/deteccao`             | Manda o evento que ocorreu                                       | `{msg.payload.evento: livre}`                                                           | 0       |
| `placa1/status  `             | Publica o status da placa 1                                      | `{msg.payload: online}`                                                                 | 0       |
| `placa2/status  `             | Publica o status da placa 2                                      | `{msg.payload: online}`                                                                 | 0       |
 
---
## Evidências de Funcionamento

Seriel arduino Placa1:

<img width="792" height="134" alt="image" src="https://github.com/user-attachments/assets/e2d7328c-a3c0-4e4c-8a15-f7c4d8db53e4" />


Serial arduino placa2:
 
<img width="976" height="65" alt="image" src="https://github.com/user-attachments/assets/bc260674-9ff7-48b2-90f1-c920f5f47b2a" />

Dashboard:

<img width="976" height="841" alt="image" src="https://github.com/user-attachments/assets/89d4a124-1b8c-4a92-afda-42aeaee65eaf" />


## Registro de Testes
| **Teste**                       | **Ação Realizada**                   | **Resultado Esperado**                             | **Resultado Obtido** | **Status** |
| ------------------------------- | ------------------------------------ | -------------------------------------------------- | -------------------- | ---------- |
| Entrada de pessoa simulada      | Passagem diante dos sensores HC-SR04 | Incremento de +1 na ocupação                       | +1                   | OK         |
| Saída de pessoa simulada        | Passagem de saída                    | Decremento de -1 na ocupação                       | -1                   | OK         |
| Placa1/Placa2 desconectada      | Desconexão de uma das Placas         | Status “offline” exibido no dashboard via LWT      | “offline”            | OK         |
| Alteração do limite de ocupação | Mudança pelo dashboard               | Limite atualizado no tópico `placa1/config/limite` | Limite atualizado    | OK         |
| LED RGB de sinalização          | Ocupação Máxima atingida             | LED acende vermelho                                | LED acendeu vermelho | OK         |


