// Arduino, ESP32, C/C++ ********************************** FrameStream.ino ***
//
// Формирование потока изображений, постоянной записи фрагментов видео на 
// SD-диск или при возникновении движения, а также передача снимков и фрагментов 
// видео через сокеты на сайт KwinFlat 

// Copyright © 2026 tve                               Труфанов В.Е., 11.01.2026
static const char vernum[]="v2.2.2, 12.06.2026";  
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

//#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_camera.h"
#include "sensor.h"

#include "inimem.h"
#include "fs_camera.h"
#include "fs_eprom.h"
#include "fs_sd.h"
#include "fs_server.h"
#include "fs_stream.h"
#include "fs_trass.h"
#include "fs_wifi.h"

bool InternetOff = true;

String czone;
//char apssid[30];
//char appass[14];

TaskHandle_t the_camera_loop_task;
TaskHandle_t the_sd_loop_task;
TaskHandle_t the_streaming_loop_task;

static SemaphoreHandle_t wait_for_sd;
static SemaphoreHandle_t sd_go;

long current_frame_time;
long last_frame_time;

camera_fb_t * fb_curr = NULL;
camera_fb_t * fb_next = NULL;

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "soc/soc.h"
#include "esp_cpu.h" //#include "soc/cpu.h"
#include "soc/rtc_cntl_reg.h"

static esp_err_t cam_err;

uint8_t* fb_curr_record_buf;
uint8_t* fb_streaming;

int fb_curr_record_len;
int fb_streaming_len;
long fb_curr_record_time = 0;
long fb_streaming_time = 0;

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

#include "FlMgr.h"          
ESPxWebFlMgr filemgr(filemanagerport); // we want a different port than the webserver

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//
#include <HTTPClient.h>

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream81_httpd = NULL;
httpd_handle_t stream82_httpd = NULL;

#include "lwip/sockets.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  Streaming stuff based on Random Nerd
//

bool start_streaming = false;
bool stream_82 = false;
bool stream_81 = false;

httpd_req_t *req_82;
httpd_req_t *req_81;

#define PART_BOUNDARY "123456789000000000000987654321"

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

void the_streaming_loop (void* pvParameter);
int stream_81_frames ;
long stream_81_start ;
int stream_82_frames ;
long stream_82_start ;

static esp_err_t stream_82_handler(httpd_req_t *req) {

  esp_err_t res;
  long start = millis();

  print_mem("stream_82_handler");

  stream_82 = true;
  req_82 = req;
  stream_82_frames = 0;
  stream_82_start = millis();

  if (stream_82) {
    res = httpd_resp_set_type(req_82, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
      stream_82 = false;
    }
  }

  time_in_web1 += (millis() - start);

  while (stream_82 == true) {          // we have to keep the *req alive
    delay(1000);
    //Serial.print("<82>");
  }
  Serial.println(" stream_82 done");
  delay(500);
  httpd_resp_send_408(req_82);
  req_82 = NULL;

  return ESP_OK;
}

static esp_err_t stream_81_handler(httpd_req_t *req) {

  esp_err_t res;
  long start = millis();

  print_mem("stream_81_handler");

  stream_81 = true;
  req_81 = req;
  stream_81_frames = 0;
  stream_81_start = millis();

  time_in_web1 += (millis() - start);

  if (stream_81) {
    res = httpd_resp_set_type(req_81, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
      stream_81 = false;
    }
  }

  while (stream_81 == true) {          // we have to keep the *req alive
    delay(1000);
    //Serial.print("<81>");
  }
  Serial.println(" stream_81 done");
  delay(500);
  httpd_resp_send_408(req_81);
  req_81 = NULL;
  return ESP_OK;
}


////////////////////////////////
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  Streaming stuff based on Random Nerd
//


