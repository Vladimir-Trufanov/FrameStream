/** Arduino, ESP32, C/C++ **************************************** camera.h ***
 * 
 *                                             Обслужить работу с видео-камерой
 *                                                     
 * v1.0.6, 21.02.2026                                 Автор:      Труфанов В.Е.
 * Copyright © 2026 tve                               Дата создания: 25.01.2026
 * 
**/

#pragma once   

/** 
Ошибки при инициализации камеры:

10:38:16.695 -> E (3218) cam_hal: cam_dma_config(509): frame buffer malloc failed
10:38:16.695 -> E (3219) cam_hal: cam_config(599): cam_dma_config failed
10:38:16.695 -> E (3219) camera: Camera config failed with error 0xffffffff

Ошибки означают, что библиотека esp_camera.h не смогла выделить достаточно памяти в ESP32CAM для обработки изображений. 
ESP32CAM при захвате изображения хранит данные в буфере, который предварительно выделяется программой. 
PSRAM (Pseudo Static RAM) — это память ESP32CAM, в которой выделяется память для входящих данных изображения. 

Для устранения ошибок рекомендуется:
- проверить конфигурацию платы. Убедиться, что в Arduino IDE выбрана правильная плата;
- проверить назначение контактов в коде и, если нужно, изменить его согласно назначению для платы;
- проверить питание — если ESP32-CAM питается через USB-порт, стоит попробовать использовать другой порт или внешний источник питания;
- проверить подключение камеры — камера имеет маленький разъём, и важно, чтобы она была подключена правильно и с надёжным контактом;
- включить опцию PSRAM в настройках платы, если она отключена. Например, в некоторых случаях ошибка возникает из-за того, что опция PSRAM («OPI PSRAM») отключена;
- удалить и повторно подключить разъём камеры. Это может помочь, если ошибка связана с повреждением платы;
- проверить код и, если нужно, исправить ошибку. Например, в некоторых случаях помогает добавление флагов сборки -D PREFER_PSRAM -mfix-esp32-psram-cache-issue в скетч;
- проверить код на наличие ошибок, связанных с выделением памяти. Например, в некоторых случаях ошибка возникает из-за ошибки в коде, из-за которой PSRAM не распознаётся ESP32-CAM;
- использовать старую версию программного обеспечения ESP32-CAM — в некоторых случаях ошибка возникала из-за ошибки в новой версии, и временное решение — использовать старую версию, которая работала в прошлом. 

!!! 2026-02-23 для 
11:34:26.716 -> ------------------------------------------
11:34:26.716 -> SPIRAM Memory Info:
11:34:26.716 -> ------------------------------------------
11:34:26.716 ->   Total Size        :  2097152 B (2048.0 KB)
11:34:26.716 ->   Free Bytes        :  1319824 B (1288.9 KB)
11:34:26.716 ->   Allocated Bytes   :   775184 B ( 757.0 KB)
11:34:26.716 ->   Minimum Free Bytes:  1319824 B (1288.9 KB)
11:34:26.716 ->   Largest Free Block:  1310708 B (1280.0 KB)
11:34:26.716 -> ------------------------------------------
сделал 
Core Debug Level: "Debug"
Erase All Flash Before Scetch Upload: "Enabled"
Partition Scheme: "Huge App"

**/

#include "esp_camera.h"
#include "sensor.h"

#include "inimem.h"
#include "trass.h"

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Выделяем переменную по ошибкам: esp_err_t — тип, который в ESP-IDF представляет коды ошибок. 
// Это целое число со знаком. Успешный возврат (отсутствие ошибки) обозначается кодом ESP_OK, 
// который определён как 0. Общие коды ошибок для традиционных отказов (out of memory, timeout, invalid argument и т. п.) 
// определены в файле esp_err.h. Различные компоненты в ESP-IDF могут определять дополнительные коды ошибок для отдельных ситуаций. 
static esp_err_t cam_err;

/*
uint8_t* fb_streaming;
uint8_t* fb_capture;

int fb_streaming_len;
int fb_capture_len;
long fb_streaming_time = 0;
long fb_capture_time = 0;

int first = 1;
long frame_start = 0;
long frame_end = 0;
long frame_total = 0;
long frame_average = 0;
long loop_average = 0;
long loop_total = 0;
long total_frame_data = 0;
long last_frame_length = 0;
int done = 0;

// Переменные отлова каждого 50 кадра до 1000
int gframe_cnt;
int gfblen;
int gj;
int gmdelay;
int do_it_now = 0;

// Сбрасываем первый флаг записи по событию (после первой проверки 12-ого контакта)
int start_record_1st_opinion = 0;
// Сбрасываем второй флаг записи по событию (после проверки 12-ого контакта на следующем цикле)
int start_record_2nd_opinion = 0; 
// Сбрасываем флаг запуска записи очередного видео файла
int start_record = 0;

uint8_t avi1_buf[4]        = {0x41, 0x56, 0x49, 0x31};    // "AVI1"
uint8_t idx1_buf[4]        = {0x69, 0x64, 0x78, 0x31};    // "idx1"
uint8_t zero_buf[4]        = {0x00, 0x00, 0x00, 0x00};    // "    "
uint8_t dc_buf[4]          = {0x30, 0x30, 0x64, 0x63};    // "00dc"
uint8_t dc_and_zero_buf[8] = {0x30, 0x30, 0x64, 0x63, 0x00, 0x00, 0x00, 0x00};

// Декодирование JPEG для чайников - https://habr.com/ru/articles/102521/
// Изобретаем JPEG                 - https://habr.com/ru/articles/206264/

// Заголовок AVI-файла
#define AVIOFFSET 240  // длина заголовка
const int avi_header[AVIOFFSET] PROGMEM = 
{
  0x52, 0x49, 0x46, 0x46, 0xD8, 0x01, 0x0E, 0x00, 0x41, 0x56, 0x49, 0x20, 0x4C, 0x49, 0x53, 0x54,
  0xD0, 0x00, 0x00, 0x00, 0x68, 0x64, 0x72, 0x6C, 0x61, 0x76, 0x69, 0x68, 0x38, 0x00, 0x00, 0x00,
  0xA0, 0x86, 0x01, 0x00, 0x80, 0x66, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x80, 0x02, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54, 0x84, 0x00, 0x00, 0x00,
  0x73, 0x74, 0x72, 0x6C, 0x73, 0x74, 0x72, 0x68, 0x30, 0x00, 0x00, 0x00, 0x76, 0x69, 0x64, 0x73,
  0x4D, 0x4A, 0x50, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x66,
  0x28, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00, 0xe0, 0x01, 0x00, 0x00,
  0x01, 0x00, 0x18, 0x00, 0x4D, 0x4A, 0x50, 0x47, 0x00, 0x84, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x4E, 0x46, 0x4F,
  0x10, 0x00, 0x00, 0x00, 0x6A, 0x61, 0x6D, 0x65, 0x73, 0x7A, 0x61, 0x68, 0x61, 0x72, 0x79, 0x20,
  0x76, 0x36, 0x32, 0x20, 0x4C, 0x49, 0x53, 0x54, 0x00, 0x01, 0x0E, 0x00, 0x6D, 0x6F, 0x76, 0x69,
};
*/


