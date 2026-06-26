/** Arduino, ESP32, C/C++ ************************************* fs_server.h ***
 * 
 *                               Управлять контроллером ESP32-CAM через браузер 
 *                             по локальной сети и собственной сети контроллера 
 *                                                     
 * v2.2.1, 14.06.2026                                 Автор:      Труфанов В.Е.
 * Copyright © 2026 tve                               Дата создания: 29.01.2026
 * 
**/

#pragma once 

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
#endif
#ifdef ESP32
  #include <WiFi.h>
  #include <WebServer.h>
#endif
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include "esp_http_server.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>

#include "inimem.h"
#include "fs_wifi.h"

static esp_err_t index_handler(httpd_req_t *req); 
static esp_err_t capture_handler(httpd_req_t *req); 
static esp_err_t sphotos_handler(httpd_req_t *req); 
static esp_err_t fphotos_handler(httpd_req_t *req); 
static esp_err_t photos_handler(httpd_req_t *req); 
static esp_err_t restart_handler(httpd_req_t *req); // обработчик запроса на запись нового avi-файла: restart_handler
static esp_err_t reboot_handler(httpd_req_t *req); 
static esp_err_t start_handler(httpd_req_t *req); 
static esp_err_t stop_handler(httpd_req_t *req);    // обработчик запроса на остановку записи avi-файла: stop_handler
static esp_err_t time_handler(httpd_req_t *req); 
static esp_err_t status_handler(httpd_req_t *req); 
static esp_err_t find_handler(httpd_req_t *req); 
static esp_err_t edit_handler(httpd_req_t *req); 
static esp_err_t ota_handler(httpd_req_t *req);
static esp_err_t delete_handler(httpd_req_t *req); 
static esp_err_t reindex_handler(httpd_req_t *req); 

/*
// Определяем экземпляры HTTP-серверов. 
// Тип httpd_handle_t используется для создания и управления веб-серверами и
// возвращается функцией httpd_start(). Она создаёт экземпляр HTTP-сервера, 
// выделяет память и ресурсы в зависимости от указанной конфигурации и 
// возвращает указатель на экземпляр. 
httpd_handle_t camera_httpd = NULL;
*/

char the_page[4200];
char file_to_read[50];
char file_to_write[50];
bool do_the_reindex = false;
bool done_the_reindex = false;

// ----------------------------------------------------------------------------
static esp_err_t delete_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;

  Serial.print("delete_handler, core ");  Serial.print(xPortGetCoreID());
  Serial.print(", priority = "); Serial.println(uxTaskPriorityGet(NULL));


  //httpd_resp_send(req, page_html, strlen(page_html));
  delay(100);
  //delete_all_files = 1;
  return res;;
}
/*
static esp_err_t delete_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;

  Serial.print("delete_handler, core ");  Serial.print(xPortGetCoreID());
  Serial.print(", priority = "); Serial.println(uxTaskPriorityGet(NULL));


  httpd_resp_send(req, page_html, strlen(page_html));
  delay(100);
  delete_all_files = 1;
  return res;;
  }
*/

// ----------------------------------------------------------------------------
static esp_err_t reindex_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;

  print_mem("reindex_handler");

  char  buf[150];
  size_t buf_len;

  buf_len = httpd_req_get_url_query_len(req) + 1;

  if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
    //Serial.printf("Query => %s\n", buf);
    if (httpd_query_key_value(buf, "o", file_to_read, sizeof(file_to_read)) == ESP_OK) {
      //Serial.printf( "Found URL query parameter => file_to_read=>%s<\n", file_to_read);
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      //Serial.printf("Query => %s\n", buf);
      if (httpd_query_key_value(buf, "n", file_to_write, sizeof(file_to_write)) == ESP_OK) {
        //Serial.printf( "Found URL query parameter => file_to_write=>%s<\n", file_to_write);
      }
    }
  }

  do_the_reindex = true;

  while (!done_the_reindex) {
    delay(1000);
  }
  String x = " {\"status\":\"!!!DONE!!!\" }";
  const char* str = x.c_str();
  httpd_resp_send(req, str,  strlen(str));

  return res;
}
/*
static esp_err_t reindex_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;

  //print_mem("reindex_handler");

  char  buf[150];
  size_t buf_len;

  buf_len = httpd_req_get_url_query_len(req) + 1;

  if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
    //Serial.printf("Query => %s\n", buf);
    if (httpd_query_key_value(buf, "o", file_to_read, sizeof(file_to_read)) == ESP_OK) {
      //Serial.printf( "Found URL query parameter => file_to_read=>%s<\n", file_to_read);
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      //Serial.printf("Query => %s\n", buf);
      if (httpd_query_key_value(buf, "n", file_to_write, sizeof(file_to_write)) == ESP_OK) {
        //Serial.printf( "Found URL query parameter => file_to_write=>%s<\n", file_to_write);
      }
    }
  }

  do_the_reindex = true;

  while (!done_the_reindex) {
    delay(1000);
  }
  String x = " {\"status\":\"!!!DONE!!!\" }";
  const char* str = x.c_str();
  httpd_resp_send(req, str,  strlen(str));

  return res;
}
*/

// ----------------------------------------------------------------------------
static esp_err_t edit_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;
  char  buf[120];
  size_t buf_len;
  char  new_res[20];

  print_mem("edit_handler");

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
    Serial.printf("Query => %s\n", buf);
    char param[32];

    if (httpd_query_key_value(buf, "f", file_to_edit, sizeof(file_to_edit)) == ESP_OK) {
      Serial.printf( "Found URL query parameter => f=>%s<\n", file_to_edit);

    }
  }

  httpd_resp_send(req, edit_html, strlen(edit_html));

  return res;
}
/*
static esp_err_t edit_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;
  char  buf[120];
  size_t buf_len;
  char  new_res[20];

  //print_mem("edit_handler");

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
    Serial.printf("Query => %s\n", buf);
    char param[32];

    if (httpd_query_key_value(buf, "f", file_to_edit, sizeof(file_to_edit)) == ESP_OK) {
      Serial.printf( "Found URL query parameter => f=>%s<\n", file_to_edit);

    }
  }

  httpd_resp_send(req, edit_html, strlen(edit_html));

  return res;;
}
*/

// ----------------------------------------------------------------------------
static esp_err_t ota_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;

  print_mem("ota_handler");

  delay(100);

  start_record = 0;
  web_stop = true;

  ///  ota updates always enabled without password at either softap ip or router ip
  ArduinoOTA.setHostname(ssidota);
  ArduinoOTA.setPassword("mrpeanut");
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()

    Serial.println("\n\nStop Recording due to OTA ! \n\n" );
    start_record = 0;
    web_stop = true;
    delay(500);
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  do_the_ota = true;

  long start = millis();

  Serial.printf("Do the ota %d\n");

  const char the_message[] = "Status";

  time(&now);
  const char *strdate = ctime(&now);

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
Do the ota, or reboot ...
 <br>
