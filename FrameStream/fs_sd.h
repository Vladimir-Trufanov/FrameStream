/** Arduino, ESP32, C/C++ ***************************************** fs_sd.h ***
 * 
 *                                                 Обслужить работу с SD-картой
 *                                                     
 * v1.0.4, 24.02.2026                                 Автор:      Труфанов В.Е.
 * Copyright © 2026 tve                               Дата создания: 24.01.2026
**/

#pragma once  

// MicroSD
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "FS.h"
#include <SD_MMC.h>

#include "inimem.h"
#include "fs_eprom.h"

//#include "fs_trass.h"

// Writes an uint32_t in Big Endian at current file position
static void inline print_quartet(unsigned long i, File fd); 
// Writes 2 uint32_t in Big Endian at current file position
static void inline print_2quartet(unsigned long i, unsigned long j, File fd);
// Открыть avi-файл и записать заголовки
static void start_avi(); 
// Считать или создать файл конфигурации "config.txt" и настроить переменные
void read_config_file();

File avifile;
File idxfile;
String cssid1, cssid2, cssid3;
String cpass1, cpass2, cpass3;

bool configfile = false;

char file_to_edit[50] = "/JamCam0481.0007.avi"; //61.3
char avi_file_name[100];    // название записываемого файла *.avi
long avi_start_time = 0;    // время начала видео-записи
long avi_end_time = 0;

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

static int i = 0;
uint16_t frame_cnt = 0;
uint16_t remnant = 0;
uint32_t length = 0;
uint32_t startms;
uint32_t elapsedms;

long boot_time = 0;

#define BUFFSIZE 512
uint8_t buf[BUFFSIZE];

uint8_t zero_buf[4] = {0x00, 0x00, 0x00, 0x00};
uint8_t dc_buf[4] = {0x30, 0x30, 0x64, 0x63};    // "00dc"
uint8_t dc_and_zero_buf[8] = {0x30, 0x30, 0x64, 0x63, 0x00, 0x00, 0x00, 0x00};

uint8_t avi1_buf[4] = {0x41, 0x56, 0x49, 0x31};    // "AVI1"
uint8_t idx1_buf[4] = {0x69, 0x64, 0x78, 0x31};    // "idx1"

// Окончательные данные по кадрам, защищенные мьютексом
uint8_t* fb_record;           // копия буфера снятого кадра
int fb_record_len;            // длина буфера снятого кадра
long fb_record_time = 0;      // время записи снятого кадра с начала запуска программы (мс)

long totalp;                  // общее время съемки всех кадров записанного файла avi
long totalw;                  // общее время записи всех кадров файла avi
long time_in_loop=0;
long wait_for_cam=0;          // общее время ожидания камеры между съемками
long time_in_camera=0;        // общее время работы камеры
long time_in_good=0;          // время камеры с получением целых (хороших) кадров
long time_total=0;            // общее время съёмки и записи на SD
long time_in_web1=0;          // время пребывания на страницах браузера
long time_in_web2=0;
long delay_wait_for_sd=0;     // общее время ожидания записи на SD
long time_in_sd=0;            // время, потраченное на работу с sd-картой
int very_high = 0;

unsigned long jpeg_size=0;    // размер текущего кадра для сбора статистики
unsigned long movi_size=0;    // текущий размер видео-файла для статистики

long bytes_before_last_100_frames=0;  // размер видео с последними 100 кадрами
long time_before_last_100_frames=0;   // время записи последних 100 кадров
float most_recent_fps=0;              // количество недавних кадров в секунду
int most_recent_avg_framesize=0;      // средний размер недавних кадров

uint32_t uVideoLen = 0;        // общий размер всех блоков кадров в видео-файле
//unsigned long idx_offset=0;    // индексные метки кадров в видео-файле
unsigned long idx_offset = 4;

int bad_jpg = 0;               // количество плохих кадров
int extend_jpg = 0;            // количество расширенных кадров
int normal_jpg = 0;            // количество нормальных кадров
int we_are_already_stopped=0;  // 1 - "видео-запись уже остановлена"

