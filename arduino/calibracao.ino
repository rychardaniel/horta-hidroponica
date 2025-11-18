#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;  // cria objeto para o ADS1115
float ph;
int16_t adc0;

void setup(void) {
  Serial.begin(9600);
  Wire.begin(4, 5); // SDA = D2, SCL = D1 (para ESP8266)

  if (!ads.begin()) {
    Serial.println("Erro ao iniciar ADS1115.");
    while (1);
  }

  // Ganho de 1: para medir entre 0 e 4.096V
  ads.setGain(GAIN_ONE); 
  delay(1000);
}

void loop(void) {
  adc0 = ads.readADC_SingleEnded(0); // Leitura do canal A0 do ADS1115

  float voltage = adc0 * 0.125 / 1000.0; // ADS1115 com GAIN_ONE = 0.125 mV por bit
  ph = 2.8 * voltage; // Exemplo de cálculo de pH (ajuste conforme sua calibração real)

  Serial.print("ADC: ");
  Serial.print(adc0);
  Serial.print(" | Tensão: ");
  Serial.print(voltage, 4);
  Serial.print(" V | pH estimado: ");
  Serial.println(ph, 2);

  delay(500);
}