<br>
</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);

  return ESP_OK;
}
/*
static esp_err_t ota_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;

  //print_mem("ota_handler");

  delay(100);

  start_record = 0;
  // Устанавливаем флаг "Остановить запись avi-файла"
  web_stop = true;

  ///  ota updates always enabled without password at either softap ip or router ip
  ArduinoOTA.setHostname(ssidota);
  ArduinoOTA.setPassword("mrpeanut");
  ArduinoOTA
  .onStart([]() 
  {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()

    Serial.println("\n\nStop Recording due to OTA ! \n\n" );
    start_record = 0;
    // Устанавливаем флаг "Остановить запись avi-файла"
    web_stop = true;
    delay(500);
    Serial.println("Start updating " + type);
  })
  .onEnd([]() 
  {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) 
  {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) 
  {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  do_the_ota = true;

  long start = millis();

  Serial.printf("Do the ota %d\n");

  const char the_message[] = "Status";

  time(&now);
  const char *strdate = ctime(&now);

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
Do the ota, or reboot ...
 <br>
<br>
</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);

  return ESP_OK;
}
*/

// ----------------------------------------------------------------------------
static esp_err_t status_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;
  print_mem("status_handler");
  delay(101);
  int remain = (-millis() + (avi_start_time + avi_length * 1000) ) / 1000;
  //Serial.printf("remain %d\n", remain);
  String x = " {\"OnOff\":\"";

  if (start_record == 1) {
    if (frame_interval == 1000) {
      x = x + "TL";
    } else {
      x = x + "On";
    }
  } else {
    x = x + "Off";
  }
  x = x + "\",\"File\":\"";

  int fnl = strlen(avi_file_name);
  //Serial.printf("fnl %d \n", fnl);

  String fn(avi_file_name);
  //Serial.println(fn);

  x = x + fn + "\", \"Remain\": ";
  x = x + String(remain) ;

  int total =  SD_MMC.totalBytes() / (1024 * 1024);
  int used =  SD_MMC.usedBytes() / (1024 * 1024) ;
  int freesp = total - used;

  x = x + ",\"Size\":" + String(total);
  x = x + ",\"Free\":" + String(freesp);
  if (0) { //                                     if (no_wifi) {
    x = x + ",\"rssi\":" + String(0);
  } else {
    x = x + ",\"rssi\":" + String(WiFi.RSSI());
  }

  x = x + ",\"IP\":" + "\"" + String(localip) + "\"" ;

  x = x + ",\"file_to_edit\":" + "\"" + String(file_to_edit) + "\"" ;

  x = x + ",\"Power\":" + String(99) + "}";  // x = x + ",\"Power\":" + String(power) + "}";

  const char* str = x.c_str();

  httpd_resp_send(req, str,  strlen(str));
  return res;
}
/*
static esp_err_t status_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;
  //print_mem("status_handler");

  delay(101);

  int remain = (-millis() + (avi_start_time + avi_length * 1000) ) / 1000;
  //Serial.printf("remain %d\n", remain);

  String x = " {\"OnOff\":\"";

  if (start_record == 1) {
    if (frame_interval == 1000) {
      x = x + "TL";
    } else {
      x = x + "On";
    }
  } else {
    x = x + "Off";
  }
  x = x + "\",\"File\":\"";

  int fnl = strlen(avi_file_name);
  //Serial.printf("fnl %d \n", fnl);

  String fn(avi_file_name);
  //Serial.println(fn);

  x = x + fn + "\", \"Remain\": ";
  x = x + String(remain) ;

  int total =  SD_MMC.totalBytes() / (1024 * 1024);
  int used =  SD_MMC.usedBytes() / (1024 * 1024) ;
  int freesp = total - used;

  x = x + ",\"Size\":" + String(total);
  x = x + ",\"Free\":" + String(freesp);
  if (0) { //                                     if (no_wifi) {
    x = x + ",\"rssi\":" + String(0);
  } else {
    x = x + ",\"rssi\":" + String(WiFi.RSSI());
  }

  x = x + ",\"IP\":" + "\"" + String(localip) + "\"" ;

  x = x + ",\"file_to_edit\":" + "\"" + String(file_to_edit) + "\"" ;

  x = x + ",\"Power\":" + String(99) + "}";  // x = x + ",\"Power\":" + String(power) + "}";

  const char* str = x.c_str();

  httpd_resp_send(req, str,  strlen(str));
  return res;
}
*/

