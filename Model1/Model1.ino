// Arduino, ESP32, C/C++ *************************************** Model1.ino ***
//
//        В потоке снимать кадры и по кнопке из браузера показывать изображение 
//                                  текущего кадра в отдельных задачах FreeRTOS

// Copyright © 2026 tve                               Труфанов В.Е., 11.01.2026
static const char vernum[]="v2.3.0, 25.06.2026";  
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

// Определяем экземпляр HTTP-серверов (тип httpd_handle_t используется для создания 
// и управления веб-серверами и возвращается функцией httpd_start(). Она создаёт 
// экземпляр HTTP-сервера, выделяет память и ресурсы в зависимости от указанной 
// конфигурации и возвращает указатель на экземпляр. 
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

// Объявляем функции модуля
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

  // Показываем наличие PSRAM 
  say("PSRAM - псевдооперативная память ");
  if (psramFound()) sayln("доступна")
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
  // Трассируем 17-ые циклы
  loops++;
  if (loops % 10000 == 17) 
  {
    //Serial.printf("loops %10d\n",loops);
  }
  // Взаимодействуем с файловым менеджером
  for (int x = 0; x < 1; x++) 
  {
    filemgr.handleClient(); 
  }
  // Взаимодействуем с клиентом OTA
  if (do_the_ota) 
  {
    ArduinoOTA.handle();
  }
  // При необходимости делаем ревизию пространства SD
  if (delete_old_stuff_flag == 1) 
  {
    delete_old_stuff_flag = 0;
    delete_old_stuff();
  }
  start_record_2nd_opinion = start_record_1st_opinion;
  start_record_1st_opinion = digitalRead(12);
  // Делаем реиндексирование avi-файла
  if (do_the_reindex) 
  {
    done_the_reindex = false;
    do_the_reindex = false;
    re_index(file_to_read, file_to_write);
    //re_index_bad ( file_to_read );
    done_the_reindex = true;
  }
  // 
  wakeup = millis();
  if (wakeup - last_wakeup > (10  * 60 * 1000) ) 
  //if (wakeup - last_wakeup > (2  * 60 * 1000) ) 
  {
    last_wakeup = millis();
    // print_mem("---------- 10 Minute Internet Check -----------");
    print_mem("---------- 10 Minute Internet Check -----------");
    time(&now); say("Local time: "); sayln(ctime(&now));
    
    //if (!isWiFi) 
    //{
      //
      InternetCheck10(camera_httpd, stream81_httpd, stream82_httpd);
  }  // wakeup

  if (reboot_now == true) 
  {
    jprln(" \n\n\n Rebooting in 5 seconds... \n\n\n");
    delay(5000);
    ESP.restart();
  }

  if (web_stop == true) 
  {
    if (start_record == 1) 
    {
      start_record = 0;
      jprln("web_stop web_stop code");
    }
  } 
  else 
  {
    //jpr("first %d, second %d, web %d\n", start_record_1st_opinion, start_record_2nd_opinion, web_stop);
    if (start_record == 1) {
      if (start_record_1st_opinion == 0 && start_record_2nd_opinion == 0) 
      {
        start_record = 0;
        jprln("stopping in web_stop code");
      }
    } 
    else 
    {
      if (start_record_1st_opinion == 1 && start_record_2nd_opinion == 1) 
      {
        start_record = 1;
        jprln("starting in web_stop code");
      }
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
//                                  startCameraServer()                      //
///////////////////////////////////////////////////////////////////////////////
void startCameraServer() 
{
  // Конфигурируем CameraServer: 
  //    httpd_handle_t используется для создания и управления веб-серверами,
  // и возвращается функцией httpd_start(). Она создаёт экземпляр HTTP-сервера, 
  // выделяет память и ресурсы в зависимости от указанной конфигурации и возвращает указатель на экземпляр.
  //   httpd_config_t — структура в ESP32, которая используется для конфигурации HTTP-сервера,
  // она передаётся в вызов httpd_start — функцию, которая создаёт экземпляр HTTP-сервера, 
  // выделяет ему память и ресурсы в зависимости от заданной конфигурации. 
  //   Некоторые особенности использования: 
  // - настройка приоритета задачи и размера стека во время создания экземпляра сервера;
  // - указание портов для данных и управления (контрольный порт используется для внутренней сигнализации);
  // - настройка очереди ожидающих соединений (параметр backlog_conn) — помогает 
  // справляться с кратковременными всплесками запросов, не теряя соединения.
  //   httpd_uri_t - строит функциональность сервера на регистрации URI-обработчиков, 
  // которые сопоставляют конкретные URI и методы HTTP с функциями. 
  //   httpd_req_t - объекты, которые получают функции-обработчики для доступа к 
  // деталям запроса. Далее обработчики используют httpd_resp_send() для отправки ответов. 
  //
  // Пример: в этом примере сервер по умолчанию слушает на порту 80 и регистрирует обработчик URI, 
  // который отправляет «Hello, world!» в ответ на запрос GET по пути /hello.
  /*
  void start_server() 
  {
    httpd_handle_t server = NULL; 
    httpd_config_t config = HTTPD_DEFAULT_CONFIG(); 
    httpd_uri_t hello_uri = 
    { 
      .uri      = "/hello", 
      .method   = HTTP_GET, 
      .handler  = hello_get_handler, 
      .user_ctx = NULL
    };
    if (httpd_start(&server, &config) == ESP_OK) 
    { 
      httpd_register_uri_handler(server, &hello_uri);  
    }  
  } 
  static esp_err_t hello_get_handler(httpd_req_t *req) 
  {
    const char* resp_str = (const char*) "Hello, world!";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
  }
  */
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();  // https://circuitlabs.net/http-https-server-implementation/
  config.max_uri_handlers = 17;                    // задали максимальное число регистрируемых обработчиков URI 
  config.stack_size = 4096 + 1024 + 1024 + 1024;   // определили размер стека задачи
  // Включаем опцию очистки наименее использующихся соединений (LRU), 
  // если достигается максимальное количество одновременных подключений 
  // клиентов (max_open_sockets). 
  config.lru_purge_enable = true;                  // включили механизм LRU-очистки старых соединений 
  // Меняем поведение сокета при закрытии с еще заполненным буфером отправки:
  // a) по умолчанию, если enable_so_linger = false, при вызове функции закрытия соединения (close()) 
  // система сразу возвращает управление. При этом операционная система в фоновом режиме пытается 
  // отправить оставшиеся данные. Соединение закрывается, даже если часть данных не была доставлена;
  // b) если enable_so_linger = true, то при вызове close() выполнение программы блокируется на время, 
  // заданное параметром linger_timeout (в секундах). В течение этого времени ОС пытается отправить все данные из буфера.
  // Если данные успешно отправлены до истечения таймаута — соединение закрывается корректно. Если таймаут истекает, 
  // соединение принудительно разрывается.
  config.enable_so_linger = true;
  config.linger_timeout = 1;
  // Оставляем умалчиваемые свойства
  /*
  config.max_open_sockets = 7;     // максимальное количество одновременных клиентских подключений (для HTTPS по умолчанию 4)
  config.keep_alive_enable = true; // вместо открытия для запроса нового TCP-соединения оставляет соединение открытым для нескольких запросов
  config.backlog_conn = 5;         // максимальное количество ожидающих подключения в очереди прослушивания.
  config.core_id = 0;              // from tskNO_AFFINITY
  */
  Serial.print("Приоритет startCameraServer: "); Serial.println(config.task_priority);

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
  // Регистрируем обработчик запроса на запись нового avi-файла: restart_handler
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
  // Регистрируем обработчик запроса на остановку записи avi-файла: stop_handler
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
    httpd_register_uri_handler(camera_httpd, &edit_uri); 
    httpd_register_uri_handler(camera_httpd, &find_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);
    httpd_register_uri_handler(camera_httpd, &reindex_uri);
    httpd_register_uri_handler(camera_httpd, &ota_uri);
  }

  Serial.println("startCameraServer стартовала");
}

void stopCameraServer() 
{
  httpd_stop(camera_httpd);
}

// ****************************************************************************
// *      Выполнить циклы фотографирования и записи avi (приоритет = 4)       *
// ****************************************************************************
void the_camera_loop (void* pvParameter) 
{
  long wait_for_cam_start;         // промежуточная точка начала ожидания камеры
  long delay_wait_for_sd_start;    // промежуточная точка начала ожидания работы с SD
  int we_are_already_stopped=0;    // 1 - "видео-запись уже остановлена"
  
  // Объявляем указатель на структуру camera_fb_t*, которая содержит данные кадра изображения 
  camera_fb_t* fb_curr=NULL;       // структура с буфером снятого кадра
  
  saymem("Cтартовала задача the_camera_loop");
  // Инициируем счетчик кадров в файле
  frame_cnt = 0;
  // Считываем состояние 12 контакта (начинать запись видео или нет)
  start_record_2nd_opinion = digitalRead(12);
  start_record_1st_opinion = digitalRead(12);
  // Сбрасываем флаг записи видео (пока не начинать)
  start_record = 0;
  delay(1000);
  // Выполняем циклы задачи
  while (1) 
  {
    delay(1);
    // Правило работы блоков цикла:
    // if (frame_cnt == 0 && start_record == 0)  // do nothing
    // if (frame_cnt == 0 && start_record == 1)  // start a movie
    // if (frame_cnt > 0 && start_record == 0)   // stop the movie
    // if (frame_cnt > 0 && start_record != 0)   // another frame

    /////////////////////////////  NOTHING TO DO //////////////////////////////
    if ((frame_cnt == 0)&&(start_record == 0)) 
    {
      //sayln("NOTHING TO DO");
      if (we_are_already_stopped == 0) 
      {
        say("\nОтсоедините Pin12 от GND для того, чтобы начать запись или http://.../start\n");
      }
      we_are_already_stopped = 1;
      delay(100);
    }
    //// ----------------------------------------------------------------- //// 
    //// START A MOVIE - подготовить новый avi-файл и записать первый кадр ////
    //// ----------------------------------------------------------------- //// 
    else if ((frame_cnt == 0)&&(start_record == 1)) 
    {
      //sayln("START A MOVIE");
      // Сбрасываем флаг "видео-запись уже остановлена"
      we_are_already_stopped = 0;
      // Отмечаем время начала видео-записи
      avi_start_time = millis();
      // Отмечаем точку начала ожидания камеры
      wait_for_cam_start = millis();
      
      sayln("Началась видеозапись на %d мс. ", avi_start_time);
      sayln("Размер кадра %d, качество %d, время %d секунд\n", framesize, quality, avi_length);
      logfile.flush();

      // Отмечаем точку начала ожидания работы с SD
      delay_wait_for_sd_start = millis();
      // Пересчитываем время ожидания камеры
      wait_for_cam += millis() - wait_for_cam_start;
      // Открываем avi-файл и записываем заголовки
      start_avi();
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
      memcpy(fb_record, fb_curr_record_buf, fb_curr_record_len);   
      fb_record_time = fb_curr_record_time;
      xSemaphoreGive(baton);
      
      // Освобождаем буфер камеры и отмечаем новую точку начала ожидания камеры
      esp_camera_fb_return(fb_curr);  
      wait_for_cam_start = millis();
      
      // Пересчитываем время ожидания работы с SD и заносим кадр в видео-файл
      delay_wait_for_sd += millis() - delay_wait_for_sd_start;
      another_save_avi(fb_curr_record_buf, fb_curr_record_len);
      // Пересчитываем время ожидания камеры
      wait_for_cam += millis() - wait_for_cam_start;
      if (blinking) digitalWrite(33, frame_cnt % 2); 
    }
    /// ------------------------------------------------------------------- /// 
    /// ANOTHER FRAME AVI - снять кадр и записать не первый кадр в avi-файл ///
    /// ------------------------------------------------------------------- /// 
    else if (frame_cnt > 0 && start_record == 1) 
    { 
      //sayln("ANOTHER FRAME AVI");
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
      memcpy(fb_record, fb_curr_record_buf, fb_curr_record_len); 
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
          if ( (Lots_of_Stats && frame_cnt < 1011) || (Lots_of_Stats && frame_cnt % 1000 == 10)) 
          {
            sayln("Всего: %5d кадров за %6.1f секунд, среднее недавних 100 кадров: размер и частота %6.1f kb, %.2f fps", frame_cnt, 0.001 * (millis() - avi_start_time), 1.0 / 1024  * most_recent_avg_framesize, most_recent_fps);
          }
          bytes_before_last_100_frames = movi_size;
          time_before_last_100_frames = millis();
        }
      }
    }
    ////////////////////// ------------------------------------ ///////////////
    ////////////////////// END THE MOVIE - завершить запись avi ///////////////
    ////////////////////// ------------------------------------ ///////////////
    else if (restart_now || reboot_now || (frame_cnt > 0 && start_record == 0) ||  millis() > (avi_start_time + avi_length * 1000)) 
    {
      //sayln("END THE MOVIE");
      restart_now = false;
      if (blinking) digitalWrite(33, frame_cnt % 2);
      end_avi();                                
      if (blinking) digitalWrite(33, HIGH); // light off
      // Устанавливаем флаг "удалить старые файлы по завершению записи текущего файла avi"
      delete_old_stuff_flag = 1;
      delay(50);
      avi_end_time = millis();
      //
      float fps = 1.0 * frame_cnt / ((avi_end_time - avi_start_time) / 1000);
      sayln("End the avi at %d.  It was %d frames, %d ms at %.2f fps...\n", millis(), frame_cnt, avi_end_time, avi_end_time - avi_start_time, fps);
      // Инициируем запись нового файла в следующем цикле
      if (!reboot_now) frame_cnt = 0;             
    } 
    else say("Нужно разобраться, почему сюда вышли?");
  }
}

// ************************************************************* Model1.ino ***
