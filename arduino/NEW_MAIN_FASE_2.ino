#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h>

// --- BIBLIOTECAS ESPECÍFICAS DO ESP8266 ---
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>       // Necessário para o HTTPClient do ESP8266

// --- BIBLIOTECAS COMUNS ---
#include <ArduinoJson.h>    // Para formatar e ler JSON
#include <NTPClient.h>      // Para obter a hora da internet
#include <WiFiUdp.h>        // Dependência do NTPClient
#include <time.h>           // Para formatar a hora

// --- CONFIGURAÇÃO DA REDE ---
const char* ssid = "SETREM-1102"; 
const char* password = "";                      

const char* nomeHorta = "Horta11"; // Nome da horta (hardcoded)
String urlBase = "http://138.94.76.170:51883";

// --- OBJETOS DOS SENSORES E LCD ---
Adafruit_ADS1115 ads;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- OBJETOS DE REDE E HORA (ESP8266) ---
WiFiClient client;      // Cliente WiFi global
HTTPClient http;        // Objeto HTTP
WiFiUDP ntpUDP;
// Ajuste o fuso horário (em segundos). -3 horas * 3600 seg = -10800
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000); 

// --- PINOS DOS SENSORES DE NÍVEL ---
const int nivel50Pin = 12; 
const int nivel100Pin = 13;

// --- VARIÁVEIS GLOBAIS DOS SENSORES ---
float ph = 0.0;
float temperatura = 0.0;
float condutividade = 0.0;
bool nivel50 = false;
bool nivel100 = false;
int nivelAgua = 0; // Variável unificada para o POST (0, 50 ou 100)

// --- VARIÁVEIS DE PARÂMETROS DA API (GET) ---
float tempoBombaMinutos = 0.0;
float tempoIntervaloMinutos = 0.0;

// --- CONTROLE DA BOMBA D'ÁGUA ---
const int RELE_BOMBA_PIN = 14; // Pino D5 (GPIO 14)
bool bombaLigada = false;
unsigned long ultimoAcionamentoBomba = 0;

// --- CONTROLE DE TELAS DO LCD ---
int telaAtual = 0;
unsigned long ultimaTroca = 0;

// --- CONTROLE DE TEMPO DAS REQUISIÇÕES ---
unsigned long ultimoPost = 0;
unsigned long ultimoGet = 0;
// const long intervaloPost = 900000;  // 15 minutos (15 * 60 * 1000)
// const long intervaloGet = 1800000; // 30 minutos (30 * 60 * 1000)

// --- VALORES DE TESTE (MAIS CURTOS) ---
const long intervaloPost = 60000;  // TESTE: 1 minuto
const long intervaloGet = 120000; // TESTE: 2 minutos

