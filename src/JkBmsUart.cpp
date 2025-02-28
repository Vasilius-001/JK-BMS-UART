#include "JkBmsUart.h"

/* Локальные переменные */
static uint8_t cellDataLength = 0;         // Длина данных блока напряжений ячеек
static uint16_t packetLength = 0;          // Длина пакета в ответе
static uint16_t indexAnswer = 0;           // Индекс массива принятых данных
static unsigned long lastReceivedTime = 0; // Время последнего принятого байта
static uint16_t calculatedChecksum = 0;    // Контрольная сумма
static uint8_t bmsCommandSendMessage = 0;  // Отправляемая команда в сообщении
uint8_t requestRegisterBms = 0;            // ID регистра запроса для дальнейшей проверки
uint8_t stateRegisterBms = 0;              // Состояние регистра в запросе
uint16_t numberErrorConnect = 0;           // Количество ошибочных соединений
unsigned long lastErrorTime = 0;           // Время последней ошибки

/* Массивы для хранения данных */
uint8_t generalAnswer[BMS_GENERAL_ANSWER_SIZE] = {0};  // Массив принятых общих данных
uint8_t toggleAnswer[BMS_TOGGLE_ANSWER_SIZE] = {0};    // Массив принятых данных переключателя регистров
uint16_t bmsCellVoltages[MAX_CELL_COUNT] = {0};        // Массив напряжений на каждой ячейке

BMSConfig bmsSet;              // Определяем экземпляр структуры BMSConfig
BMSData bmsData;               // Определяем экземпляр структуры BMSData
StatusFlags bmsStatus;         // Определяем экземпляр структуры StatusFlags
#ifndef DISABLE_PARSE_OX8B
ProtectionFlags bmsProtection; // Определяем экземпляр структуры ProtectionFlags
#endif

BMSCommands bmsCommands = { // Определяем экземпляр структуры BMSCommands
    {0x4E, 0x57, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00},        // readAllRegisters
    {0x4E, 0x57, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x02, 0xAB, 0x01, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00},  // enableCharging
    {0x4E, 0x57, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x02, 0xAB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00},  // disableCharging
    {0x4E, 0x57, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x02, 0xAC, 0x01, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00},  // enableDischarging
    {0x4E, 0x57, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x02, 0xAC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00},  // disableDischarging
    {0x4E, 0x57, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x02, 0x9D, 0x01, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00},  // enableBalancer
    {0x4E, 0x57, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x02, 0x9D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00}   // disableBalancer
};

uint8_t *readAll = bmsCommands.readAllRegisters;       // Определяем указатель на команду readAll
uint8_t *onCharg = bmsCommands.enableCharging;         // Определяем указатель на команду onCharg
uint8_t *offCharg = bmsCommands.disableCharging;       // Определяем указатель на команду offCharg
uint8_t *onDischarg = bmsCommands.enableDischarging;   // Определяем указатель на команду onDischarg
uint8_t *offDischarg = bmsCommands.disableDischarging; // Определяем указатель на команду offDischarg
uint8_t *onBalance = bmsCommands.enableBalancer;       // Определяем указатель на команду onBalance
uint8_t *offBalance = bmsCommands.disableBalancer;     // Определяем указатель на команду offBalance



#ifndef DISABLE_PARSE_OX8C
#ifndef DISABLE_OUTPUT_LOG_OX8B
/* Массивы строковых сообщений структуры ProtectionFlags*/
const char *bmsProtectMessagesRU[] = {
    "🪫 Низкая емкость батареи",
    "🚨 Перегрев MOSFET",
    "🚨 Перенапряжение при зарядке",
    "🚨 Пониженное напряжение разряда",
    "🚨 Перегрев батареи",
    "🚨 Перегрузка по току зарядки",
    "🚨 Перегрузка по току разрядки",
    "🚨 Разница напряжений на ячейках/рядах",
    "🚨 Перегрев батарейного отсека",
    "🚨 Низкая температура батареи",
    "🚨 Перенапряжение ячейки/ряда",
    "🚨 Пониженное напряжение ячейки/ряда",
    "🚨 309_A защита",
    "🚨 309_B защита"
};
extern const char *bmsProtectMessagesRU[];

const char *bmsProtectMessagesEN[] = {
    "🪫 Low battery capacity",
    "🚨 MOSFET overheating",
    "🚨 Overvoltage during charging",
    "🚨 Low discharge voltage",
    "🚨 Battery overheating",
    "🚨 Charging current overload",
    "🚨 Discharge current overload",
    "🚨 Voltage difference between cells/rows",
    "🚨 Battery compartment overheating",
    "🚨 Low battery temperature",
    "🚨 Cell/row overvoltage",
    "🚨 Low cell/row voltage",
    "🚨 309_A protection",
    "🚨 309_B protection"
};
extern const char *bmsProtectMessagesEN[];
#endif
#endif

/* Массивы строковых сообщений структуры StatusFlags*/
const char *bmsElementMessagesRU[] = {
    "🔋 Зарядные MOSFET: ",
    "🔋 Разрядные MOSFET: ",
    "🔋 Балансир: ",
    "🔋 Батарея: "
};
extern const char *bmsElementMessagesRU[];

const char *bmsElementMessagesEN[] = {
    "🔋 Charge MOSFET: ",
    "🔋 Discharge MOSFET: ",
    "🔋 Balancer: ",
    "🔋 Battery: "
};
extern const char *bmsElementMessagesEN[];

const char *bmsStatusMessagesRU[] = {
    "Выкл",
    "Вкл"
};
extern const char *bmsStatusMessagesRU[];

const char *bmsStatusMessagesEN[] = {
    "Off",
    "On"
};
extern const char *bmsStatusMessagesEN[];


