/** Arduino, ESP32, C/C++ ******************************************** sd.h ***
 * 
 *                                                 Обслужить работу с SD-картой
 *                                                     
 * v1.0.4, 24.02.2026                                 Автор:      Труфанов В.Е.
 * Copyright © 2026 tve                               Дата создания: 24.01.2026
**/

#pragma once   

#include "FS.h"
#include <SD_MMC.h>

#include "inimem.h"
#include "trass.h"

/*
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

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

/*
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
    jpr("Ошибка открытия %s\n", val);
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

  jpr("Общий размер SD-карты памяти:                %5dMB\n", card);  // %llu
  jpr("Общее количество байтов, доступных на карте: %5dMB\n", total);
  jpr("Количество используемых байтов на карте SD:  %5dMB\n", used);

  float full = 1.0 * used / total;
  if (full  <  0.8) 
  {
    jpr("Удаление невозможно, SD-карта заполнена на %.1f%%\n", 100.0 * full);
  } 
  else 
  {
    jpr("SD-карта заполнена на %.1f%%, удаляются старые файлы ...\n", 100.0 * full);
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
          jpr("Удален пустой каталог\n"); Serial.println("/" + the_dir);
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
    xdir.close();
    // Сортируем список по датам
    dirList.sort([](const records & f, const records & l) 
    {                                
      return get<3>(f) < get<3>(l);
      return false;
    });

    jprln("Перебор файлов и сортировка для удаления старых заняло %d мс", millis() - x);

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

// ******************************************************************* sd.h ***