// ----------------------------------------------------------------------------
static esp_err_t find_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;
  char  buf[120];
  size_t buf_len;
  char  new_res[20];

  oneframe x;

  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];
  int frame_pct;
  char filename[50];

  print_mem("find_handler");

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
    //Serial.printf("Query => %s\n", buf);
    char param[32];
    if (httpd_query_key_value(buf, "f", filename, sizeof(filename)) == ESP_OK) {
      //Serial.printf( "Found URL query parameter => f=>%s<\n", filename);
    }
    if (httpd_query_key_value(buf, "n", param, sizeof(param)) == ESP_OK) {
      int nn = atoi(param);
      if (nn >= 0 && nn <= 30000 ) {
        frame_pct = nn;
        //Serial.printf( "Found URL query parameter => n=%d\n", frame_pct);
      }
    }
  }

  //uint8_t* the_frame = find_a_frame ( "/JamCam0090.0001.avi", 12);

  x = find_a_frame ( filename, frame_pct);
  //the_frame = x.the_frame;

  _jpg_buf_len = x.the_frame_length;
  _jpg_buf = x.the_frame;


  if (x.the_frame == NULL) {
    Serial.printf("no frame\n");
    res = httpd_resp_send_408(req);
    //61.3 httpd_resp_send(req, page_html, strlen(page_html));
  } else {

    res = httpd_resp_set_type(req, "image/jpeg");
    if (res != ESP_OK) {
      return res;
    }

    if (res == ESP_OK) {
      char fname[50];
      char frame_num_char[8];
      char frame_pct_char[8];
      char frame_total_char[8];

      sprintf(fname, "inline; filename=frame_%d.jpg", frame_pct);
      sprintf(frame_num_char, "%d", x.the_frame_number);
      sprintf(frame_total_char, "%d", x.the_frame_total - 1); //61.4

      sprintf(frame_pct_char, "%d", frame_pct);

      httpd_resp_set_hdr(req, "Content-Disposition", fname);
      httpd_resp_set_hdr(req, "FrameNum", frame_num_char);
      httpd_resp_set_hdr(req, "Total", frame_total_char);
      httpd_resp_set_hdr(req, "FramePct", frame_pct_char);
      httpd_resp_set_hdr(req, "File", filename);


    }
    if (res == ESP_OK) {
      res = httpd_resp_send(req, (const char *)_jpg_buf, _jpg_buf_len);
    }

    free (x.the_frame);
  }
  return res;;
}
/*
static esp_err_t find_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;
  char  buf[120];
  size_t buf_len;
  char  new_res[20];

  oneframe x;

  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];
  int frame_pct;
  char filename[50];

  //print_mem("find_handler");

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
    //Serial.printf("Query => %s\n", buf);
    char param[32];
    if (httpd_query_key_value(buf, "f", filename, sizeof(filename)) == ESP_OK) {
      //Serial.printf( "Found URL query parameter => f=>%s<\n", filename);
    }
    if (httpd_query_key_value(buf, "n", param, sizeof(param)) == ESP_OK) {
      int nn = atoi(param);
      if (nn >= 0 && nn <= 30000 ) {
        frame_pct = nn;
        //Serial.printf( "Found URL query parameter => n=%d\n", frame_pct);
      }
    }
  }

  //uint8_t* the_frame = find_a_frame ( "/JamCam0090.0001.avi", 12);

  x = find_a_frame ( filename, frame_pct);
  //the_frame = x.the_frame;

  _jpg_buf_len = x.the_frame_length;
  _jpg_buf = x.the_frame;


  if (x.the_frame == NULL) {
    Serial.printf("no frame\n");
    res = httpd_resp_send_408(req);
    //61.3 httpd_resp_send(req, page_html, strlen(page_html));
  } else {

    res = httpd_resp_set_type(req, "image/jpeg");
    if (res != ESP_OK) {
      return res;
    }

    if (res == ESP_OK) {
      char fname[50];
      char frame_num_char[8];
      char frame_pct_char[8];
      char frame_total_char[8];

      sprintf(fname, "inline; filename=frame_%d.jpg", frame_pct);
      sprintf(frame_num_char, "%d", x.the_frame_number);
      sprintf(frame_total_char, "%d", x.the_frame_total - 1); //61.4

      sprintf(frame_pct_char, "%d", frame_pct);

      httpd_resp_set_hdr(req, "Content-Disposition", fname);
      httpd_resp_set_hdr(req, "FrameNum", frame_num_char);
      httpd_resp_set_hdr(req, "Total", frame_total_char);
      httpd_resp_set_hdr(req, "FramePct", frame_pct_char);
      httpd_resp_set_hdr(req, "File", filename);


    }
    if (res == ESP_OK) {
      res = httpd_resp_send(req, (const char *)_jpg_buf, _jpg_buf_len);
    }

    free (x.the_frame);
  }
  return res;
}
*/

// ****************************************************************************
// *  Обработать запрос запуска остановленной записи avi-файла: start_handler *    
// ****************************************************************************
static esp_err_t start_handler(httpd_req_t *req) 
{
  // Сбрасываем флаг остановленной записи avi-файла
  web_stop = false;
  Serial.printf("Старт (отмена остановки) записи avi-файла (из браузера) %d\n", web_stop);
  delay(500);
  esp_err_t xx = index_handler(req); // возврат на главную страницу, 2026-02-05 не срабатывает!
  return ESP_OK;
}
// ****************************************************************************
// *      Обработать запрос на остановку записи avi-файла: stop_handler       *
// ****************************************************************************
static esp_err_t stop_handler(httpd_req_t *req) 
{
  // Устанавливаем флаг команды из браузера на остановку записи avi-файла
  web_stop = true;
  Serial.printf("Команда из браузера на остановку записи avi-файла %d\n", web_stop);
  delay(500);
  esp_err_t xx = index_handler(req); // возврат на главную страницу, 2026-02-05 не срабатывает!
  return ESP_OK;
}
// ----------------------------------------------------------------------------
static esp_err_t time_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;

  char  buf[120];
  size_t buf_len;
  char  new_res[20];
  struct tm timeinfo;
  time_t now;

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      //Serial.printf("Found URL query => %s", buf);
      char param[32];

      if (httpd_query_key_value(buf, "time", param, sizeof(param)) == ESP_OK) {

        now = (time_t)atol(param);
        //Serial.print("new time: "); Serial.println(ctime(&now));
        //Serial.printf(">%i<", now);

        char tzchar[60];
        TIMEZONE.toCharArray(tzchar, TIMEZONE.length() + 1);        // name of your camera for mDNS, Router, and filenames
        setenv("TZ", tzchar, 1);  // mountain time zone from #define at top
        tzset();

        struct timeval tv;
        tv.tv_sec = now;
        tv.tv_usec = 0;
        settimeofday(&tv, NULL);

        //time(&now);
        //Serial.print("\nLocal time: "); Serial.println(ctime(&now));
        /*
                time_t rawtime;
                struct tm * ptm;
                time ( &rawtime );
                ptm = gmtime ( &rawtime );
                Serial.printf ("GMT: %2d:%02d\n", (ptm->tm_hour) % 24, ptm->tm_min);
        */
      }
    }
  }
  const char the_message[] = "Status";

  time(&now);
  const char *strdate = ctime(&now);

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
 got a time sync ...
 <br>


</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));

  return ESP_OK;
}
/*
static esp_err_t time_handler(httpd_req_t *req) 
{
  esp_err_t res = ESP_OK;

  char  buf[120];
  size_t buf_len;
  char  new_res[20];
  struct tm timeinfo;
  time_t now;

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      //Serial.printf("Found URL query => %s", buf);
      char param[32];

      if (httpd_query_key_value(buf, "time", param, sizeof(param)) == ESP_OK) {

        now = (time_t)atol(param);
        //Serial.print("new time: "); Serial.println(ctime(&now));
        //Serial.printf(">%i<", now);


        char tzchar[60];
        TIMEZONE.toCharArray(tzchar, TIMEZONE.length() + 1);        // name of your camera for mDNS, Router, and filenames
        setenv("TZ", tzchar, 1);  // mountain time zone from #define at top
        tzset();

        struct timeval tv;
        tv.tv_sec = now;
        tv.tv_usec = 0;
        settimeofday(&tv, NULL);

        //time(&now);
        //Serial.print("\nLocal time: "); Serial.println(ctime(&now));
        / *
                time_t rawtime;
                struct tm * ptm;
                time ( &rawtime );
                ptm = gmtime ( &rawtime );
                Serial.printf ("GMT: %2d:%02d\n", (ptm->tm_hour) % 24, ptm->tm_min);
        * /
      }
    }
  }
  const char the_message[] = "Status";

  time(&now);
  const char *strdate = ctime(&now);

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
 got a time sync ...
 <br>


</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));

  return ESP_OK;
}
*/

