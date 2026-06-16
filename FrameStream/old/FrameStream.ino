// Arduino, ESP32, C/C++ ********************************** FrameStream.ino ***
//
// Формирование потока изображений, постоянной записи фрагментов видео на 
// SD-диск или при возникновении движения, а также передача снимков и фрагментов 
// видео через сокеты на сайт KwinFlat 

// Copyright © 2026 tve                               Труфанов В.Е., 11.01.2026
static const char vernum[]="v2.2.0, 06.06.2026";  
/** 
 * Arduino IDE 2.3.7 
 * Esp32 от Espressif Systems версии 3.3.5
 * Payment:           "Al Thinker ESP32-CAM"
 * CPU Frequency:     "240MHz (WiFi/BT)"
 * Flash Frequency:   "80MHz"
 * Flash Mode:        "QIO"
**/

#include "WiFi.h"
WiFiEventId_t eventID;      
/*
#include <WiFiMulti.h>
WiFiMulti jMulti;
#include "ESPmDNS.h"
#include "esp_wifi.h" 
*/ 

//#include "esp_camera.h"
//#include "sensor.h"

//#include "soc/soc.h"
//#include "soc/rtc_cntl_reg.h"

//#include "eprom.h"
//#include <pgmspace.h>

/*
#include "esp_log.h"
#include "esp_http_server.h"

#include <stdio.h>
#include "time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_cpu.h" 

#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "FS.h"
#include <SD_MMC.h>

#include "lwip/sockets.h"
#include <lwip/netdb.h>

#include <ArduinoOTA.h>

*/

#include "inimem.h"
#include "hwifi.h"
#include "sd.h"
#include "camera.h"
#include "eprom.h"
#include "stream32.h"
#include "CameraServer.h"

/*
#include <ESPping.h>
// Определяем переменную "времени текущего пробуждения"
long wakeup;
// Инициируем переменную "времени прошлого пробуждения"
long last_wakeup = 0;
*/
// Инициируем начальный номер фонового цикла 
int loops = 0;   