void the_streaming_loop (void* pvParameter) {

  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  long start = millis();

  print_mem("the_streaming_loop");

  jprln("Starting the streaming");

  while (true) {

    if (!stream_81 && !stream_82) {
      delay(5);
    } else {
      if (stream_81) stream_81_frames++;
      if (stream_82) stream_82_frames++;

      xSemaphoreTake( baton, portMAX_DELAY );

      if (fb_record_time > (millis() - 500)) {
        //Serial.printf("*");
        fb_streaming_len = fb_record_len;
        fb_streaming_time = fb_record_time;
        memcpy(fb_streaming, fb_record,  fb_record_len);  // v59.5
        xSemaphoreGive( baton );
      } else {
        xSemaphoreGive( baton );
        fb = esp_camera_fb_get(); //get_good_jpeg();
        //Serial.println("loop take");
        //Serial.printf("millis %d, fb1 %d, fb2 %d\n", millis(), fb_record_time, fb_streaming_time);
        if (!fb) {
          Serial.println("Photos - Camera Capture Failed");  // i guess we stream the previous contents of fb_streaming //34
          //start_streaming = false;
        } else {
          //34 xSemaphoreTake( baton, portMAX_DELAY );
          fb_streaming_len = fb->len;
          fb_streaming_time = millis();
          memcpy(fb_streaming, fb->buf, fb->len);
          //34 xSemaphoreGive( baton );
          esp_camera_fb_return(fb);
        }
      }

      _jpg_buf_len = fb_streaming_len;
      _jpg_buf = fb_streaming;

      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

      long send_time = millis();
      long xx;
      xx = millis();

      if (stream_82) {
        res = httpd_resp_send_chunk(req_82, (const char *)part_buf, hlen);
        if (res != ESP_OK) {
          stream_82 = false;
          Serial.printf("Stream error - 82/1st %d\n", res);
        }
      }
      if (stream_81) {
        res = httpd_resp_send_chunk(req_81, (const char *)part_buf, hlen);
        if (res != ESP_OK) {
          stream_81 = false;
          Serial.printf("Stream error - 81/1st %d\n", res);
        }
      }

      xx = millis();

      if (stream_82) {
        res = httpd_resp_send_chunk(req_82, (const char *)_jpg_buf, _jpg_buf_len);
        if (res != ESP_OK) {
          stream_82 = false;
          Serial.printf("Stream error - 82/2nd %d\n", res);
        }
      }
      if (stream_81) {
        res = httpd_resp_send_chunk(req_81, (const char *)_jpg_buf, _jpg_buf_len);
        if (res != ESP_OK) {
          stream_81 = false;
          Serial.printf("Stream error - 81/2nd %d\n", res);
        }
      }

      xx = millis();

      if (stream_82) {
        res = httpd_resp_send_chunk(req_82, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if (res != ESP_OK) {
          stream_82 = false;
          Serial.printf("Stream error - 82/3rd %d\n", res);
        }
      }
      if (stream_81) {
        res = httpd_resp_send_chunk(req_81, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if (res != ESP_OK) {
          stream_81 = false;
          Serial.printf("Stream error - 81/3rd %d\n", res);
        }
      }

      if (stream_81_frames % 100 == 10) {
        if (Lots_of_Stats) {
          jpr("Stream 81 at %3.3f fps\n", (float)1000 * stream_81_frames / (millis() - stream_81_start));
        }
      }
      if (stream_82_frames % 100 == 10) {
        if (Lots_of_Stats) {
          jpr("Stream 82 at %3.3f fps\n", (float)1000 * stream_82_frames / (millis() - stream_82_start));
        }
      }

      int new_delay = stream_delay - (millis() - send_time);
      //Serial.printf(", streamdelay %5d, send_time %5d, newdelay %5d\n", stream_delay, millis() - send_time, new_delay);
      if (millis() - send_time > 5000) {
        new_delay = 1000;
        Serial.printf("wifi slow %d - take a 1s break\n", millis() - send_time);
      }

      if (new_delay < 10) {
        new_delay = 10;
      }

      delay(new_delay) ; //delay(stream_delay);

      start = millis();

    }
  }  // stream forever
}

void start_Stream_81_server() {
  httpd_config_t config2 = HTTPD_DEFAULT_CONFIG();
  config2.server_port = 81;
  config2.ctrl_port = 32123; //         = 32768,
  Serial.print("http Stream task prio: "); Serial.println(config2.task_priority);

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_81_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream81_httpd, &config2) == ESP_OK) {
    httpd_register_uri_handler(stream81_httpd, &stream_uri);
  } else {
    Serial.println("Error with stream start 81");
  }

  Serial.println("Stream 81 http started");
}

void start_Stream_82_server() {
  httpd_config_t config2 = HTTPD_DEFAULT_CONFIG();
  config2.server_port = 82;
  config2.ctrl_port = 32124; //         = 32768,
  Serial.print("http Stream task prio: "); Serial.println(config2.task_priority);

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_82_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream82_httpd, &config2) == ESP_OK) {
    httpd_register_uri_handler(stream82_httpd, &stream_uri);
  } else {
    Serial.println("Error with stream start 82");
  }

  Serial.println("Stream 82 http started");
}

//int newstart;
//int newend;
//int newskip;
//bool do_the_reparse = false;