/* Предварительное объявление `static`-функций (если используется раньше в коде) */
static bms_error_t bmsParseAllData(uint8_t *data);
static bms_error_t convertTemperature(uint16_t rawTemp, int8_t &temperature);
static bms_error_t clearRxBuffer();
static bms_error_t checkAnswer(uint8_t *dataAnswer, uint16_t length);
static bms_error_t calculateChecksum16(uint8_t *data, uint16_t length);
static bms_error_t printHexAnswer(uint8_t *dataPacket, uint16_t length);
static uint16_t calculationAnswerPacketSize();
static bms_error_t resetAnswerArray(uint8_t *dataAnswer, uint16_t length);
static void printMessage(const char* messageRU, const char* messageEN);
static void printlnMessage(const char* messageRU, const char* messageEN);
static void printErrorPointer();
static void logTimeoutErrorNoConnection();
static bms_error_t logErrorPacket(uint8_t *dataAnswer, uint16_t length);


/*
* @brief Функция отправки и приема
*
* Функция отправляет запрос в BMS, принимает ответ и 
* парсит данные
*
* @param message Указатель на массив данных запроса
*
* @return bms_error_t
*/
bms_error_t bmsSendRecive(uint8_t *message) {
    if (!message) {
        printErrorPointer();
        return ERROR_POINTER;
    }

	if (bmsSet.modeErrorLimit && bmsSet.numberErrorCycles > 0) {
		if (numberErrorConnect >= bmsSet.limitErrorCycles) {
			if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
				printlnMessage("❌ Достигнут лимит ошибок, прекращаем отправку запросов", 
							   "❌ Limit of errors reached, stopping requests");
			}
			return ERROR_LIMIT_CYCLES;  // Останавливаем запросы
		}
	
		if (numberErrorConnect >= bmsSet.numberErrorCycles && 
			(millis() - lastErrorTime) < bmsSet.errorCyclePollingMs) {
			return ERROR_WAIT; // Ждем перед следующим запросом
		}
	
		lastErrorTime = millis(); // Обновляем время последнего запроса
	}

    /* Отправка команды */ 
    bms_error_t sendResult = bmsSendMessage(message);
    if (sendResult != SUCCESS) {
        return sendResult;  // Ошибка при отправке
    }

    /* Получение данных */
    bms_error_t receiveResult = bmsReceiveAnswer();

    // Обработка ошибки ERROR_CONNECT и ERROR_DATA
    if (receiveResult == ERROR_CONNECT || receiveResult == ERROR_DATA) {
        numberErrorConnect++; // Увеличиваем количество ошибок

        // Выставляем флаги состояния на ошибку
        bmsStatus.dataState = false;
        bmsStatus.connectState = false;

        // Логируем ошибку
        if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
            printMessage("❌ Ошибка при получении данных: ", "❌ Error receiving data: ");
            Serial.println(receiveResult == ERROR_CONNECT ? "ERROR_CONNECT" : "ERROR_DATA");
        }

        return receiveResult;  // Возвращаем ошибку
    }

    /* Проверка и коррекция при ошибке данных и нужной команде */
    if (receiveResult == ERROR_DATA && bmsCommandSendMessage == 0x06 && bmsSet.modeFastCorrect == true) {
        bms_error_t correctionResult = bmsCorrectionCycle();
        if (correctionResult != SUCCESS) {
            return correctionResult; // Если коррекция не удалась, возвращаем ошибку
        }
    }

    // Если ошибок нет (успешно получили данные)
    if (receiveResult == SUCCESS) {
        // Сброс флагов и счетчика ошибок при успешной операции
        bmsStatus.dataState = true;
        bmsStatus.connectState = true;
        numberErrorConnect = 0; // Сбрасываем количество ошибок

        if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
            printlnMessage("✅ Данные успешно получены!", "✅ Data received successfully!");
        }

        return SUCCESS;  // Успешно завершено
    }

    return ERROR_UNKNOWN; // Неизвестная ошибка
}

/*
* @brief Функция отправки запроса
*
* Функция отправляет запрос в BMS
*
* @param message Указатель на массив данных запроса
*
* @return bms_error_t
*/
bms_error_t bmsSendMessage(uint8_t *message) {
	if (!message) {
        printErrorPointer();
        return ERROR_POINTER;
    }
    /* Извлекаем размер сообщения из 3 и 4 байта (индексы 2 и 3) массива message */
    uint16_t messageLength = (message[2] << 8) | message[3];  // Формируем размер из двух байтов
    bmsCommandSendMessage = message[8]; // Извлекаем команду из сообщения
	requestRegisterBms = message[11]; // Сохранем ID регистра запроса для дальнейшей проверки
	if (bmsCommandSendMessage == 0x02) {
	stateRegisterBms = message[12]; // Сохраняем состояние регистра в запросе
	}
	if (bmsCommandSendMessage != 0x06 && bmsCommandSendMessage != 0x02) {
		Serial.println("");
		printMessage("❓ Неизвестная команда BMS = ", "❓ Unknown BMS command = ");
		Serial.print("0x0");
		Serial.println(bmsCommandSendMessage, HEX);
		Serial.println("");
		return ERROR_COMMAND;
	}
	/* Вставляем в запрос его номер */
	if (bmsSet.modeNum) {
        bmsData.requestNumber++;  // Увеличиваем номер запроса
    }
	message[12] = 0x00; // старший байт, всегда 0x00
    message[13] = (bmsData.requestNumber >> 16) & 0xFF;
    message[14] = (bmsData.requestNumber >> 8) & 0xFF;
    message[15] = bmsData.requestNumber & 0xFF; // младший байт

    calculateChecksum16(message, messageLength);
    bmsSet.uart.write(message, messageLength);
	/* Добавляем контрольную сумму */
    bmsSet.uart.write(calculatedChecksum >> 8);
    bmsSet.uart.write(calculatedChecksum & 0xFF);

	if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
		if (bmsCommandSendMessage == 0x06) {
			Serial.println("");
			printMessage("🚀 Отправлен запрос на чтение всех данных BMS = ", "🚀 Request sent to read all BMS data = ");
		} 
		if (bmsCommandSendMessage == 0x02) {
			Serial.println("");
			printMessage("🚀 Отправлен запрос на запись регистра BMS = ", "🚀 Request to write BMS register sent = ");
		}
		Serial.print("0x0");
		Serial.println(bmsCommandSendMessage, HEX);
		Serial.println("");
	}
	return SUCCESS;
}

