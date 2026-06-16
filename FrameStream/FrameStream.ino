// Arduino, ESP32, C/C++ ********************************** FrameStream.ino ***
//
// Формирование потока изображений, постоянной записи фрагментов видео на 
// SD-диск или при возникновении движения, а также передача снимков и фрагментов 
// видео через сокеты на сайт 

// Copyright © 2026 tve                               Труфанов В.Е., 11.01.2026
static const char vernum[]="v2.2.5, 15.06.2026";  
/** 
 * Modify by James Zahary Sep 12, 2020 - jamzah.plc@gmail.com
 * 
 * По версии https://github.com/jameszah/ESP32-CAM-Video-Recorder,
 * которая включает работу с Wi-Fi, потоковым видео, управлением по http, 
 * через telegram, pir-контроль, сенсорное управление, загрузка по ftp и другое.
 * 
 * Программа записывает видео в формате mjpeg avi на sd-карту ESP 32-CAM. 
 * По умолчанию файлы имеют такие названия, как: desklens001.003.avi
 * "desklens" - имя определяемое разработчиком,
 * 001 - это число, сохраненное в eprom, которое будет увеличиваться при каждой загрузке устройства
 * 003 - это третий файл, созданный во время текущей загрузки
 * 
 * Arduino IDE 2.3.7 - 1.8.18
 * Esp32 от Espressif Systems версии 3.3.8
 * Payment:           "Al Thinker ESP32-CAM"
 * CPU Frequency:     "240MHz (WiFi/BT)"
 * Flash Frequency:   "80MHz"
 * Flash Mode:        "QIO"
 * Partition Scheme:  "Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)
**/

#include "esp_http_server.h"
#include "esp_camera.h"
#include "sensor.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "soc/soc.h"
#include "esp_cpu.h" 
#include "soc/rtc_cntl_reg.h"
#include <ESPping.h>
#include "lwip/sockets.h"
//#include <esp32-hal-psram.h>

#include <HTTPClient.h>
httpd_handle_t camera_httpd = NULL;

// Объявить/проинициализировать переменные, общие для всех модулей
#include "inimem.h"
// Обслужить работу с данными из постоянной памяти
#include "fs_eprom.h"
// Обеспечить работу, связанную с wifi
#include "fs_wifi.h"
// Обслужить работу с SD-картой
#include "fs_sd.h"
// Обеспечить вывод сообщений в последовательный порт и запись лог-файла на SD-карту
#include "fs_trass.h"
// Настроить видео-камеру и обеспечить снятие потока изображений
#include "fs_camera.h"
// Обеспечить передачу потоков изображений от контроллера по локальной и собственной сети контроллера ESP32-CAM
#include "fs_stream.h"
// Сформировать avi-файлы из снятых потоков изображений на SD-карту 
#include "fs_avi.h"
// Управлять контроллером ESP32-CAM через браузер по локальной сети и собственной сети контроллера 
#include "fs_server.h"

#include "FlMgr.h"          
ESPxWebFlMgr filemgr(filemanagerport); // we want a different port than the webserver

void startCameraServer(); 
void stopCameraServer(); 
void the_camera_loop (void* pvParameter); 

bool isWiFi = false;           // false - WiFi отсутствует
long last_wakeup = 0;          // временная метка начала отсчета до проверки интернета
long wakeup;                   // текущая метка для отсчета 10 минут работы до проверки интернета
int loops = 0;                 // текущий номер фонового цикла 

TaskHandle_t the_camera_loop_task;
TaskHandle_t the_streaming_loop_task;