// ****************************************************************************
// *      Обработать запрос на запись нового avi-файла: restart_handler       *
// ****************************************************************************
static esp_err_t restart_handler(httpd_req_t *req) 
{
  long start = millis();
  print_mem("restart_handler");
  restart_now = true;
  const char the_message[] = "Status";
  time(&now);
  const char *strdate = ctime(&now);
  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
 Ending current recording, and starting next video ...
 <br>
<br>
</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );
  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);
  return ESP_OK;
}
/*
static esp_err_t restart_handler(httpd_req_t *req) 
{
  long start = millis();
  //print_mem("restart_handler");
  // Отмечаем, что началась запись нового avi-видео
  restart_now = true;
  const char the_message[] = "Status";
  time(&now);
  const char *strdate = ctime(&now);
  const char msg[] PROGMEM = R"rawliteral(
  
  <!doctype html>
  <html lang="ru">
  <head>
  <meta http-equiv="content-type" content="text/html; charset=utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>%s - начало следующего видео</title>
  </head>
  <body>
  <h1>%s<br>ESP32-CamRecorder - начало следующего видео %s <br><font color="red">%s</font></h1><br>
  <br>
  Завершена текущая запись, и начинается следующее видео ...
  <br>
  <br>
  </body>
  </html>
 
  )rawliteral";

  // Возвращаем клиенту ответ на запрос 
  sprintf(the_page, msg, devname, devname, vernum, strdate );
  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);
  return ESP_OK;
}
*/

// ----------------------------------------------------------------------------
static esp_err_t reboot_handler(httpd_req_t *req) 
{
  long start = millis();
  print_mem("reboot_handler");
  start_record = 0;
  web_stop = true;
  reboot_now = true;
  const char the_message[] = "Status";
  time(&now);
  const char *strdate = ctime(&now);
  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
 Ending current recording, and rebooting ...
 <br>

<br>
</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);

  return ESP_OK;
}
/*
static esp_err_t reboot_handler(httpd_req_t *req) 
{

  long start = millis();

  //print_mem("reboot_handler");

  start_record = 0;
  // Устанавливаем флаг "Отменить запись avi-файла"
  web_stop = true;
  reboot_now = true;

  const char the_message[] = "Status";

  time(&now);
  const char *strdate = ctime(&now);

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
 Ending current recording, and rebooting ...
 <br>

<br>
</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);

  return ESP_OK;
}
*/

// ----------------------------------------------------------------------------
static esp_err_t sphotos_handler(httpd_req_t *req) 
{
  long start = millis();
  print_mem("sphotos_handler");
  const char the_message[] = "Status";
  time(&now);
  const char *strdate = ctime(&now);
  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
 One photo every 15 seconds for 30 minutes - roll forward or back - refresh for more live photos
 <br>

<br><div id="image-container"></div>
<script>
document.addEventListener('DOMContentLoaded', function() {
  var c = document.location.origin;
  const ic = document.getElementById('image-container');  
  var i = 1;
  
  var timing = 15000; // time between snapshots for multiple shots

  function loop() {
    ic.insertAdjacentHTML('beforeend','<img src="'+`${c}/capture?_cb=${Date.now()}`+'">')
    ic.insertAdjacentHTML('beforeend','<br>')
    ic.insertAdjacentHTML('beforeend',Date())
    ic.insertAdjacentHTML('beforeend','<br>')

    i = i + 1;
    if ( i <= 120 ) {             
      window.setTimeout(loop, timing);
    }
  }
  loop();
  
})
</script><br>
</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);
  return ESP_OK;
}
/*
static esp_err_t sphotos_handler(httpd_req_t *req) 
{

  long start = millis();

  //print_mem("sphotos_handler");

  const char the_message[] = "Status";

  time(&now);
  const char *strdate = ctime(&now);

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
 One photo every 15 seconds for 30 minutes - roll forward or back - refresh for more live photos
 <br>

<br><div id="image-container"></div>
<script>
document.addEventListener('DOMContentLoaded', function() {
  var c = document.location.origin;
  const ic = document.getElementById('image-container');  
  var i = 1;
  
  var timing = 15000; // time between snapshots for multiple shots

  function loop() {
    ic.insertAdjacentHTML('beforeend','<img src="'+`${c}/capture?_cb=${Date.now()}`+'">')
    ic.insertAdjacentHTML('beforeend','<br>')
    ic.insertAdjacentHTML('beforeend',Date())
    ic.insertAdjacentHTML('beforeend','<br>')

    i = i + 1;
    if ( i <= 120 ) {             
      window.setTimeout(loop, timing);
    }
  }
  loop();
  
})
</script><br>
</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);
  return ESP_OK;
}
*/

// ----------------------------------------------------------------------------
static esp_err_t fphotos_handler(httpd_req_t *req) 
{
  long start = millis();
  print_mem("fphotos_handler");
  const char the_message[] = "Status";
  time(&now);
  const char *strdate = ctime(&now);
  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
 One photo every 1 seconds for 10 seconds - roll forward or back - refresh for more live photos
 <br>

<br><div id="image-container"></div>
<script>
document.addEventListener('DOMContentLoaded', function() {
  var c = document.location.origin;
  const ic = document.getElementById('image-container');  
  var i = 1;
  
  var timing = 1000; // time between snapshots for multiple shots

  function loop() {
    ic.insertAdjacentHTML('beforeend','<img src="'+`${c}/capture?_cb=${Date.now()}`+'">')
    ic.insertAdjacentHTML('beforeend','<br>')
    ic.insertAdjacentHTML('beforeend',Date())
    ic.insertAdjacentHTML('beforeend','<br>')

    i = i + 1;
    if ( i <= 10 ) {             // 10 frames
      window.setTimeout(loop, timing);
    }
  }
  loop();
  
})
</script><br>
</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);
  return ESP_OK;
}
/*
static esp_err_t fphotos_handler(httpd_req_t *req) 
{

  long start = millis();

  //print_mem("fphotos_handler");

  const char the_message[] = "Status";

  time(&now);
  const char *strdate = ctime(&now);

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
 One photo every 1 seconds for 10 seconds - roll forward or back - refresh for more live photos
 <br>

<br><div id="image-container"></div>
<script>
document.addEventListener('DOMContentLoaded', function() {
  var c = document.location.origin;
  const ic = document.getElementById('image-container');  
  var i = 1;
  
  var timing = 1000; // time between snapshots for multiple shots

  function loop() {
    ic.insertAdjacentHTML('beforeend','<img src="'+`${c}/capture?_cb=${Date.now()}`+'">')
    ic.insertAdjacentHTML('beforeend','<br>')
    ic.insertAdjacentHTML('beforeend',Date())
    ic.insertAdjacentHTML('beforeend','<br>')

    i = i + 1;
    if ( i <= 10 ) {             // 10 frames
      window.setTimeout(loop, timing);
    }
  }
  loop();
  
})
</script><br>
</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);
  return ESP_OK;
}
*/

