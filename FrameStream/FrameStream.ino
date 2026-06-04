// Arduino, ESP32, C/C++ ********************************** FrameStream.ino ***
//
// Формирование потока изображений, постоянной записи фрагментов видео на 
// SD-диск или при возникновении движения, а также передача снимков и фрагментов 
// видео через сокеты на сайт KwinFlat 

// Copyright © 2026 tve                               Труфанов В.Е., 11.01.2026
static const char vernum[]="v2.1.0, 24.02.2026";  
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
// ****************************************************************************
// *       Подключить локальные WiFi и создать одну свою от контроллера       *
// ****************************************************************************
bool init_wifi() 
{
  // Устанавливаем режим работы WiFi, как станции (STA). В этом режиме контроллер 
  // не создаёт собственную сеть, а подключается к уже существующей сети WiFi, 
  // например, к локальной сети (роутеру или иному «раздающему» устройству). 
  //
  // В режиме STA ESP32 действует как другое клиентское устройство в сети (например, ноутбук или смартфон). 
  // Его основная цель — обнаружить и подключиться к точке доступа (AP) с помощью SSID (имени сети) и пароля (обычно Pre-Shared Key или PSK). 
  // После подключения ESP32 получает IP-адрес от AP (обычно через DHCP от роутера) 
  // и может общаться с другими устройствами в локальной сети и, если AP предоставляет это, получать доступ в интернет.
  //
  // Важно: подключение к Wi-Fi не является мгновенным, поэтому необходимо регулярно проверять статус соединения 
  // с помощью функции WiFi.status(). После успешного подключения функция возвращает WL_CONNECTED. 
  WiFi.mode(WIFI_STA);
  
  // Настраиваем имя, которое клиент DHCP использует для идентификации устройства
  // (в типичной сетевой настройке это имя отображается в списке устройств маршрутизатора Wi-Fi.
  // По умолчанию имя ESP32 — «espressif». С помощью этой функции можно изменить стандартное имя, 
  // например, для дифференциации устройств в режиме мягкого доступа к точке доступа. 
  // Аргумент - это строка, которая содержит новое имя. Она должна быть не длиннее 
  // 32 символов, содержать только буквы, цифры и символ «-»). 

  // Важно: функцию нужно вызвать до начала Wi-Fi с помощью WiFi.begin(), WiFi.softAP(), 
  // WiFi.mode() или WiFi.run(). Чтобы изменить имя, можно сбросить Wi-Fi с помощью WiFi.mode(WIFI_MODE_NULL), 
  // затем вызвать WiFi.setHostname() и перезагрузить Wi-Fi с нуля. 
  // Если hostname не указан, будет назначено стандартное имя на основе типа чипа и MAC-адреса. 
  WiFi.setHostname(devname);
  // Резервируем место для параметров входа локальной сети
  char ssidch1[20]; char passch1[20];
  // Заполняем параметры для WiFi сети и показываем их
  ssid.toCharArray(ssidch1, ssid.length() + 1);
  pass.toCharArray(passch1, pass.length() + 1);
  say("\n>>>>>>>>>>>>>>>>>>>>>%s<\n", ssidch1);
  // Подключаемся к локальной сетим
  jMulti.addAP(ssidch1, passch1);
  jMulti.run();
  
   // Выбираем и показываем Mac-адрес WiFi
  String wifiMacString = WiFi.macAddress();
  Serial.println("Mac-адрес точки доступа: "+wifiMacString);

  const char _localIP[] = "  Локальный IP: ";
  sayln("Контроллер подключается к локальной точке доступа");
  // Инициируем нулевую попытку подключения
  int connAttempts = 0;
  Serial.println(" ");
  while (WiFi.status() != WL_CONNECTED ) 
  {
    delay(1000);
    Serial.print(".");
    if (connAttempts++ == 15) break;   
  }
  sprintf(localip,"%s",WiFi.localIP().toString().c_str());
  Serial.print(_localIP); Serial.println(localip); 
  
  Serial.println(" ");
  sayln("Определяется местное время");
  // configTime(0, 0, "pool.ntp.org");
  configTime(10800, 0, "ntp.msk-ix.ru");
  char tzchar[60];
  TIMEZONE.toCharArray(tzchar, TIMEZONE.length() + 1);        // name of your camera for mDNS, Router, and filenames
  Serial.printf("Char >%s<\n", tzchar);
  setenv("TZ", tzchar, 1);  // mountain time zone from #define at top
  tzset();
  time(&now);
  // try for 15 seconds to get the time, then give up - 10 seconds after boot
  while (now < 5) 
  {        
    delay(1000);
    Serial.print("o");
    time(&now);
  }
  Serial.print("Местное время: "); Serial.print(ctime(&now));

  // Запускаем службу multicast DNS (mDNS). Это позволяет использовать имя хоста 
  // в веб-браузере вместо IP-адреса при доступе к ESP32 с компьютера. 
  // Особенности работы: динамическая обработка IP-адресов — даже если IP-адрес 
  // устройства изменится, mDNS автоматически преобразует новый IP-адрес в то же имя хоста;
  // нулевая настройка — дополнительная настройка DNS не требуется, поскольку mDNS 
  // работает автономно в локальных сетях; автоматическое добавление суффикса — 
  // если имя хоста уже существует в локальной сети, библиотека автоматически добавляет суффикс. 
  // Например, если esp32.local уже существует, она переименовывает его в esp32-2.local. 
  //
  // В коде ESP32 функция MDNS.begin() вызывается в функции setup() после подключения к сети. 
  // В качестве аргумента метода передаётся желаемое имя хоста. Например, если IP-адрес устройства
  // — 192.168.4.1, можно получить доступ к нему с помощью «esp32.local в веб-браузере. 
  // Важно: имя хоста не должно быть длиннее 63 символов. 
  // 
  // Некоторые ошибки, которые могут возникать при использовании функции MDNS.begin, и способы их решения:
  // а) mDNS не запущен в коде — если MDNS.begin() не вызван или помещён перед подключением к Wi-Fi, сервис не будет рекламироваться. 
  // Решение: инициализировать mDNS после успешного подключения, поместив MDNS.begin("esp32") 
  // внутри события подключения к Wi-Fi;
  // б) Firewall или антивирус блокируют mDNS — в системах без встроенной поддержки mDNS 
  // (Linux, Windows) правила брандмауэра могут препятствовать входящим ответам mDNS 
  // на порт 5353. Решение: проверить правила брандмауэра и, если нужно, изменить их;
  // в) сеть не позволяет многоадресный трафик — некоторые режимы AP, горячие точки или
  // гостевые сети Wi-Fi изолируют клиентов и блокируют многоадресный трафик, включая mDNS. 
  // Решение: проверить политики сети и, если нужно, изменить их.
  if (!MDNS.begin(devname)) 
  {
    sayln("Ошибка при запуске multicast DNS (mDNS)");
  } 
  else 
  {
    sayln("multicast DNS (mDNS) стартовал с именем '%s'", devname);
  }

  eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) 
  {
    //  info.disconnected.reason ==>  info.wifi_sta_disconnected.reason - update with esp32_arduino 2.00 v58
    if (info.wifi_sta_disconnected.reason != 201) 
    {
      //say( "\nframe_cnt: %8d, WiFi event Reason: %d , Status: %d\n", frame_cnt, info.wifi_sta_disconnected.reason, WiFi.status());
    }
  });
  return true;
}
*/