void re_index( char * avi_file_name, char * out_file_name) {

  //once++;
  //if (once > 1) return;

  extern uint8_t* fb_faf;
  uint16_t remnant = 0;

  // JamCam0005.0037.avi
  //const char * avi_file_name = "JamCam0090.0001.avi";
  const char * idx_file_name = "/re_idx.tmp"; // "/JamCam0190.0001.idx";
  //const char * out_file_name = "/JamCam0090.0001new.avi";

  uint8_t fb_faf_static[fbs * 1024 + 20];

  File avifile = SD_MMC.open(avi_file_name, "r"); // avifile = SD_MMC.open(avi_file_name, "w");
  File idxfile = SD_MMC.open(idx_file_name, "w"); //idxfile = SD_MMC.open("/idx.tmp", "w");
  File outfile = SD_MMC.open(out_file_name, "w"); //idxfile = SD_MMC.open("/idx.tmp", "w");

  if (avifile) {
    //Serial.printf("File open: %s\n", avi_file_name);
  }  else  {
    Serial.printf("Could not open %s file\n", avi_file_name);
  }

  if (idxfile)  {
    //Serial.printf("File open: %s\n", idx_file_name);
  }  else  {
    Serial.printf("Could not open file %s\n", idx_file_name);
  }

  if (!avifile) {
    return;
  } else {
    //size_t err = avifile.read( fb_faf_static, 240);

    avifile.seek( 0x24 , SeekSet);
    int max_bytes_per_sec  = read_quartet( avifile);
    //Serial.printf("Max bytes per sec %d\n", max_bytes_per_sec);

    avifile.seek( 0x30 , SeekSet);
    int frame_cnt = read_quartet( avifile);
    //Serial.printf("Frames %d\n", frame_cnt);

    /*
        if (frame_cnt < frame_start) {
          Serial.printf("Only %d frames, less than %d frame_start -- start at 0\n", frame_cnt, frame_end);
          frame_start = 0;
        }
        // if frame_end is 0, or too high, it will go to frame_cnt,
        if (frame_cnt < frame_end || frame_end == 0) {
          Serial.printf("Only %d frames, less than %d frame_end -- end at max frames\n", frame_cnt, frame_end);
          frame_end = frame_cnt;
        }
    */
    int num_out_frames = frame_cnt;   // / (skip_frames + 1);
    Serial.printf("Original %d frames, so %d output frames\n", frame_cnt, num_out_frames );


    //avifile.seek( 0x8c , SeekSet);
    //int frame_cnt8c = read_quartet( avifile);

    avifile.seek( 0x84 , SeekSet);
    int iAttainedFPS = read_quartet( avifile);
    //Serial.printf("fps %d\n", iAttainedFPS);

    avifile.seek( 0xe8 , SeekSet);
    int index_start = read_quartet( avifile);
    //Serial.printf("Len of movi %d\n", index_start);

    Serial.printf("-----------------\n");

    avifile.seek( 0 , SeekSet);
    size_t err = avifile.read(fb_faf_static, AVIOFFSET);
    Serial.printf("avi read header %d\n", err);
    size_t err2 = outfile.write(fb_faf_static, AVIOFFSET);
    Serial.printf("avi write header %d\n", err2);
    outfile.seek( 240 , SeekSet);


    Serial.printf("-----------------\n");

    int xx2;
    int flen;
    int prev_frame_length = 0;
    int next_frame_start = 240;
    int new_frame_length;
    int prev_frame_start = 240;
    int index_frame_length;
    int index_frame_start;
    int movi_size = 0;
    int frame_cnt_out = 0;
    int frame_num = 0;

    avifile.seek( next_frame_start  , SeekSet);  //240

    bool one_more_frame = true;

    //for (int frame_num = 0; frame_num < frame_cnt; frame_num = frame_num + 1) {

    while (one_more_frame) {

      //i avifile.seek( index_start + 244 + frame_num * 16 + 8 , SeekSet);
      //i index_frame_start = read_quartet( avifile);
      //i index_frame_length = read_quartet( avifile);

      //avifile.seek( next_frame_start  , SeekSet);   // start at 240, then read everything - 2 quart + frame, then repeat

      int the_oodc = read_quartet (avifile);   //240
      if (the_oodc == 1667510320) {
        //Serial.printf("%d, good frame, num %d\n",the_oodc,frame_num);
      } else {
        Serial.printf("%d, bad frame, num %d\n", the_oodc, frame_num);
        break;
      }


      //avifile.seek( next_frame_start + 4  , SeekSet);
      new_frame_length = read_quartet( avifile); //244

      index_frame_length = new_frame_length; // reuse the variable

      prev_frame_start = next_frame_start;
      index_frame_start = prev_frame_start; // reuse

      next_frame_start = prev_frame_start + new_frame_length + 8;

      //prev_frame_length = new_frame_length;

      if (frame_num < 5 || frame_num % 500 == 0) {
        Serial.printf("Frame %4d, index len %9d, frame len %9d, index start %9d, frame start %9d\n", frame_num, index_frame_length, new_frame_length, index_frame_start + 236 , prev_frame_start);
      }

      if (frame_num < 5 || frame_cnt_out % 100 == 0) {
        Serial.printf("Frame %4d, index len %9d, frame len %9d, index start %9d, frame start %9d\n", frame_num, index_frame_length, new_frame_length, index_frame_start + 236 , prev_frame_start);
        Serial.printf("in %d, out %d\n", frame_num, frame_cnt_out);
      }

      //avifile.seek( index_frame_start + 244 , SeekSet); // already 248

      remnant = (4 - (index_frame_length & 0x00000003)) & 0x00000003;
      int index_frame_length_rem = index_frame_length + remnant;

      int left_to_write = index_frame_length_rem;

      // check next frame start
      int where_now = avifile.position();
      avifile.seek( where_now + left_to_write  , SeekSet);
      the_oodc = read_quartet (avifile);
      if (the_oodc == 1667510320) {
        //Serial.printf("%d, good frame, num %d\n",the_oodc,frame_num);
      } else {
        Serial.printf("%d, next frame is bad frame, num %d\n", the_oodc, frame_num);

        break;
      }
      avifile.seek( where_now  , SeekSet);
      // now write the dc and length

      print_dc_quartet( index_frame_length_rem, outfile);

      while (left_to_write > 0) {
        if (left_to_write > fbs * 1024) {
          size_t err = avifile.read(fb_faf_static, fbs * 1024);
          size_t err2 = outfile.write(fb_faf_static, fbs * 1024);
          left_to_write = left_to_write - fbs * 1024;
        } else {
          size_t err = avifile.read(fb_faf_static, left_to_write);
          size_t err2 = outfile.write(fb_faf_static, left_to_write);
          left_to_write = 0;
        }
      }

      movi_size += index_frame_length;
      movi_size += remnant;

      print_2quartet(idx_offset, index_frame_length, idxfile);

      idx_offset = idx_offset + index_frame_length_rem + 8;

      frame_cnt_out++;
      frame_num++;

    }  // every frame in file


    //Serial.printf("frame %4d, outfile %9d, avifile %9d, idxfile %9d\n", frame_cnt , outfile.position(), avifile.position(), avifile.position());

    idxfile.close();
    size_t i1_err = outfile.write(idx1_buf, 4);
    if (!i1_err) Serial.printf("idx write\n");

    print_quartet(frame_cnt_out * 16, outfile);

    idxfile = SD_MMC.open(idx_file_name, "r");
    if (idxfile)  {
      Serial.printf("File open: %s\n", idx_file_name);
    }  else  {
      Serial.printf("Could not open file %s\n", idx_file_name);
    }

    char * AteBytes;
    AteBytes = (char*) malloc (8);

    for (int i = 0; i < frame_cnt_out; i++) {
      size_t res = idxfile.readBytes( AteBytes, 8);
      if (!res) Serial.printf("idx read\n");
      size_t i1_err = outfile.write(dc_and_zero_buf, 8);
      if (!i1_err) Serial.printf("dc write\n");
      //size_t i2_err = outfile.write(zero_buf, 4);
      //if (!i2_err) Serial.printf("zero write\n");
      size_t i3_err = outfile.write((uint8_t *)AteBytes, 8);
      if (!i3_err) Serial.printf("ate write\n");
    }

    free(AteBytes);

    outfile.seek( 4 , SeekSet);         //shit
    print_quartet(movi_size + 240 + 16 * frame_cnt_out + 8 * frame_cnt_out, outfile);

    avifile.seek( 0xe8 , SeekSet);
    int lom = read_quartet( avifile);
    Serial.printf("Len of movi was %d, now is %d\n", lom, movi_size);

    outfile.seek( 0xe8 , SeekSet);
    // shit print_quartet (movi_size, outfile);
    print_quartet(movi_size + frame_cnt_out * 8 + 4, outfile);

    avifile.seek( 0x30 , SeekSet);
    int fc = read_quartet( avifile);
    Serial.printf("Frames was %d, now is %d\n", fc, frame_cnt_out);
    outfile.seek( 0x30 , SeekSet);
    print_quartet (frame_cnt_out, outfile);
    outfile.seek( 0x8c , SeekSet);
    print_quartet (frame_cnt_out, outfile);

    //avifile.seek( 0x84 , SeekSet);
    //int fps = read_quartet( avifile);
    //Serial.printf("fps was %d\n", fps);

    //float fnewfps = ( 1.0f * fps ) / (1 + skip_frames)   ;
    //int newfps = round(fnewfps);

    //Serial.printf("newfps is %f, %d\n", fnewfps, newfps);
    //avifile.seek( 0x84 , SeekSet);
    //print_quartet(newfps, avifile);

    //avifile.seek( 0x20 , SeekSet);
    //int us_per_frame = read_quartet( avifile);
    //Serial.printf("us_per_frame was %d\n", us_per_frame);

    //float newus = 1000000.0f / fnewfps;
    //uint32_t new_us_per_frame = round (newus);


    //Serial.printf("new_us_per_frame is %f, %d\n", newus, new_us_per_frame);
    //avifile.seek( 0x20 , SeekSet);
    //print_quartet(new_us_per_frame, avifile);


    idxfile.close();
    avifile.close();
    outfile.close();

    int xx = SD_MMC.remove(idx_file_name);
  }
}

