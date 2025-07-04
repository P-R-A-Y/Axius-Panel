# AxiusDashboard
Настольный дешборд на esp8266. Работает на ядре [Axius](https://github.com/P-R-A-Y/Axius) с использованием 128x64 2.4-дюймового дисплея на драйвере SSD1306

## Подробнее
Фотки и технические подробности можно найти [тут](https://misc.pray-xdd.ru#axiusdashboard)

## Коротко о фичах
Умеет отображать:
- Дату/Время
- Состояние домашнего сервера
- Потребление трафика в виде диаграммы
- Данные о подключении (ipv4/6, аптайм соединения, мак адрес)
- Температуры и влажности в комнате
- Базовую информацию о моем майнкрафт сервере
В планах:
- настроить отображение погоды
- настроить отображение списка подключеных к сети основных устройств
- Отправка через Axius по протоколу Link всем девайсам в радиусе доступности информацию о дате и времени


## Преднастройка окружения
Чтобы дешборд мог общаться с роутером (подойдет только кинетик), нужно открыть порт, по которому будет доступно API роутера без авторизации. Да, проще было бы сделать SSH соединение но еспшка слишком слабая для криптографии.

Настроить переадресацию порта нужно следующим образом:
Вход: 
- IP: 192.168.0.0
- Маска: 255.255.255.0 (24)
- Порт: 81 (необязательно прям такой, но лучше его чтобы небыло конфликтов)
Выход:
- Должен вести в локальную сеть
- Порт: 79
- Протокол: TCP