void setup() 
{
  Serial.begin(115200);
  delay(3000);
  Serial.println("\n\n");
  Serial.println("---------------------------------------");
  Serial.println("Arduino IDE 2.3.7 - Espressif ESP32 3.3.5");
  String idfver = esp_get_idf_version();
  Serial.println("ESP IDF: "+idfver);
  Serial.print("FrameStream "); Serial.println(vernum);
  Serial.println("---------------------------------------");

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
  // Выделяем память под рабочие буферы для хранения jpg в движении 
  // (должны быть больше больших кадров с ov2640,ov5640),
  // размер устанавливаем от ранее сформированного и расчитанного config_camera
  fb_record =          (uint8_t*)ps_malloc(frame_buffer_size); 
  fb_curr_record_buf = (uint8_t*)ps_malloc(frame_buffer_size);
  fb_streaming =       (uint8_t*)ps_malloc(frame_buffer_size); 
  fb_capture =         (uint8_t*)ps_malloc(frame_buffer_size); 
  */
  //saymem("MEM - после выделения памяти кадрам потока");

  // Объявляем мьютекс между задачами, который будет держать и передавать кадры камеры
  baton = xSemaphoreCreateMutex();
  sayln("Создаётся задача the_camera_loop на 0 ядре");

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
  
  // Запускаем задачи
  saymem("MEM - перед запуском Web-сервисов");
  //startCameraServer();
  //start_Stream_81_server();
  //start_Stream_82_server();
 
  /*
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
  
  
  jprln("Создаётся задача the_streaming_loop");
  xTaskCreate(
    the_streaming_loop,    // TaskFunction_t pvTaskCode          - имя функции, которая содержит код
    "the_streaming_loop",  // const char * const pcName          - имя задачи
    8000,                  // const uint32_t usStackDepth        - количество байт, выделенное для стека задачи
    NULL,                  // void * const pvParameters          - указатель на параметры для задачи 
    2,                     // UBaseType_t uxPriority             - приоритет задачи
    &the_streaming_loop_task
  );
  if (the_streaming_loop_task == NULL ) 
  {
    //vTaskDelete( xHandle );
    Serial.printf("Не удалось запустить задачу do_the_steaming_task! %d\n", the_streaming_loop_task);
  }

  jprln("Проверяется SD-карта на наличие свободного места ...");
  delete_old_stuff();

  char logname[60];
  char the_directory[50];

  sprintf(the_directory, "/%s%03d",  devname, file_group);
  SD_MMC.mkdir(the_directory);

  sprintf(logname, "/%s%03d/%s%03d.999.txt",  devname, file_group, devname, file_group);
  jprln("Создается logfile %s\n", logname);
  if (logfile) logfile.close();
  logfile = SD_MMC.open(logname, FILE_WRITE);
  if (!logfile) 
  {
    Serial.println("Ошибка открытия logfile для записи");
  }
  const char *strdate = ctime(&now);
  //logfile.println(strdate);
  digitalWrite(33, HIGH);         // red light turns off when setup is complete
  */

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
    print_mem("---------- 10 Minute Internet Check -----------");
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