// ****************************************************************************
// *  -----Обработать запрос запуска остановленной записи avi-файла: start_handler *    
// ****************************************************************************
static esp_err_t photos_handler(httpd_req_t *req) 
{

  long start = millis();

  print_mem("photos_handler");

  const char the_message[] = "Status";

  time(&now);
  const char *strdate = ctime(&now);

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
 One photo every 3 seconds for 30 seconds - roll forward or back - refresh for more live photos
 <br>

<br><div id="image-container"></div>
<script>
document.addEventListener('DOMContentLoaded', function() {
  var c = document.location.origin;
  const ic = document.getElementById('image-container');  
  var i = 1;
  
  var timing = 3000; // time between snapshots for multiple shots

  function loop() {
    ic.insertAdjacentHTML('beforeend','<img src="'+`${c}/capture?_cb=${Date.now()}`+'">')
    ic.insertAdjacentHTML('beforeend','<br>')
    ic.insertAdjacentHTML('beforeend',Date())
    ic.insertAdjacentHTML('beforeend','<br>')

    i = i + 1;
    if ( i <= 10 ) {             // 10 frames
      window.setTimeout(loop, timing);
    }
  }
  loop();
  
})
</script><br>
</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);
  return ESP_OK;
}
/*
static esp_err_t photos_handler(httpd_req_t *req) 
{

  long start = millis();

  //print_mem("photos_handler");

  const char the_message[] = "Status";

  time(&now);
  const char *strdate = ctime(&now);

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>
 <br>
 One photo every 3 seconds for 30 seconds - roll forward or back - refresh for more live photos
 <br>

<br><div id="image-container"></div>
<script>
document.addEventListener('DOMContentLoaded', function() {
  var c = document.location.origin;
  const ic = document.getElementById('image-container');  
  var i = 1;
  
  var timing = 3000; // time between snapshots for multiple shots

  function loop() {
    ic.insertAdjacentHTML('beforeend','<img src="'+`${c}/capture?_cb=${Date.now()}`+'">')
    ic.insertAdjacentHTML('beforeend','<br>')
    ic.insertAdjacentHTML('beforeend',Date())
    ic.insertAdjacentHTML('beforeend','<br>')

    i = i + 1;
    if ( i <= 10 ) {             // 10 frames
      window.setTimeout(loop, timing);
    }
  }
  loop();
  
})
</script><br>
</body>
</html>)rawliteral";

  sprintf(the_page, msg, devname, devname, vernum, strdate );

  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);
  return ESP_OK;
}
*/
// ****************************************************************************
// *    Обработать запрос к странице  "/capture" - предоставить изображение   *
// *        снимка с камеры по обращению из корневой страницы сервера         *
// ****************************************************************************

/*
  Предполагается, что вызов показа снимков с камеры (то ли из браузера, то ли 
по внешнему событию от ??12 или 16-того контакта) будет происходить не ранее, чем 
через тридцать секунд. Этот интервал определяет и перезагрузку счетчиков.
  Предполагается, что серии снятия кадров с камеры укладываются в 30 секунд.
*/

int capture_timer = 0;        // начало отсчета 30-секундного интервала
int captures = 0;             // снято снимков по запросу или событию за текущий 30-секундного интервала
uint16_t total_captures = 0;  // всего снято снимков по запросу или событию с начала перезагрузки контроллера (<65535)
int previous_capture = 0;     // время начала снятия с камеры предыдущего кадра
int skips = 0;                // пропущено снимков
int extras = 0;               // снято новых кадров

