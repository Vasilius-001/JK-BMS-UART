#pragma once

#include <Arduino.h>

//#define DISABLE_PARSE_OX8B      // Не разбирать байты регистра 0x8B, уменьшает размер прошивки
//#define DISABLE_OUTPUT_LOG_OX8B // Вывод лога регистра 0x8B, уменьшает размер прошивки
//#define DISABLE_OUTPUT_LOG_OX8C // Вывод лога регистра 0x8C, уменьшает размер прошивки

#define MAX_CELL_COUNT  24          // Максимальное количество ячеек
#define START_BLOCK_LENGTH  11      // Длина начального блока данных
#define REGISTERS_BLOCK_LENGTH  221 // Длина блока данных регистров
#define END_BLOCK_LENGTH  9         // Длина конечного блока данных
#define BMS_GENERAL_ANSWER_SIZE  (MAX_CELL_COUNT*3 + 2 + \
										START_BLOCK_LENGTH + \
										REGISTERS_BLOCK_LENGTH + \
										END_BLOCK_LENGTH) // Размер массива общих данных
#define BMS_TOGGLE_ANSWER_SIZE  21  // Размер пакета ответа на переключение регистров

/* Делаем массивы доступными для других файлов */
extern uint16_t bmsCellVoltages[MAX_CELL_COUNT];       // Массив напряжений на каждой ячейке

enum BmsLanguage { RU, EN };        // Определяем тип языка
enum BmsModeLog { NO, ALL, ERROR }; // Определяем режим логирования

/* Структура с настройками BMS по умолчанию */
struct BMSConfig {
    BmsLanguage language = RU;                 // Язык вывода логов "RU"-русский, "EN"-английский
    BmsModeLog modeLog = ALL;                  // Логирование "NO"-без логов, "ALL"-выводить все, "ERROR"-только ошибки
    bool modeHex = false;                      // Логирование полученного пакета: true-выводить, false-не выводить
    bool modeNum = false;                      // Нумерация запросов: выключена
    HardwareSerial &uart = Serial1;            // Аппаратный UART для общения с BMS
    uint16_t rxBufferSize = 64;                // Размер буфера приема UART
    unsigned long timeoutMs = 10;              // Таймаут приема (мс)
    bool modeFastCorrect = true;               // Ускоренная коррекция количества ячеек
    uint8_t correctIterations = 3;             // Количество итераций коррекции
    bool modeErrorLimit = true;                // Ограничение по количеству ошибочных пакетов
    uint8_t numberErrorCycles = 10;            // Количество ошибочных циклов
    uint16_t limitErrorCycles = 20;            // Лимит ошибок до прекрашения отправки запросов
    unsigned long errorCyclePollingMs = 10000; // Время опроса ошибочных циклов, мс
};
extern BMSConfig bmsSet; // Объявляем глобальный объект

/* Определяем структуру для хранения данных BMS */
struct BMSData {
    uint8_t specifiedCellCount = MAX_CELL_COUNT;         // Заданное количество ячеек
	uint16_t answerPacketSize = BMS_GENERAL_ANSWER_SIZE; // Расчитанный размер принимаемого пакета
    uint32_t countError = 0;                             // Количество ошибок при приеме и обработке пакетов
	uint32_t countNoConnect = 0;                         // Количество отсутсвия связи с BMS
    uint32_t countParsePacket = 0;                       // Количество корректно обработанных пакетов
    uint32_t requestNumber = 0;                          // Номер запроса
    uint32_t answerNumber = 0;                           // Номер ответа
    uint16_t batteryVoltage = 0;                         // Напряжение на батарее
    int16_t batteryCurrent = 0;                          // Ток из/в батарею
    uint8_t batterySOC = 0;                              // Состояние заряда
    int8_t batteryTemp1 = 0;                             // Температура датчика 1
    int8_t batteryTemp2 = 0;                             // Температура датчика 2
    int8_t mosfetTemp = 0;                               // Температура MOSFET
    uint8_t receivedCellCount = 0;                       // Количество ячеек
    uint16_t minCellVoltage = 0;                         // Минимальное напряжение ячейки
    uint16_t maxCellVoltage = 0;                         // Максимальное напряжение ячейки
    uint8_t statusChargMosfet = 0;                       // Состояние зарядных MOSFET
    uint8_t statusDischargeMosfet = 0;                   // Состояние разрядных MOSFET
    uint8_t statusBalancer = 0;                          // Состояние балансира
};
extern BMSData bmsData; // Экспортируем экземпляр структуры (но не создаём!)