/* 
* @brief Функция приема пакета данных всех от BMS
*
* Функция принимает пакет данных от BMS:
* - Принимает пакет или выход по таймауту
* - Автоматически вычисляет количество ячеек
*
* @return bms_error_t
*/
bms_error_t bmsReceiveAnswer() {
	indexAnswer = 0;
	uint16_t packetSize = 0;

	/* Проверяем корректность команды */
	if (bmsCommandSendMessage == 0x02) {
		packetSize = BMS_TOGGLE_ANSWER_SIZE;
	} else if (bmsCommandSendMessage == 0x06) {
		packetSize = bmsData.answerPacketSize;
		} else {
			Serial.println("");
			printMessage("❓ Неизвестная команда BMS = ", "❓ Unknown BMS command = ");
			Serial.print("0x0");
			Serial.println(bmsCommandSendMessage, HEX);
			Serial.println("");
			return ERROR_COMMAND;
		}

	lastReceivedTime = millis();
	/* Прием пакета в течение `bmsSet.timeoutMs` */
    while (millis() - lastReceivedTime < bmsSet.timeoutMs) {
#ifdef ESP32
		yield();  // Позволяет другим процессам выполняться (ESP, многозадачность)
#endif	
        if (bmsSet.uart.available()) {
            uint8_t incomingByte = bmsSet.uart.read();			
            if (indexAnswer < packetSize) {
				if (bmsCommandSendMessage == 0x02) {
					toggleAnswer[indexAnswer++] = incomingByte;
				}
				if (bmsCommandSendMessage == 0x06) {
					generalAnswer[indexAnswer++] = incomingByte;
				}
                lastReceivedTime = millis();  // Обновляем таймер при каждом принятом байте
            }
			/* Если приняли все данные проверяем пакет */
            if (indexAnswer == packetSize) {
				bms_error_t checkError = SUCCESS;
				if (bmsCommandSendMessage == 0x02) {
					checkError = checkAnswer(toggleAnswer, indexAnswer);
				}
				if (bmsCommandSendMessage == 0x06) {
					checkError = checkAnswer(generalAnswer, indexAnswer);
				}
				if (checkError != SUCCESS) {
					bmsData.countError++;
					if (bmsCommandSendMessage == 0x02) {
						logErrorPacket(toggleAnswer, indexAnswer);
						resetAnswerArray(toggleAnswer, BMS_TOGGLE_ANSWER_SIZE);
						return ERROR_DATA;  // Выход по ошибке пакета данных
					}
					if (bmsCommandSendMessage == 0x06) {
						logErrorPacket(generalAnswer, indexAnswer);
						if (bmsSet.modeLog == ERROR) {
							printMessage("🔋 Количество указанных ячеек: ", "🔋 Number of specified cells: ");
							Serial.println(bmsData.specifiedCellCount);
						}
						if (bmsData.specifiedCellCount != MAX_CELL_COUNT) {
							if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
								printMessage("🔹 Задаем максимальное количество ячеек = ", "🔹 Set the maximum number of cells = ");
								Serial.println(MAX_CELL_COUNT);
							}
							bmsData.specifiedCellCount = MAX_CELL_COUNT; // Задаем максимальное количество ячеек
							bmsData.answerPacketSize = calculationAnswerPacketSize(); // Рассчитываем размер пакета данных ответа
						}
						resetAnswerArray(generalAnswer, BMS_GENERAL_ANSWER_SIZE); // Сбрасываем массив приема и обнуляем его индекс
						return ERROR_DATA;  // Выход по ошибке пакета данных
					}
				}
				if (bmsCommandSendMessage == 0x02) {
					/* Проверяем ID регистра  */
					uint8_t answerRegisterBms = toggleAnswer[11]; // Сохранем ID регистра ответа для дальнейшей проверки
					if (requestRegisterBms == answerRegisterBms) {
						if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
							printlnMessage("✅ Регистр изменен успешно!", "✅ Register changed successfully!");
							uint8_t msgIndex = (answerRegisterBms == 0xAB) ? 0 : 
											(answerRegisterBms == 0xAC) ? 1 : 
											(answerRegisterBms == 0x9D) ? 2 : 255;
							if (msgIndex != 255) {
								printMessage(bmsElementMessagesRU[msgIndex], bmsElementMessagesEN[msgIndex]);
								printlnMessage(bmsStatusMessagesRU[stateRegisterBms], bmsStatusMessagesEN[stateRegisterBms]);
							}
						}
						resetAnswerArray(toggleAnswer, BMS_TOGGLE_ANSWER_SIZE); // Сбрасываем массив приема и обнуляем его индекс
						return SUCCESS; // Выход по завершению разбора принятого пакета
					}
					printlnMessage("❓ Неизвестный ID регистра !", "❓ Unknown registry ID!");
					return ERROR_DATA;
				}
				if (bmsCommandSendMessage == 0x06) {
					bmsParseAllData(generalAnswer); // Парсим принятые данные
					resetAnswerArray(generalAnswer, BMS_GENERAL_ANSWER_SIZE); // Сбрасываем массив приема и обнуляем его индекс
					return SUCCESS; // Выход по завершению разбора принятого пакета
				}
            }
        }
    }
	/* Выводим сообщение в лог */
    logTimeoutErrorNoConnection();
	if (indexAnswer == 0) {
		bmsData.countNoConnect++; // Увеличиваем счетчик отсутствия связи
		return ERROR_CONNECT;  // Выход по таймауту из-за отсутсвия связи
	}
	if (bmsCommandSendMessage == 0x06) { 
	/* Выводим сообщение в лог */
		if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
			printMessage("🔹 Ожидалось байт: ", "🔹 Expected bytes: ");
			Serial.println(bmsData.answerPacketSize);
			printMessage("🔋 Количество указанных ячеек: ", "🔋 Number of specified cells: "); Serial.println(bmsData.specifiedCellCount);
		}
		/* Если количество принятых данных не 0 выводим сообщение в лог и пробуем парсить принятый пакет */
		if (bmsParseAllData(generalAnswer) == SUCCESS) {
			bmsData.specifiedCellCount = bmsData.receivedCellCount; // Задаем количество ячеек полученных в пакете ответа
			bmsData.answerPacketSize = calculationAnswerPacketSize(); // Рассчитываем размер пакета данных ответа
			if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
				printlnMessage("❗ Пакет разобран частично!", "❗ The package is partially disassembled!");
				printMessage("🔹 Указываем количество ячеек из пакета = ", "🔹 Specify the number of cells from the package = ");
				Serial.println(bmsData.receivedCellCount);
				Serial.println("");
			}
		}
	}
	bmsData.countError++; // Увеличиваем счетчик ошибок
	if (bmsCommandSendMessage == 0x06) {
    	resetAnswerArray(generalAnswer, BMS_GENERAL_ANSWER_SIZE); // Сбрасываем массив приема и обнуляем его индекс
	} else {
		resetAnswerArray(toggleAnswer, BMS_TOGGLE_ANSWER_SIZE);
	}
    return ERROR_TIMEOUT;  // Выход по таймауту
}