// --- FUNÇÃO DE SETUP (COM CORREÇÃO DO NTP) ---
void setup(void) {
  Serial.begin(9600); 
  
  // ATENÇÃO: Pinos I2C padrão do ESP8266 (NodeMCU/Wemos)
  // D2 (GPIO 4) = SDA
  // D1 (GPIO 5) = SCL
  Wire.begin(4, 5); 

  // Inicia ADS1115
  if (!ads.begin()) {
    Serial.println("Erro ao iniciar ADS1115.");
    while (1);
  }
  ads.setGain(GAIN_ONE);

  // Inicia LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");

  // Inicia Pinos de Nível
  pinMode(nivel50Pin, INPUT_PULLUP);
  pinMode(nivel100Pin, INPUT_PULLUP);

  // Inicia Pino da Bomba
  pinMode(RELE_BOMBA_PIN, OUTPUT);
  digitalWrite(RELE_BOMBA_PIN, LOW); // Garante que a bomba comece desligada

  // Conecta ao WiFi
  setupWiFi();

  // Inicia cliente de hora
  timeClient.begin();
  
  // --- ADICIONADO: Loop de espera pela sincronização do NTP ---
  Serial.println("Sincronizando hora (NTP)...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sincronizando");
  lcd.setCursor(0, 1);
  lcd.print("Relogio...");

  // Espera até o NTP retornar um tempo válido (após 1/1/2024)
  // 1704067200 é o Epoch Time de 1 de Jan de 2024
  while (timeClient.getEpochTime() < 1704067200) {
    timeClient.update(); // Tenta atualizar
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nHora sincronizada!");
  // --- FIM DA ADIÇÃO ---

  // Faz a primeira requisição GET para buscar os parâmetros
  fazerRequisicaoGET();
  // Zera o timer do GET para a próxima atualização em 30 min
  ultimoGet = millis();
}

// --- FUNÇÃO DE LOOP PRINCIPAL ---
void loop(void) {
  unsigned long agora = millis();

  // 1. LÊ OS SENSORES
  lerSensores();

  // 2. ATUALIZA O DISPLAY LCD
  atualizarLCD(agora);

  // 3. CONTROLA A BOMBA
  controlarBomba();

  // 4. VERIFICA SE É HORA DE FAZER O POST (a cada 15 min)
  if (agora - ultimoPost > intervaloPost) {
    ultimoPost = agora;
    fazerRequisicaoPOST();
  }

  // 5. VERIFICA SE É HORA DE ATUALIZAR PARÂMETROS (a cada 30 min)
  if (agora - ultimoGet > intervaloGet) {
    ultimoGet = agora;
    Serial.println("Atualizando parametros (GET)...");
    fazerRequisicaoGET();
  }

  delay(200); // Delay original para não sobrecarregar o loop
}

// --- FUNÇÃO DE CONTROLE DA BOMBA ---
void controlarBomba() {
  // Se os tempos ainda não foram baixados da API, não faz nada.
  if (tempoIntervaloMinutos == 0 || tempoBombaMinutos == 0) {
    return;
  }

  unsigned long agora = millis();
  
  // Converte os minutos (que podem ser float, ex: 2.5) para milissegundos
  unsigned long tempoLigadaMs = (unsigned long)(tempoBombaMinutos * 60 * 1000);
  unsigned long tempoDesligadaMs = (unsigned long)(tempoIntervaloMinutos * 60 * 1000);

  // Se a bomba está LIGADA, verifica se é hora de DESLIGAR
  if (bombaLigada) {
    if (agora - ultimoAcionamentoBomba > tempoLigadaMs) {
      Serial.println("Tempo da bomba [ON] acabou. Desligando bomba.");
      digitalWrite(RELE_BOMBA_PIN, LOW); // DESLIGA a bomba
      bombaLigada = false;
      ultimoAcionamentoBomba = agora;
    }
  } 
  // Se a bomba está DESLIGADA, verifica se é hora de LIGAR
  else {
    if (agora - ultimoAcionamentoBomba > tempoDesligadaMs) {
      Serial.println("Tempo de intervalo [OFF] acabou. Ligando bomba.");
      digitalWrite(RELE_BOMBA_PIN, HIGH); // LIGA a bomba
      bombaLigada = true;
      ultimoAcionamentoBomba = agora;
    }
  }
}

// --- FUNÇÕES DE REDE ---

void setupWiFi() {
  WiFi.begin(ssid, password);
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(0, 1);
    lcd.print("Tentativa: " + String(tentativas));
    tentativas++;
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("\nWiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.print("WiFi Conectado!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println("\nFalha ao conectar.");
    lcd.clear();
    lcd.print("Falha no WiFi");
  }
}

void fazerRequisicaoGET() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sem WiFi. Pulando GET.");
    return;
  }

  String url = urlBase + "/horta/nome/" + nomeHorta;
  
  Serial.println("--------------------------");
  Serial.println("Iniciando Requisicao GET");
  Serial.println("URL: " + url);

  http.begin(client, url); 
  
  int httpCode = http.GET();
  String payload = http.getString(); // Pega a resposta, independente do código

  if (httpCode == HTTP_CODE_OK) { // 200
    Serial.println("Resposta GET (200 OK):");
    Serial.println(payload); // Resposta completa

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    tempoBombaMinutos = doc["tempoBombaMinutos"];
    tempoIntervaloMinutos = doc["tempoIntervaloMinutos"];

    Serial.printf("Parametros atualizados: Bomba=%.2f min, Intervalo=%.2f min\n", tempoBombaMinutos, tempoIntervaloMinutos);

  } else {
    Serial.printf("Erro no GET. Codigo: %d\n", httpCode);
    Serial.println("Resposta completa (erro):");
    Serial.println(payload); // Mostra a resposta mesmo se for erro
  }
  Serial.println("--------------------------");
  http.end();
}