// Здесь используются две ссылки на Git-репозитарии, которые сохранены в каталоге Info приложения: 
// data structure from here https://github.com/s60sc/ESP32-CAM_MJPEG2SD/blob/master/avi.cpp, extended for ov5640
// must match https://github.com/espressif/esp32-camera/blob/b6a8297342ed728774036089f196d599f03ea367/driver/include/sensor.h#L87
// which changed in Nov 2024
struct frameSizeStruct 
{
  uint8_t frameWidth[2];
  uint8_t frameHeight[2];
};

/*
// Перечисление идентификаторов типов камер, которое определено в sensor.h
typedef enum {
    OV9650_PID = 0x96,
    OV7725_PID = 0x77,
    OV2640_PID = 0x26,
    OV3660_PID = 0x3660,
    OV5640_PID = 0x5640,
    OV7670_PID = 0x76,
    NT99141_PID = 0x1410,
    GC2145_PID = 0x2145,
    GC032A_PID = 0x232a,
    GC0308_PID = 0x9b,
    BF3005_PID = 0x30,
    BF20A6_PID = 0x20a6,
    SC101IOT_PID = 0xda4a,
    SC030IOT_PID = 0x9a46,
    SC031GS_PID = 0x0031,
    MEGA_CCM_PID =0x039E, 
    HM1055_PID = 0x0955,
    HM0360_PID = 0x0360
} camera_pid_t;

// Перечисление форматов изображений, которое определено в sensor.h
typedef enum {
    FRAMESIZE_96X96,    // 96x96
    FRAMESIZE_QQVGA,    // 160x120
    FRAMESIZE_128X128,  // 128x128
    FRAMESIZE_QCIF,     // 176x144
    FRAMESIZE_HQVGA,    // 240x176
    FRAMESIZE_240X240,  // 240x240
    FRAMESIZE_QVGA,     // 320x240
    FRAMESIZE_320X320,  // 320x320
    FRAMESIZE_CIF,      // 400x296
    FRAMESIZE_HVGA,     // 480x320
    FRAMESIZE_VGA,      // 640x480
    FRAMESIZE_SVGA,     // 800x600
    FRAMESIZE_XGA,      // 1024x768
    FRAMESIZE_HD,       // 1280x720
    FRAMESIZE_SXGA,     // 1280x1024
    FRAMESIZE_UXGA,     // 1600x1200
    // 3MP Sensors
    FRAMESIZE_FHD,      // 1920x1080
    FRAMESIZE_P_HD,     //  720x1280
    FRAMESIZE_P_3MP,    //  864x1536
    FRAMESIZE_QXGA,     // 2048x1536
    // 5MP Sensors
    FRAMESIZE_QHD,      // 2560x1440
    FRAMESIZE_WQXGA,    // 2560x1600
    FRAMESIZE_P_FHD,    // 1080x1920
    FRAMESIZE_QSXGA,    // 2560x1920
    FRAMESIZE_5MP,      // 2592x1944
    FRAMESIZE_INVALID
} framesize_t;
*/

static const frameSizeStruct frameSizeData[] = 
{                                                      //           framesize:
  {{0x60, 0x00}, {0x60, 0x00}}, // FRAMESIZE_96X96,    // 96x96     0 
  {{0xA0, 0x00}, {0x78, 0x00}}, // FRAMESIZE_QQVGA,    // 160x120   1
  {{0x60, 0x00}, {0x60, 0x00}}, // FRAMESIZE_128X128   // 128x128   2
  {{0xB0, 0x00}, {0x90, 0x00}}, // FRAMESIZE_QCIF,     // 176x144   3
  {{0xF0, 0x00}, {0xB0, 0x00}}, // FRAMESIZE_HQVGA,    // 240x176   4
  {{0xF0, 0x00}, {0xF0, 0x00}}, // FRAMESIZE_240X240,  // 240x240   5
  {{0x40, 0x01}, {0xF0, 0x00}}, // FRAMESIZE_QVGA,     // 320x240   6
  {{0x40, 0x01}, {0xF0, 0x00}}, // FRAMESIZE_320X320,  // 320x320   7
  {{0x90, 0x01}, {0x28, 0x01}}, // FRAMESIZE_CIF,      // 400x296   8
  {{0xE0, 0x01}, {0x40, 0x01}}, // FRAMESIZE_HVGA,     // 480x320   9
  {{0x80, 0x02}, {0xE0, 0x01}}, // FRAMESIZE_VGA,      // 640x480   10
  //               38,400    61,440    153,600
  {{0x20, 0x03}, {0x58, 0x02}}, // FRAMESIZE_SVGA,     // 800x600   11
  {{0x00, 0x04}, {0x00, 0x03}}, // FRAMESIZE_XGA,      // 1024x768  12
  {{0x00, 0x05}, {0xD0, 0x02}}, // FRAMESIZE_HD,       // 1280x720  13 - по умолчанию
  {{0x00, 0x05}, {0x00, 0x04}}, // FRAMESIZE_SXGA,     // 1280x1024 14
  {{0x40, 0x06}, {0xB0, 0x04}}, // FRAMESIZE_UXGA,     // 1600x1200 15
  // 3MP Sensors
  {{0x80, 0x07}, {0x38, 0x04}}, // FRAMESIZE_FHD,      // 1920x1080 16
  {{0xD0, 0x02}, {0x00, 0x05}}, // FRAMESIZE_P_HD,     //  720x1280 17
  {{0x60, 0x03}, {0x00, 0x06}}, // FRAMESIZE_P_3MP,    //  864x1536 18
  {{0x00, 0x08}, {0x00, 0x06}}, // FRAMESIZE_QXGA,     // 2048x1536 19
  // 5MP Sensors
  {{0x00, 0x0A}, {0xA0, 0x05}}, // FRAMESIZE_QHD,      // 2560x1440 20
  {{0x00, 0x0A}, {0x40, 0x06}}, // FRAMESIZE_WQXGA,    // 2560x1600 21
  {{0x38, 0x04}, {0x80, 0x07}}, // FRAMESIZE_P_FHD,    // 1080x1920 22
  {{0x00, 0x0A}, {0x80, 0x07}}  // FRAMESIZE_QSXGA,    // 2560x1920 23
};

