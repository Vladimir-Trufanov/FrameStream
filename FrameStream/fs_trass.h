/** Arduino, ESP32, C/C++ ************************************** fs_trass.h ***
 * 
 *                           Обеспечить вывод сообщений в последовательный порт 
 *                                               и запись лог-файла на SD-карту
 *                                                     
 * v2.2.1, 14.06.2026                                 Автор:      Труфанов В.Е.
 * Copyright © 2026 tve                               Дата создания: 24.01.2026
**/

#pragma once 

//#include "time.h"
#include "esp_log.h"
#include "FS.h"
//#include <SD_MMC.h>

#define jpr         say  
#define jprln       sayln  
#define print_mem   saymem  
#define major_fail  blinkRestart  

#define say(format, ...) \
  { \
    char buffer[256]; \
    snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__); \
    Serial.print(buffer); \
    if (logfile) { \
      logfile.print(buffer); \
    } \
  }

#define sayln(format, ...) \
  { \
    char buffer[256]; \
    snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__); \
    Serial.println(buffer); \
    if (logfile) { \
      logfile.println(buffer); \
    } \
  }
  
// Подсчитать число символов UTF-8 в последовательности char* 
size_t utf8len(const char* str); 
// Показать состояние памяти с заданным префиксом  
void saymem(char* text); 
// Отмигать аварийную ситуацию контрольным светодиодом и перезагрузить контроллер 
void blinkRestart(); 

File logfile;           // дескриптор файла информационных сообщений и об ошибках
//char buffer[256];       // буфер сообщения
  
// ****************************************************************************
// *              Показать состояние памяти с заданным префиксом              *
// ****************************************************************************
// Специальные сообщения - это сообщения по использованию памяти, по времени ... 
#define isSAYLOG true   // true - вести файл дублирования сообщений
bool isSAY=true;        // true - разрешить вывод неспециальных сообщений
bool isSAYMEM=false;    // true - разрешить трассировку состояния памяти

void saymem(char* text) 
{
  if (isSAYMEM)
  {
    // Запоминаем состояние разрешения на вывод сообщений
    bool oldSay=isSAY;       
    // Разрешаем вывод сообщений
    isSAY=true;      
    // Определяем наибольшее число символов текста вместе с пробелами
    // и дополняем текст пробелами слева 
    int nfill=48;   
    int j=nfill-utf8len(text)-2;
    for (int i = 0; i<j; i++) 
    {
      say(" "); 
    }
    // Выводим специальное сообщение по памяти
    /**
     * xPortGetCoreID()        - функция возвращает номер ядра, на котором выполняется текущая задача
     * uxTaskPriorityGet(NULL) - возвращает приоритет текущей задачи (задачи, из которой была вызвана функция)
     * ESP.getFreeHeap()       - возвращает размер свободной кучи (heap) в байтах
     * ESP.getHeapSize()       - возвращает полный размер внутренней оперативной памяти в байтах (ОЗУ)
     * ESP.getFreePsram()      - свободный объём внешней оперативной памяти PSRAM
     * ESP.getPsramSize()      - полный объём внешней оперативной памяти PSRAM
    **/
    say("[%s]",text);
    say(" ядро: %d,",          int(xPortGetCoreID()));        say(" приоритет: %d, ", int(uxTaskPriorityGet(NULL)));
    say("свободная куча %6d ", uint32_t(ESP.getFreeHeap()));  say("от ОЗУ %6d, ",     uint32_t(ESP.getHeapSize()));
    say("FreePSRAM %6d ",      uint32_t(ESP.getFreePsram())); say("от FLASH %6d",     uint32_t(ESP.getPsramSize()));
    sayln(" ");
    // Восстанавливаем состояние разрешения на вывод сообщений
    isSAY=oldSay;      
  }
}
// ****************************************************************************
// *      Отмигать аварийную ситуацию контрольным светодиодом в 10 циклов     *
// *               в случае неудачной работы камеры или sd-карты              *
// ****************************************************************************
void blinkRestart() 
{
  for  (int i = 0;  i < 10; i++) 
  {                
    for (int j = 0; j < 3; j++) 
    {
      digitalWrite(33, LOW);   delay(150);
      digitalWrite(33, HIGH);  delay(150);
    }
    delay(1000);
    for (int j = 0; j < 3; j++) 
    {
      digitalWrite(33, LOW);  delay(500);
      digitalWrite(33, HIGH); delay(500);
    }
    delay(1000);
    sayln("Аварийная ситуация [%d/10], перезагрузка контроллера",i);
  }
  if (logfile) logfile.close();
  ESP.restart();
}
// ****************************************************************************
// *        Подсчитать число символов UTF-8 в последовательности char*        *
// * https://stackoverflow.com/questions/4063146/getting-the-actual-length-of-a-utf-8-encoded-stdstring
// ****************************************************************************
size_t utf8len(const char* str) 
{
  size_t len = 0;
  for (size_t i = 0; *str != 0; ++len) 
  {
    int v01 = ((*str & 0x80) >> 7) & ((*str & 0x40) >> 6);
    int v2 = (*str & 0x20) >> 5;
    int v3 = (*str & 0x10) >> 4;
    str += 1 + ((v01 << v2) | (v01 & v3));
  }
  return len;
}
// ****************************************************************************
// *           Показать установленные настройки камеры, видео и другие        *
// ****************************************************************************
void sayconfig() 
{
  say("============ Установленные настройки камеры, видео и другие =========\n");
  //say("Название камеры                      %s\n",      devname);
  say("Название контроллера                 %s\n",      devname);
  say("Размер кадра                         %d\n",      framesize);
  say("Качество                             %d\n",      quality);
  say("Количество буферов для кадров        %d\n",      buffersconfig);
  say("Размер видео в секундах              %d\n",      avi_length);
  say("Интервал между записями кадров (ms)  %d\n",      frame_interval);
  say("Ускорение воспроизведения            %d\n",      speed_up_factor);
  say("Интервал между кадрами в потоке (ms) %d\n",      stream_delay);
  say("TIMEZONE                             %d, %s\n",  TIMEZONE.length(), TIMEZONE.c_str());
  say("Сеть WiFi                            %s\n",      ssid);
  say("Тип карты SD_MMC                     "); 
  if (ncardType == 1)                       sayln("MMC")      
  else if (ncardType == 2)                  sayln("SDSC") 
  else if (ncardType == 3)                  sayln("SDHC")
  else if (ncardType == 4)                  sayln("неопределён")
  else                                      sayln("карта не подключена");
  say("Ёмкость SD_MMC-карты                 %llu MB\n", cardSize);
}

// ************************************************************* fs_trass.h ***