/*
* @brief Функция ускорения коррекции количества ячеек
*
* Функция ускоряет коррекцию количества подключенных ячеек,
* примерно в 10 раз
*
* @return bms_error_t
*/
bms_error_t bmsCorrectionCycle() {
    if (bmsCommandSendMessage == 0x02) {
        return ERROR_COMMAND;
    }

    for (int iteration = 1; iteration <= bmsSet.correctIterations; iteration++) {
        unsigned long previousTimeCycle = millis();
        while (millis() - previousTimeCycle < 100) {
#ifdef ESP32
            yield();  // Позволяет другим процессам выполняться (ESP, многозадачность)
#endif
        }

        bmsSendMessage(readAll); // Отправляем запрос на чтение всех данных в BMS

        bms_error_t receiveStatus = bmsReceiveAnswer(); // Принимаем пакет ответа от BMS
        if (receiveStatus == SUCCESS) {
            if (bmsSet.modeLog == ALL) {
                printlnMessage("✅ Количество ячеек изменено успешно!", "✅ Number of cells changed successfully!");
                printMessage("🔹 Количество итераций: ", "🔹 Number of iterations: ");
                Serial.println(iteration);
            }
            return SUCCESS; // Выход при успешной коррекции
        }
    }

    if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
        printlnMessage("❌ Ошибка: не удалось изменить количество ячеек!", "❌ Error: Failed to change the number of cells!");
    }
    return ERROR_CORRECTION;  // Возвращаем таймаут, если коррекция не удалась
}