static esp_err_t capture_handler(httpd_req_t *req) 
{
  // Резервируем буфер для снятия кадра с камеры функцией esp_camera_fb_get,
  // esp_camera_fb_get будет возвращать указатель на структуру camera_fb_t. 
  // В этой структуре хранятся: указатель на пиксельные данные (поле buf);
  // длина буфера в байтах (поле len); ширина изображения в пикселях (поле width);
  // высота изображения в пикселях (поле height); формат структуры пиксельных данных (поле format);
  // отметка времени (поле timestamp).
  // Алгоритм работы: для получения снимка с камеры делается вызов: camera_fb_t* fb = esp_camera_fb_get(), 
  // если fb = null (захват камеры не удался), можно вывести сообщение об ошибке,
  // подождать 1 секунду и затем перезагрузить плату ESP32.
  // Если снимок получен, то после использования буфера память, выделенную функцией esp_camera_fb_get(),
  // нужно освободить с помощью функции esp_camera_fb_return() 
  camera_fb_t* fb = NULL;
  // Резервируем переменную для кода ошибок (целое со знаком). 
  // Успешный возврат (отсутствие ошибки) обозначается кодом ESP_OK, который определён как 0.
  esp_err_t res = ESP_OK;

  char cdfname[100];     // буфер для заголовка "Content-Disposition" 

  // Отмечаем начало обращения к текущей странице в браузере
  long start = millis();
  
  // Если начало новой серии снимков с камеры (новый 30-секундный интервал), то
  // настраиваем счетчики и отмечаем новую серию снимков
  if (capture_timer + 30000 <  millis()) 
  {
    sayln("Начало отправки новой серии снимков с камеры");
    /*
    if  (frame_cnt < 1000 ) 
    {
      jpr("Total captures %5d, Last 30 sec: captures %d, %0.1f per second, skips %d, extras %d\n", total_captures, captures, 1000.0 * captures / (millis() - capture_timer), skips, extras);
      print_mem("capture");
      int sock = httpd_req_to_sockfd(req);
      jpr("Socket: %d\n", httpd_req_to_sockfd(req));
      print_sock(sock);
    }
    */
    captures = 1;              // отметили снятие с камеры 1 кадра серии
    total_captures++;          // изменили счетчик снятых кадров с камеры 
    skips = 0;                 // инициировали счетчик пропущенных кадров
    extras = 0;                
    capture_timer = millis();  // отметили начало новой серии
  } 
  // Изменяем счетчики снимков с камеры
  else 
  {
    captures++;
    total_captures++;
    // sayln("      captures = %5d", captures);
    // sayln("total_captures = %5d", total_captures);
  }
   
  /*
    Предполагается, что на снятие и показ кадра изображения с камеры уходит
  менее 50 мсек (то есть частота не выше 20 кадров в секунду).
    В дальнейшем можно перенастроить на 13 кадров в секунду (75 мсек).
  */
  
  // Если время от предыдущего кадра менее 50 мсек, то пропускаем съемку и говорим об ошибке 408  
  if (millis() - previous_capture < 50) 
  { 
    skips++;
    // Функцией httpd_resp_send_408 отправляем ответ клиенту с кодом 408, чтобы сообщить ему, 
    // что запрос не был обработан в течение заданного времени
    // (просто пропускается запрос, а не отклоняется)
    res = httpd_resp_send_408(req); // just let the requests be missed rather than rejecting it //61
  }
  // Готовим изображение снятого кадра для отображения на странице, отправившей запрос "img=scr"
  else 
  {
    // Фиксируем время на будущее, как предыдущий кадр
    previous_capture = millis();
    // Формирум текст заголовка "Content-Disposition"
    sprintf(cdfname, "inline; filename=img%d.jpg", total_captures);

    // Выводим ранее сделанный кадр
    if (fb_record_time > (millis() - 500)) 
    {
      sayln("Выводится ранее сделанный кадр img%d.jpg", total_captures);
      xSemaphoreTake(baton, portMAX_DELAY);
      fb_capture_len = fb_record_len;
      fb_capture_time = fb_record_time;
      memcpy(fb_capture, fb_record,  fb_record_len); 
      xSemaphoreGive(baton);

      /*
      [Content-Disposition](https://developer.mozilla.org/ru/docs/Web/HTTP/Reference/Headers/Content-Disposition) 
        В обычном HTTP-ответе заголовок Content-Disposition является индикатором того, что ожидаемый контент ответа 
      будет отображаться в браузере, как веб-страница или часть веб-страницы, или же как вложение, 
      которое затем может быть скачано и сохранено локально.
        В случае, если тело HTTP-запроса типа multipart/form-data, то общий заголовок Content-Disposition 
      используется для каждой из составных частей multipart тела для указания дополнительных сведений по полю,
      к которому применён заголовок. Каждая часть отделена с помощью границы (boundary), определённой в заголовке Content-Type. 
      Content-Disposition, используемый непосредственно для всего тела HTTP-запроса, ни на что не влияет.
        Заголовок Content-Disposition определён для более широкого контекста MIME-сообщений для e-mail, 
      поэтому для HTTP-форм и POST-запросов используются только несколько допустимых параметров. В контексте HTTP 
      можно использовать только значение form-data, а также опциональные директивы name и filename.
        Синтаксис для заголовка ответа с обычным телом:
      (первым параметром в контексте HTTP должен быть или inline - это значение по умолчанию, 
      указывающее, что контент должен быть отображён внутри веб-страницы или как веб-страница 
      или attachment - указывает на скачиваемый контент; большинство браузеров отображают диалог
      "Сохранить как" с заранее заполненным именем файла из параметра filename, если он задан)
      Content-Disposition: inline
      Content-Disposition: attachment
      Content-Disposition: attachment; filename="filename.jpg"
        Синтаксис для заголовка ответа в составном теле:
      (первым параметром в контексте HTTP всегда является form-data; дополнительные параметры регистронезависимые 
      и могут иметь аргументы, значения которых следуют после знака '=' и берутся в кавычки. Несколько параметров 
      разделяются через точку с запятой ';')
      Content-Disposition: form-data
      Content-Disposition: form-data; name="fieldName"
      Content-Disposition: form-data; name="fieldName"; filename="filename.jpg"
        Директивы:
        name - за параметром следует строка с именем HTML-поля на форме, к которому относится 
      данная часть составного тела. При работе с несколькими файлами в том же самом поле 
      (например, атрибуты multiple элемента <input type=file>), могут быть несколько частей 
      с одинаковым именем. Если name имеет значение '_charset_', указывающее, что данная часть 
      не является HTML-полем, то она содержит кодировку по умолчанию для всех частей, в которых явно кодировка не указана;
        filename - за параметром указана строка с оригинальным именем передаваемого файла. 
      Это имя опционально и не может слепо использоваться приложением: информация о пути 
      должна быть очищена и должно быть сделано преобразование к файловой системе сервера. 
      Этот параметр предоставляет в основном справочную информацию. Когда используется 
      в комбинации с Content-Disposition: attachment, это значение будет использовано как 
      имя файла по умолчанию для диалога "Сохранить как";
      filename* - оба параметра "filename" и "filename*" отличаются только тем, что "filename*" 
      использует кодирование, определённое в RFC 5987. Когда присутствуют оба параметра "filename" 
      и "filename*" в одном поле заголовке, то преимущество имеет "filename*" над "filename", 
      но только в случае когда оба значения корректны.
        Пример - ответ, вызывающий диалог "Сохранить как":
      200 OK
      Content-Type: text/html; charset=utf-8
      Content-Disposition: attachment; filename="cool.html"
      Content-Length: 22
      <HTML>Save me!</HTML>
      */
      
      // Отправляем изображение кадра в ответ на запрос
      httpd_resp_set_type(req, "image/jpeg");
      httpd_resp_set_hdr(req, "Content-Disposition", cdfname);
      res = httpd_resp_send(req, (const char *)fb_capture, fb_capture_len);
    } 
    // Снимаем и выводим новый кадр из камеры
    else 
    {
      sayln("Снимается и выводится новый кадр img%d.jpg", total_captures);
      fb = esp_camera_fb_get(); //get_good_jpeg();
      extras++;
      if (!fb)
      {
        sayln("Ошибка получения кадра из камеры");
        res = httpd_resp_send_408(req);
      } 
      else 
      {
        xSemaphoreTake(baton, portMAX_DELAY);
        fb_capture_len = fb->len;
        fb_capture_time = millis();
        memcpy(fb_capture, fb->buf, fb->len);
        xSemaphoreGive(baton);
        esp_camera_fb_return(fb);
        // Отправляем изображение кадра в ответ на запрос
        httpd_resp_set_type(req, "image/jpeg");
        httpd_resp_set_hdr(req, "Content-Disposition", cdfname);
        res = httpd_resp_send(req, (const char *)fb_capture, fb_capture_len);
      }
    }
  }
  // Пересчитываем время работы контроллера в браузере
  time_in_web1 += (millis() - start);
  return res;
}
// ----------------------------------------------------------------------------
static esp_err_t index_handler(httpd_req_t *req) 
{
  long start = millis();

  int buf_len;
  char  buf[120];
  int hdr_len ;

  buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;

  if (httpd_req_get_hdr_value_str(req, "Host", localip, buf_len) == ESP_OK) {
    //Serial.printf( "Found header => Host: %s\n", localip);
  }

  //sprintf(localip, "%s", buf);
  /*
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
      if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
        Serial.printf("Found URL query => %s", buf);
      }
    }
  */
  print_mem("index_handler");

  const char the_message[] = "Status";

  time(&now);
  const char *strdate = ctime(&now);

  int tot = SD_MMC.totalBytes() / (1024 * 1024);
  int use = SD_MMC.usedBytes() / (1024 * 1024);
  long rssi = WiFi.RSSI();

  //const query = `${baseHost}:8080/e?edit=config.txt`

  const char msg[] PROGMEM = R"rawliteral(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%s ESP32-CAM Video Recorder Junior</title>
<script>
function initialize() {
   var baseHost = document.location.origin
   const query = `${baseHost}/time?time=`
   const x = new Date();
   var timing = x.getTime() / 1000;
   const query2 = query + String(timing)
   fetch(query2)
      .then(response => {
         console.log(`request to ${query2} finished, status: ${response.status}`)
      })
}   
   </script>
      </head>
       <body onload="initialize()" style="background-color: white">
        
</head>
<body>
<h1>%s<br>ESP32-CAM Video Recorder Junior %s <br><font color="red">%s</font></h1><br>

 Used / Total SD Space <font color="red"> %d MB / %d MB</font>, Rssi %d<br>

 Filename: %s <br>
 Framesize %d, Quality %d, Frame %d <br>
 Record Interval %dms, Stream Interval %dms <br>
 Avg framesize %d, fps %.1f <br>
 Time left in current video %d seconds<br>

 <h3><a href="http://%s/">http://%s/</a></h3>
 Current Frame:<br>
 <img src="http://%s/capture"/> <br>
 First Frame of Current Recording: (see more in File Management section)<br>
 <img src="http://%s/find?f=/%s&n=0"> <br>
 <h3>Streaming</h3>
 <a href="http://%s:81/stream"><button>Stream 81</button></a>
 <a href="http://%s:82/stream"><button>Stream 82</button></a>
 <h3>Series of pictures</h3>
 <a href="http://%s/photos"><button>10 x 3 sec</button> </a>
 <a href="http://%s/fphotos"><button>10 x 1 sec</button></a>
 <a href="http://%s/sphotos"><button>120 x 15 sec</button></a> 
 <h3>Recording is <font color="red"> %s </font> - overrides hardware pin 12 stop/start</h3>
 <a href="http://%s/start"><button>start</button> </a>
 <a href="http://%s/stop"><button>stop</button></a> 
 <h3>File Management</h3>
 <h4>
 <a href="http://%s:%d/e?edit=config2.txt"><button>edit config2.txt </button></a>
 <a href="http://%s:%d"><button>File Manager - download, delete, view videos </button></a> </h4>

 <h4><a href="http://%s/restart"><button>End recording, and start new video (write the index) </button></a></h4>
 <h4><a href="http://%s/reboot"><button>End recording, and reboot (using new settings)</button> </a></h4>
 <br>
</body>
</html>)rawliteral";

  int time_left = (- millis() +  (avi_start_time + avi_length * 1000)) / 1000;
  if (start_record == 0) {
    time_left = 0;
  }

  String stopstart = "Stopped";
  if (start_record) {
    stopstart = "Recording";
  }

  sprintf(the_page, msg, devname, devname, vernum, strdate, use, tot, rssi, avi_file_name,
          framesize, quality, frame_cnt, frame_interval, stream_delay,
          most_recent_avg_framesize, most_recent_fps, time_left, localip,  localip,  localip,  localip, avi_file_name,
          localip, localip, localip, localip, localip, stopstart.c_str(), localip, localip, localip, filemanagerport, localip, filemanagerport,
          localip, localip,  localip  );

  httpd_resp_send(req, the_page, strlen(the_page));

  time_in_web1 += (millis() - start);
  return ESP_OK;
}
/*
// ****************************************************************************
// *              Сформировать главную страницу CameraServer                  *
// ****************************************************************************
static esp_err_t index_handler(httpd_req_t *req) 
{
  // Начать отсчет времени с начала страницы
  long start = millis();
  
  int  buf_len;
  char buf[120];
  int  hdr_len ;

  //   Функцией httpd_req_get_hdr_value_len определяем длину поля "Host" заголовка 
  // запроса. Она используется в контексте HTTP-сервера, где функция обработчика URI 
  // получает указатель на структуру httpd_req_t, которая содержит информацию о входящем запросе. 
  //   Узнавание длины заголовка в запросе нужно для выделения буфера, в который 
  // будет скопировано значение поля. Если поле не найдено, функция вернет 0. 
  //   Параметры: req — [in] запрос, для которого предоставляется ответ; field, 
  // в нашем случае "Host" - [in] поле, которое нужно найти в заголовке; val — 
  // [out] указатель на буфер, в который будет скопировано значение, если поле найдено;
  // val_size — [in] размер буфера val.
  //   Функция httpd_req_get_hdr_value_len может возвращать следующие значения: 
  // ESP_OK — поле найдено в заголовке запроса и строка значения скопирована;
  // ESP_ERR_NOT_FOUND — поле не найдено;
  // ESP_ERR_INVALID_ARG — нулевые аргументы;
  // ESP_ERR_HTTPD_INVALID_REQ — недопустимый указатель на HTTP-запрос.
  //   Если размер вывода больше, чем вход, значение обрезается, что сопровождается 
  // ошибкой обрезания в возвращаемом значении.
  //   Важно: функция должна быть вызвана только из контекста обработчика URI, 
  // где указатель на запрос httpd_req_t* действителен. Если возвращается ошибка, 
  // обработчик URI должен дополнительно вернуть ошибку, чтобы обеспечить закрытие 
  // и очистку ошибочного сокета веб-сервером. 
  buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
  // Получаем значение поля из заголовка HTTP-запроса: 
  // esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *r, const char *field, char *val, size_t val_size);
  // r — указатель на запрос, для которого формируется ответ (httpd_req_t *r = reg);
  // field — название поля заголовка, которое нужно найти в запросе (const char *field = "Host");
  // val — указатель на буфер, в который будет скопировано значение, если поле найдено (char *val = localip)
  // val_size — размер буфера val.
  // 2026-01-30 - убедились, что поле "Host" в заголовке присутствовало:
  // 09:53:44.673 -> Found header => Host: 10.120.175.2
  if (httpd_req_get_hdr_value_str(req, "Host", localip, buf_len) == ESP_OK) 
  {
    Serial.printf( "Значение поля Host в заголовке: %s\n", localip);
  }
  //
  sprintf(localip, "%s", buf);
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) 
  {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) 
    {
      Serial.printf("Found URL query => %s", buf);
    }
  }
  saymem("MEM - в начале обработчика index_handler");
  const char the_message[] = "Status";
  time(&now);
  const char *strdate = ctime(&now);

  int tot = SD_MMC.totalBytes() / (1024 * 1024);  // общее количество доступных байтов на SD-карте
  int use = SD_MMC.usedBytes() / (1024 * 1024);   // количество используемых байтов на карте SD/SDIO/MMC
  long rssi = WiFi.RSSI();                        // уровень сигнала Wi-Fi сети
  // const query = `${baseHost}:8080/e?edit=config.txt`

  const char msg[] PROGMEM = R"rawliteral(
  <!doctype html>
  <html lang="ru">
  <head>
  <meta http-equiv="content-type" content="text/html; charset=utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>%s: ESP32-CamRecorder</title>

  <script>
  function initialize() 
  {
    var baseHost = document.location.origin;
    const query = `${baseHost}/time?time=`;
    const x = new Date();
    var timing = x.getTime() / 1000;
    const query2 = query + String(timing)
    fetch(query2).then(response => 
    {
      console.log(`request to ${query2} finished, status: ${response.status}`)
    })
  }   
  </script>
  </head>
  
  <body onload="initialize()" style="background-color: white">
  <h1>%s %s<font color="red">%s</font></h1><br>
  Используемое/общее пространство на SD-карте <font color="red"> %d Мб / %d Мб</font>, Rssi %d <br>
  Имя файла: %s <br>
  Размер %d, качество %d, кадров %d <br>
  Интервалы при записи %dms, интервалы в потоке %dмс<br>
  Размер файлов %d, кадров в сек. %.1f <br>
  Time left in current video %d seconds<br>
  

  <h3><a href="http://%s/">http://%s/</a></h3>
  Текущий кадр:<br>
  <img src="http://%s/capture"/> <br>
  Первый кадр в этой записи: (см. больше в разделе File Management)<br>
  <img src="http://%s/find?f=/%s&n=0"> <br>
  <h3>Потоки</h3>
  <a href="http://%s:81/stream"><button>Stream 81</button></a>
  <a href="http://%s:82/stream"><button>Stream 82</button></a>
  <h3>Series of pictures</h3>
  <a href="http://%s/photos"><button>10 x 3 sec</button> </a>
  <a href="http://%s/fphotos"><button>10 x 1 sec</button></a>
  <a href="http://%s/sphotos"><button>120 x 15 sec</button></a> 
  <h3>Recording is <font color="red"> %s </font> - overrides hardware pin 12 stop/start</h3>
  <a href="http://%s/start"><button>start</button> </a>
  <a href="http://%s/stop"><button>stop</button></a> 
  <h3>File Management</h3>
  <h4>
  <a href="http://%s:%d/e?edit=config2.txt"><button>edit config2.txt </button></a>
  <a href="http://%s:%d"><button>File Manager - download, delete, view videos </button></a> </h4>

  <h4><a href="http://%s/restart"><button>Завершить запись и начать новое видео (записать индекс)</button></a></h4>
  <h4><a href="http://%s/reboot"><button>End recording, and reboot (using new settings)</button> </a></h4>
  <br>
  James Zahary - Dec 8, 2024 -- May 18, 2022<br>
  <br>
  </body>
  </html>
  
  )rawliteral";

  int time_left = (- millis() +  (avi_start_time + avi_length * 1000)) / 1000;
  if (start_record == 0) 
  {
    time_left = 0;
  }

  String stopstart = "Stopped";
  if (start_record) 
  {
    stopstart = "Recording";
  }

  sprintf
  (
    the_page, msg, devname, devname, vernum, strdate, use, tot, rssi, avi_file_name,
    framesize, quality, frame_cnt, frame_interval, stream_delay,
    most_recent_avg_framesize, most_recent_fps, time_left, localip,  localip,  localip,  localip, avi_file_name,
    localip, localip, localip, localip, localip, stopstart.c_str(), localip, localip, localip, filemanagerport, localip, filemanagerport,
    localip, localip,  localip  
  );  
  
  Serial.println(the_page); logfile.print(the_page);
  
  //   Функцией httpd_resp_send отправляем данные в качестве HTTP-ответа на запрос. 
  // Подразумевается, что полный готовый ответ находится в одном буфере. 
  // Если код статуса и тип содержимого не были заданы, по умолчанию будет отправлен 
  // код статуса 200 OK и тип содержимого как text/html. Перед вызовом функции можно 
  // вызвать другие функции для настройки заголовков ответа: 
  // httpd_resp_set_status() — для установки строки статуса HTTP, 
  // httpd_resp_set_type() — для установки типа содержимого, 
  // httpd_resp_set_hdr() — для добавления любых дополнительных значений полей в заголовок ответа.
  //   Функция вызывается только из контекста URI-обработчика, где указатель запроса httpd_req_t* достоверный.
  // После вызова функции на запрос выдаётся ответ, никаких дополнительных данных 
  // не может быть отправлено для запроса. После вызова функции все заголовки запроса 
  // выпиливаются, поэтому их необходимо предварительно копировать в отдельные буферы, 
  // если они позже могут понадобиться.
  //   Параметры: req — запрос, на который формируется ответ; the_page — буфер, откуда 
  // вычитывается полный пакет, сконструированный пользователем; strlen(the_page) — 
  // длина буфера. Можно указать HTTPD_RESP_USE_STRLEN, который автоматически рассчитывает 
  // длину для строк, заканчивающихся нулём.
  //   Важно: когда завершены отправка всех фрагментов ответа на запрос, 
  // необходимо вызвать функцию, где в параметре buf_len указан 0. 
  httpd_resp_send(req, the_page, strlen(the_page));
  time_in_web1 += (millis() - start);
  return ESP_OK;
}
*/