void setup() 
{
  Serial.begin(115200);
  delay(3000);
  saymem("Начало setup");

  Serial.println("\n\n");
  Serial.println("---------------------------------------");
  Serial.println("Arduino IDE 1.8.18 - Espressif ESP32 3.3.8");
  String idfver = esp_get_idf_version();
  Serial.println("ESP IDF: "+idfver);
  Serial.print("FrameStream "); Serial.println(vernum);
  Serial.println("---------------------------------------");

  pinMode(33, OUTPUT);             // little red led on back of chip
  digitalWrite(33, LOW);           // turn on the red LED on the back of chip
  pinMode(4, OUTPUT);               // Blinding Disk-Avtive Light
  digitalWrite(4, LOW);             // turn off
  pinMode(12, INPUT_PULLUP);        // pull this down to stop recording
  pinMode(13, INPUT_PULLUP);        // pull this down switch wifi

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

  // Показываем надичие PSRAM 
  say("PSRAM - псевдооперативная память ");
  if (psramFound()) sayln("доступна");
  else sayln("ОТКЛЮЧЕНА или ОТСУТСТВУЕТ");
  sayln("");
  
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

  // Инициируем SD-карту
  sayln("Инициирование SD-карты ...");
  esp_err_t card_err = init_sdcard();
  if (card_err != ESP_OK) 
  {
    sayln("Ошибка инициирования SD-карты 0x%x", card_err);
    blinkRestart();
    return;
  } 
  else 
  {
    logfile = SD_MMC.open("/frstream.txt", FILE_WRITE);
  }

  // Запускаем продолжение нумерации файлов avi 
  // (или инициируем новую нумерацию)
  do_eprom_read();
  // Считываем файл конфигурации и настраиваем параметры
  sayln("Считывание файла конфигурации и настройка параметров ...");
  read_config_file();
  // Конфигурируем камеру
  sayln("Конфигурирование камеры ...");
  config_camera();
  // Выделяем память под рабочие буферы для снятых кадров 
  // (должны быть больше больших кадров с ov2640,ov5640),
  // размер устанавливаем от ранее сформированного и расчитанного config_camera
  fb_record =          (uint8_t*)ps_malloc(frame_buffer_size); 
  fb_curr_record_buf = (uint8_t*)ps_malloc(frame_buffer_size);
  fb_streaming =       (uint8_t*)ps_malloc(frame_buffer_size); 
  fb_capture =         (uint8_t*)ps_malloc(frame_buffer_size); 
  saymem("Выделена память под кадры");
  // Инициируем мьютекс между задачами, который будет держать и передавать кадры камеры
  baton = xSemaphoreCreateMutex();

  /*
  Программное обеспечение в микропроцессоре ESP32 распределяется по ядрам (CPU0 и CPU1) 
  с помощью встроенного программного обеспечения FreeRTOS — операционной системы реального времени. 
  ESP32 — двухъядерный микроконтроллер, и задачи могут выполняться независимо на обоих ядрах. 
    Пример распределения задач: 
    Ядро 0 выполняет задачу loopCore0 — захват изображения и сетевое взаимодействие 
  (кадры с камеры отправляются по HTTP, обмен данными — по WebSocket).
    Ядро 1 занято задачей loopCore1 — навигацией и управлением (опрашивает датчики, 
  фильтрует данные, вычисляет ошибки и корректирует движение).
    Важно: по умолчанию код, загруженный в ESP32 с помощью Arduino IDE, выполняется только на ядре 1, 
    поскольку ядро 0 уже запрограммировано для радиочастотной связи. 
  При распределении задач по ядрам могут возникнуть, например:
  - ошибка таймаута Watchdog — если код для задачи не содержит задержки (например, 
  бесконечный цикл без задержки). Решение: добавить задержку (delay(1) или vTaskDelay(1));
  - переполнение стека — если для задачи выделено мало стека. Решение: увеличить размер 
  стека задачи или изменить большие массивы на динамическое выделение;
  - конкуренция задач за таймер — если в обе задачи добавлены задержки (например, delay(1)), 
  это может привести к перезапускам. Решение: использовать разные таймеры для каждой задачи;
  - рекомендуется использовать неблокирующий подход, например, вместо функции delay() применять millis().
  Она не останавливает программу, а возвращает количество миллисекунд, прошедших с момента запуска ESP32, 
  что позволяет организовать выполнение задач по расписанию без блокировки основного цикла loop();
  - нельзя блокировать задачу Idle. Она (приоритет 0) отвечает за очистку фона, поэтому не стоит блокировать 
  её с помощью интенсивных циклов.  
  https://www.teachmemicro.com/multitask-with-esp32-and-freertos/
  https://www.iotsharing.com/2017/06/arduino-esp32-freertos-how-to-use-task-param-task-priority-task-handle.html
  Задача Idle — это задача во FreeRTOS для ESP32, которая выполняется, когда другие задачи не готовы к выполнению. 
  Idle создаются автоматически для каждого ядра процессора (называются «IDLE0» и «IDLE1»). 
  Основная задача Idle - очистка памяти, выделенной ядром задачам, которые были удалены. 
  */

  // Создаем задачи на ядрах контроллера
  sayln("Создание задач на ядрах контроллера ...");

  xTaskCreatePinnedToCore(
    the_camera_loop,       // TaskFunction_t pvTaskCode          - имя функции, которая содержит код
    "the_camera_loop",     // const char * const pcName          - имя задачи
    5000,                  // const uint32_t usStackDepth        - количество байт, выделенное для стека задачи
    NULL,                  // void * const pvParameters          - указатель на параметры для задачи
    4,                     // UBaseType_t uxPriority             - приоритет задачи
    &the_camera_loop_task, // TaskHandle_t * const pxCreatedTask - указатель на задачу, который можно использовать для ссылки на задачу позже (например, для её завершения)
    0                      // const BaseType_t xCoreID           - ядро процессора, на которое нужно назначить задачу (0 для ядра 0, 1 для 1 или tskNO_AFFINITY - на обоих ядрах
  ); 
  delay(100);
  xTaskCreate(
    the_streaming_loop,    // TaskFunction_t pvTaskCode          - имя функции, которая содержит код
    "the_streaming_loop",  // const char * const pcName          - имя задачи
    8000,                  // const uint32_t usStackDepth        - количество байт, выделенное для стека задачи
    NULL,                  // void * const pvParameters          - указатель на параметры для задачи 
    2,                     // UBaseType_t uxPriority             - приоритет задачи
    &the_streaming_loop_task
  );
  if (the_streaming_loop_task == NULL) 
  {
    //vTaskDelete( xHandle );
    sayln("Не удалось запустить задачу do_the_steaming_task! %d\n", the_streaming_loop_task);
  }
  
  // Подключаемся к WiFi если еще нет подключения
  if (!isWiFi) 
  {
    sayln("Подключение к WiFi ...");
    init_wifi();
    saymem("После подключения к WiFi");
    
    sayln("Запуск файлового менеджера ...");
    filemgr.begin();
    filemgr.setBackGroundColor("Gray");
    say("Filemanager по http://"); Serial.print(WiFi.softAPIP()); sayln(":%d/", filemanagerport);
    logfile.print(WiFi.softAPIP());
    say("Filemanager по http://"); Serial.print(WiFi.localIP());  sayln(":%d/", filemanagerport);
    logfile.print(WiFi.localIP());
    saymem("После загрузки FlMgr");

    sayln("Запуск потоков задач ...");
    startCameraServer();
    start_Stream_81_server();
    start_Stream_82_server();
    saymem("После запуска потоков");

    isWiFi = true;
  }

  sayln("Ревизия доступного пространства SD ...");
  delete_old_stuff();

  // Начинаем запись нового лог-файла
  char logname[60];
  char the_directory[50];
  sprintf(the_directory, "/%s%03d",  devname, file_group);
  SD_MMC.mkdir(the_directory);
  sprintf(logname, "/%s%03d/%s%03d.999.txt",  devname, file_group, devname, file_group);
  sayln("Создание лог-файла %s ...", logname);
  if (logfile) 
  {
    logfile.close();
  }
  logfile = SD_MMC.open(logname, FILE_WRITE);
  if (!logfile) 
  {
    sayln("Ошибка открытия лог-файла для записи");
  }

  boot_time = millis();
  const char *strdate = ctime(&now);
  logfile.println(strdate);
  digitalWrite(33, HIGH);         // red light turns off when setup is complete

  // Показываем установленные настройки камеры и видео
  sayconfig(); 
  saymem("Завершение setup");
}