// Сбрасываем флаг "удалить старые файлы по завершению записи текущего файла avi"
int delete_old_stuff_flag = 0;

// Считать или создать файл конфигурации "config.txt" и настроить переменные
#include "config.h"
void read_config_file() 
{
  // if there is a config.txt, use it plus defaults
  // else use defaults, and create a config.txt
  // put a file "config.txt" onto SD card, to set parameters different from your hardcoded parameters
  // it should look like this - one paramter per line, in the correct order, followed by 2 spaces, and any comments you choose
  String junk;
  String cname ;
  int cframesize ;
  int cquality = 12 ;
  int cbuffersconfig = 4;
  int clength ;
  int cinterval ;
  int cspeedup ;
  int cstreamdelay ;
  String czone ;
  delay(1000);

  File config_file = SD_MMC.open("/config2.txt", "r");

  if (config_file) {
    jpr("Opened config2.txt from SD");
  } else {
    jpr("Failed to open config2.txt - writing a default");

    // lets make a simple.txt config file
    File new_simple = SD_MMC.open("/config2.txt", "w");
    new_simple.print(config_txt);
    new_simple.close();

    file_group = 1;
    file_number = 1;

    do_eprom_write();

    config_file = SD_MMC.open("/config2.txt", "r");
  }

  jpr("Reading config2.txt\n");
  cname = config_file.readStringUntil(' ');
  junk = config_file.readStringUntil('\n');
  cframesize = config_file.parseInt();
  junk = config_file.readStringUntil('\n');

  clength = config_file.parseInt();
  junk = config_file.readStringUntil('\n');
  cinterval = config_file.parseInt();
  junk = config_file.readStringUntil('\n');
  cspeedup = config_file.parseInt();
  junk = config_file.readStringUntil('\n');
  cstreamdelay = config_file.parseInt();
  junk = config_file.readStringUntil('\n');
  czone = config_file.readStringUntil(' ');
  junk = config_file.readStringUntil('\n');
  cssid1 = config_file.readStringUntil('#');
  junk = config_file.readStringUntil('\n');
  cpass1 = config_file.readStringUntil('#');
  junk = config_file.readStringUntil('\n');
  cssid2 = config_file.readStringUntil(' ');
  junk = config_file.readStringUntil('\n');
  cpass2 = config_file.readStringUntil(' ');
  junk = config_file.readStringUntil('\n');
  cssid3 = config_file.readStringUntil(' ');
  junk = config_file.readStringUntil('\n');
  cpass3 = config_file.readStringUntil(' ');
  junk = config_file.readStringUntil('\n');
  config_file.close();

  jpr("=========   Data from config2.txt and defaults  =========\n");
  jpr("Name %s\n", cname);
  jpr("Framesize %d\n", cframesize);
  jpr("Quality %d\n", cquality);
  jpr("Buffers config %d\n", cbuffersconfig);
  jpr("Length %d\n", clength);
  jpr("Interval %d\n", cinterval);
  jpr("Speedup %d\n", cspeedup);
  jpr("Streamdelay %d\n", cstreamdelay);

  jpr("Zone len %d, %s\n", czone.length(), czone.c_str());
  jpr("ssid1 %s\n", cssid1);
  //jpr("pass1 %s\n", cpass1);
  jpr("ssid2 %s\n", cssid2);
  //jpr("pass2 %s\n", cpass2);
  jpr("ssid3 %s\n", cssid3);
  jpr("pass3 %s\n", cpass3);


  framesize = cframesize;
  quality = cquality;
  buffersconfig = cbuffersconfig;
  avi_length = clength;
  frame_interval = cinterval;
  speed_up_factor = cspeedup;
  stream_delay = cstreamdelay;
  configfile = true;
  TIMEZONE = czone;

  cname.toCharArray(devname, cname.length() + 1);

}