/*
uint16_t remnant = 0;
uint32_t startms;             // время начала работы с камерой и файлом avi    
uint32_t elapsedms;           // общее время работы с камерой и файлом avi 

long current_frame_time=0;    // 
long last_frame_time=0;

camera_fb_t * fb_curr=NULL;   // структура с буфером снятого кадра
uint8_t* fb_curr_record_buf;  // копия буфера снятого кадра
int fb_curr_record_len;       // длина буфера снятого кадра
long fb_curr_record_time=0;   // время записи снятого кадра с начала запуска программы (мс)

// Окончательные данные по кадрам, защищенные мьютексом
uint8_t* fb_record;           // копия буфера снятого кадра
int fb_record_len;            // длина буфера снятого кадра
long fb_record_time = 0;      // время записи снятого кадра с начала запуска программы (мс)

long totalp;                  // общее время съемки всех кадров записанного файла avi
long totalw;                  // общее время записи всех кадров файла avi
long time_in_loop;
long wait_for_cam;            // общее время ожидания камеры между съемками
long time_in_camera;          // общее время работы камеры
long time_in_good;            // время камеры с получением целых (хороших) кадров
long time_total;              // общее время съёмки и записи на SD
long time_in_web1;            // время пребывания на страницах браузера
long delay_wait_for_sd;       // общее время ожидания записи на SD
long time_in_sd;              // время, потраченное на работу с sd-картой

unsigned long jpeg_size;      // размер текущего кадра для сбора статистики
unsigned long movi_size;      // текущий размер видео-файла для статистики

long bytes_before_last_100_frames=0;  // размер видео с последними 100 кадрами
long time_before_last_100_frames=0;   // время записи последних 100 кадров
float most_recent_fps=0;              // количество недавних кадров в секунду
int most_recent_avg_framesize=0;      // средний размер недавних кадров

uint32_t uVideoLen = 0;       // общий размер всех блоков кадров в видео-файле
unsigned long idx_offset=0;   // индексные метки кадров в видео-файле

int bad_jpg = 0;              // количество плохих кадров
int extend_jpg = 0;           // количество расширенных кадров
int normal_jpg = 0;           // количество нормальных кадров

int very_high = 0;

int we_are_already_stopped=0; // 1 - "видео-запись уже остановлена"

// Сбрасываем флаг "удалить старые файлы по завершению записи текущего файла avi"
int delete_old_stuff_flag = 0;
*/