// loop() - loop runs at low prio, so I had to move it to the task the_camera_loop at higher priority
void loop() 
{
  long run_time = millis() - boot_time;
  loops++;
  if (loops % 10000 == 17) {
    //Serial.printf("loops %10d\n",loops);
  }

  for (int x = 0; x < 1; x++) {
    filemgr.handleClient();  //soc.6
  }

  if (do_the_ota) {
    ArduinoOTA.handle();
  }

  if (delete_old_stuff_flag == 1) {
    delete_old_stuff_flag = 0;
    delete_old_stuff();
  }
  start_record_2nd_opinion = start_record_1st_opinion;
  start_record_1st_opinion = digitalRead(12);

  if (do_the_reindex) {
    done_the_reindex = false;
    do_the_reindex = false;
    re_index ( file_to_read, file_to_write );
    //re_index_bad ( file_to_read );
    done_the_reindex = true;
  }

  wakeup = millis();
  if (wakeup - last_wakeup > (10  * 60 * 1000) ) 
  {
    last_wakeup = millis();
    print_mem("---------- 10 Minute Internet Check -----------\n");
    time(&now);
    jpr("Local time: "); jpr(ctime(&now));
    
    if (!isWiFi ) 
    {
      esp_err_t client_err;
      struct sockaddr_in *client_list;
      size_t clients = 10;
      size_t client_count = 10;
      int    client_fds[10];

      client_err = httpd_get_client_list(camera_httpd, &client_count, client_fds);
      jpr("camera_httpd Sockets , Num = %d\n", client_count);
      for (size_t i = 0; i < client_count; i++) {
        int sock = client_fds[i];
        int x = httpd_ws_get_fd_info(camera_httpd, sock) ;
        jpr("Socket %d, fd=%d, info=%d \n", i, sock, x);
        print_sock(sock);
      }

      client_err = httpd_get_client_list(stream81_httpd, &client_count, client_fds);
      jpr("stream81_httpd Sockets , Num = %d\n", client_count);
      for (size_t i = 0; i < client_count; i++) {
        int sock = client_fds[i];
        //Serial.printf("%d, sock %d\n", i, sock);
        int x = httpd_ws_get_fd_info(camera_httpd, sock) ;
        jpr("Socket %d, fd=%d, info=%d \n", i, sock, x);
        print_sock(sock);
      }
      client_err = httpd_get_client_list(stream82_httpd, &client_count, client_fds);
      jpr("stream82_httpd Sockets , Num = %d\n", client_count);
      for (size_t i = 0; i < client_count; i++) {
        int sock = client_fds[i];
        //Serial.printf("%d, sock %d\n", i, sock);
        int x = httpd_ws_get_fd_info(camera_httpd, sock) ;
        jpr("Socket %d, fd=%d, info=%d \n", i, sock, x);
        print_sock(sock);
      }

      if (found_router) {
        // Ping local IP
        Serial.println(WiFi.gatewayIP());
        if (Ping.ping(WiFi.gatewayIP()) > 0) {
          jpr(" -- response time : %d/%.2f/%d ms\n", Ping.minTime(), Ping.averageTime(), Ping.maxTime());
        } else {

          jprln("\n\nCannot Ping the gateway - REBOOT");
          jprln("***** WiFi reconnect *****");
          WiFi.reconnect();
          delay(8000);
          if (WiFi.status() != WL_CONNECTED) {
            jprln("***** WiFi restart *****");
            init_wifi();
          }
          delay(15000);
          if (WiFi.status() != WL_CONNECTED) {
            jprln("***** Reboot *****");
            reboot_now = true;
          }

        }
        delay(1000);

        // Ping Host
        const char* remote_host = "google.com";
        jpr(remote_host);
        if (Ping.ping(remote_host) > 0) {
          jpr(" -- response time : %d/%.2f/%d ms\n", Ping.minTime(), Ping.averageTime(), Ping.maxTime());
        } else {
          jprln(" Ping Error !");
        }
        delay(1000);


        if (WiFi.status() != WL_CONNECTED) {

          jprln("***** WiFi reconnect *****");
          WiFi.reconnect();
          delay(8000);

          if (WiFi.status() != WL_CONNECTED) {
            jprln("***** WiFi restart *****");
            init_wifi();
          }
        }
      }

      Serial.println(WiFi.softAPIP());  logfile.println(WiFi.softAPIP());
      Serial.println(WiFi.localIP()); logfile.println(WiFi.localIP());

      if (!MDNS.begin(devname)) {
        jprln("Error setting up MDNS responder!");
      } else {
        jpr("mDNS responder started '%s'\n", devname);
      }
    }  // not internet off
  }  // wakeup

  if (reboot_now == true) {
    jprln(" \n\n\n Rebooting in 5 seconds... \n\n\n");
    delay(5000);
    ESP.restart();
  }

  if (web_stop == true) {
    if (start_record == 1) {
      start_record = 0;
      jprln("web_stop web_stop code");
    }
  } else {
    //jpr("first %d, second %d, web %d\n", start_record_1st_opinion, start_record_2nd_opinion, web_stop);
    if (start_record == 1) {
      if (start_record_1st_opinion == 0 && start_record_2nd_opinion == 0) {
        start_record = 0;
        jprln("stopping in web_stop code");
      }
    } else {
      if (start_record_1st_opinion == 1 && start_record_2nd_opinion == 1) {
        start_record = 1;
        jprln("starting in web_stop code");
      }
    }
  }
}

