/** Arduino, ESP32, C/C++ **************************************** fs_avi.h ***
 * 
 *             Сформировать avi-файлы из снятых потоков изображений на SD-карту 
 *                                                     
 * v2.2.0, 14.06.2026                                 Автор:      Труфанов В.Е.
 * Copyright © 2026 tve                               Дата создания: 14.06.2026
 * 
**/

#pragma once  

long frame_start = 0;
long frame_end = 0;
 

void re_index( char * avi_file_name, char * out_file_name); 
void re_index_bad( char * avi_file_name); 

void re_index( char * avi_file_name, char * out_file_name) 
{

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

//
void re_index_bad( char * avi_file_name) 
{

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




// *************************************************************** fs_avi.h ***