/* Создание типа bms_error_t */
typedef enum : uint8_t {
	SUCCESS = 0,             // Успешное завершение
	ERROR_TIMEOUT = 1,       // Завершение по таймауту
	ERROR_UNKNOWN = 2,       // Неизвестная ошибка
	ERROR_CONNECT = 3,       // Ошибка соединения
	ERROR_POINTER = 4,       // Ошибка указателя (nullptr)
    ERROR_DATA = 5,          // Неверные данные
    ERROR_COMMAND = 6,       // Ошибочная команда
    ERROR_CORRECTION = 7,    // Ошибка коррекции
    ERROR_LIMIT_CYCLES = 8,  // Достигнут лимит ошибочных циклов
    ERROR_WAIT = 9           // Ожидание отправки
} bms_error_t;

/* Определяем структуру для хранения команд BMS */
struct BMSCommands {
    uint8_t readAllRegisters[19];   // Прочитать все регистры
    uint8_t enableCharging[20];     // Включить зарядные MOSFET
    uint8_t disableCharging[20];    // Выключить зарядные MOSFET
    uint8_t enableDischarging[20];  // Включить разрядные MOSFET
    uint8_t disableDischarging[20]; // Выключить разрядные MOSFET
    uint8_t enableBalancer[20];     // Включить балансир
    uint8_t disableBalancer[20];    // Выключить балансир
};
extern BMSCommands bmsCommands;  // Экспортируем экземпляр структуры (но не создаём!)
/* Объявляем указатели команд для использования в других файлах */
extern uint8_t *readAll;     // Прочитать все регистры
extern uint8_t *onCharg;     // Включить зарядные MOSFET
extern uint8_t *offCharg;    // Выключить зарядные MOSFET
extern uint8_t *onDischarg;  // Включить разрядные MOSFET
extern uint8_t *offDischarg; // Выключить разрядные MOSFET
extern uint8_t *onBalance;   // Включить балансир
extern uint8_t *offBalance;  // Выключить балансир

#ifndef DISABLE_PARSE_OX8B
/* Определяем структуру для флагов защиты регистра 0x8B */
struct ProtectionFlags {
    uint16_t protectionFlagsDetailed = 0; // Флаги защиты BMS
    bool lowCapacity : 1;                 // Низкая емкость батареи
    bool mosfetOverheat : 1;              // Перегрев MOSFET
    bool chargeOvervoltage : 1;           // Перенапряжение при зарядке
    bool dischargeUndervoltage : 1;       // Пониженное напряжение разряда
    bool batteryOverheat : 1;             // Перегрев батареи
    bool chargeOvercurrent : 1;           // Перегрузка по току зарядки
    bool dischargeOvercurrent : 1;        // Перегрузка по току разрядки
    bool cellVoltageImbalance : 1;        // Разница напряжений на ячейках/рядах
    bool batteryCompartmentOverheat : 1;  // Перегрев батарейного отсека
    bool lowBatteryTemperature : 1;       // Низкая температура батареи
    bool cellOvervoltage : 1;             // Перенапряжение ячейки/ряда
    bool cellUndervoltage : 1;            // Пониженное напряжение ячейки/ряда
    bool protection309A : 1;              // 309_A защита
    bool protection309B : 1;              // 309_B защита
};
extern ProtectionFlags bmsProtection; // Экспортируем экземпляр структуры (но не создаём!)
#endif

/* Определяем структуру для флагов состояния регистра 0x8C */
struct StatusFlags {
    uint16_t statusFlagsDetailed = 0; // Переменная флагов состояния BMS
	bool chargeMOSFETState : 1;       // Зарядные MOSFET
	bool dischargeMOSFETState : 1;    // Разрядные MOSFET
	bool balancerState : 1;           // Балансир
	bool batteryState : 1;            // Батарея
    bool dataState : 1;               // Состояние данных
    bool connectState : 1;            // Состояние соединения
};
extern StatusFlags bmsStatus; // Экспортируем экземпляр структуры (но не создаём!)


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
bms_error_t bmsSendRecive(uint8_t *message);

/*
* @brief Функция отправки запроса
*
* Функция отправляет запрос в BMS
*
* @param message Указатель на массив данных запроса
*
* @return bms_error_t
*/
bms_error_t bmsSendMessage(uint8_t *message);

/* 
* @brief Функция приема пакета данных всех от BMS
*
* Функция принимает пакет данных от BMS:
* - Принимает пакет или выход по таймауту
* - Автоматически вычисляет количество ячеек
*
* @return bms_error_t
*/
bms_error_t bmsReceiveAnswer();

/*
* @brief Функция ускорения коррекции количества ячеек
*
* Функция ускоряет коррекцию количества подключенных ячеек,
* примерно в 10 раз
*
* @return bms_error_t
*/
bms_error_t bmsCorrectionCycle ();