void startCameraServer() 
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 17; //61.3 from 12
  config.stack_size = 4096 + 1024 + 1024 + 1024;
  config.lru_purge_enable = true;
  //61 config.enable_so_linger = true;
  //61 config.linger_timeout = 1;
  //61 config.keep_alive_enable = true;
  //config.enable_so_linger = true;
  //61 config.max_open_sockets   = 10;
  //61 config.backlog_conn       = 10; //from def of 5
  //61 config.core_id = 0; // from tskNO_AFFINITY

  Serial.print("http task prio: "); Serial.println(config.task_priority);

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t capture_uri = {
    .uri       = "/capture",
    .method    = HTTP_GET,
    .handler   = capture_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t photos_uri = {
    .uri       = "/photos",
    .method    = HTTP_GET,
    .handler   = photos_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t fphotos_uri = {
    .uri       = "/fphotos",
    .method    = HTTP_GET,
    .handler   = fphotos_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t sphotos_uri = {
    .uri       = "/sphotos",
    .method    = HTTP_GET,
    .handler   = sphotos_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t reboot_uri = {
    .uri       = "/reboot",
    .method    = HTTP_GET,
    .handler   = reboot_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t restart_uri = {
    .uri       = "/restart",
    .method    = HTTP_GET,
    .handler   = restart_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t time_uri = {
    .uri       = "/time",
    .method    = HTTP_GET,
    .handler   = time_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t start_uri = {
    .uri       = "/start",
    .method    = HTTP_GET,
    .handler   = start_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t stop_uri = {
    .uri       = "/stop",
    .method    = HTTP_GET,
    .handler   = stop_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t find_uri = {
    .uri       = "/find",
    .method    = HTTP_GET,
    .handler   = find_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t status_uri = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = status_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t delete_uri = {
      .uri       = "/delete",
      .method    = HTTP_GET,
      .handler   = delete_handler,
      .user_ctx  = NULL
    };
  httpd_uri_t edit_uri = {
    .uri       = "/edit",
    .method    = HTTP_GET,
    .handler   = edit_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t reindex_uri = {
    .uri       = "/reindex",
    .method    = HTTP_GET,
    .handler   = reindex_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t ota_uri = {
    .uri       = "/ota",
    .method    = HTTP_GET,
    .handler   = ota_handler,
    .user_ctx  = NULL
  };
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    httpd_register_uri_handler(camera_httpd, &photos_uri);
    httpd_register_uri_handler(camera_httpd, &fphotos_uri);
    httpd_register_uri_handler(camera_httpd, &sphotos_uri);
    httpd_register_uri_handler(camera_httpd, &reboot_uri);
    httpd_register_uri_handler(camera_httpd, &restart_uri);
    httpd_register_uri_handler(camera_httpd, &time_uri);
    httpd_register_uri_handler(camera_httpd, &start_uri);
    httpd_register_uri_handler(camera_httpd, &stop_uri);
    httpd_register_uri_handler(camera_httpd, &edit_uri); //61.3 index->camera
    httpd_register_uri_handler(camera_httpd, &find_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);
    httpd_register_uri_handler(camera_httpd, &reindex_uri);
    httpd_register_uri_handler(camera_httpd, &ota_uri);
  }

  Serial.println("Camera http started");
}

void stopCameraServer() 
{
  httpd_stop(camera_httpd);
}

// ****************************************************************************
// *               Выполнить циклы фотографирования и записи avi              *
// *                     (имеет наибольший приоритет = 4)                     *
// ****************************************************************************
void the_camera_loop (void* pvParameter) 
{
  // Объявляем указатель на структуру camera_fb_t*, которая содержит данные кадра изображения 
  camera_fb_t* fb_curr=NULL;   // структура с буфером снятого кадра
  
  // print_mem("the_camera_loop");

  frame_cnt = 0;
  start_record_2nd_opinion = digitalRead(12);
  start_record_1st_opinion = digitalRead(12);
  start_record = 0;

  delay(1000);

  while (1) 
  {
    delay(1);

    // if (frame_cnt == 0 && start_record == 0)  // do nothing
    // if (frame_cnt == 0 && start_record == 1)  // start a movie
    // if (frame_cnt > 0 && start_record == 0)   // stop the movie
    // if (frame_cnt > 0 && start_record != 0)   // another frame

    ///////////////////  NOTHING TO DO //////////////////
    if ( (frame_cnt == 0 && start_record == 0)) {

      // Serial.println("Do nothing");
      if (we_are_already_stopped == 0) jpr("\n\nDisconnect Pin 12 from GND to start recording or http://192.168.1.100/start \n\n");
      we_are_already_stopped = 1;
      delay(100);

      ///////////////////  START A MOVIE  //////////////////
    } else if (frame_cnt == 0 && start_record == 1) {

      //Serial.println("Ready to start");

      we_are_already_stopped = 0;

      avi_start_time = millis();

      jpr("\nStart the avi ... at %d\n", avi_start_time);
      jpr("Framesize %d, quality %d, length %d seconds\n\n", framesize, quality, avi_length);
      logfile.flush();

      //88 frame_cnt++;

      long wait_for_cam_start = millis();
      wait_for_cam += millis() - wait_for_cam_start;

      start_avi();

      wait_for_cam_start = millis();

      ///
      frame_cnt++;

      long delay_wait_for_sd_start = millis();

      delay_wait_for_sd += millis() - delay_wait_for_sd_start;

      fb_curr = get_good_jpeg();    //7

      fb_curr_record_len = fb_curr->len;
      memcpy(fb_curr_record_buf, fb_curr->buf, fb_curr->len);
      fb_curr_record_time = millis();

      xSemaphoreTake( baton, portMAX_DELAY );

      fb_record_len = fb_curr_record_len;
      memcpy(fb_record, fb_curr_record_buf, fb_curr_record_len);   // v59.5
      fb_record_time = fb_curr_record_time;
      xSemaphoreGive( baton );

      esp_camera_fb_return(fb_curr);  //7

      another_save_avi( fb_curr_record_buf, fb_curr_record_len );

      ///
      wait_for_cam += millis() - wait_for_cam_start;
      if (blinking) digitalWrite(33, frame_cnt % 2);                // blink

      ///////////////////  END THE MOVIE //////////////////
    } else if ( restart_now || reboot_now || (frame_cnt > 0 && start_record == 0) ||  millis() > (avi_start_time + avi_length * 1000)) { // end the avi

      jpr("End the Avi");
      restart_now = false;

      if (blinking)  digitalWrite(33, frame_cnt % 2);

      end_avi();                                // end the movie

      if (blinking) digitalWrite(33, HIGH);          // light off

      delete_old_stuff_flag = 1;
      delay(50);

      avi_end_time = millis();

      float fps = 1.0 * frame_cnt / ((avi_end_time - avi_start_time) / 1000) ;

      jpr("End the avi at %d.  It was %d frames, %d ms at %.2f fps...\n", millis(), frame_cnt, avi_end_time, avi_end_time - avi_start_time, fps);

      if (!reboot_now) frame_cnt = 0;             // start recording again on the next loop

      ///////////////////  ANOTHER FRAME  //////////////////
    } else if (frame_cnt > 0 && start_record != 0) {  // another frame of the avi

      //Serial.println("Another frame");

      current_frame_time = millis();
      if (current_frame_time - last_frame_time < frame_interval) 
      {
        delay(frame_interval - (current_frame_time - last_frame_time));             // delay for timelapse
      }
      last_frame_time = millis();

      frame_cnt++;

      long delay_wait_for_sd_start = millis();
      delay_wait_for_sd += millis() - delay_wait_for_sd_start;

      fb_curr = get_good_jpeg();    //7

      fb_curr_record_len = fb_curr->len;
      memcpy(fb_curr_record_buf, fb_curr->buf, fb_curr->len);
      fb_curr_record_time = millis();

      xSemaphoreTake( baton, portMAX_DELAY );

      fb_record_len = fb_curr_record_len;
      memcpy(fb_record, fb_curr_record_buf, fb_curr_record_len);   // v59.5
      fb_record_time = fb_curr_record_time;
      xSemaphoreGive( baton );

      esp_camera_fb_return(fb_curr);  //7

      another_save_avi( fb_curr_record_buf, fb_curr_record_len );

      long wait_for_cam_start = millis();

      wait_for_cam += millis() - wait_for_cam_start;

      if (blinking) digitalWrite(33, frame_cnt % 2);

      if (frame_cnt % 100 == 10 ) {     // print some status every 100 frames
        if (frame_cnt == 10) {
          bytes_before_last_100_frames = movi_size;
          time_before_last_100_frames = millis();
          most_recent_fps = 0;
          most_recent_avg_framesize = 0;
        } else {

          most_recent_fps = 100.0 / ((millis() - time_before_last_100_frames) / 1000.0) ;
          most_recent_avg_framesize = (movi_size - bytes_before_last_100_frames) / 100;

          if ( (Lots_of_Stats && frame_cnt < 1011) || (Lots_of_Stats && frame_cnt % 1000 == 10)) {
            jpr("So far: %04d frames, in %6.1f seconds, for last 100 frames: avg frame size %6.1f kb, %.2f fps ...\n", frame_cnt, 0.001 * (millis() - avi_start_time), 1.0 / 1024  * most_recent_avg_framesize, most_recent_fps);
          }

          //total_delay = 0;

          bytes_before_last_100_frames = movi_size;
          time_before_last_100_frames = millis();
        }
      }
    }
  }
}
/*
void the_camera_loop (void* pvParameter) 
{
  long wait_for_cam_start;       // Промежуточная точка начала ожидания камеры
  long delay_wait_for_sd_start;  // Промежуточная точка начала ожидания работы с SD

  //print_mem("MEM - стартовала задача the_camera_loop        ");
  // Инициируем счетчик кадров в файле
  frame_cnt = 0;
  // Считываем состояние 12 контакта (начинать запись видео или нет)
  start_record_2nd_opinion = digitalRead(12);
  start_record_1st_opinion = digitalRead(12);
  // Сбрасываем флаг записи видео (пока не начинать)
  start_record = 0;
  delay(1000);

  while (1) 
  {
    delay(1);

    // if (frame_cnt == 0 && start_record == 0)  // do nothing
    // if (frame_cnt == 0 && start_record == 1)  // start a movie
    // if (frame_cnt > 0 && start_record == 0)   // stop the movie
    // if (frame_cnt > 0 && start_record != 0)   // another frame

    ///////////////////  NOTHING TO DO //////////////////
    if ( (frame_cnt == 0 && start_record == 0)) 
    {
      // Serial.println("Do nothing");
      if (we_are_already_stopped == 0) 
      {
        //jpr("\nОтсоедините Pin12 от GND для того, чтобы начать запись или http://192.168.1.100/start \n");
      }
      we_are_already_stopped = 1;
      delay(100);
    } 
    ///////////////////  START A MOVIE  //////////////////
    else if (frame_cnt == 0 && start_record == 1) 
    {
      // Сбрасываем флаг "видео-запись уже остановлена"
      we_are_already_stopped = 0;
      // Отмечаем время начала видео-записи
      avi_start_time = millis();
      // Отмечаем точку начала ожидания камеры
      wait_for_cam_start = millis();
      
      //jprln("Началась видеозапись на %d мс. ", avi_start_time);
      //jprln("Размер кадра %d, качество %d, время %d секунд\n", framesize, quality, avi_length);
      logfile.flush();

      // Открываем avi-файл и записываем заголовки
      start_avi();
      // Отмечаем точку начала ожидания работы с SD
      delay_wait_for_sd_start = millis();
      // Пересчитываем время ожидания камеры
      wait_for_cam += millis() - wait_for_cam_start;
      // Меняем счетчик и делаем кадр
      frame_cnt++;
      fb_curr = get_good_jpeg();    
      // Копируем изображение в буфер текущего кадра
      fb_curr_record_len = fb_curr->len;
      memcpy(fb_curr_record_buf, fb_curr->buf, fb_curr->len);
      fb_curr_record_time = millis();
      
      // В мьютексе выбираем в буфер снятый кадр и отмечаем время
      xSemaphoreTake(baton, portMAX_DELAY);
      fb_record_len = fb_curr_record_len;
      memcpy(fb_record, fb_curr_record_buf, fb_curr_record_len);   // v59.5
      fb_record_time = fb_curr_record_time;
      xSemaphoreGive(baton);
      
      // Освобождаем буфер камеры и отмечаем новую точку начала ожидания камеры
      esp_camera_fb_return(fb_curr);  //7
      wait_for_cam_start = millis();
      // Пересчитываем время ожидания работы с SD и заносим кадр в видео-файл
      delay_wait_for_sd += millis() - delay_wait_for_sd_start;
      another_save_avi(fb_curr_record_buf, fb_curr_record_len);
      // Пересчитываем время ожидания камеры
      wait_for_cam += millis() - wait_for_cam_start;
      if (blinking) digitalWrite(33, frame_cnt % 2); // blink
    } 
    ///////////////////  END THE MOVIE //////////////////
    else if (restart_now || reboot_now || (frame_cnt > 0 && start_record == 0) ||  millis() > (avi_start_time + avi_length * 1000)) 
    { 
      //jprln("Завершается запись avi-файла");
      restart_now = false;
      if (blinking)  digitalWrite(33, frame_cnt % 2);

      end_avi();                                

      if (blinking) digitalWrite(33, HIGH);          // light off

      // Устанавливаем флаг "удалить старые файлы по завершению записи текущего файла avi"
      delete_old_stuff_flag = 1;
      delay(50);

      avi_end_time = millis();

      float fps = 1.0 * frame_cnt / ((avi_end_time - avi_start_time) / 1000) ;

      //jpr("End the avi at %d.  It was %d frames, %d ms at %.2f fps...\n", millis(), frame_cnt, avi_end_time, avi_end_time - avi_start_time, fps);

      if (!reboot_now) frame_cnt = 0;             // start recording again on the next loop

    } 
    ///////////////////  ANOTHER FRAME AVI  //////////////////
    else if (frame_cnt > 0 && start_record != 0) 
    { 
      // Если интервал между кадрами не истек, делаем паузу (timelapse)
      current_frame_time = millis();
      if (current_frame_time - last_frame_time < frame_interval) 
      {
        delay(frame_interval - (current_frame_time - last_frame_time));     
      }
      last_frame_time = millis();
      // Отмечаем точку начала ожидания работы с SD
      delay_wait_for_sd_start = millis();
      // Меняем счетчик и делаем кадр
      frame_cnt++;
      fb_curr = get_good_jpeg();   
      // Копируем изображение в буфер текущего кадра
      fb_curr_record_len = fb_curr->len;
      memcpy(fb_curr_record_buf, fb_curr->buf, fb_curr->len);
      fb_curr_record_time = millis();
      
      // В мьютексе выбираем в буфер снятый кадр и отмечаем время
      xSemaphoreTake(baton, portMAX_DELAY);
      fb_record_len = fb_curr_record_len;
      memcpy(fb_record, fb_curr_record_buf, fb_curr_record_len);   // v59.5
      fb_record_time = fb_curr_record_time;
      xSemaphoreGive(baton);
      
      // Освобождаем буфер камеры и отмечаем новую точку начала ожидания камеры
      esp_camera_fb_return(fb_curr);  //7
      wait_for_cam_start = millis();

      // Пересчитываем время ожидания работы с SD и заносим кадр в видео-файл
      delay_wait_for_sd += millis() - delay_wait_for_sd_start;
      another_save_avi(fb_curr_record_buf, fb_curr_record_len);
      // Пересчитываем время ожидания камеры
      wait_for_cam += millis() - wait_for_cam_start;
      if (blinking) digitalWrite(33, frame_cnt % 2);
      
      // Выводим усредненные статистические данные через каждые 100 кадров,
      // начиная с 10-ого (среди первых 1011 кадров)
      if (frame_cnt % 100 == 10 ) 
      {    
        if (frame_cnt == 10) 
        {
          bytes_before_last_100_frames = movi_size;
          time_before_last_100_frames = millis();
          most_recent_fps = 0;            // количество недавних кадров в секунду
          most_recent_avg_framesize = 0;  // средний размер недавних кадров
        } 
        else 
        {
          most_recent_fps = 100.0 / ((millis() - time_before_last_100_frames) / 1000.0) ;
          most_recent_avg_framesize = (movi_size - bytes_before_last_100_frames) / 100;

          //if ( (Lots_of_Stats && frame_cnt < 1011) || (Lots_of_Stats && frame_cnt % 1000 == 10)) 
          //{
            //jprln("Всего: %5d кадров за %6.1f секунд, среднее недавних 100 кадров: размер и частота %6.1f kb, %.2f fps", frame_cnt, 0.001 * (millis() - avi_start_time), 1.0 / 1024  * most_recent_avg_framesize, most_recent_fps);
          //}
          bytes_before_last_100_frames = movi_size;
          time_before_last_100_frames = millis();
        }
      }
    }
  }
}
*/

// ******************************************************** FrameStream.ino ***