/*
* @brief Функция разборки данных ответа от BMS
*
* Функция принимает указатель на массив данных ответа и его длину,
* - Разбирает данные из пакета ответа;
*
* @param data Указатель на массив данных ответа
*
* @return bms_error_t
*/
static bms_error_t bmsParseAllData(uint8_t *data) {
	if (!data) {
        printErrorPointer();
        return ERROR_POINTER;
    }
	
	/* Вывод в отладочный порт полученного пакета */
	if (bmsSet.modeHex) {
		if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
		    printlnMessage("   🔍 Получены данные:", "   🔍 Data received:");
            printHexAnswer(generalAnswer, packetLength + 2);
		}
	}	
	
	/* Парсим данные напряжения ячеек регитсра 0x79 */
    uint16_t indexParse = START_BLOCK_LENGTH; // Смещение на начало блока регистра напряжений ячеек	
    if (data[indexParse] == 0x79) {
        cellDataLength = data[indexParse + 1];
        bmsData.receivedCellCount = cellDataLength / 3;		
		/* Проверка корректности указаного количества ячеек */
		if (bmsData.receivedCellCount != bmsData.specifiedCellCount) {
			if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
			    printMessage("❌ Ошибка: Ожидалось ячеек ", "❌ Error: Expected cells ");
			    Serial.print(bmsData.specifiedCellCount);
			    printMessage(", но получено ", ", but received ");
			    Serial.println(bmsData.receivedCellCount);
			}
		}		
		/* Нахождение максимального и минимального напряжения на ячейках */
		bmsData.minCellVoltage = UINT16_MAX;
        bmsData.maxCellVoltage = 0;
        for (uint8_t i = 0; i < bmsData.receivedCellCount; i++) {
            uint8_t cellIndex = data[indexParse + 2 + (i * 3)];			
			if (cellIndex == 0 || cellIndex > bmsData.specifiedCellCount) {
				if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
			        printlnMessage("❌ Ошибка: Неверный индекс ячейки!", "❌ Error: Invalid cell index!");
				}
			    return ERROR_DATA;
		    }
            uint16_t cellVoltage = (data[indexParse + 3 + (i * 3)] << 8) | data[indexParse + 4 + (i * 3)];
            bmsCellVoltages[cellIndex - 1] = cellVoltage;			
			if (bmsCellVoltages[cellIndex - 1] < bmsData.minCellVoltage) {
                bmsData.minCellVoltage = bmsCellVoltages[cellIndex - 1];
            }
            if (bmsCellVoltages[cellIndex - 1] > bmsData.maxCellVoltage) {
                bmsData.maxCellVoltage = bmsCellVoltages[cellIndex - 1];
            }
        }
		/* Вывод в порт отладочной информации */
		if (bmsSet.modeLog == ALL) {
		    printMessage("🔋 Количество ячеек: ", "🔋 Number of cells: "); Serial.println(bmsData.receivedCellCount);
			printMessage("🔋 Максимальное напряжение ячейки: ", "🔋 Maximum cell voltage: ");
            Serial.printf("%0.3f", (float)bmsData.maxCellVoltage / 1000); printlnMessage(" В", " V");
			printMessage("🔋 Минимальное напряжение ячейки: ", "🔋 Minimum cell voltage: ");
            Serial.printf("%0.3f", (float)bmsData.minCellVoltage / 1000); printlnMessage(" В", " V");
		}
    }
		
	/* Парсинг регистров 0x80, 0x81, 0x82, 0x83, 0x84, 0x85 */
	indexParse += cellDataLength + 2; // Смещение на начало блока регистров
	if (data[indexParse] == 0x80) {
        bms_error_t tempError1 = convertTemperature((data[indexParse + 1] << 8) | data[indexParse + 2], bmsData.mosfetTemp);
		bms_error_t tempError2 = convertTemperature((data[indexParse + 4] << 8) | data[indexParse + 5], bmsData.batteryTemp1);
		bms_error_t tempError3 = convertTemperature((data[indexParse + 7] << 8) | data[indexParse + 8], bmsData.batteryTemp2);
        bmsData.batteryVoltage = (data[indexParse + 10] << 8) | data[indexParse + 11];
        bmsData.batteryCurrent = (data[indexParse + 13] << 8) | data[indexParse + 14];
        bmsData.batterySOC = data[indexParse + 16];
		/* Вывод в порт отладочной информации */
		if (bmsSet.modeLog == ALL) {
		    printlnMessage("   🔍 Расшифрованные данные:", "   🔍 Decrypted data:");
			if (tempError1 == ERROR_DATA) {
				printMessage("❌ Ошибка данных температуры MOSFET = ", "❌ MOSFET temperature data error = ");
				Serial.println(bmsData.mosfetTemp);
			} else {
				printMessage("🌡 Температура MOSFET: ", "🌡 MOSFET temperature: ");
				Serial.print(bmsData.mosfetTemp); printlnMessage(" °C", " °C");
			}
			if (tempError2 == ERROR_DATA) {
				printMessage("❌ Ошибка данных температуры датчика 1 = ", "❌ Sensor 1 Temperature Data Error = ");
				Serial.println(bmsData.batteryTemp1);
			} else {
				printMessage("🌡 Температура датчика 1: ", "🌡 Sensor 1 temperature: ");
				Serial.print(bmsData.batteryTemp1); printlnMessage(" °C", " °C");
			}
			if (tempError3 == ERROR_DATA) {
				printMessage("❌ Ошибка данных температуры датчика 2 = ", "❌ Sensor 2 Temperature Data Error = ");
				Serial.println(bmsData.batteryTemp2);
			} else {
				printMessage("🌡 Температура датчика 2: ", "🌡 Sensor 2 temperature: ");
				Serial.print(bmsData.batteryTemp2); printlnMessage(" °C", " °C");
			}	
            printMessage("🔋 Напряжение батареи: ", "🔋 Battery voltage: ");
			Serial.print((float)bmsData.batteryVoltage / 100); printlnMessage(" В", " V");
            printMessage("⚡ Ток батареи: ", "⚡ Battery Current: ");
			Serial.print((float)bmsData.batteryCurrent / 100); printlnMessage(" А", " A");
			printMessage("⚡ Мощность: ", "⚡ Power: ");
			Serial.print((float)bmsData.batteryVoltage * (float)bmsData.batteryCurrent / 10000); printlnMessage(" Вт", " Wt");
            printMessage("🔋 SOC: ", "🔋 SOC: ");
			Serial.print(bmsData.batterySOC); printlnMessage(" %", " %");
		    printlnMessage("   🔍 Статус батареи: ", "   🔍 Battery status: ");
		    if (bmsData.batteryCurrent > 10) {
                printlnMessage("🔋 Зарядка", "🔋 Charging");
		    } else if (bmsData.batteryCurrent < -10) {
                printlnMessage("⚡ Разрядка", "⚡ Discharge");
			    } else {
				    printlnMessage("⏸ Ожидание", "⏸ Waiting");
			        }
		}
	}	
	
	/* Парсинг регистра 0x8B */
	if (data[indexParse + 30] == 0x8B) {
#ifndef DISABLE_PARSE_OX8B
		bmsProtection.protectionFlagsDetailed = (data[indexParse + 31] << 8) | data[indexParse + 32];
		bmsProtection.lowCapacity = bmsProtection.protectionFlagsDetailed & (1 << 0);
		bmsProtection.mosfetOverheat = bmsProtection.protectionFlagsDetailed & (1 << 1);
		bmsProtection.chargeOvervoltage = bmsProtection.protectionFlagsDetailed & (1 << 2);
		bmsProtection.dischargeUndervoltage = bmsProtection.protectionFlagsDetailed & (1 << 3);
		bmsProtection.batteryOverheat = bmsProtection.protectionFlagsDetailed & (1 << 4);
		bmsProtection.chargeOvercurrent = bmsProtection.protectionFlagsDetailed & (1 << 5);
		bmsProtection.dischargeOvercurrent = bmsProtection.protectionFlagsDetailed & (1 << 6);
		bmsProtection.cellVoltageImbalance = bmsProtection.protectionFlagsDetailed & (1 << 7);
		bmsProtection.batteryCompartmentOverheat = bmsProtection.protectionFlagsDetailed & (1 << 8);
		bmsProtection.lowBatteryTemperature = bmsProtection.protectionFlagsDetailed & (1 << 9);
		bmsProtection.cellOvervoltage = bmsProtection.protectionFlagsDetailed & (1 << 10);
		bmsProtection.cellUndervoltage = bmsProtection.protectionFlagsDetailed & (1 << 11);
		bmsProtection.protection309A = bmsProtection.protectionFlagsDetailed & (1 << 12);
		bmsProtection.protection309B = bmsProtection.protectionFlagsDetailed & (1 << 13);
#ifndef DISABLE_OUTPUT_LOG_OX8B
		/* Вывод в порт отладочной информации */
		if (bmsSet.modeLog == ALL) {
            printlnMessage("   🔍 Флаги защиты BMS:", "   🔍 BMS protection flags:");
		}
		if (bmsProtection.protectionFlagsDetailed != 0) {
			for (uint8_t i = 0; i < 14; i++) {
				if (bmsProtection.protectionFlagsDetailed & (1 << i)) {
					if (bmsSet.modeLog == ALL) {
					    printlnMessage(bmsProtectMessagesRU[i], bmsProtectMessagesEN[i]);
					}
				}
			}
		} else {
			printlnMessage("✅ Нет ошибок", "✅ No errors");
			}
#endif
#endif
    }

	/* Парсинг регистра 0x8С */
	if (data[indexParse + 33] == 0x8C) {
        bmsStatus.statusFlagsDetailed = (data[indexParse + 34] << 8) | data[indexParse + 35];
        bmsStatus.chargeMOSFETState = bmsStatus.statusFlagsDetailed & (1 << 0);
		bmsStatus.dischargeMOSFETState = bmsStatus.statusFlagsDetailed & (1 << 1);
		bmsStatus.balancerState = bmsStatus.statusFlagsDetailed & (1 << 2);
		bmsStatus.batteryState = bmsStatus.statusFlagsDetailed & (1 << 3);	
#ifndef DISABLE_OUTPUT_LOG_OX8C
		/* Вывод в порт отладочной информации */
		if (bmsSet.modeLog == ALL) {
			printlnMessage("   🔍 Статус BMS:", "   🔍 BMS status:");

			struct {
				bool state;
				const char* ru;
				const char* en;
			} bmsStatusItems[] = {
				{bmsStatus.chargeMOSFETState,  bmsElementMessagesRU[0], bmsElementMessagesEN[0]},
				{bmsStatus.dischargeMOSFETState, bmsElementMessagesRU[1], bmsElementMessagesEN[1]},
				{bmsStatus.balancerState, bmsElementMessagesRU[2], bmsElementMessagesEN[2]},
				{bmsStatus.batteryState, bmsElementMessagesRU[3], bmsElementMessagesEN[3]}
			};

			for (const auto& item : bmsStatusItems) {
				printMessage(item.ru, item.en); // Вывод названия элемента структуры bmsElementMessages
				printlnMessage(
					item.state ? bmsStatusMessagesRU[1] : bmsStatusMessagesRU[0], // Выбор сообщения на русском (Вкл/Выкл)
					item.state ? bmsStatusMessagesEN[1] : bmsStatusMessagesEN[0]  // Выбор сообщения на английском (On/Off)
				);
			}
		}
#endif		
    }

	/* Парсинг регистров 0x9D, 0xAB, 0xAC*/
	if (data[indexParse + 81] == 0x9D) {bmsData.statusBalancer = data[indexParse + 82];}
	if (data[indexParse + 123] == 0xAB) {bmsData.statusChargMosfet = data[indexParse + 124];}
	if (data[indexParse + 125] == 0xAB) {bmsData.statusDischargeMosfet = data[indexParse + 126];}
	if (bmsSet.modeLog == ALL) {
		printlnMessage("   🔍 Статус ручного управления BMS:", "   🔍 BMS Manual Control Status:");

		struct {
			bool state;
			const char* ru;
			const char* en;
		} bmsStatusItems[] = {
			{static_cast<bool>(bmsData.statusBalancer),  bmsElementMessagesRU[0], bmsElementMessagesEN[0]},
			{static_cast<bool>(bmsData.statusChargMosfet), bmsElementMessagesRU[1], bmsElementMessagesEN[1]},
			{static_cast<bool>(bmsData.statusDischargeMosfet), bmsElementMessagesRU[2], bmsElementMessagesEN[2]}
		};

		for (const auto& item : bmsStatusItems) {
			printMessage(item.ru, item.en); // Вывод названия элемента структуры bmsElementMessages
			printlnMessage(
				item.state ? bmsStatusMessagesRU[1] : bmsStatusMessagesRU[0], // Выбор сообщения на русском (Вкл/Выкл)
				item.state ? bmsStatusMessagesEN[1] : bmsStatusMessagesEN[0]  // Выбор сообщения на английском (On/Off)
			);
		}
	}
	/* Выводим сообщение в лог об успешном парсинге пакета и выходим */
	bmsData.countParsePacket++; // Увеличиваем счетчик успешно разобранных пакетов
	if (bmsSet.modeLog == ALL) {
		printMessage("📨 Количество пакетов: ", "📨 Number of packages: "); Serial.println(bmsData.countParsePacket);
		printMessage("❗ Количество ошибок: ", "❗ Number of errors: "); Serial.println(bmsData.countError);
	}
	Serial.print("✅ ");Serial.print(bmsData.countParsePacket);
	printlnMessage("-й пакет успешно разобран!", "-th package successfully disassembled!");
	Serial.println("");
    return SUCCESS;
}