/*                                                          *** Обработчики ***
fs_server.h
--------------
--httpd_register_uri_handler(camera_httpd, &index_uri);     index_handler
--httpd_register_uri_handler(camera_httpd, &capture_uri);   capture_handler
--httpd_register_uri_handler(camera_httpd, &photos_uri);    photos_handler
--httpd_register_uri_handler(camera_httpd, &fphotos_uri);   fphotos_handler
--httpd_register_uri_handler(camera_httpd, &sphotos_uri);   sphotos_handler
--httpd_register_uri_handler(camera_httpd, &reboot_uri);    reboot_handler
--httpd_register_uri_handler(camera_httpd, &restart_uri);   restart_handler
--httpd_register_uri_handler(camera_httpd, &time_uri);      time_handler         /time
--httpd_register_uri_handler(camera_httpd, &start_uri);     start_handler        /start
--httpd_register_uri_handler(camera_httpd, &stop_uri);      stop_handler
--httpd_register_uri_handler(camera_httpd, &edit_uri);      edit_handler         /edit
--httpd_register_uri_handler(camera_httpd, &find_uri);      find_handler         /find
--httpd_register_uri_handler(camera_httpd, &status_uri);    status_handler       /status
--httpd_register_uri_handler(camera_httpd, &reindex_uri);   reindex_handler      /reindex_handler
--httpd_register_uri_handler(camera_httpd, &ota_uri);       ota_handler          /ota

stream32.h
----------
httpd_register_uri_handler(stream81_httpd, &stream_uri);    stream_81_handler    /stream
httpd_register_uri_handler(stream82_httpd, &stream_uri);    stream_82_handler    /stream
*/

// ************************************************************ fs_server.h ***
