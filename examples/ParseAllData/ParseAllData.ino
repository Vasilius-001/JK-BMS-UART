#include <Arduino.h>
#include "JkBmsUart.h"

unsigned long startParseTime = 0; // Время начала выполнения

void setup() {
  	bmsSet.modeLog = ALL; //Выводим все логи
	bmsSet.modeErrorLimit = true;
	bmsSet.uart = Serial1;

	Serial.begin(115200); // Аппаратный UART для информационных сообщений
	bmsSet.uart.begin(115200); // Аппаратный UART для общения с BMS
	bmsSet.uart.setRxBufferSize(bmsSet.rxBufferSize); // Устанавливаем размер буфера приема для bmsSet.uart
}

void loop() {
    if (millis() - startParseTime >= 1000) { // Интервал опроса BMS, не чаще 100 мс!
		startParseTime = millis(); // Фиксируем время начала выполнения
		unsigned long startCycleTime = millis(); // Для точного измерения времени цикла
		uint32_t freeRAM = esp_get_free_heap_size();
		uint32_t totalRAM = ESP.getHeapSize();
		uint32_t usedRAM = totalRAM - freeRAM;
        
		bmsSendRecive(readAll); // Общаемся с BMS, парсим все данные
	
		Serial.print("🔹 Занято ОЗУ: "); Serial.print(usedRAM);
		Serial.print(" байт / Свободно: "); Serial.print(freeRAM);
		Serial.println(" байт");
		unsigned long timeCycle = millis() - startCycleTime; // Вычисляем время выполнения
		Serial.print("🔹 Время выполнения цикла: ");
		Serial.print(timeCycle);
		Serial.println(" мс");
    }
}