/* 
* @brief Функция конвертации температуры
*
* Функция преобразует полученные данные в действительное 
* значение температуры в диапазоне -40...100 
*
* @return bms_error_t
*/
static bms_error_t convertTemperature(uint16_t rawTemp, int8_t &temperature) {
    if (rawTemp <= 100) {
        temperature = static_cast<int8_t>(rawTemp);  // Положительная температура (0–100)
        return SUCCESS;  // Ошибок нет
    } else if (rawTemp >= 101 && rawTemp <= 140) {
        temperature = static_cast<int8_t>(-(rawTemp - 101));  // Отрицательная температура (-1…-40)
        return SUCCESS;  // Ошибок нет
    } else {
		temperature = 0x7F; // Задаем значение 127 и обрабатываем его как ошибку
        return ERROR_DATA;  // Ошибка данных
    }
}

/* 
* @brief Функция очистки буфера приема
*
* Функция очищает буфер приема от мусора выход из 
* функции по таймауту или вычитки всего буфера
*
* @return bms_error_t
*/
static bms_error_t clearRxBuffer() {
	uint16_t index = 0;
	unsigned long startTime = millis();
	while (millis() - startTime < bmsSet.timeoutMs) {
#ifdef ESP32
		yield();  // Позволяет другим процессам выполняться (ESP, многозадачность)
#endif
		if (bmsSet.uart.available()) {
			if (index < bmsSet.rxBufferSize) {
				bmsSet.uart.read();
				index++;
			} else {
				return SUCCESS;
			}
		}
	}
	return ERROR_TIMEOUT;
}