void re_index_bad( char * avi_file_name) {

  //once++;
  //if (once > 1) return;

  extern uint8_t* fb_faf;
  uint16_t remnant = 0;

  const char * idx_file_name = "/reidx.tmp"; // "/JamCam0190.0001.idx";

  //#define fbs 4 //  how many kb of static ram for psram -> sram buffer for sd write
  //  uint8_t fb_faf_static[fbs * 1024 + 20];

  File  avifile = SD_MMC.open(avi_file_name, "w"); // avifile = SD_MMC.open(avi_file_name, "w");
  File idxfile = SD_MMC.open(idx_file_name, "w"); //idxfile = SD_MMC.open("/idx.tmp", "w");
  //outfile = SD_MMC.open(out_file_name, "w"); //idxfile = SD_MMC.open("/idx.tmp", "w");

  if (avifile) {
    Serial.printf("File open: %s\n", avi_file_name);
  }  else  {
    Serial.printf("Could not open %s file\n", avi_file_name);
  }

  if (idxfile)  {
    Serial.printf("File open: %s\n", idx_file_name);
  }  else  {
    Serial.printf("Could not open file %s\n", idx_file_name);
  }

  if (!avifile) {
    return;
  } else {
    //size_t err = avifile.read( fb_faf_static, 240);

    avifile.seek( 0x24 , SeekSet);
    int max_bytes_per_sec  = read_quartet( avifile);
    Serial.printf("Max bytes per sec %d\n", max_bytes_per_sec);

    avifile.seek( 0x30 , SeekSet);
    int frame_cnt = read_quartet( avifile);
    Serial.printf("Frames %d\n", frame_cnt);

    /*
        if (frame_cnt < frame_start) {
          Serial.printf("Only %d frames, less than %d frame_start -- start at 0\n", frame_cnt, frame_end);
          frame_start = 0;
        }
        // if frame_end is 0, or too high, it will go to frame_cnt,
        if (frame_cnt < frame_end || frame_end == 0) {
          Serial.printf("Only %d frames, less than %d frame_end -- end at max frames\n", frame_cnt, frame_end);
          frame_end = frame_cnt;
        }
    */
    int num_out_frames = frame_cnt;   // / (skip_frames + 1);
    Serial.printf("Original %d frames, so %d output frames\n", frame_cnt, num_out_frames );


    //avifile.seek( 0x8c , SeekSet);
    //int frame_cnt8c = read_quartet( avifile);

    avifile.seek( 0x84 , SeekSet);
    int iAttainedFPS = read_quartet( avifile);
    Serial.printf("fps %d\n", iAttainedFPS);

    avifile.seek( 0xe8 , SeekSet);
    int index_start = read_quartet( avifile);
    Serial.printf("Len of movi %d\n", index_start);

    Serial.printf("-----------------\n");

    /*
        avifile.seek( 0 , SeekSet);
        size_t err = avifile.read(fb_faf_static, AVIOFFSET);
        Serial.printf("avi read header %d\n", err);
        size_t err2 = outfile.write(fb_faf_static, AVIOFFSET);
        Serial.printf("avi write header %d\n", err2);

        outfile.seek( 240 , SeekSet);
    */

    Serial.printf("-----------------\n");

    int xx2;
    int flen;
    int prev_frame_length = 0;
    int next_frame_start = 240;
    int new_frame_length;
    int prev_frame_start = 240;
    int index_frame_length;
    int index_frame_start;
    int movi_size = 0;
    int frame_cnt_out = 0;
    int frame_num = 0;

    int the;
    next_frame_start = 232;
    avifile.seek(  next_frame_start , SeekSet);
    Serial.printf("addr %d, ", next_frame_start);
    the = read_quartet (avifile);

    next_frame_start = next_frame_start + 4;
    avifile.seek(  next_frame_start , SeekSet);
    Serial.printf("addr %d, ", next_frame_start);
    the = read_quartet (avifile);

    next_frame_start = next_frame_start + 4;
    avifile.seek(  next_frame_start , SeekSet);
    Serial.printf("addr %d, ", next_frame_start);
    the = read_quartet (avifile);

    next_frame_start = next_frame_start + 4;
    avifile.seek(  next_frame_start , SeekSet);
    Serial.printf("addr %d, ", next_frame_start);
    the = read_quartet (avifile);

    next_frame_start = next_frame_start + 4;
    avifile.seek(  next_frame_start , SeekSet);
    Serial.printf("addr %d, ", next_frame_start);
    the = read_quartet (avifile);



    next_frame_start = 240;


    bool one_more_frame = true;

    //for (int frame_num = 0; frame_num < frame_cnt; frame_num = frame_num + 1) {

    size_t start_index_here;

    while (one_more_frame) {

      //i avifile.seek( index_start + 244 + frame_num * 16 + 8 , SeekSet);
      //i index_frame_start = read_quartet( avifile);
      //i index_frame_length = read_quartet( avifile);

      //avifile.seek( next_frame_start  , SeekSet);   // start at 240, then read everything - 2 quart + frame, then repeat

      avifile.seek( next_frame_start  , SeekSet);
      start_index_here = avifile.position();
      int the_oodc = read_quartet (avifile);   //240
      if (the_oodc == 1667510320) {
        //Serial.printf("%d, good frame, num %d\n",the_oodc,frame_num);
      } else {
        Serial.printf("%d, bad frame, num %d\n", the_oodc, frame_num);
        break;
      }




      //avifile.seek( next_frame_start + 4  , SeekSet);
      new_frame_length = read_quartet( avifile); //244

      index_frame_length = new_frame_length; // reuse the variable

      prev_frame_start = next_frame_start;
      index_frame_start = prev_frame_start; // reuse

      next_frame_start = prev_frame_start + new_frame_length + 8;

      //prev_frame_length = new_frame_length;

      if (frame_num < 5 || frame_num % 500 == 0) {
        Serial.printf("Frame %4d, index len %9d, frame len %9d, index start %9d, frame start %9d\n", frame_num, index_frame_length, new_frame_length, index_frame_start + 236 , prev_frame_start);
      }

      if (frame_num < 5 || frame_cnt_out % 100 == 0) {
        Serial.printf("Frame %4d, index len %9d, frame len %9d, index start %9d, frame start %9d\n", frame_num, index_frame_length, new_frame_length, index_frame_start + 236 , prev_frame_start);
        Serial.printf("in %d, out %d\n", frame_num, frame_cnt_out);
      }

      //avifile.seek( index_frame_start + 244 , SeekSet); // already 248

      remnant = (4 - (index_frame_length & 0x00000003)) & 0x00000003;
      int index_frame_length_rem = index_frame_length + remnant;

      int left_to_write = index_frame_length_rem;

      // print_dc_quartet( index_frame_length_rem, outfile);
      /*
            while (left_to_write > 0) {
              if (left_to_write > fbs * 1024) {
                size_t err = avifile.read(fb_faf_static, fbs * 1024);
                size_t err2 = outfile.write(fb_faf_static, fbs * 1024);
                left_to_write = left_to_write - fbs * 1024;
              } else {
                size_t err = avifile.read(fb_faf_static, left_to_write);
                size_t err2 = outfile.write(fb_faf_static, left_to_write);
                left_to_write = 0;
              }
            }
      */
      movi_size += index_frame_length;
      movi_size += remnant;

      print_2quartet(idx_offset, index_frame_length, idxfile);

      idx_offset = idx_offset + index_frame_length_rem + 8;

      frame_cnt_out++;
      frame_num++;

    }  // every frame in file

    //frame_cnt_out--;
    //movi_size = movi_size - remnant;
    //movi_size = movi_size - index_frame_length;

    //Serial.printf("frame %4d, outfile %9d, avifile %9d, idxfile %9d\n", frame_cnt , outfile.position(), avifile.position(), avifile.position());



    idxfile.close();
    //avifile.close();
    //avifile = SD_MMC.open(avi_file_name, "w");

    avifile.seek(  start_index_here  , SeekSet);

    //size_t i1_err = outfile.write(idx1_buf, 4);
    //if (!i1_err) Serial.printf("idx write\n");

    //print_quartet(frame_cnt_out * 16, outfile);

    idxfile = SD_MMC.open(idx_file_name, "r");
    if (idxfile)  {
      Serial.printf("File open: %s\n", idx_file_name);
    }  else  {
      Serial.printf("Could not open file %s\n", idx_file_name);
    }

    char * AteBytes;
    AteBytes = (char*) malloc (8);


    for (int i = 0; i < frame_cnt_out; i++) {
      size_t res = idxfile.readBytes( AteBytes, 8);
      if (!res) Serial.printf("idx read\n");
      size_t i1_err = avifile.write(dc_and_zero_buf, 8);
      if (!i1_err) Serial.printf("dc write\n");
      //size_t i2_err = outfile.write(zero_buf, 4);
      //if (!i2_err) Serial.printf("zero write\n");
      size_t i3_err = avifile.write((uint8_t *)AteBytes, 8);
      if (!i3_err) Serial.printf("ate write\n");
    }

    free(AteBytes);

    avifile.seek( 4 , SeekSet);         //shit
    print_quartet(movi_size + 240 + 16 * frame_cnt_out + 8 * frame_cnt_out, avifile);

    //avifile.seek( 0xe8 , SeekSet);
    //int lom = read_quartet( avifile);
    //Serial.printf("Len of movi was %d, now is %d\n", lom, movi_size);

    avifile.seek( 0xe8 , SeekSet);
    // shit print_quartet (movi_size, outfile);
    print_quartet(movi_size + frame_cnt_out * 8 + 4, avifile);

    //avifile.seek( 0x30 , SeekSet);
    //int fc = read_quartet( avifile);
    //Serial.printf("Frames was %d, now is %d\n", fc, frame_cnt_out);

    avifile.seek( 0x30 , SeekSet);
    print_quartet (frame_cnt_out, avifile);
    avifile.seek( 0x8c , SeekSet);
    print_quartet (frame_cnt_out, avifile);

    //avifile.seek( 0x84 , SeekSet);
    //int fps = read_quartet( avifile);
    //Serial.printf("fps was %d\n", fps);

    //float fnewfps = ( 1.0f * fps ) / (1 + skip_frames)   ;
    //int newfps = round(fnewfps);

    //Serial.printf("newfps is %f, %d\n", fnewfps, newfps);
    //avifile.seek( 0x84 , SeekSet);
    //print_quartet(newfps, avifile);

    //avifile.seek( 0x20 , SeekSet);
    //int us_per_frame = read_quartet( avifile);
    //Serial.printf("us_per_frame was %d\n", us_per_frame);

    //float newus = 1000000.0f / fnewfps;
    //uint32_t new_us_per_frame = round (newus);


    //Serial.printf("new_us_per_frame is %f, %d\n", newus, new_us_per_frame);
    //avifile.seek( 0x20 , SeekSet);
    //print_quartet(new_us_per_frame, avifile);


    idxfile.close();
    avifile.close();
    //outfile.close();

    int xx = SD_MMC.remove(idx_file_name);
  }
}


