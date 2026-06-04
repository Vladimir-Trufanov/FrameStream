/** Arduino, ESP32, C/C++ ***************************************** hwifi.h ***
 * 
 *                                          Обеспечить работу, связанную с wifi
 *                                                     
 * v2.1.0, 24.02.2026                                 Автор:      Труфанов В.Е.
 * Copyright © 2026 tve                               Дата создания: 24.01.2026
**/

#pragma once   

#include "WiFi.h"
// Обслуживаем режим энергосбережения
#include "esp_wifi.h" 
// Подключаем библиотеку для связи с сервером SNTP, которая является 
// библиотекой ядра ESP32 по умолчанию и не требуют установки
#include "esp_sntp.h"
#include "trass.h"

// Вводим имя и пароль точки доступа
const char* tssid = "OPPO A9 2020";  // "TP-Link_B394" "tve-DESKTOP" "linksystve"
const char* tpass = "b277a4ee84e8";  // "18009217"     "Ue18-647"    "x93k6kq6wf"

// Создаем переменную локального времени (секунды с начала эпохи)
time_t now;
// Создаем структуру времени timeinfo в которую будем вкладывать
// выбранное и преобразованное время в секундах с начала эпохи
struct tm timeinfo;

// ****************************************************************************
// *      Обеспечить заглушку через макрос на событие с WiFi=201, когда не    *
// *                 удаётся подключиться к локальной сети                    *
// ****************************************************************************
// По настройке в IDE "Core Debug Level: Error", выводим только ошибки,
// сообщения "Reason: 201 - NO_AP_FOUND" зажимаем в обработке события  WiFi.onEvent:
/*
12:56:58.974 -> [366612][W][STA.cpp:137] _onStaArduinoEvent(): Reason: 201 - NO_AP_FOUND
12:57:01.399 -> [369029][W][STA.cpp:137] _onStaArduinoEvent(): Reason: 201 - NO_AP_FOUND
12:57:03.821 -> [371446][W][STA.cpp:137] _onStaArduinoEvent(): Reason: 201 - NO_AP_FOUND
[ESP 32 Arduino: Получение информации о событиях Wi-Fi](https://techtutorialsx.wordpress.com/2019/08/15/esp32-arduino-getting-wifi-event-information/)
[tools/sdk/include/esp32/esp_event_legacy.h](https://github.com/espressif/arduino-esp32/blob/04963009eedfbc1e0ea2e1378ae69e7cebda6fd6/tools/sdk/include/esp32/esp_event_legacy.h#L56)
[libraries/WiFi/src/WiFiType.h](https://github.com/espressif/arduino-esp32/blob/a59eafbc9dfa3ce818c110f996eebf68d755be24/libraries/WiFi/src/WiFiType.h#L35)
*/
#define onEventWiFi();                                                                       \
  eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info)                         \ 
  {                                                                                          \
    if (info.wifi_sta_disconnected.reason == 201) {}                                         \
    else if (info.wifi_sta_disconnected.reason == 64) {}                                     \
    else sayln("info.wifi_sta_disconnected.reason =  %d",info.wifi_sta_disconnected.reason); \
  });