// ****************************************************************************
// *                      Установить параметры камеры                         *
// ****************************************************************************
static bool config_camera() 
{
  // Определяем конфигурацию камеры
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;   // номер канала для LEDC (светодиодного ШИМ-контроллера) 
  config.ledc_timer = LEDC_TIMER_0;       // номер таймера для LEDC (светодиодного ШИМ-контроллера)
  config.pin_d0 = Y2_GPIO_NUM;            // номер 0 контакта для передачи данных с камеры
  config.pin_d1 = Y3_GPIO_NUM;            // 1
  config.pin_d2 = Y4_GPIO_NUM;            // 2
  config.pin_d3 = Y5_GPIO_NUM;            // 3
  config.pin_d4 = Y6_GPIO_NUM;            // 4
  config.pin_d5 = Y7_GPIO_NUM;            // 5
  config.pin_d6 = Y8_GPIO_NUM;            // 6
  config.pin_d7 = Y9_GPIO_NUM;            // номер 7 контакта для передачи данных с камеры
  config.pin_xclk = XCLK_GPIO_NUM;        // номер контакта для тактового сигнала камеры
  config.pin_pclk = PCLK_GPIO_NUM;        // номер контакта тактового сигнала для пикселей камеры 
  config.pin_vsync = VSYNC_GPIO_NUM;      // номер контакта для вертикальной синхронизации 
  config.pin_href = HREF_GPIO_NUM;        // номер контакта для горизонтальной синхронизации
  config.pin_sscb_sda = SIOD_GPIO_NUM;    // номер контакта для передачи данных 
  config.pin_sscb_scl = SIOC_GPIO_NUM;    // номер контакта тактового сигнала SCCB (по протоколу I2C) камеры 
  config.pin_pwdn = PWDN_GPIO_NUM;        // номер контакта для отключения питания камеры
  config.pin_reset = RESET_GPIO_NUM;      // номер контакта для сброса настроек камеры
  config.xclk_freq_hz = 20000000;         // частота тактового сигнала в герцах
  config.pixel_format = PIXFORMAT_JPEG;   // формат пикселей камеры, указанный как PIXFORMAT_JPEG

  // Задаём размер кадра камеры как, например, FRAMESIZE_UXGA
  config.frame_size=(framesize_t)framesize;   
  // Задаём качество JPEG-изображения камеры как, например, 12
  config.jpeg_quality=quality;    
  // Задаём количество отдельных буферов для кадров      
  config.fb_count=buffersconfig;  
  // Обеспечиваем размещение в буферах последних кадров
  // https://github.com/espressif/esp32-camera/issues/357#issuecomment-1047086477
  // Для ESP32-CAM доступны два режима захвата изображений через параметр grab_mode:
  // CAMERA_GRAB_WHEN_EMPTY. Драйвер записывает данные в буфер кадров, пока есть свободный буфер. 
  // Когда все буферы заполняются, новые данные, отправленные сенсором камеры, принудительно 
  // отбрасываются из-за отсутствия свободного буфера.
  // CAMERA_GRAB_LATEST. Драйвер занимает один буфер кадров и пытается обновить в нём последние данные.
  // Количество буферов кадров, которые может получить уровень приложения, равно fb_count - 1.
  config.grab_mode = CAMERA_GRAB_LATEST; 

  // Показываем состояние памяти
  saymem("MEM - перед инициированием камеры");
  // Задаём 5 попыток инициации камеры
  cam_err = ESP_FAIL;
  int attempt = 5;
  while (attempt && cam_err != ESP_OK) 
  {
    Serial.print("attempt="); Serial.println(attempt);
    cam_err = esp_camera_init(&config);
    if (cam_err != ESP_OK) 
    {
      sayln("Ошибка инициировании камеры 0x%x", cam_err);
      // Передёргиваем контакт питания
      digitalWrite(PWDN_GPIO_NUM, 1);
      delay(500);
      digitalWrite(PWDN_GPIO_NUM, 0); // power cycle the camera (OV2640)
      // Уменьшаем счётчик
      attempt--;
    }
  }
  // Если неудачное инициирование камеры, то будем перезагружать контроллер
  if (cam_err != ESP_OK) return false;
  // Показываем состояние памяти
  saymem("MEM - после инициирования камеры");
  // Получаем указатель (дескриптор) на структуру данных сенсора (камеры)
  sensor_t * ss = esp_camera_sensor_get();
  sayln("Камера стартовала корректно, идентификатор камеры: %x (hex)", ss->id.PID);
  // Для камеры OV5640 достраиваем параметры
  if (ss->id.PID == OV5640_PID ) 
  {
    ss->set_hmirror(ss, 1);   // 0 = disable , 1 = enable
  } 
  else 
  {
    ss->set_hmirror(ss, 0);    // 0 = disable , 1 = enable
  }
  ss->set_brightness(ss, 1);   // up the blightness just a bit
  ss->set_saturation(ss, -2);  // lower the saturation
  
  sayln("Выполняются 30 пробных кадров с камеры");
  int x = 0;                   // буфер наибольшего кадра
  int y = 0;                   // номер наибольшего кадра
  int j;                       // счётчик пробных кадров
  camera_fb_t* fb;             // указатель на структуру кадра. 
  delay(500);
  for (j = 0; j < 30; j++) 
  {
    // Получаем буфер кадра с камеры (esp_camera_fb_get)
    // (функция esp_camera_fb_get возвращает указатель на структуру camera_fb_t. 
    // В этой структуре хранятся, например: 
    //   указатель на пиксельные данные (поле buf);
    //   длина буфера в байтах (поле len);
    //   ширина изображения в пикселях (поле width);
    //   высота изображения в пикселях (поле height);
    //   формат структуры пиксельных данных (поле format);
    //   отметка времени (поле timestamp).
    // После использования буфера память, выделенную функцией esp_camera_fb_get(),
    // нужно освободить с помощью функции esp_camera_fb_return(). 
    //
    // Функция esp_camera_fb_get можно использовать, например, для получения контрольного снимка 
    // с камеры. В коде может быть вызов: camera_fb_t *fb = esp_camera_fb_get(). 
    // Если fb = null (захват камеры не удался), вывести сообщение об ошибке,
    // подождать 1 секунду и затем перезагрузить плату ESP32.
    fb = esp_camera_fb_get(); 
    if (!fb) sayln("Не удалось выполнить захват с камеры на кадре %2d",j);
    else 
    {
      if (j < 3 || j > 27) 
      {
        say("Кадр %2d, ",j); say("длина=%7d, ", long(fb->len)); sayln("адрес в памяти %X",(long)fb->buf);
        if (fb->len > x) {x = fb->len; y=j;}
      }
      else if (fb->len > x) 
      {
        x = fb->len; y=j;
        say("Кадр %2d, ",j); say("длина=%7d, ", long(fb->len)); sayln("адрес в памяти %X",(long)fb->buf);
      }
      esp_camera_fb_return(fb);
      delay(30);
    }
  }
  // Если не удалось выполнить захват с камеры, то будем перезагружать контроллер
  if (!fb) return false;
  // Вычисляем 4-кратный размер наибольшего буфера, округленный до 16 Кбайт
  frame_buffer_size  = (( (x * 4) / (16 * 1024) ) + 1) * 16 * 1024  ;
  say("Размер наибольшего буфера 4 изображений по кадру %d ",y); sayln("равен %d",frame_buffer_size);
  saymem("MEM - после пробных фотографий");
  return true;
}

