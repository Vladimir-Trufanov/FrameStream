/** Arduino, ESP32, C/C++ ************************************** stream32.h ***
 * 
 *         Обеспечить формирование потоков изображений от контроллера Esp32-CAM
 *                                                     
 * v1.0.2, 17.02.2026                                 Автор:      Труфанов В.Е.
 * Copyright © 2026 tve                               Дата создания: 31.01.2026
 * 
**/

#pragma once   

/*
#define PART_BOUNDARY "123456789000000000000987654321"

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

void the_streaming_loop (void* pvParameter);
int stream_81_frames;
long stream_81_start;
int stream_82_frames;
long stream_82_start;

bool start_streaming = false;
bool stream_82 = false;         // true - из браузера заказано вещание 2-ого потока
bool stream_81 = false;         // true - из браузера заказано вещание 1-ого потока

httpd_req_t *req_82;
httpd_req_t *req_81;

// Определяем экземпляры HTTP-серверов. 
// Тип httpd_handle_t используется для создания и управления веб-серверами и
// возвращается функцией httpd_start(). Она создаёт экземпляр HTTP-сервера, 
// выделяет память и ресурсы в зависимости от указанной конфигурации и 
// возвращает указатель на экземпляр. 
httpd_handle_t stream81_httpd = NULL;
httpd_handle_t stream82_httpd = NULL;

///////////////////////////////////////////////////////////////////////////////
//                            stream_81_handler()                            //
///////////////////////////////////////////////////////////////////////////////
static esp_err_t stream_81_handler(httpd_req_t *req) 
{
  esp_err_t res;
  long start = millis();
  print_mem("MEM - перед запуском stream_81_handler         ");
  req_81 = req;
  stream_81_frames = 0;
  stream_81_start = millis();
  // Отмечаем запуск потока
  stream_81 = true;
  // Устанавливаем тип содержимого, который будем передавать:
  // _STREAM_CONTENT_TYPE - application/octet-stream — общий MIME-тип, определённый в RFC 2046. 
  // Он предназначен для неинтерпретируемых бинарных данных — произвольных байтов, 
  // которые не относятся к конкретному типу медиа. Octet означает 8-битный байт 
  // (основную единицу бинарных данных в вычислениях), а stream — непрерывную последовательность байтов без внутренней структуры. 
  if (stream_81) 
  {
    res = httpd_resp_set_type(req_81,_STREAM_CONTENT_TYPE);
    if (res != ESP_OK) 
    {
      stream_81 = false;
    }
  }
  time_in_web1 += (millis() - start);
  // Циклимся - держим задачу в рабочем состоянии
  while (stream_81 == true) 
  {         
    //delay(1000);
    delay(200);
  }
  // При сбросе флага завершаем поток
  Serial.println("Поток stream_81 завершен");
  delay(500);
  // Функцией httpd_resp_send_408 отправляем ответ клиенту с кодом 408, чтобы сообщить ему, 
  // что запрос не был обработан в течение указанного времени.
  httpd_resp_send_408(req_81);
  req_81 = NULL;
  return ESP_OK;
}
///////////////////////////////////////////////////////////////////////////////
//                            stream_82_handler()                            //
///////////////////////////////////////////////////////////////////////////////
static esp_err_t stream_82_handler(httpd_req_t *req) 
{
  esp_err_t res;
  long start = millis();
  print_mem("MEM - перед запуском stream_82_handler         ");
  stream_82_frames = 0;
  stream_82_start = millis();
  // Связываем идентификатор запроса с потоком the_streaming_loop через req_82 
  req_82 = req;
  // Отмечаем запуск потока и устанавливаем тип содержимого
  stream_82 = true;
  if (stream_82) 
  {
    res = httpd_resp_set_type(req_82, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) 
    {
      stream_82 = false;
    }
  }
  time_in_web1 += (millis() - start);
  // Циклимся - держим задачу в рабочем состоянии
  while (stream_82 == true) 
  {          
    delay(1000);
  }
  // При сбросе флага завершаем поток
  Serial.println("Поток stream_82 завершен");
  delay(500);
  httpd_resp_send_408(req_82);
  req_82 = NULL;
  return ESP_OK;
}
///////////////////////////////////////////////////////////////////////////////
//                         start_Stream_81_server()                          //
///////////////////////////////////////////////////////////////////////////////
void start_Stream_81_server() 
{
  httpd_config_t config2 = HTTPD_DEFAULT_CONFIG();
  config2.server_port = 81;
  config2.ctrl_port = 32123; 
  // По умолчанию задача стартуется с пятым приоритетом
  // Serial.print("http Stream task prio: "); Serial.println(config2.task_priority);

  httpd_uri_t stream_uri = 
  {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_81_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream81_httpd, &config2) == ESP_OK) 
  {
    httpd_register_uri_handler(stream81_httpd, &stream_uri);
  } 
  else 
  {
    Serial.println("Ошибка старта потока 81");
  }
  Serial.println("Поток 81 http стартовал");
}
///////////////////////////////////////////////////////////////////////////////
//                         start_Stream_82_server()                          //
///////////////////////////////////////////////////////////////////////////////
void start_Stream_82_server() 
{
  httpd_config_t config2 = HTTPD_DEFAULT_CONFIG();
  config2.server_port = 82;
  config2.ctrl_port = 32124; 
  // По умолчанию задача стартуется с пятым приоритетом
  // Serial.print("http Stream task prio: "); Serial.println(config2.task_priority);

  httpd_uri_t stream_uri = 
  {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_82_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream82_httpd, &config2) == ESP_OK) 
  {
    httpd_register_uri_handler(stream82_httpd, &stream_uri);
  } 
  else 
  {
    Serial.println("Ошибка старта потока 82");
  }
  Serial.println("Поток 82 http стартовал");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// the_streaming_loop - видео-поток отправки изображений (имеет приоритет 2)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void the_streaming_loop (void* pvParameter) 
{
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];
  long start = millis();
  print_mem("MEM - стартовала задача the_streaming_loop     ");

  while (true) 
  {
    if (!stream_81 && !stream_82) 
    {
      delay(5);
    } 
    else 
    {
      if (stream_81) stream_81_frames++;
      if (stream_82) stream_82_frames++;
      // Если уже запись идет больше, чем полсекунды, то в буфер потока
      // выбираем текущий кадр через семафор
      xSemaphoreTake(baton, portMAX_DELAY );
      if (fb_record_time > (millis() - 500)) 
      {
        fb_streaming_len = fb_record_len;
        fb_streaming_time = fb_record_time;
        memcpy(fb_streaming, fb_record,  fb_record_len); 
        xSemaphoreGive( baton );
      } 
      else 
      {
        xSemaphoreGive(baton);
        // Если запись ведётся менее полсекунды, то делаем
        // самостоятельный кадр и выбираем его в буфер потока 
        fb = esp_camera_fb_get();
        // Ошибка может случиться, уже транслируем предыдущее содержимое fb_streaming 
        if (!fb) 
        {
          Serial.println("В потоке не удалось выполнить захват камерой");
        } 
        else 
        {
          xSemaphoreTake(baton,portMAX_DELAY);
          fb_streaming_len = fb->len;
          fb_streaming_time = millis();
          memcpy(fb_streaming, fb->buf, fb->len);
          xSemaphoreGive(baton);
          esp_camera_fb_return(fb);
        }
      }

      _jpg_buf_len = fb_streaming_len;
      _jpg_buf = fb_streaming;
      
      // Формируем заголовок отправки изображения
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

      long send_time = millis();
      long xx;
      xx = millis();

      // С помощью функции httpd_resp_send_chunk отправляем HTTP-ответ последовательными кусками. 
      //   Функция использует метод передачи HTTP-данных Chunked Transfer Encoding, 
      // при котором тело ответа или запроса разбивается на отдельные «чанки» (куски). 
      // Каждый чанк имеет свой размер и передаётся отдельно. Если код статуса и тип содержимого 
      // не были заданы, по умолчанию отправляется код статуса 200 OK и тип содержимого как text/html. 
      //   Функция вызывается только из контекста URI-обработчика, где указатель запроса httpd_req_t* достоверный.
      // После её вызова на запрос выдаётся ответ, никаких дополнительных данных не может быть отправлено для запроса. 
      
      // 1-ый чанк
      if (stream_82) 
      {
        res = httpd_resp_send_chunk(req_82, (const char *)part_buf, hlen);
        if (res != ESP_OK) 
        {
          stream_82 = false;
          Serial.printf("Ошибка потока 82_STREAM_PART: %d\n", res);
        }
      }
      if (stream_81) 
      {
        res = httpd_resp_send_chunk(req_81, (const char *)part_buf, hlen);
        if (res != ESP_OK) 
        {
          stream_81 = false;
          Serial.printf("Ошибка потока 81_STREAM_PART: %d\n", res);
        }
      }
      // 2-ой чанк
      xx = millis();
      if (stream_82) 
      {
        res = httpd_resp_send_chunk(req_82, (const char *)_jpg_buf, _jpg_buf_len);
        if (res != ESP_OK) 
        {
          stream_82 = false;
          Serial.printf("Ошибка потока 82_jpg_buf: %d\n", res);
        }
      }
      if (stream_81) 
      {
        res = httpd_resp_send_chunk(req_81, (const char *)_jpg_buf, _jpg_buf_len);
        if (res != ESP_OK) 
        {
          stream_81 = false;
          Serial.printf("Ошибка потока 81_jpg_buf: %d\n", res);
        }
      }
      // 3-ий чанк
      xx = millis();
      if (stream_82) 
      {
        res = httpd_resp_send_chunk(req_82, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if (res != ESP_OK) 
        {
          stream_82 = false;
          Serial.printf("Ошибка потока 82_STREAM_BOUNDARY: %d\n", res);
        }
      }
      if (stream_81) 
      {
        res = httpd_resp_send_chunk(req_81, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if (res != ESP_OK) 
        {
          stream_81 = false;
          jprln("Ошибка потока 81_STREAM_BOUNDARY: %d\n", res);
        }
      }
      // Трассируем каждый 10-ый кадр с расчетными данными по частоте кадров
      if (stream_81_frames % 100 == 10) 
      {
        if (Lots_of_Stats) 
        {
           jprln("Поток 81: время = %3.3f сек, частота кадров = %3.3f кадров/сек", (float)(millis() - stream_81_start)/1000, (float)(stream_81_frames/((millis() - stream_81_start)/1000)));
        }
      }
      if (stream_82_frames % 100 == 10) 
      {
        if (Lots_of_Stats) 
        {
          jprln("Поток 81: время = %3.3f сек, частота кадров = %3.1f кадров/сек", (float)(millis() - stream_82_start)/1000, (float)(stream_82_frames/((millis() - stream_82_start)/1000)));
        }
      }
      // Делаем задержку между кадрами в потоке
      int new_delay = 10; 
      if (millis() - send_time > 5000) 
      {
        new_delay = 1000;
        jprln("Медленный доступ к Wi-Fi: отправка кадра %d мсек", millis() - send_time);
      }
      delay(new_delay); 
      start = millis();
    }
  }  
}
*/

// ************************************************************* stream32.h ***