// ****************************************************************************
// *      Подключить ESP32 к указанной сети Wi-Fi (непрерывно проверять       *
// *  состояние подключения до тех пор, пока оно не будет успешно установлено *
// ****************************************************************************
bool initWiFi() 
{
  bool Result=false;
  int j;
  say("Контроллер подключается к WiFi ");
  WiFi.begin(tssid, tpass);
  for (j = 0; j < 16; j++) 
  {
    delay (500);
    say(".");
    if (WiFi.status() == WL_CONNECTED) 
    {
      sayln("\nWiFi подключен!");
      ssid = String(tssid); 
      pass = String(tpass);   
      Result=true;
      break;
    }
  }
  if (j==16) sayln("\nНе удалось подключиться к локальной сети WiFi!");
  return Result;
}
// ****************************************************************************
// *                           Настроить часовой пояс                         *
// ****************************************************************************
void setTimezone() 
{ 
  // Здесь устанавливается стандартное время для региона - Европа/Москва
  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  setenv("TZ", "MSK-3", 1);
  tzset();
}
// ****************************************************************************
// *   Извлечь информацию о текущем времени и вывести ее в отформатированном  *
// *   виде с помощью struct tm структуры данных:                             *
// *                               https://cplusplus.com/reference/ctime/tm/  *
// ****************************************************************************
/*
 "%A, %B %d %Y %H:%M:%S" - это спецификаторы формата,  которые определяют,  как
 в struct tm timeinfo; будет отформатирован текст, а члены tm struct следующие:
  
  Тип элемента Значение                        Диапазон
  -----------------------------------------------------
  tm_sec  int  секунды после минуты            0-61*
  tm_min  int  минуты после часа               0-59
  tm_hour int  часы с полуночи                 0-23
  tm_mday int  день месяца                     1-31
  tm_mon  int  месяцы с января                 0-11
  tm_year int  годы с 1900
  tm_wday  —   количество дней с воскресенья   0-6
  tm_yday  —   количество дней с 1 января      0-365
  tm_isdst —   флаг перехода на летнее время 
  
  function strftime() - format time as string:
  https://cplusplus.com/reference/ctime/strftime/
*/
void printTime() 
{
  getLocalTime(&timeinfo);
  say("%d",  timeinfo.tm_year+1900); say("-%d", timeinfo.tm_mon+1);  say("-%d", timeinfo.tm_mday);
  say(" %d", timeinfo.tm_hour);      say(":%d", timeinfo.tm_min);  sayln(":%d", timeinfo.tm_sec);
}
// ****************************************************************************
// * Настроить интервал синхронизации, имя сервера, режим работы и часовой пояс
// ****************************************************************************
void notify(struct timeval* t) 
{
  say("\nВремя синхронизировано: "); printTime(); 
}
void initSNTP() 
{ 
  if (esp_sntp_enabled()) 
  { 
    esp_sntp_stop(); 
  }
  // Определяем, как часто синхронизировать внутренние часы ESP32 с сервером 
  // SNTP. Интервал указывается в микросекундах. Интервал в 60*60*1000UL 
  // микросекунд означает синхронизацию каждый час. Разумные интервалы запросов 
  // обычно составляют от одного-двух раз в день до 5 раз в час. 
  sntp_set_sync_interval(1 * 60 * 60 * 10 * 1000UL);  // через 10 часов
  // Указываем функцию уведомления (callback), которая вызывается при каждой
  // cинхронизации. В данном коде для этой цели определяем функцию notify(). 
  sntp_set_time_sync_notification_cb(notify);
  // Устанавливаем режим работы: ESP_SNTP_OPMODE_POLL — просто опрашивать
  // сервер SNTP (есть также ESP_SNTP_OPMODE_LISTENONLY)
  esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
  // Указываем имя/адрес сервера. При желании можно указать несколько серверов.
  // Например:
  //   esp_sntp_setservername(0, «pool.ntp.org»);
  //   esp_sntp_setservername(1, «de.pool.ntp.org»);
  //   esp_sntp_setservername(2, «time.nist.gov»);
  esp_sntp_setservername(0, "ntp.msk-ix.ru");
  // Запускаем службу SNTP с указанными выше параметрами
  esp_sntp_init();
  // Устанавливаем часовой пояс, поскольку сервер SNTP 
  // возвращает время в формате UTC
  setTimezone();
}
// ****************************************************************************
// *                            Ожидать синхронизацию                         *
// ****************************************************************************
void wait4SNTP() 
{
  say("Ожидание синхронизации времени ");
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) 
  {
    delay(300);
    say(".");
  }
}
// ****************************************************************************
// *                 Подключить локальную WiFi и установить время             *
// ****************************************************************************
void iniLocalWiFi() 
{
  isLocalWiFi=initWiFi();
  if (isLocalWiFi)
  {
    initSNTP();
    wait4SNTP();
  }
}
// ****************************************************************************
// *                      Отключить режим энергосбережения                    *
// ****************************************************************************
void iniWiFiPsNone() 
{
  // Отключаем режим энергосбережения: 
  // esp_wifi_get_ps — функция из API драйвера Wi-Fi для платы ESP32, которая получает 
  // режим энергосбережения (sleep mode) Wi-Fi. 
  // Функция входит в класс WiFi и возвращает значение типа wifi_ps_type_t. 
  // Режим энергосбережения WiFi влияет на скорость соединения. Можно установить один из трёх режимов: 
  // WIFI_PS_NONE — режим отключён; WIFI_PS_MIN_MODEM — минимальное энергосбережение; WIFI_PS_MAX_MODEM — максимальное энергосбережение.
  // Функция позволяет системе автоматически просыпаться из сна, когда это требуется драйвером Wi-Fi, 
  // и поддерживать соединение с точкой доступа (AP). 
  // Функция esp_wifi_get_ps вызывается в коде приложения, когда нужно получить текущий 
  // режим энергосбережения Wi-Fi. Например, в функции, которая управляет работой Wi-Fi, 
  // можно вызвать WiFi.getSleep() — она вернёт значение типа wifi_ps_type_t.
  // Важно: режим энергосбережения влияет на то, как драйвер Wi-Fi обрабатывает 
  // пакеты данных — при включённом режиме энергосбережения полученные данные могут 
  // быть задержаны на период, указанный в настройках DTIM. 
  wifi_ps_type_t the_type;
  esp_err_t get_ps = esp_wifi_get_ps(&the_type);
  sayln("\nНачальный режим энергосбережения: %d", the_type);
  esp_err_t set_ps = esp_wifi_set_ps(WIFI_PS_NONE);
  esp_err_t new_ps = esp_wifi_get_ps(&the_type);
  sayln("-Текущий- режим энергосбережения: %d", the_type);
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, brown_reg_temp);
}

// **************************************************************** hwifi.h ***