// Writes an uint32_t in Big Endian at current file position
static void inline print_quartet(unsigned long i, File fd) 
{
  uint8_t y[4];
  y[0] = i % 0x100;
  y[1] = (i >> 8) % 0x100;
  y[2] = (i >> 16) % 0x100;
  y[3] = (i >> 24) % 0x100;
  size_t i1_err = fd.write(y , 4);
}
// Writes 2 uint32_t in Big Endian at current file position
static void inline print_2quartet(unsigned long i, unsigned long j, File fd) 
{
  uint8_t y[8];
  y[0] = i % 0x100;
  y[1] = (i >> 8) % 0x100;
  y[2] = (i >> 16) % 0x100;
  y[3] = (i >> 24) % 0x100;
  y[4] = j % 0x100;
  y[5] = (j >> 8) % 0x100;
  y[6] = (j >> 16) % 0x100;
  y[7] = (j >> 24) % 0x100;
  size_t i1_err = fd.write(y , 8);
}
// start_avi - open the files and write in headers
static void start_avi() 
{
  char the_directory[50];

  long start = millis();

  jpr("Starting an avi ");
  sprintf(the_directory, "/%s%03d",  devname, file_group);
  SD_MMC.mkdir(the_directory);

  sprintf(avi_file_name, "/%s%03d/%s%03d.%03d.avi",  devname, file_group, devname, file_group, file_number);

  file_number++;

  avifile = SD_MMC.open(avi_file_name, "w");
  idxfile = SD_MMC.open("/idx.tmp", "w");

  if (avifile) {
    jpr("File open: %s\n", avi_file_name);
  }  else  {
    jpr("Could not open avi file");
    major_fail();
  }

  if (idxfile)  {
    //Serial.printf("File open: %s\n", "//idx.tmp");
  }  else  {
    jpr("Could not open file /idx.tmp");
    major_fail();
  }

  for ( i = 0; i < AVIOFFSET; i++) {
    char ch = pgm_read_byte(&avi_header[i]);
    buf[i] = ch;
  }

  memcpy(buf + 0x40, frameSizeData[framesize].frameWidth, 2);
  memcpy(buf + 0xA8, frameSizeData[framesize].frameWidth, 2);
  memcpy(buf + 0x44, frameSizeData[framesize].frameHeight, 2);
  memcpy(buf + 0xAC, frameSizeData[framesize].frameHeight, 2);

  size_t err = avifile.write(buf, AVIOFFSET);

  uint8_t ex_fps = 1;
  if (frame_interval == 0) {
    if (framesize >= 11) {
      ex_fps = 12.5 * speed_up_factor ;;
    } else {
      ex_fps = 25.0 * speed_up_factor;
    }
  } else {
    ex_fps = round(1000.0 / frame_interval * speed_up_factor);
  }

  avifile.seek( 0x84 , SeekSet);
  print_quartet((int)ex_fps, avifile);

  avifile.seek( 0x30 , SeekSet);
  print_quartet(3, avifile);  // magic number 3 means frame count not written // 61.3

  avifile.seek( AVIOFFSET, SeekSet);

  jpr("Recording %d seconds\n", avi_length);

  startms = millis();

  totalp = 0;
  totalw = 0;

  jpeg_size = 0;
  movi_size = 0;
  uVideoLen = 0;
  idx_offset = 4;

  bad_jpg = 0;
  extend_jpg = 0;
  normal_jpg = 0;

  time_in_loop = 0;
  time_in_camera = 0;
  time_in_sd = 0;
  time_in_good = 0;
  time_total = 0;
  time_in_web1 = 0;
  time_in_web2 = 0;
  delay_wait_for_sd = 0;
  wait_for_cam = 0;
  very_high = 0;

  time_in_sd += (millis() - start);

  logfile.flush();
  avifile.flush();

} // end of start avi