void loop() 
{
  loops++;
  delay(10);
  /*
  if (loops % 10000 == 17) / *Serial.printf("looooooooooooooooooooooooooooops %10d\n",loops)* /;
  //
  for (int x = 0; x < 1; x++) 
  {
    filemgr.handleClient();  //soc.6
  }
  //
  if (do_the_ota) 
  {
    ArduinoOTA.handle();
  }
  // Если установлен флаг удаления старых файлов,
  // то удаляем старые файлы и сбрасываем флаг
  if (delete_old_stuff_flag == 1) 
  {
    delete_old_stuff();
    delete_old_stuff_flag = 0;
  }
  
  start_record_2nd_opinion = start_record_1st_opinion;
  start_record_1st_opinion = digitalRead(12);

  if (do_the_reindex) 
  {
    done_the_reindex = false;
    do_the_reindex = false;
    re_index ( file_to_read, file_to_write );
    //re_index_bad ( file_to_read );
    done_the_reindex = true;
  }

  // Если прошло 10 минут, то выполняем контроль интернета
  wakeup = millis();
  if (wakeup - last_wakeup > (10  * 60 * 1000) ) 
  {
    last_wakeup = millis();
    Serial.println(" "); 
    //print_mem("---------- 10 Minute Internet Check -----------");
    time(&now);
    jpr("Текущее время: "); jpr(ctime(&now));
    if (!InternetOff ) 
    {
      // Выводим информацию по сокетам
      esp_err_t client_err;
      //struct sockaddr_in *client_list;
      size_t clients = 10;
      size_t client_count = 10;
      int    client_fds[10];
      client_err = httpd_get_client_list(camera_httpd, &client_count, client_fds);
      jpr("camera_httpd Sockets , Num = %d\n", client_count);
      for (size_t i = 0; i < client_count; i++) 
      {
        int sock = client_fds[i];
        int x = httpd_ws_get_fd_info(camera_httpd, sock) ;
        jpr("Socket %d, fd=%d, info=%d \n", i, sock, x);
        print_sock(sock);
      }
      client_err = httpd_get_client_list(stream81_httpd, &client_count, client_fds);
      jpr("stream81_httpd Sockets , Num = %d\n", client_count);
      for (size_t i = 0; i < client_count; i++) 
      {
        int sock = client_fds[i];
        //Serial.printf("%d, sock %d\n", i, sock);
        int x = httpd_ws_get_fd_info(camera_httpd, sock) ;
        jpr("Socket %d, fd=%d, info=%d \n", i, sock, x);
        print_sock(sock);
      }
      client_err = httpd_get_client_list(stream82_httpd, &client_count, client_fds);
      jpr("stream82_httpd Sockets , Num = %d\n", client_count);
      for (size_t i = 0; i < client_count; i++) 
      {
        int sock = client_fds[i];
        //Serial.printf("%d, sock %d\n", i, sock);
        int x = httpd_ws_get_fd_info(camera_httpd, sock) ;
        jpr("Socket %d, fd=%d, info=%d \n", i, sock, x);
        print_sock(sock);
      }
      //
      if (found_router) 
      {
        // Получаем IP-адрес шлюза (роутера) текущей подключённой сети Wi-Fi и
        // пингуем её. Функция возвращает IP-адрес шлюза подключённой сети Wi-Fi. 
        // Если модуль не подключён к сети, функция вернёт 0.0.0.0. 
        Serial.println("IP-адрес шлюза (роутера): "); Serial.println(WiFi.gatewayIP());
        if (Ping.ping(WiFi.gatewayIP())>0) 
        {
          jpr("Время отклика: %d/%.2f/%d ms\n", Ping.minTime(), Ping.averageTime(), Ping.maxTime());
        } 
        else 
        {
          jprln("Пинг роутера не прошел, отключается WiFi");
          WiFi.reconnect();
          delay(8000);
          if (WiFi.status() != WL_CONNECTED) 
          {
            // Подключаем локальные WiFi и создаём одну свою от контроллера
            jprln("Подключается WiFi заново");
            init_wifi();
          }
          delay(15000);
          if (WiFi.status() != WL_CONNECTED) 
          {
            jprln("Нет поключения к WiFi - перезагрузка контроллера");
            reboot_now = true;
          }
        }
        delay(1000);

        if (WiFi.status() != WL_CONNECTED) 
        {
          jprln("Отключается WiFi");
          WiFi.reconnect();
          delay(8000);

          if (WiFi.status() != WL_CONNECTED) 
          {
            // Подключаем локальные WiFi и создаём одну свою от контроллера
            jprln("Подключается WiFi заново");
            init_wifi();
          }
        }
      }
      Serial.print(_hsoftIP);  Serial.println(WiFi.softAPIP()); 
      Serial.print(_hlocalIP); Serial.println(WiFi.localIP()); 

      logfile.println(WiFi.softAPIP());
      logfile.println(WiFi.localIP());

      if (!MDNS.begin(devname)) 
      {
        jprln("Ошибка установки MDNS responder!");
      } 
      else 
      {
        jprln("mDNS responder стартовал: '%s'", devname);
      }
    }  
  } 

  // Перезагружаем контроллер если установлен флаг "Перезагрузить контроллер" 
  if (reboot_now == true) 
  {
    jprln(" \n\n\n Перезагрузка контроллера в течение 5 секунд ... \n\n\n");
    delay(5000);
    ESP.restart();
  }
  // Реагируем на команду "Остановить запись avi-файла", поступившую из браузера
  if (web_stop == true) 
  {
    // Если запись велась, то сбрасываем флаг запуска записи очередного видео-файла
    if (start_record == 1) 
    {
      start_record = 0;
      jprln("Поступила команда 'Остановить запись avi-файла'");
    }
  } 
  // Если команды на остановку записи видео нет, то реагируем на "12"-ый контакт
  else 
  {
    if (start_record == 1) 
    {
      // Если запись велась, но обе проверки дали останов записи,
      // то сбрасываем флаг запуска записи очередного видео-файла
      if (start_record_1st_opinion == 0 && start_record_2nd_opinion == 0) 
      {
        start_record = 0;
        //jprln("'Остановить запись avi-файла' по событию на 12-том контакте");
      }
    } 
    else 
    {
      // Если запись НЕ велась, но обе проверки дали запуск записи,
      // то устанавливаем флаг запуска записи очередного видео-файла
      if (start_record_1st_opinion == 1 && start_record_2nd_opinion == 1) 
      {
        start_record = 1;
        //jprln("'Запустить запись avi-файла' по событию на 12-том контакте");
      }
    }
  }
  */
}

// ******************************************************** FrameStream.ino ***