////////////////////////////////

void startCameraServer() {
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

void stopCameraServer() {
  httpd_stop(camera_httpd);
}

void the_camera_loop (void* pvParameter);
void the_sd_loop (void* pvParameter);
void delete_old_stuff();

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup() 
{
  Serial.begin(115200);
  Serial.println("\n\n---");
  Serial.println("Arduino IDE 2.3.7 - Espressif ESP32 3.3.5");
  delay(10000);

  pinMode(33, OUTPUT);             // little red led on back of chip
  digitalWrite(33, LOW);           // turn on the red LED on the back of chip

  pinMode(4, OUTPUT);               // Blinding Disk-Avtive Light
  digitalWrite(4, LOW);             // turn off

  pinMode(12, INPUT_PULLUP);        // pull this down to stop recording
  pinMode(13, INPUT_PULLUP);        // pull this down switch wifi

  // SD camera init
  Serial.println("Mounting the SD card ...");
  esp_err_t card_err = init_sdcard();
  if (card_err != ESP_OK) {
    Serial.printf("SD Card init failed with error 0x%x", card_err);
    major_fail();
    return;
  } else {
    logfile = SD_MMC.open("/boot.txt", FILE_WRITE);
  }

  jprln("                                    ");
  jprln("---------------------------------------");
  jprln("ESP32-CAM-Video-Recorder-junior %s", vernum);
  jprln("---------------------------------------");

  print_mem("setup");

  esp_reset_reason_t reason = esp_reset_reason();

  //logfile.print("--- reboot ------ because: ");
  jpr("--- reboot ------ because: ");

  switch (reason) {
    case ESP_RST_UNKNOWN : jprln("ESP_RST_UNKNOWN");  break;
    case ESP_RST_POWERON : jprln("ESP_RST_POWERON"); break;
    case ESP_RST_EXT : jprln("ESP_RST_EXT");  break;
    case ESP_RST_SW : jprln("ESP_RST_SW");  break;
    case ESP_RST_PANIC : jprln("ESP_RST_PANIC");  break;
    case ESP_RST_INT_WDT : jprln("ESP_RST_INT_WDT");  break;
    case ESP_RST_TASK_WDT : jprln("ESP_RST_TASK_WDT");  break;
    case ESP_RST_WDT : jprln("ESP_RST_WDT");  break;
    case ESP_RST_DEEPSLEEP : jprln("ESP_RST_DEEPSLEEP");  break;
    case ESP_RST_BROWNOUT : jprln("ESP_RST_BROWNOUT");  break;
    case ESP_RST_SDIO : jprln("ESP_RST_SDIO");  break;
    default  : jprln("Reset resaon"); break;
  }

  do_eprom_read();

  jprln("Try to get parameters from config2.txt ...");

  read_config_file();

  jprln("Setting up the camera ...");
  config_camera();

  //fb_record = (uint8_t*)ps_malloc(512 * 1024); // buffer to store a jpg in motion // needs to be larger for big frames from ov5640

  // frame_buffer_size set by config_camera
  fb_record = (uint8_t*)ps_malloc(frame_buffer_size); // buffer to store a jpg in motion // needs to be larger for big frames from ov5640
  fb_curr_record_buf = (uint8_t*)ps_malloc(frame_buffer_size);
  fb_streaming = (uint8_t*)ps_malloc(frame_buffer_size); // buffer to store a jpg in motion // needs to be larger for big frames from ov5640
  fb_capture = (uint8_t*)ps_malloc(frame_buffer_size); // buffer to store a jpg in motion // needs to be larger for big frames from ov5640

  print_mem("setup - after malloc");

  jprln("Creating the_camera_loop_task");

  baton = xSemaphoreCreateMutex();


  xTaskCreatePinnedToCore( the_camera_loop, "the_camera_loop", 5000, NULL, 4, &the_camera_loop_task, 0); //soc14
  delay(100);

  xTaskCreate( the_streaming_loop, "the_streaming_loop", 8000, NULL, 2, &the_streaming_loop_task);
  if ( the_streaming_loop_task == NULL ) {
    //vTaskDelete( xHandle );
    Serial.printf("do_the_steaming_task failed to start! %d\n", the_streaming_loop_task);
  }


  if (InternetOff) {
    print_mem("Starting the wifi ...");
    init_wifi();
    print_mem("Starting the fileman ...");

    jprln("");
    filemgr.begin();
    filemgr.setBackGroundColor("Gray");
    jpr("Open Filemanager with http://");
    Serial.print(WiFi.softAPIP()); logfile.print(WiFi.softAPIP());
    jprln(":%d/", filemanagerport);
    jpr("Open Filemanager with http://");
    Serial.print(WiFi.localIP()); logfile.print(WiFi.localIP());
    jprln(":%d/", filemanagerport);

    print_mem("Starting Web Services ...");
    startCameraServer();
    start_Stream_81_server();
    start_Stream_82_server();

    InternetOff = false;
    print_mem("After the WiFi");
  }

  jprln("Checking SD for available space ...");
  delete_old_stuff();

  char logname[60];
  char the_directory[50];

  sprintf(the_directory, "/%s%03d",  devname, file_group);
  SD_MMC.mkdir(the_directory);

  sprintf(logname, "/%s%03d/%s%03d.999.txt",  devname, file_group, devname, file_group);
  jprln("Creating logfile %s\n",  logname);
  if (logfile) {
    logfile.close();
  }
  logfile = SD_MMC.open(logname, FILE_WRITE);
  if (!logfile) {
    Serial.println("Failed to open logfile for writing");
  }

  boot_time = millis();

  const char *strdate = ctime(&now);
  //logfile.println(strdate);

  digitalWrite(33, HIGH);         // red light turns off when setup is complete

  print_mem("End of setup");
  jpr("\n---  End of setup()  ---\n\n");

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// the_sd_loop()
//
/* //8
  void the_sd_loop (void* pvParameter) {

  Serial.print("the_sd_loop, core ");  Serial.print(xPortGetCoreID());
  Serial.print(", priority = "); Serial.println(uxTaskPriorityGet(NULL));

  while (1) {
    xSemaphoreTake( sd_go, portMAX_DELAY );            // we wait for camera loop to tell us to go
    another_save_avi( fb_curr);                        // do the actual sd wrte
    xSemaphoreGive( wait_for_sd );                     // tell camera loop we are done
  }
  }
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// the_camera_loop()

void the_camera_loop (void* pvParameter) {

  print_mem("the_camera_loop");

  frame_cnt = 0;
  start_record_2nd_opinion = digitalRead(12);
  start_record_1st_opinion = digitalRead(12);
  start_record = 0;

  delay(1000);

  while (1) {
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
      if (current_frame_time - last_frame_time < frame_interval) {
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

//


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// loop() - loop runs at low prio, so I had to move it to the task the_camera_loop at higher priority
#include <ESPping.h>
long wakeup;
long last_wakeup = 0;
int loops = 0;
void loop() {
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
  if (wakeup - last_wakeup > (10  * 60 * 1000) ) {
    last_wakeup = millis();
    print_mem("---------- 10 Minute Internet Check -----------\n");
    time(&now);
    jpr("Local time: "); jpr(ctime(&now));
    if (!InternetOff ) {

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

// ************************************ VideoRecorderESP32-CAM-junior62.ino ***