// ****************************************************************************
// *                    Открыть avi-файл и записать заголовки                 *
// ****************************************************************************
/*
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
  //jprln("Начинается формирование avi по снимаемым кадрам");
  sprintf(the_directory,"/%s%03d",devname,file_group);
  SD_MMC.mkdir(the_directory);
  sprintf(avi_file_name, "/%s%03d/%s%03d.%03d.avi",  devname, file_group, devname, file_group, file_number);
  file_number++;
  avifile = SD_MMC.open(avi_file_name, "w");
  idxfile = SD_MMC.open("/idx.tmp", "w");
  if (avifile) 
  {
    //jpr("Файл открыт: %s\n", avi_file_name);
  }  
  else  
  {
    //jprln("Не получилось открыть файл avi, контроллер будет перезагружен");
    //major_fail();
  }
  if (idxfile)  
  {
    //Serial.printf("File open: %s\n", "//idx.tmp");
  }  
  else  
  {
    //jpr("Не получилось открыть файл /idx.tmp, контроллер будет перезагружен");
    //major_fail();
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
  //jprln("Запускается запись видео на %d секунд", avi_length);
  // Пересчитываем время работы на записи на SD-карту при записи видео
  time_in_sd += (millis() - start);
  // Очищаем оставшуюся информацию в буферах файлов, для того,
  // чтобы начать с новых данных, а также для того, чтобы последующие вызовы функции 
  // available() показывали, что данных нет, пока не поступят новые. 
  // Это важно, чтобы оставшиеся данные не мешали последующим чтениям. 
  logfile.flush();
  avifile.flush();
} 
*/
/*
// Обеспечиваем ускорение записи на SD-карту [https://github.com/espressif/esp32-camera/issues/182],
// ранее было fbs=64 - столько КБ статической оперативной памяти 
// для psram -> буфер sram для записи на sd
#define fbs  1 
uint8_t fb_record_static[fbs * 1024 + 20]; 

//////////////////////////////
//61.3 oneframe find_a_frame (char * avi_file_name, int frame_pct) ; // from avi.cpp file

struct oneframe {
  uint8_t* the_frame;
  int the_frame_length;
  long the_frame_number;
  long the_frame_total;
};

*/
// ****************************************************************************
// *                                                Инициализировать SD-карту *
// *  CARD_NONE — карта не подключена;                                        *
// *  CARD_MMC  — карта MMC;                                                  *
// *  CARD_SD   — карта SDSC;                                                 *
// *  CARD_SDHC — карта SDHC.                                                 *
// ****************************************************************************
static esp_err_t init_sdcard()
{
  int succ = SD_MMC.begin("/sdcard", true, false, BOARD_MAX_SDMMC_FREQ, 7);
  if (succ) {
    Serial.printf("SD_MMC Begin: %d\n", succ);
    uint8_t cardType = SD_MMC.cardType();
    Serial.print("SD_MMC Card Type: ");
    if (cardType == CARD_MMC) {
      Serial.println("MMC");
    } else if (cardType == CARD_SD) {
      Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
      Serial.println("SDHC");
    } else {
      Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
  } else {
    Serial.printf("Failed to mount SD card VFAT filesystem. \n");
    Serial.println("Do you have an SD Card installed?");
    Serial.println("Check pin 12 and 13, not grounded, or grounded with 10k resistors!\n\n");
    major_fail();
  }
  return ESP_OK;
}
/*
static bool init_sdcard()
{
  ncardType=0;  // тип карты SD_MMC для инфо.сообщения
  bool Result=true;
  int succ = SD_MMC.begin("/sdcard", true, false, BOARD_MAX_SDMMC_FREQ, 7);
  if (succ) 
  {
    sayln("SD_MMC инициализирована успешно");
    uint8_t cardType = SD_MMC.cardType();
    cardSize = SD_MMC.cardSize() / (1024 * 1024);
    if (cardType == CARD_MMC)  ncardType=1;         // "MMC"
    else if (cardType == CARD_SD) ncardType=2;      // "SDSC"
    else if (cardType == CARD_SDHC) ncardType=3;    // "SDHC"
    else ncardType=4;                               // "неопределён"
  } 
  else 
  {
    sayln("Ошибка инициализации SD-карты на файловой системе VFAT\n");
    ncardType = 5;                                  // "карта не подключена"
    cardSize = 0;
    Result=false;
  }
  return Result;
}


//
// Reads an uint32_t in Big Endian at current file position
//
int read_quartet( File fd) 
{

  uint8_t y[4];
  size_t i1_err = fd.read(y , 4);
  uint32_t value = y[0] | y[1] << 8 | y[2] << 16 | y[3] << 24;
  //Serial.printf("read_quartet %d %d %d %d, %d\n", y[0], y[1], y[2], y[3], value);
  return value;
}
// ****************************************************************************
// *    Записать в файл беззнаковое целое, как 4 последовательных байта -     *
// *            от первого (с наименьшим адресом) к четвертому                *
// *                      c текущего положения в файле                        *
// ****************************************************************************
static void inline print_quartet(unsigned long i, File fd) 
{
  uint8_t y[4];
  y[0] = i % 0x100;
  y[1] = (i >> 8) % 0x100;
  y[2] = (i >> 16) % 0x100;
  y[3] = (i >> 24) % 0x100;
  size_t i1_err = fd.write(y,4);
}
// ****************************************************************************
// *     Записать uint32_t в порядке возрастания в текущей позиции файла      *
// ****************************************************************************
static void inline print_2quartet(unsigned long i, unsigned long j, File fd) 
{
  uint8_t y[8];
  y[0] = i % 0x100;
  y[1] = (i >> 8) % 0x100;
  y[2] = (i >> 16) % 0x100;
  y[3] = (i >> 24) % 0x100;
  y[4] = j % 0x100;
  y[5] = (j >> 8) % 0x100;
  y[6] = (j >> 16) % 0x100;
  y[7] = (j >> 24) % 0x100;
  size_t i1_err = fd.write(y,8);
}
//
// Writes an uint32_t in Big Endian at current file position
//
static void inline print_dc_quartet(unsigned long i, File fd) 
{
  uint8_t y[8];
  y[0] = 0x30;       // "00dc"
  y[1] = 0x30;
  y[2] = 0x64;
  y[3] = 0x63;

  y[4] = i % 0x100;
  y[5] = (i >> 8) % 0x100;
  y[6] = (i >> 16) % 0x100;
  y[7] = (i >> 24) % 0x100;
  size_t i1_err = fd.write(y , 8);
}

#include <list>
#include <tuple>

// ****************************************************************************
// *      Удалить самые старые файлы, чтобы освободить место на SD-диске      *
// ****************************************************************************
// Удаляем файлы и каталоги по списку, подготовленному функцией delete_old_stuff();
void deleteFolderOrFile(const char * val) 
{
  Serial.printf("Удаление: %s\n", val);
  File f = SD_MMC.open("/" + String(val));
  if (!f) 
  {
    //jpr("Ошибка открытия %s\n", val);
    return;
  }
  if (f.isDirectory()) 
  {
    File file = f.openNextFile();
    while (file) 
    {
      if (file.isDirectory()) 
      {
        Serial.print("  DIR : ");
        Serial.println(file.name());
      } 
      else 
      {
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("  SIZE: ");
        Serial.print(file.size());
        if (SD_MMC.remove("/" + String(val) + "/" + file.name())) 
        {
          Serial.println(" удалён");
        } 
        else 
        {
          Serial.println(" FAILED.");
        }
      }
      int total = SD_MMC.totalBytes()/(1024 * 1024);
      int used = SD_MMC.usedBytes()/(1024 * 1024);
      float full = 1.0 * used / total;
      Serial.println(full);
      if (full < 0.7) break;
      file = f.openNextFile();
    }
    f.close();
    // Удаляем опустевший каталог
    if (SD_MMC.rmdir("/" + String(val))) 
    {
      Serial.printf("Каталог %s удалён\n", val);
    } 
    else 
    {
      Serial.printf("Ошибка удаления каталога %s\n", val);
    }
  } 
  else 
  {
    if (SD_MMC.remove("/" + String(val))) 
    {
      Serial.printf("Файл %s удален\n", val);
    } 
    else 
    {
      Serial.printf("Ошибка удаления файла %s\n", val);
    }
  }
}
// Подготавливаем список для удаления функцией deleteFolderOrFile();
void delete_old_stuff() 
{
  // Разрешаем использовать все идентификаторы из пространства имён std без указания префикса std::
  using namespace std;
  
  //   Директива #include <tuple> в языке программирования C++ включает в программу шаблон класса std::tuple — 
  // контейнер для работы с кортежами (tuples). Кортежи — это контейнеры, которые могут содержать 
  // фиксированное количество элементов разных типов. 
  //   После включения директивы можно создавать кортеж: передавать типы элементов в качестве 
  // аргументов шаблона, а их значения — в качестве аргументов конструктора. 
  //   При работе с кортежами важно учитывать, что индекс элемента не может быть больше, 
  // чем размер кортежа. Например, если в кортеже четыре элемента, нельзя получить элемент с индексом, 
  // большим или равным 4, потому что в кортеже только четыре элемента.
  //   Также при получении элементов по индексу необходимо использовать константы, 
  // определённые во время компиляции, использование динамически назначенной переменной 
  // для индекса приводит к ошибке компиляции

  // Объявляем контейнер для кортежа существующих файлов
  using records = tuple<String, String, size_t, time_t>;
  // Объявляем кортеж
  list<records> dirList;

  int card = SD_MMC.cardSize()/(1024*1024);    // общий размер карты памяти
  int total = SD_MMC.totalBytes()/(1024*1024); // общее количество байтов, доступных на карте
  int used = SD_MMC.usedBytes()/(1024*1024);   // количество используемых байтов на карте SD

  //jpr("Общий размер SD-карты памяти:                %5dMB\n", card);  // %llu
  //jpr("Общее количество байтов, доступных на карте: %5dMB\n", total);
  //jpr("Количество используемых байтов на карте SD:  %5dMB\n", used);

  float full = 1.0 * used / total;
  if (full  <  0.8) 
  {
    //jpr("Удаление невозможно, SD-карта заполнена на %.1f%%\n", 100.0 * full);
  } 
  else 
  {
    //jpr("SD-карта заполнена на %.1f%%, удаляются старые файлы ...\n", 100.0 * full);
    // Фиксируем начало перебора файлов, формирования списка и сортировки
    int x = millis();
    // Задаём корневой каталог
    File xdir = SD_MMC.open("/");
    // Выбираем первый файл в каталоге
    File xf = xdir.openNextFile();
    // Перебираем все каталоги и файлы SD и формируем список dirList
    while (xf) 
    {
      if (xf.isDirectory()) 
      {
        String the_dir = xf.name();
        // Если каталог пустой, то удаляем его
        if (SD_MMC.rmdir("/" + the_dir )) 
        {                
          //jpr("Удален пустой каталог\n"); Serial.println("/" + the_dir);
        } 
        else 
        {
          String log_name = "/" + the_dir + "/" + the_dir + ".999.txt";
          //Serial.println(log_name);
          File the_log = SD_MMC.open(log_name, "r");
          // Определяем последнее время записи в каталог
          time_t the_fold = xf.getLastWrite();
          // Определяем последнее время записи в лог-файл
          time_t the_logfile = the_log.getLastWrite();
          the_log.close();

          // С помощью emplace_back добавляем элемент в конец контейнера. Это означает, 
          // что элемент создаётся непосредственно в памяти, выделенной для контейнера, 
          // что позволяет избежать ненужных копий или перемещений
          if (the_fold > the_logfile) 
          {
            // Tак как avi-файл более поздний, чем лог-файл,
            // то помещаем в список имя и дату avi-файла 
            dirList.emplace_back("", the_dir, 0, the_fold);
          } 
          else 
          {
            // Помещаем в список более поздний лог-файл
            dirList.emplace_back("", the_dir, 0, the_logfile);
            //Serial.printf("Log is newer than dir by %d -- ", the_logfile - the_fold);
          }
        }
      } 
      else 
      {
        // skip files
        // dirList.emplace_back("", xf.name(), xf.size(), xf.getLastWrite());
        // Serial.printf("Added: "); Serial.println(xf.name());
      }
      xf = xdir.openNextFile();
    }
    ÿ ir ﰧ翴 se();
    // Сортируем список по датам
    dirList.sort([](const records & f, const records & l) 
    {                                
      return get<3>(f) < get<3>(l);
      return false;
    });

    //jprln("Перебор файлов и сортировка для удаления старых заняло %d мс", millis() - x);

    for ( auto& iter : dirList) 
    {
      String fn =  get<1>(iter);
      //jpr("Oldest file is "); Serial.print(fn);
      deleteFolderOrFile(fn.c_str());

      total = SD_MMC.totalBytes()  / (1024 * 1024);
      used = SD_MMC.usedBytes()  / (1024 * 1024);

      full = 1.0 * used / total;

      Serial.println(full);
      if (full < 0.7) break;
    }
  }
}

oneframe find_a_frame (char * avi_file_name, long frame_num) 
{
  File findfile;

  oneframe x;
  findfile = SD_MMC.open(avi_file_name, "r");
  if (!findfile) {
    Serial.printf("Could not open %s file\n", avi_file_name);
    x.the_frame = NULL;
    return x;
  }  else  {
    //Serial.printf("Size %d\n",findfile.size());
    //Serial.printf("Last %d\n",findfile.getLastWrite());
    time_t lastw = findfile.getLastWrite();
    //Serial.printf("Lastw %d\n",lastw);

    time_t current;
    time (&current);

    int age = current - lastw;
    //Serial.printf("Age %d\n", age);

    //Serial.printf("File open: %s\n", avi_file_name);
    findfile.seek( 0x30 , SeekSet);
    long frame_total = read_quartet( findfile);
    //Serial.printf("Frames from file %ld\n", frame_total);

    if (age < 10) {
      //Serial.printf("Frame file %d, current %d\n", frame_total, frame_cnt - 1);
      frame_total = frame_cnt - 1;

      File idxfile =  SD_MMC.open("/idx.tmp", "r");
      if (!idxfile) {
        Serial.printf("Could not open /idx.tmp file\n");
        x.the_frame = NULL;
        return x;
      }

      int the_offset = frame_num * 8;
      idxfile.seek(the_offset, SeekSet);
      //Serial.printf("the frame %d, the offset %d\n", frame_num, the_offset);
      int the_addr = read_quartet (idxfile);
      int the_idx_len = read_quartet (idxfile);
      //Serial.printf("from index, the addr %d, the length %d\n",the_addr,the_idx_len);

      idxfile.close();

      findfile.seek( the_addr  + 236 , SeekSet);

      int the_oodc = read_quartet (findfile);
      //Serial.printf("the oodc %d\n",the_oodc);

      if (the_oodc != 1667510320) {
        Serial.printf("No frame %d, %d, %d\n%s file, num %d\n", frame_num, the_addr, the_idx_len);
        x.the_frame = NULL;
        return x;
      }
      //findfile.seek( the_addr + 8 + 236 , SeekSet);
      int the_len = read_quartet (findfile);
      //Serial.printf("frame len %d \n", the_len);

      //Serial.printf("Your frame is %d bytes, at address %d or %X\n", index_frame_length, index_frame_start, index_frame_start);
      uint8_t* fb_faf;
      fb_faf = (uint8_t*)ps_malloc(the_len + 24);

      // findfile.seek( 4 + 244 , SeekSet);

      size_t err = findfile.read(fb_faf, the_len);

      x.the_frame = fb_faf;
      x.the_frame_length = the_len;
      x.the_frame_number = frame_num;
      x.the_frame_total = frame_total;
      return x;


    }

    //findfile.seek( 0x8c , SeekSet);
    //long frame_cnt8c = read_quartet( findfile);
    //Serial.printf("Frames8c is %ld\n", frame_cnt8c);

    //int frame_num = 0.01 * frame_pct * frame_cnt;
    //Serial.printf("Frames pct %d, Frame num %d \n", frame_pct, frame_num);

    if (frame_total < frame_num) {
      Serial.printf("Only %ld frames, less than %ld frame_num -- start at 0\n", frame_cnt, frame_num);
      frame_num = 0;
    }
    if (frame_total == 3) {
      Serial.printf("Three 3 frames - we dont know how many! -- start at 0\n");

      frame_num = 0;
      findfile.seek( 4 + 236 , SeekSet);
      int the_oodc = read_quartet (findfile);
      if (the_oodc != 1667510320) {
        Serial.printf("No frame %s file, num %d\n", avi_file_name, frame_num);
        x.the_frame = NULL;
        return x;
      }
      int the_len = read_quartet (findfile);
      //Serial.printf("frame len %d \n", the_len);

      //Serial.printf("Your frame is %d bytes, at address %d or %X\n", index_frame_length, index_frame_start, index_frame_start);
      uint8_t* fb_faf;
      fb_faf = (uint8_t*)ps_malloc(the_len + 24);

      findfile.seek( 4 + 244 , SeekSet);

      size_t err = findfile.read(fb_faf, the_len);

      x.the_frame = fb_faf;
      x.the_frame_length = the_len;
      x.the_frame_number = frame_num;
      x.the_frame_total = frame_total;
      return x;
    }

    findfile.seek( 0xe8 , SeekSet);
    long index_start = read_quartet( findfile);
    //Serial.printf("Len of movi / index_start %ld\n", index_start);

    //bool success = findfile.seek(  , SeekEnd);

    //Serial.printf("Len of file %ld\n", findfile.size());
    //Serial.printf("Seek %d\n",  index_start + 244 + frame_num * 16 + 8);

    if (findfile.size() < index_start + 244 + frame_num * 16 + 8 , SeekSet) {
      Serial.printf("File too small / broken %s file\n", avi_file_name);
      x.the_frame = NULL;
      return x;
    }
    bool success = findfile.seek( index_start + 244 + frame_num * 16 + 8 , SeekSet);
    if (!success) {
      Serial.printf("File incomplete %s file\n", avi_file_name);
      x.the_frame = NULL;
      return x;

    }
    long index_frame_start = read_quartet( findfile);
    long index_frame_length = read_quartet( findfile);

    findfile.seek( index_frame_start + 236 , SeekSet);
    int the_oodc = read_quartet (findfile);
    if (the_oodc != 1667510320) {
      Serial.printf("No frame %s file, num %d\n", avi_file_name, frame_num);
      x.the_frame = NULL;
      return x;
    }
    int the_len = read_quartet (findfile);
    //Serial.printf("frame len %d \n", the_len);

    //Serial.printf("Your frame is %d bytes, at address %d or %X\n", index_frame_length, index_frame_start, index_frame_start);
    uint8_t* fb_faf;

    //fb_faf = (uint8_t*)ps_malloc(48 * 1024);  // danger 48kb may not be enough
    fb_faf = (uint8_t*)ps_malloc(the_len + 24);
    findfile.seek( index_frame_start + 244 , SeekSet);

    size_t err = findfile.read(fb_faf, index_frame_length);

    x.the_frame = fb_faf;
    x.the_frame_length = index_frame_length;
    x.the_frame_number = frame_num;
    x.the_frame_total = frame_total;
    return x;

  } // else yes to no avi file
}
*/



// **************************************************************** fs_sd.h ***