/*
// ****************************************************************************
// *       Cделать снимок и убедиться, что он имеет хороший формат jpeg       *
// ****************************************************************************
camera_fb_t *  get_good_jpeg() 
{
  //   Объявляем fb типа camera_fb_t — структура в ESP32, которая содержит указатель 
  // на данные кадра изображения, полученного с камеры и некоторые метаданные: ширину и 
  // высоту изображения, а также длину буфера, в котором оно находится.
  //   Для получения изображения с камеры используется функция esp_camera_fb_get,
  // которая не принимает аргументов и возвращает указатель на структуру типа camera_fb_t. 
  //   Структура camera_fb_t включает следующие поля:
  // uint8_t * buf      — указатель на пиксельные данные;
  // size_t len         — длина буфера в байтах;
  // size_t width       — ширина изображения в пикселях;
  // size_t height      — высота изображения в пикселях;
  // pixformat_t format — формат структуры пиксельных данных;
  // timeval timestamp  — отметка времени.
  //   Функция esp_camera_fb_get() выделяет память для поля buf. После использования буфера 
  // память освобождается с помощью функции esp_camera_fb_return(). 
  //   Структура camera_fb_t используется для получения кадра из камеры. Например, 
  // в коде может быть объявлена переменная, которая содержит указатель на структуру camera_fb_t, 
  // и вызывается функция esp_camera_fb_get(). Эта функция не принимает аргументов 
  // и возвращает указатель на структуру camera_fb_t.
  //   Важно: при работе с камерой рекомендуется освобождать память, выделенную 
  // функцией esp_camera_fb_get(). Это позволяет использовать буфер изображения 
  // повторно, что полезно, например, при непрерывном захвате новых снимков. 
  
  // Как устроен jpg-файл? 
  //   Файл поделен на секторы, предваряемые маркерами. Маркеры имеют длину 2 байта, 
  // причем первый байт [FF]. Почти все секторы хранят свою длину в следующих 2 байта после маркера.
  // [FF D8] — маркер начала. Он всегда находится в начале всех jpg-файлов. Следом идут 
  // байты [FF FE]. Это маркер, означающий начало секции с комментарием. Например, следующие 
  // 2 байта [00 04] — длина секции (включая эти 2 байта). Значит в следующих двух [3A 29] — сам комментарий. 
  // Это коды символов ":" и ")", т.е. обычного смайлика.
  //   Маркер [FF DB] называется DQT — таблица квантования. Маркер [FF C0], это SOF0 - 
  // означает, что изображение закодировано базовым методом (сушествуют и другие методы,
  // например, progressive-метод, когда сначала загружается изображение с низким 
  // разрешением, а потом и нормальная картинка.
  //   Маркер [FF C4]: DHT (таблица Хаффмана).
  //   Маркер [FF DA]: SOS (Start of Scan).
  //   [FF D9] — маркер EOI, что означает конец изображения. При этом данные сжатого 
  // изображения никогда не содержат маркер [FF D9] (байты FF всегда следуют за байтом 00). 
  // Однако некоторые поля могут содержать этот маркер.
  
  camera_fb_t * fb;
  long start;
  // Инициируем нулевую попытку захвата изображения камеры
  int failures = 0;
  // Делаем до 10 попыток захвата изображения,
  // обычно цикл завершается по break
  do 
  {
    int fblen = 0;
    int foundffd9 = 0;
    // Отмечаем начало процедуры взятия изображения с камеры
    long bp = millis();
    long mstart = micros();
    // Выделяем память и делаем изображение
    fb = esp_camera_fb_get();
    if (!fb) 
    {
      Serial.println("get_good_jpeg: Не удалось выполнить захват с камеры");
      failures++;
    } 
    else 
    {
      // Определяем время ушедшее на получение изображения (микрос)
      long mdelay = micros() - mstart;
      int get_fail = 0;
      totalp = totalp + millis() - bp;
      time_in_camera = totalp;
      fblen = fb->len;
      // Отлавливаем признак конца изображения в последних 1025 байтах
      for (int j = 1; j <= 1025; j++) 
      {
        if (fb->buf[fblen - j] != 0xD9) 
        {
          // no d9, try next for
        } 
        else 
        { 
          // Через предшествующий байт убеждаемся, что это точно конец изображения
          if (fb->buf[fblen - j - 1] == 0xFF ) 
          {     
            // Отмечаем, что кадр обычный ("конец файла найден сразу")
            if (j == 1) 
            {
              normal_jpg++;
            } 
            // Отмечаем, что кадр расширенный ("имеет хвостик с доп.информацией")
            else 
            {
              extend_jpg++;
            }
            foundffd9 = 1;  // отметили, что кадр хороший
            // Lots_of_Stats = true, включена трассировка
            if (Lots_of_Stats) 
            {
              / *
              if (j > 9000) 
              {
                // Ранее 900 - иногда случалось на 2640
                jpr("9000: Кадр %d, длина %d, Extra %d ", frame_cnt, fblen, j - 1 );
                logfile.flush();
              }
              * /
              // Отлавливаем и помечаем 50-ые кадры для их показа при трассировке
              if ( (frame_cnt % 1000 == 50) || (frame_cnt < 1000 && frame_cnt % 100 == 50)) 
              {
                gframe_cnt = frame_cnt;
                gfblen = fblen;
                gj = j;
                gmdelay = mdelay;
                do_it_now = 1;
              }
            }
            break;
          }
        }
      }
      // Отмечаем плохой кадр и чистим буфер камеры (кадр не уйдет в выходной файл)
      if (!foundffd9) 
      {
        bad_jpg++;
        jprln("Плохой кадр %d, длина = %d", frame_cnt, fblen);
        esp_camera_fb_return(fb);
        failures++;
      } 
      else 
      {
        break;
      }
    }

  } 
  while (failures < 10);  

  // !!! Если мы получаем 10 плохих кадров подряд, значит, параметры качества 
  // слишком высоки - понизьте их (+5) и запустите новый ролик
  if (failures == 10) 
  {
    jprln("\n10 плохих кадров подряд!");
    sensor_t * ss = esp_camera_sensor_get();
    int qual = ss->status.quality ;
    ss->set_quality(ss, qual + 5);
    quality = qual + 5;
    jprln("Снижение качества из-за сбоев кадров: %d -> %d\n", qual, qual + 5);
    delay(1000);
    start_record = 0;
  }
  return fb;
}

// ****************************************************************************
// *                    Открыть avi-файл и записать заголовки                 *
// ****************************************************************************
static void start_avi() 
{
  // Отмечаем начало работы с камерой и файлом avi - инициируем переменные
  startms = millis();    // начало работы с камерой и файлом avi
  totalp=0;              // общее время съемки всех кадров записанного файла avi
  totalw=0;              // общее время записи всех кадров файла avi
  jpeg_size=0;           // размер текущего кадра для сбора статистика
  movi_size=0;           // текущий размер видео-файла для статистики
  uVideoLen = 0;         // общий размер всех блоков кадров в видео-файле
  idx_offset = 4;        // индексные метки кадров в видео-файле
  bad_jpg = 0;           // количество плохих кадров
  extend_jpg = 0;        // количество расширенных кадров
  normal_jpg = 0;        // количество нормальных кадров
  time_in_loop = 0;
  wait_for_cam = 0;      // общее время ожидания камеры между съемками
  time_in_camera=0;      // общее время работы камеры
  time_in_good=0;        // время камеры с получением целых (хороших) кадров
  delay_wait_for_sd=0;   // общее время ожидания записи на SD
  time_in_sd=0;          // время, потраченное на работу с sd-картой
  time_total = 0;        // общее время съёмки и записи на SD
  time_in_web1 = 0;      // время пребывания на страницах браузера
  very_high = 0;
  
  // Отмечаем начало работы функции
  long start = millis();
  // Создаем/открываем каталог и начинаем запись видео-файла
  char the_directory[50];
  jprln("Начинается формирование avi по снимаемым кадрам");
  sprintf(the_directory,"/%s%03d",devname,file_group);
  SD_MMC.mkdir(the_directory);
  sprintf(avi_file_name, "/%s%03d/%s%03d.%03d.avi",  devname, file_group, devname, file_group, file_number);
  file_number++;
  avifile = SD_MMC.open(avi_file_name, "w");
  idxfile = SD_MMC.open("/idx.tmp", "w");
  if (avifile) 
  {
    jpr("Файл открыт: %s\n", avi_file_name);
  }  
  else  
  {
    jprln("Не получилось открыть файл avi, контроллер будет перезагружен");
    major_fail();
  }
  if (idxfile)  
  {
    //Serial.printf("File open: %s\n", "//idx.tmp");
  }  
  else  
  {
    jpr("Не получилось открыть файл /idx.tmp, контроллер будет перезагружен");
    major_fail();
  }
  // Формируем и записываем в avi заголовок файла в соответствии с размером изображения
  for (int i = 0; i < AVIOFFSET; i++) 
  {
    char ch = pgm_read_byte(&avi_header[i]);
    buf[i] = ch;
  }
  memcpy(buf + 0x40, frameSizeData[framesize].frameWidth, 2);
  memcpy(buf + 0xA8, frameSizeData[framesize].frameWidth, 2);
  memcpy(buf + 0x44, frameSizeData[framesize].frameHeight, 2);
  memcpy(buf + 0xAC, frameSizeData[framesize].frameHeight, 2);
  size_t err = avifile.write(buf, AVIOFFSET);
  // Назначаем скорость воспроизведения (ex_fps) на основании данных:
  // frame_interval - интервал между записями кадров в миллисекундах, по умолчанию 0 => самая быстрая запись;
  // framesize - формат изображения, по умолчанию 13 => hd 720p 1280x720;
  // speed_up_factor - ускорение воспроизведения, по умолчанию 1 => в реальном времени
  uint8_t ex_fps = 1;
  if (frame_interval == 0) 
  {
    if (framesize >= 11)                 
    {
      ex_fps = 12.5 * speed_up_factor; // от 12.5 кадров в секунду
    } 
    else 
    {
      ex_fps = 25.0 * speed_up_factor; // от 25 кадров в секунду
    }
  } 
  else 
  {
    ex_fps = round(1000.0 / frame_interval * speed_up_factor);
  }
  
  // SeekSet — один из режимов функции file.seek(offset, mode) в файловой системе SPIFFS, 
  // используемой в микроконтроллере ESP32. В зависимости от значения режима функция 
  // перемещает текущую позицию в файле:
  // SeekSet — позиция устанавливается на отсчет байтов с начала файла.
  // SeekCur — текущая позиция перемещается на отсчет байтов.
  // SeekEnd — позиция устанавливается на отсчет байтов с конца файла.
  // Функция возвращает true, если позиция была установлена успешно.
  
  // Включаем в заголовок скорость воспроизведения 
  avifile.seek( 0x84 , SeekSet);
  print_quartet((int)ex_fps, avifile);
  // Указываем магическое число 3, которое означает, что количество кадров не записано
  avifile.seek( 0x30 , SeekSet);
  print_quartet(3, avifile);  // magic number 3 means frame count not written // 61.3
  // Перемещаем указатель на после заголовка в файле
  avifile.seek( AVIOFFSET, SeekSet);
  jprln("Запускается запись видео на %d секунд", avi_length);
  // Пересчитываем время работы на записи на SD-карту при записи видео
  time_in_sd += (millis() - start);
  // Очищаем оставшуюся информацию в буферах файлов, для того,
  // чтобы начать с новых данных, а также для того, чтобы последующие вызовы функции 
  // available() показывали, что данных нет, пока не поступят новые. 
  // Это важно, чтобы оставшиеся данные не мешали последующим чтениям. 
  logfile.flush();
  avifile.flush();
} 
// ****************************************************************************
// *             Записать индекс, закрыть  файлы и вывести протокол           *
// ****************************************************************************
static void end_avi() 
{
  long start = millis();
  unsigned long current_end = avifile.position();
  if (frame_cnt < 5) 
  {
    jprln("Запись испорчена, менее 5 кадров, убираем индекс");
    idxfile.close();
    avifile.close();
    int xx = remove("/idx.tmp");
    int yy = remove(avi_file_name);
  } 
  else 
  {
    // Фиксируем общее время работы с камерой и файлом avi
    elapsedms = millis() - startms;
    // Считаем среднюю частоту кадров в секунду
    float fRealFPS = (1000.0f * (float)frame_cnt) / ((float)elapsedms) * speed_up_factor;
    float fmicroseconds_per_frame = 1000000.0f / fRealFPS;
    uint8_t iAttainedFPS = round(fRealFPS);
    // Считаем среднее время кадра
    uint32_t us_per_frame = round(fmicroseconds_per_frame);
    // Считаем максимальную скорость передачи байт в секунду
    unsigned long max_bytes_per_sec = (1.0f * movi_size * iAttainedFPS) / frame_cnt;

    // Изменяем заголовок в начале файла JPEG, заменив различные заполнители
    avifile.seek(4, SeekSet);
    print_quartet(movi_size + 240 + 16 * frame_cnt + 8 * frame_cnt, avifile);
    avifile.seek(0x20 , SeekSet);
    print_quartet(us_per_frame, avifile);
    avifile.seek(0x24 , SeekSet);
    print_quartet(max_bytes_per_sec, avifile);
    avifile.seek(0x30 , SeekSet);
    print_quartet(frame_cnt, avifile);
    avifile.seek(0x8c , SeekSet);
    print_quartet(frame_cnt, avifile);
    avifile.seek(0x84 , SeekSet);
    print_quartet((int)iAttainedFPS, avifile);
    avifile.seek(0xe8 , SeekSet);
    print_quartet(movi_size + frame_cnt * 8 + 4, avifile);
    avifile.seek(current_end, SeekSet);
    idxfile.close();
    size_t i1_err = avifile.write(idx1_buf, 4);
    print_quartet(frame_cnt * 16, avifile);

    idxfile = SD_MMC.open("/idx.tmp", "r");
    if (idxfile)  
    {
      //Serial.printf("File open: %s\n", "//idx.tmp");
      //logfile.printf("File open: %s\n", "/idx.tmp");
    }  
    else  
    {
      jprln("Не удалось открыть индексный файл");
      major_fail();
    }
    // Записываем индексную информацию
    char * AteBytes;
    AteBytes = (char*) malloc (8);
    for (int i = 0; i < frame_cnt; i++) 
    {
      size_t res = idxfile.readBytes(AteBytes, 8);
      size_t i1_err = avifile.write(dc_buf, 4);
      size_t i2_err = avifile.write(zero_buf, 4);
      size_t i3_err = avifile.write((uint8_t *)AteBytes, 8);
    }
    free(AteBytes);

    idxfile.close();
    avifile.close();

 
    jprln("\n*** Видео записано и сохранено ***");
    jprln("---");
    jprln("Снято и записано %5d кадров за %5d секунд", frame_cnt, elapsedms / 1000);
    jprln("Размер файла составляет %u байт", movi_size + 12 * frame_cnt + 4);
    jprln("Средняя частота равна %5.2f кадров в секунду", fRealFPS);
    jprln("Максимальная скорость передачи данных %lu байт в секунду", max_bytes_per_sec);
    jprln("Cреднее время длительности кадра %d мксек", us_per_frame);
    jprln("Средняя длина кадра составляет %d байт", uVideoLen / frame_cnt);
    jprln("Среднее время съемки (мсек) %f", 1.0 * totalp / frame_cnt);
    jprln("Среднее время записи (мсек) %f", 1.0 * totalw / frame_cnt );
    jprln("Количество нормальных  кадров %3.1f %", 100.0 * normal_jpg / frame_cnt );
    jprln("Количество расширенных кадров %3.1f %", 100.0 * extend_jpg / frame_cnt );
    jprln("Количество сломанных   кадров %3.1f %", 100.0 * bad_jpg / frame_cnt);
    jprln("Медленная запись на SD-карту %d, %5.3f %", very_high, 100.0 * very_high / frame_cnt, 5 );
    jprln("Проиндексировано (записано) %d кадров", frame_cnt);

    //    int resss = SD_MMC.mkdir(the_directory);
    //    Serial.printf("remake the foler ?? %d\n",resss);
    int xx = SD_MMC.remove("/idx.tmp");
  }
  jprln("---");
  time_in_sd += (millis() - start);
  time_total = millis() - startms;
  jpr("Время ожидания камеры %10dms, %4.1f%%\n", wait_for_cam , 100.0 * wait_for_cam  / time_total);
  jpr("Время съёмки          %10dms, %4.1f%%\n", time_in_camera, 100.0 * time_in_camera / time_total);
  jpr("Время ожидания SD     %10dms, %4.1f%%\n", delay_wait_for_sd , 100.0 * delay_wait_for_sd  / time_total);
  jpr("Время записи на SD    %10dms, %4.1f%%\n", time_in_sd    , 100.0 * time_in_sd     / time_total);
  jpr("Время работы браузера %10dms, %4.1f%%\n", time_in_web1  , 100.0 * time_in_web1   / time_total);
  jpr("Общее время           %10dms, %4.1f%%\n", time_total    , 100.0 * time_total     / time_total);
  logfile.flush();
  if (file_number == 100) 
  {
    reboot_now = true;
  }
}
// ****************************************************************************
// *         Cохранить очередной кадр в avi-файл, обновить индекс -           *
// *                указатель для fb на добавляемый кадр                      *
// ****************************************************************************
static void another_save_avi(uint8_t* fb_buf, int fblen ) 
{
  long start = millis();     // отметка начала записи на SD в этой функции
  int fb_block_length;       // длина тукущего блока кадра для записи (не более fbs*1024)
  uint8_t* fb_block_start;   // начало записи текущего блока кадра
  long bw = millis();        // отметка начала записи кадра в файл 
  int block_delay[10];       // накапливающиеся длительности записи первых 10 блоков

  // Фиксируем фактическую длину кадра
  jpeg_size = fblen;
  // Пересчитываем длину кадра, чтобы она была кратна 4
  remnant = (4 - (jpeg_size & 0x00000003)) & 0x00000003;
  int jpeg_size_rem = jpeg_size + remnant;
  // В первые 8 байт укладываем новую длину кадра, как Big Endian
  fb_record_static[0] = 0x30;       // "00dc"
  fb_record_static[1] = 0x30;
  fb_record_static[2] = 0x64;
  fb_record_static[3] = 0x63;
  fb_record_static[4] = jpeg_size_rem % 0x100;
  fb_record_static[5] = (jpeg_size_rem >> 8) % 0x100;
  fb_record_static[6] = (jpeg_size_rem >> 16) % 0x100;
  fb_record_static[7] = (jpeg_size_rem >> 24) % 0x100;

  // Ускорение записи на SD-карту на ESP32-CAM с камерой OV2640
  // (https://github.com/espressif/esp32-camera/issues/182)
  
  / *
    Информация о том, как повысить скорость записи изображений в формате JPEG с камеры esp32 на SD-карту.
  Любой, кто пытался записать фотографии с камеры esp32 на SD-карту, знает эту строку:
  file.write(fb->buf, fb->len);
    Это не лучшая строка. Она записывает данные из ПЗУ в оперативную память через шину SPI 
  на SD-карту с использованием SPI, 4-битного sd_mmc или 1-битного sd_mmc, в зависимости 
  от того, как настроена SD-карта.
  
    Но если заменить эту строку на:
    
  uint8_t framebuffer_static[32 * 1024];
  memcpy(framebuffer_static, fb->buf,  fb->len);
  file.write(framebuffer_static, fb->len);
   
    Это ускорит операцию более чем в 5 раз. Здесь вы выполняете операцию memcpy, чтобы перенести 
  фрейм из PSRAM в статическую оперативную память по шине SPI, а затем выполняете 
  запись из статической оперативной памяти в SD-карту. Когда данные передаются напрямую 
  с ПЗУ в оперативную память, драйвер должен захватывать небольшие блоки данных для 
  каждой операции записи на SD-карту, поэтому SPI-интерфейс с ПЗУ постоянно запускается 
  и останавливается, замедляя работу, в то время как memcpy работает на полную мощность 
  с буфером в 32 КБ, а функция записи на SD-карту работает с той скоростью, с которой 
  может работать сама SD-карта.
    Единственная сложность заключается в том, что вам придется использовать блок 
  ценной статической оперативной памяти, а также разбивать большие кадры на несколько 
  фрагментов по 32 или 64 КБ.
    При таком подходе можно записать видео в формате AVI на дешевую SD-карту класса 10 
  (круг 10 U1) с максимальной производительностью камеры OV2640 — 6 кадров в секунду 
  в режиме UXGA и 25 кадров в секунду в режиме SVGA.
  * /
  
  // Записываем первый или единственный блок кадра размером fbs*1024 байт
  int block_num = 0;
  fb_block_start = fb_buf;
  if (fblen > fbs * 1024 - 8 )  // fbs=1, это размер статического буфера кадров в Кб
  {                     
    fb_block_length = fbs * 1024;
    fblen = fblen - (fbs * 1024 - 8);
    memcpy(fb_record_static + 8, fb_block_start, fb_block_length - 8);
    // Передвигаем указатель на следующий блок
    fb_block_start = fb_block_start + fb_block_length - 8;
  } 
  else 
  {
    fb_block_length = fblen + 8  + remnant;
    memcpy(fb_record_static + 8, fb_block_start,  fblen);
    fblen = 0;
  }
  size_t err = avifile.write(fb_record_static, fb_block_length);
  if (err != fb_block_length) 
  {
    start_record = 0;
    jprln("Ошибка при записи в avi: %d, длина блока = %d", err, fb_block_length);
    return;
  }
  if (block_num < 10) block_delay[block_num++] = millis() - bw;

  // Записываем второй и последующие блоки кадра размером fbs*1024 байт
  while (fblen > 0) 
  {
    if (fblen > fbs * 1024) 
    {
      fb_block_length = fbs * 1024;
      fblen = fblen - fb_block_length;
    } 
    else 
    {
      fb_block_length = fblen  + remnant;
      fblen = 0;
    }
    memcpy(fb_record_static, fb_block_start, fb_block_length);
    size_t err = avifile.write(fb_record_static,  fb_block_length);
    if (err != fb_block_length) 
    {
      jprln("Ошибка при записи в avi: %d, длина блока = %d", err, fb_block_length);
      return;
    }
    if (block_num < 10) block_delay[block_num++] = millis() - bw;
    // Перемещаем указатель по буферу кадра дальше
    fb_block_start = fb_block_start + fb_block_length;
    delay(0);
  }
  // Фиксируем длину видео с текущим кадром
  movi_size += jpeg_size;
  // Пересчитываем общий размер всех блоков кадров в видео-файле
  uVideoLen += jpeg_size;
  // Записываем 2 uint32_t в порядке возрастания в текущей позиции файла
  print_2quartet(idx_offset, jpeg_size, idxfile);
  // Смещаем индексные метку кадров в видео-файле
  idx_offset = idx_offset + jpeg_size + remnant + 8;
  // Увеличиваем длину видео-файла на remnant последнего блока  
  movi_size = movi_size + remnant;
  // Трассируем 50-ые кадры из первой тысячи по признаку do_it_now,
  // установленному в get_good_jpeg()
  if (do_it_now == 1 ) 
  {  // && frame_cnt < 1011
    do_it_now = 0;
    jprln("Кадр:  %6d, длина %6d, время камеры %7d, время записи %4d", gframe_cnt, gfblen, gmdelay / 1000, millis() - bw);
    logfile.flush();
  }
  // Пересчитываем общее время записи всех кадров файла AVI
  totalw = totalw + millis() - bw;
  // Пересчитываем время, потраченное на работу с sd-картой
  time_in_sd += (millis() - start);

  // Отмечаем кадры, которые долго записывались
  if ((millis() - bw) > totalw / frame_cnt * 10) 
  {
    unsigned long x = avifile.position();
    jprln("Кадр:  %6d, время записи велико к среднему %4d > %4d, позиция в файле %X, ",  frame_cnt, millis() - bw, (totalw / frame_cnt), x );
    very_high++;
    / *
    for (int i = 1; i < block_num; i++) 
    {
      jpr("Блок %d, время %5d; ", i, block_delay[i] - block_delay[i - 1]);
    }
    * /
  }
  // Освобождаем буферы
  avifile.flush();
  idxfile.flush();
} 

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// the_camera_loop()  - цикл фотографирования и записи avi-имеет наибольший приоритет = 4;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void the_camera_loop (void* pvParameter) 
{
  long wait_for_cam_start;       // Промежуточная точка начала ожидания камеры
  long delay_wait_for_sd_start;  // Промежуточная точка начала ожидания работы с SD

  print_mem("MEM - стартовала задача the_camera_loop        ");
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
      if (we_are_already_stopped == 0) jpr("\nОтсоедините Pin12 от GND для того, чтобы начать запись или http://192.168.1.100/start \n");
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
      
      jprln("Началась видеозапись на %d мс. ", avi_start_time);
      jprln("Размер кадра %d, качество %d, время %d секунд\n", framesize, quality, avi_length);
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
      jprln("Завершается запись avi-файла");
      restart_now = false;
      if (blinking)  digitalWrite(33, frame_cnt % 2);

      end_avi();                                

      if (blinking) digitalWrite(33, HIGH);          // light off

      // Устанавливаем флаг "удалить старые файлы по завершению записи текущего файла avi"
      delete_old_stuff_flag = 1;
      delay(50);

      avi_end_time = millis();

      float fps = 1.0 * frame_cnt / ((avi_end_time - avi_start_time) / 1000) ;

      jpr("End the avi at %d.  It was %d frames, %d ms at %.2f fps...\n", millis(), frame_cnt, avi_end_time, avi_end_time - avi_start_time, fps);

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

          if ( (Lots_of_Stats && frame_cnt < 1011) || (Lots_of_Stats && frame_cnt % 1000 == 10)) 
          {
            jprln("Всего: %5d кадров за %6.1f секунд, среднее недавних 100 кадров: размер и частота %6.1f kb, %.2f fps", frame_cnt, 0.001 * (millis() - avi_start_time), 1.0 / 1024  * most_recent_avg_framesize, most_recent_fps);
          }
          bytes_before_last_100_frames = movi_size;
          time_before_last_100_frames = millis();
        }
      }
    }
  }
}
*/

// *************************************************************** camera.h ***