/* 
* @brief Функция проверки ответа на корректность
*
* Функция проверяет принятый пакет на:
* - Валидность заголовка пакета;
* - Длину принятого пакета;
* - Соответствие номера ответа номеру запроса;
* - Контрольную сумму пакета;
* - Очищает буфер приема от мусора
*
* @param dataAnswer Указатель на массив данных
* @param length Длина массива данных ответа
*
* @return bms_error_t
*/
static bms_error_t checkAnswer(uint8_t *dataAnswer, uint16_t length) {
	if (!dataAnswer) {
        printErrorPointer();
        return ERROR_POINTER;
    }

	/* Проверяем заголовок принятого пакета */
    if (dataAnswer[0] != 0x4E || dataAnswer[1] != 0x57) {
		if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
            printlnMessage("❌ Неверный заголовок пакета!", "❌ Incorrect packet header!");
		}
		clearRxBuffer();
        return ERROR_DATA;
	}
	if (bmsSet.modeLog == ALL) {
		printMessage("✅ Заголовок пакета корректный! = ", "✅ The packet header is correct! = ");
		Serial.print("0x");
		Serial.print(dataAnswer[0], HEX); Serial.println(dataAnswer[1], HEX);
	}

	/* Проверка длины пакета */
    packetLength = (dataAnswer[2] << 8) | dataAnswer[3]; // длина пакета из принятого ответа
	if (packetLength != indexAnswer - 2) {
	    if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
		    printlnMessage("❌ Ошибка: некорректная длина пакета!", "❌ Error: Incorrect packet length!");
		}
        return ERROR_DATA;
	}
	if (bmsSet.modeLog == ALL) {
		printMessage("✅ Длина пакета корректна! = ", "✅ Packet length is correct! = ");
		Serial.println(packetLength);
	}
	
    /* Проверяем номер запрос-ответ (6, 7, 8, 9 байты с конца) */
    bmsData.answerNumber = (dataAnswer[length - 9] << 24) |
                   (dataAnswer[length - 8] << 16) |
                   (dataAnswer[length - 7] << 8)  |
                   (dataAnswer[length - 6]);
    bmsData.answerNumber &= 0x00FFFFFF; // Принудительно обнуляем старший байт (делаем его = 0x00)
    /* Сравниваем с отправленным requestNumber */
    if (bmsData.answerNumber != bmsData.requestNumber) {
		if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
			Serial.println("");
			printMessage("📩 Получен answerNumber: ", "📩 Received answerNumber: ");
            Serial.printf("%06X\n", bmsData.answerNumber);
			printMessage("📤 Ожидался requestNumber: ", "📤 Expected requestNumber: ");
            Serial.printf("%06X\n", bmsData.requestNumber);
            printlnMessage("❌ Ошибка: номер запрос-ответ не совпадает!", "❌ Error: request-response number does not match!");
		}
        return ERROR_DATA; // Выход из функции из-за несовпадения номеров запроса-ответа
    }
	if (bmsSet.modeLog == ALL) {
        printMessage("✅ Номера запроса и ответа совпадают! = ", "✅ Request and response numbers match! = ");
		Serial.print("0x");
		Serial.printf("%06X\n", bmsData.answerNumber);
	}
	
	/* Проверка контрольной суммы принятого пакета */
    uint16_t receivedChecksum = (dataAnswer[length - 2] << 8) | dataAnswer[length - 1]; // Контрольная сумма из принятого пакета
    calculateChecksum16(dataAnswer, length - 2);
    if (receivedChecksum != calculatedChecksum) {
		if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
		    printlnMessage("❌ Ошибка контрольной суммы!", "❌ Checksum error!");
		}
        return ERROR_DATA; // Выход из функции по несовпадению контрольной суммы пакета
    }
	if (bmsSet.modeLog == ALL) {
		printMessage("✅ Контрольная сумма совпадает! = ", "✅ Checksum matches! = ");
		Serial.print("0x");
		Serial.println(calculatedChecksum, HEX);
	}
    return SUCCESS; // Выход из функции в случае успеха
}