void fazerRequisicaoPOST() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sem WiFi. Pulando POST.");
    return;
  }

  // --- 1. Atualiza e formata a data/hora ---
  timeClient.update(); 
  unsigned long epochTime = timeClient.getEpochTime();

  // --- CORREÇÃO 1: TRAVA DE SEGURANÇA ---
  // Verifica se a data é válida (epoch > 1 de Jan de 2024)
  if (epochTime < 1704067200) {
    Serial.println("--------------------------");
    Serial.println("ERRO: Hora (NTP) ainda nao sincronizada.");
    Serial.println("Pulando este POST para evitar erro 400.");
    Serial.println("--------------------------");
    return; // Aborta a função
  }
  // --- FIM DA CORREÇÃO 1 ---

  struct tm *ptm = gmtime((time_t *)&epochTime);
  
  // --- CORREÇÃO 2: FORMATO COM MICROSEGUNDOS ---
  // A. Formata a data e hora, mas sem o 'Z' no final
  char dataISO[30];
  strftime(dataISO, sizeof(dataISO), "%Y-%m-%dT%H:%M:%S", ptm);

  // B. Pega os milissegundos atuais do ESP (ex: 581)
  long milis = millis() % 1000;
  char microsStr[8]; // 1 ponto + 6 digitos + NUL
  
  // C. Formata os milis para 6 digitos (ex: ".581000")
  sprintf(microsStr, ".%03ld000", milis); 

  // D. Combina a data + microsegundos
  String dataCompleta = String(dataISO) + String(microsStr);
  // dataCompleta agora será algo como "2025-11-11T21:00:30.581000"
  // --- FIM DA CORREÇÃO 2 ---


  // --- 2. Cria o documento JSON ---
  DynamicJsonDocument doc(1024);
  doc["nivelAgua"] = nivelAgua;
  doc["temperatura"] = temperatura;
  doc["ph"] = ph;
  doc["condutividade"] = condutividade; 
  doc["dataRegistro"] = dataCompleta; // Usa a nova string de data

  String requestBody;
  serializeJson(doc, requestBody);

  // --- 3. Envia a requisição POST ---
  String url = urlBase + "/registro/nome/" + nomeHorta;

  Serial.println("--------------------------");
  Serial.println("Iniciando Requisicao POST");
  Serial.println("URL: " + url);
  Serial.println("Enviando dados (Body):");
  Serial.println(requestBody); // Verifique a data aqui
  
  http.begin(client, url); 
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(requestBody);
  String payload = http.getString(); 

  if (httpCode > 0) {
    Serial.printf("Resposta POST. Codigo: %d\n", httpCode);
    Serial.println("Resposta completa (payload):");
    Serial.println(payload);
  } else {
    Serial.printf("Falha no POST. Erro: %s\n", http.errorToString(httpCode).c_str());
  }
  Serial.println("--------------------------");
  http.end();
}

// --- FUNÇÕES DE LEITURA E DISPLAY ---

// --- FUNÇÃO LER SENSORES (COM CORREÇÃO DE CONDUTIVIDADE) ---
void lerSensores() {
  // 1. Leitura do pH
  int16_t adc0 = ads.readADC_SingleEnded(0);
  float voltage = adc0 * 0.125 / 1000.0;
  ph = 2.8 * voltage; // Cuidado: esta é uma calibração muito simples.

  // 2. Leitura dos níveis
  nivel50 = digitalRead(nivel50Pin) == LOW;
  nivel100 = digitalRead(nivel100Pin) == LOW;

  // Atualiza a variável unificada para o POST
  if (nivel100) {
    nivelAgua = 100;
  } else if (nivel50) {
    nivelAgua = 50;
  } else {
    nivelAgua = 0; // Ou qualquer valor que indique "abaixo de 50"
  }

  // 3. Leituras FAKES (substituir por sensores reais)
  temperatura = 25.0 + random(-100, 100) / 100.0;
  
  // --- MUDANÇA AQUI ---
  // Gera o valor fake (ex: 1162.0) e divide por 1000.0 para mS/cm (ex: 1.162)
  condutividade = (1000.0 + random(-200, 200)) / 1000.0; 
  // --- FIM DA MUDANÇA ---
}

void atualizarLCD(unsigned long agora) {
  // Troca de tela a cada 5 segundos
  if (agora - ultimaTroca > 5000) {
    lcd.clear();
    telaAtual = (telaAtual + 1) % 3; // 3 telas: 0, 1, 2
    ultimaTroca = agora;
  }

  // Desenha a tela atual
  switch (telaAtual) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("pH: ");
      lcd.print(ph, 2);
      lcd.setCursor(0, 1);
      lcd.print("Temp: ");
      lcd.print(temperatura, 1);
      lcd.print((char)223); // Símbolo de grau
      lcd.print("C");
      break;

    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Condutiv: ");
      // --- AJUSTE NO DISPLAY ---
      lcd.print(condutividade, 3); // Mostra 3 casas decimais (ex: 1.162)
      lcd.setCursor(0, 1);
      lcd.print("mS/cm"); // Unidade correta
      // --- FIM DO AJUSTE ---
      break;

    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Nivel:");
      lcd.setCursor(7, 0);
      lcd.print("50%:");
      lcd.print(nivel50 ? "OK " : "-- ");
      lcd.setCursor(0, 1);
      lcd.print("100%:");
      lcd.print(nivel100 ? "OK " : "-- ");
      break;
  }
}