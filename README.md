# JkBmsUart Arduino Library

## Описание

Эта библиотека для Arduino IDE и ей подобных позволяет обмениваться данными с JK BMS через UART GPS по протоколу 4G-GPS. Она позволяет получать данные о состоянии батареи, управлять режимами зарядки/разрядки, а также обрабатывать ошибки и статусы системы.

**P.S.** Я не программист, это моё хобби, изящность и рациональность написания того или иного участка кода не гарантирую, но к этому стремлюсь.
Конструктивная критика, а также предложения по улучшению приветствуется.

## Папки и файлы

- **src** - библиотеки, используемые в данном проекте;
- **examples** - Примеры использования, файл .ino.
- **docs** - Документация по протоколу 4G-GPS

## Возможности

- Чтение всех регистров BMS
- Управление MOSFET (заряд/разряд)
- Включение/выключение балансировочного режима
- Логирование состояний и ошибок
- Гибкая настройка параметров UART и таймаутов
- Поддержка нескольких режимов логирования
- Автоматическая коррекция количества ячеек
- Работа с флагами защиты и состояния BMS

## Установка

1. Скачайте или клонируйте репозиторий.
2. Скопируйте файлы библиотеки в папку `libraries` вашего Arduino IDE.
3. Перезапустите Arduino IDE и подключите библиотеку в вашем коде:
   ```cpp
   #include <JkBmsUart.h>
   ```

## Использование

### Инициализация

```cpp
#include <Arduino.h>
#include <JkBmsUart.h>

void setup() {
    Serial.begin(bmsSet.speedUartLoger); // Аппаратный UART для информационных сообщений
    bmsSet.uart.begin(bmsSet.speedUartBms); // Аппаратный UART для общения с BMS
}

void loop() {
   bmsSendRecive(readAll); // Запрос всех регистров
   Serial.print("Напряжение батареи: ");
   Serial.println(bmsData.batteryVoltage);
   Serial.print("Ток батареи: ");
   Serial.println(bmsData.batteryCurrent);
   Serial.print("Состояние заряда: ");
   Serial.println(bmsData.batterySOC);
   delay(1000);
}
```

### Настройка конфигурации

```cpp
bmsUpdateConfig(RU, ALL, false, false, Serial1, 115200, 115200, 320, 10, true, 3, true, 10, 20, 10000, 1000, 150);
```

### Управление MOSFET

```cpp
bmsSendMessage(onCharge);  // Включить зарядку
bmsSendMessage(offCharge); // Выключить зарядку
bmsSendMessage(onDischarge);  // Включить разрядку
bmsSendMessage(offDischarge); // Выключить разрядку
```

### Управление балансировкой

```cpp
bmsSendMessage(onBalancer);  // Включить балансировку
bmsSendMessage(offBalancer); // Выключить балансировку
```

### Получение данных о батарее

```cpp
- `bmsData.batteryVoltage` - напряжение на батарее
- `bmsData.batteryCurrent` - ток батареи
- `bmsData.batterySOC` - состояние заряда
- `bmsData.batteryTemp1` - температура датчика 1
- `bmsData.batteryTemp2` - температура датчика 2
- `bmsData.mosfetTemp` - температура MOSFET
- `bmsData.receivedCellCount` - количество ячеек
- `bmsData.minCellVoltage` - минимальное напряжение ячейки
- `bmsData.maxCellVoltage` - максимальное напряжение ячейки
- `bmsData.statusChargMosfet` - состояние зарядных MOSFET
- `bmsData.statusDischargeMosfet` - состояние разрядных MOSFET
- `bmsData.statusBalancer` - состояние балансира
```

## Конфигурационные параметры

- `bmsSet.language` - язык логирования (RU, EN)
- `bmsSet.modeLog` - уровень логирования (NO, ALL, ERROR)
- `bmsSet.modeHex` - включение логирования в HEX-формате
- `bmsSet.modeNum` - включение нумерации запросов
- `bmsSet.uart` - аппаратный UART для BMS
- `bmsSet.speedUartBms` - скорость обмена с BMS
- `bmsSet.speedUartLoger` - скорость порта для логирования
- `bmsSet.rxBufferSize` - размер буфера приема UART
- `bmsSet.timeoutMs` - таймаут приёма (мс)
- `bmsSet.modeFastCorrect` - ускоренная коррекция количества ячеек
- `bmsSet.correctIterations` - количество итераций коррекции
- `bmsSet.modeErrorLimit` - ограничение по количеству ошибочных пакетов
- `bmsSet.numberErrorCycles` - количество ошибочных циклов
- `bmsSet.limitErrorCycles` - лимит ошибок до прекращения отправки запросов
- `bmsSet.errorCyclePollingMs` - время опроса ошибочных циклов (мс)
- `bmsSet.readRequestInterval` - интервал запроса чтения всех регистров (мс)
- `bmsSet.sendCommandInterval` - интервал отправки команды (мс)

## Ошибки и коды возврата

| Код ошибки      | Описание |
|----------------|----------|
| SUCCESS        | Успешное выполнение |
| ERROR_TIMEOUT  | Таймаут ответа |
| ERROR_UNKNOWN  | Неизвестная ошибка |
| ERROR_CONNECT  | Ошибка соединения |
| ERROR_POINTER  | Ошибка указателя (nullptr) |
| ERROR_DATA     | Некорректные данные |
| ERROR_COMMAND  | Ошибка команды |
| ERROR_CORRECTION | Ошибка коррекции |
| ERROR_LIMIT_CYCLES | Превышение лимита ошибок |
| ERROR_WAIT     | Ожидание отправки |

## Флаги защиты и состояния

### Флаги защиты (0x8B)

- `bmsProtection.lowCapacity` - низкая емкость батареи
- `bmsProtection.mosfetOverheat` - перегрев MOSFET
- `bmsProtection.chargeOvervoltage` - перенапряжение при зарядке
- `bmsProtection.dischargeUndervoltage` - пониженное напряжение разряда
- `bmsProtection.batteryOverheat` - перегрев батареи
- `bmsProtection.chargeOvercurrent` - перегрузка по току зарядки
- `bmsProtection.dischargeOvercurrent` - перегрузка по току разрядки
- `bmsProtection.cellVoltageImbalance` - разбаланс напряжений ячеек
- `bmsProtection.batteryCompartmentOverheat` - перегрев батарейного отсека
- `bmsProtection.lowBatteryTemperature` - низкая температура батареи
- `bmsProtection.cellOvervoltage` - перенапряжение ячейки
- `bmsProtection.cellUndervoltage` - пониженное напряжение ячейки

### Флаги состояния (0x8C)

- `bmsStatus.chargeMOSFETState` - состояние зарядных MOSFET
- `bmsStatus.dischargeMOSFETState` - состояние разрядных MOSFET
- `bmsStatus.balancerState` - состояние балансировочного режима
- `bmsStatus.batteryState` - общее состояние батареи
- `bmsStatus.dataState` - состояние данных
- `bmsStatus.connectState` - состояние соединения

## Лицензия

Этот проект распространяется под лицензией GPL-3.0. Подробности см. в файле LICENSE.

## Контакты

Для вопросов и предложений пишите на [email@example.com](mailto:email@example.com) или создавайте issue в репозитории.

## Скриншоты
![PROJECT_PHOTO](https://github.com/Vasilius-001/JK-BMS-UART/blob/main/docs/JK_BMS_1.jpg)
![PROJECT_PHOTO](https://github.com/Vasilius-001/JK-BMS-UART/blob/main/docs/JK_BMS_2.jpg)