/*
* @brief Функция расчета контрольной суммы
*
* Функция принимает указатель на массив данных и его длину
*
* @param data Указатель на массив данных
* @param length Длина массива данных
*
* @return bms_error_t
*/
static bms_error_t calculateChecksum16(uint8_t *data, uint16_t length) {
	calculatedChecksum = 0;
	if (!data) {
        printErrorPointer();
        return ERROR_POINTER;
    }
    uint16_t checksum = 0;
    for (uint16_t i = 0; i < length; i++) {
        checksum += data[i];
    }
	calculatedChecksum = checksum;
    return SUCCESS;
}

/*
* @brief Функция вывода в UART полученного пакета (разбивка по 16 байт)
*
* Функция принимает указатель на массив данных и его длину
*
* @param dataPacket Указатель на массив данных
* @param length Длина массива данных
*
* @return bms_error_t
*/
static bms_error_t printHexAnswer(uint8_t *dataPacket, uint16_t length) {
	if (!dataPacket) {
        printErrorPointer();
        return ERROR_POINTER;
    }
    for (uint16_t i = 0; i < length; i++) {
        if (dataPacket[i] < 0x10) Serial.print("0"); // Добавляем ведущий ноль для однозначных значений
        Serial.print(dataPacket[i], HEX);
        Serial.print(" ");
        /* Разбивка по 16 байт */
        if ((i + 1) % 16 == 0) {
            Serial.println(); // Перенос строки после каждых 16 байт
        }
    }
    /* Если длина не кратна 16, завершаем строку */
    if (length % 16 != 0) {
        Serial.println();
    }
	return SUCCESS;
}

/*
* @brief Функция расчета размера принимаемого пакета
*
* Функция рассчитывает размер принимаемого пакета, исходя из
* указаного количества ячеек.
*
* @return packetSize размер пакета
*/
static uint16_t calculationAnswerPacketSize() {
	uint16_t packetSize = (bmsData.specifiedCellCount*3 + 2) + START_BLOCK_LENGTH + REGISTERS_BLOCK_LENGTH + END_BLOCK_LENGTH;
	return packetSize;
}

/*
* @brief Функция очистки принятого массива
*
* Функция очищает принятый массив и его индекс
*
* @param dataAnswer Указатель на массив данных
* @param length Длина массива данных
*
* @return bms_error_t
*/
static bms_error_t resetAnswerArray(uint8_t *dataAnswer, uint16_t length) {
	if (!dataAnswer) {
        printErrorPointer();
        return ERROR_POINTER;
    }
    memset(dataAnswer, 0, length);
    indexAnswer = 0;
	return SUCCESS;
}

/*
* @brief Функция вывода сообщения аналог Serial.print(F())
*
* Функция отправляет в отладочный порт сообщение на одном
* из выбранных языков
*
* @param messageRU Сообщение на русском языке
* @param messageEN Сообщение на английском языке
*
* @return void
*/
static void printMessage(const char* messageRU, const char* messageEN) {
    if (bmsSet.language == EN) {
        Serial.print(F(messageEN));
    } else {
        Serial.print(F(messageRU));
    }
	return;
}

/*
* @brief Функция вывода сообщения аналог Serial.println(F())
*
* Функция отправляет в отладочный порт сообщение на одном
* из выбранных языков
*
* @param messageRU Сообщение на русском языке
* @param messageEN Сообщение на английском языке
*
* @return void
*/
static void printlnMessage(const char* messageRU, const char* messageEN) {
    if (bmsSet.language == EN) {
        Serial.println(F(messageEN));
    } else {
        Serial.println(F(messageRU));
    }
	return;
}

/*
* @brief Функция вывода сообщения ошибки
*
* Функция отправляет в отладочный порт сообщение:
* Ошибка: Неверный указатель на данные!
*
* @return void
*/
static void printErrorPointer() {
	printlnMessage("❌ Ошибка: Неверный указатель на данные!", "❌ Error: Invalid message pointer!");
	return;
}

/*
* @brief Функция вывода сообщения ошибки
*
* Функция отправляет в отладочный порт сообщение:
* Ошибка: Таймаут при получении ответа!
* Отсутсвует связь с BMS!
*
* @return void
*/
static void logTimeoutErrorNoConnection() {
	/* Выводим сообщение в лог */
    if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
        printlnMessage("⏰ Ошибка: Таймаут при получении ответа!", "⏰ Error: Timeout while receiving response!");
		printMessage("🔹 Получено байт: ", "🔹 Bytes received: ");
		Serial.println(indexAnswer);
	}
	/* Проверяем количество принятых данных, если 0 выводим сообщение в лог и  выходим по таймауту */
	if (indexAnswer == 0) {
		printlnMessage("🔴 Отсутсвует связь с BMS!", "🔴 No connection with BMS!");
	}
	return;
}

/*
* @brief Функция вывода сообщения ошибки
*
* Функция отправляет в отладочный порт сообщение:
* Ошибка: неверный пакет данных!
*
* @return void
*/
static bms_error_t logErrorPacket(uint8_t *dataAnswer, uint16_t length) {
	if (!dataAnswer) {
        printErrorPointer();
        return ERROR_POINTER;
    }
	if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
		printHexAnswer(dataAnswer, length);
	}
	if (bmsSet.modeLog == ALL || bmsSet.modeLog == ERROR) {
		printlnMessage("❌ Ошибка: неверный пакет данных!", "❌ Error: invalid data packet!");
		printMessage("🔹 Было получено байт: ", "🔹 Was received byte: ");
		Serial.println(length);
	}
	return SUCCESS;
}