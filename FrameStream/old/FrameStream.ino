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

void setup() 
{

  pinMode(33, OUTPUT);              // little red led on back of chip
  digitalWrite(33, LOW);            // turn on the red LED on the back of chip
  pinMode(4, OUTPUT);               // Blinding Disk-Avtive Light
  digitalWrite(4, LOW);             // turn off
  //pinMode(16, INPUT_PULLUP);      // контакт датчика движения

  // Определяем и показываем причину последнего сброса (reset reason). 
  esp_reset_reason_t reason = esp_reset_reason();
  say("Причина перезагрузки: ");
  switch (reason) 
  {
    case ESP_RST_UNKNOWN : sayln("ESP_RST_UNKNOWN");  break;
    case ESP_RST_POWERON : sayln("ESP_RST_POWERON"); break;
    case ESP_RST_EXT : sayln("ESP_RST_EXT");  break;
    case ESP_RST_SW : sayln("ESP_RST_SW");  break;
    case ESP_RST_PANIC : sayln("ESP_RST_PANIC");  break;
    case ESP_RST_INT_WDT : sayln("ESP_RST_INT_WDT");  break;
    case ESP_RST_TASK_WDT : sayln("ESP_RST_TASK_WDT");  break;
    case ESP_RST_WDT : sayln("ESP_RST_WDT");  break;
    case ESP_RST_DEEPSLEEP : sayln("ESP_RST_DEEPSLEEP");  break;
    case ESP_RST_BROWNOUT : sayln("ESP_RST_BROWNOUT");  break;
    case ESP_RST_SDIO : sayln("ESP_RST_SDIO");  break;
    default  : sayln("не определена!"); break;
  }
  // Показываем состояние памяти 
  say("PSRAM - псевдооперативная память ");
  if (psramFound()) sayln("доступна");
  else say("ОТКЛЮЧЕНА или ОТСУТСТВУЕТ");
  saymem("MEM - В начале SETUP");
  
  /* 
  RTC_CNTL_BROWN_OUT_REG — регистр в микроконтроллере ESP32, который отключает защиту от пониженного напряжения (brownout). 
  Этот регистр содержится в файле soc/rtc_cntl_reg.h. Детектор brownout проверяет напряжение питания и сбрасывает процессор, 
  если оно ниже определённого порога. Это сделано, чтобы сохранить содержимое памяти и избежать разрушения. 
    Некоторые причины, по которым может срабатывать детектор:
  недостаточное питание (например, из-за плохого качества USB-кабеля или проблем с портом компьютера); 
  внезапная высокая нагрузка (например, при включении питания для датчика, который потребляет много тока); 
  проблемы с USB-портом компьютера, который не может обеспечить достаточно питания;
  неисправность ESP32;
  неправильная проводка компонентов в цепи, влияющая на питание.
  Сообщение «Brownout detector was triggered» в Serial Monitor появляется, когда детектор срабатывает. 
    Настройка. Чтобы отключить защиту от пониженного напряжения, нужно: 
  - включить файлы: #include "soc/soc.h" и #include "soc/rtc_cntl_reg.h".
  - В функции setup() добавить строку: WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0). 
  Это говорит ESP32 прекратить проверку на недостаточное питание и работать с тем, что есть.
  Важно: отключение защиты от пониженного напряжения может не устранить другие ошибки, например, 
  «Guru Mediation…». Также в некоторых случаях детектор brownout не работает правильно в ранних версиях ESP32 — 
  в этом случае нужно использовать версии, заканчивающиеся на «E» (например, ESP32-WROOM-32E). 
  Рекомендуется: вместо отключения защиты от пониженного напряжения лучше использовать адекватный источник питания. 
  //uint32_t brown_reg_temp = READ_PERI_REG(RTC_CNTL_BROWN_OUT_REG);
  //Serial.printf("Brownout was %d\n", brown_reg_temp);
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  */
  
  // Подключаем WiFi
  saymem("MEM - перед подключением WiFi");
  iniLocalWiFi();
  if (!isLocalWiFi) 
  {
    onEventWiFi();   // включили заглушку на событие с WiFi=201
  }
  iniWiFiPsNone();   // отключили режим энергосбережения 
  saymem("МЕМ - после подключения к WiFi");
  // Инициализируем SD-карту
  isSD=init_sdcard();
  if (isSD) logfile = SD_MMC.open("/boot.txt", FILE_WRITE);

  // Конфигурируем камеру и если неудача, то перезагружаем контроллер
  if (!config_camera()) blinkRestart();
  // По имени камеры назначаем имя устройства 
  cname.toCharArray(devname,cname.length()+1);

  // Запускаем продолжение нумерации файлов avi 
  // (или инициируем новую нумерацию)
  do_eprom_read();
  
  /*
  */
  //saymem("MEM - после выделения памяти кадрам потока");
  
  // Запускаем задачи
  saymem("MEM - перед запуском Web-сервисов");
  startCameraServer();
  start_Stream_81_server();
  start_Stream_82_server();

  //jprln("Проверяется SD-карта на наличие свободного места ...");
  //delete_old_stuff();

  char logname[60];
  char the_directory[50];

  sprintf(the_directory, "/%s%03d",  devname, file_group);
  SD_MMC.mkdir(the_directory);

  sprintf(logname, "/%s%03d/%s%03d.999.txt",  devname, file_group, devname, file_group);
  //jprln("Создается logfile %s\n", logname);
  if (logfile) logfile.close();
  logfile = SD_MMC.open(logname, FILE_WRITE);
  if (!logfile) 
  {
    Serial.println("Ошибка открытия logfile для записи");
  }
  const char *strdate = ctime(&now);
  //logfile.println(strdate);
  digitalWrite(33, HIGH);         // red light turns off when setup is complete

  // Показываем установленные настройки камеры и видео
  sayconfig(); 
  saymem("МЕМ - после завершения setup");
}

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
