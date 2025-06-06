#include <stdint.h>
#include <gfxfont.h>
#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_GrayOLED.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <ESP.h>
#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Adafruit_TinyUSB.h>
#include "ESPTelnetStream.h"
#include "Adafruit_GFX.h"
//#include "WROVER_KIT_LCD.h"
#define LGFX_ESP_WROVER_KIT
#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>


#define SCK  14
#define MISO  2
#define MOSI  15
#define CS  13
#define NUM_LEDS 1
#define DATA_PIN 2
#define R_LED 0u
#define G_LED 2u
#define B_LED 4u

SPIClass spi = SPIClass(HSPI);
LGFX tft;

extern void TStart();
extern void Tloop();
extern ESPTelnetStream telnet;
extern void TFTsetup(void* pvParameters);
extern uint16_t* bmp;
extern uint8_t* bitmap;
int xmain(int argc, char *args[]);
//char *argl[]={"nano8","focal.bin","200"};
int ctr = 0;
char bfr[256];
uint8_t *p;
File df32;
short rkdn, rkcmd, rkca, rkwc;
int rkda;
File rk05;
unsigned int bcnt;
unsigned long systime, nowtime, cntr;
int clken, clkcnt, clkfl, ndx;

#include <sys/time.h>

time_t time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (time_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

/*
// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
int32_t msc_read_cb(uint32_t lba, void *buffer, uint32_t bufsize) {
  uint8_t const *addr = msc_disk[lba];
  memcpy(buffer, addr, bufsize);

  return bufsize;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t *buffer, uint32_t bufsize) {
  uint8_t *addr = msc_disk[lba];
  memcpy(addr, buffer, bufsize);

  return bufsize;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb(void) {
  // nothing to do
}

*/

int readline(char *buffer, int len) {
  int pos = 0;
  char tm;

  while (1) {
    yield();
    if (Serial.available()) {
      tm = Serial.read();
      Serial.write(tm);
    }
    switch (tm) {
      case 0:
      case '\n':  // Ignore CR
        break;
      case '\r':  // Return on new-line
        return pos;
      default:
        if (pos < len - 1) {
          buffer[pos++] = tm;
          buffer[pos] = 0;
        }
    }
    tm = 0;
  }
}
void serial_putchar(char c) {
  Serial.write(c);
}

char serial_getchar() {
  return Serial.read();
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin PIN_SPI0_SCK as an output. (Red led)
  // pinMode(R_LED, OUTPUT);
  // initialize digital pin PIN_SPI0_SCK as an output. (Green led)
  // pinMode(G_LED, OUTPUT);
  // initialize digital pin 20 as an output. (Blue led)
  pinMode(B_LED, OUTPUT);
  // Turn off
  // digitalWrite(B_LED, HIGH);
  // digitalWrite(R_LED, HIGH);
  // FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
  // leds[0] = CRGB::Blue;
  Serial.begin(115200);
  while (!Serial)
    { yield(); }
  Serial.println("Startup.....");
  /*
	Dir dir = LittleFS.openDir("/");
	while (dir.next()) {
		Serial.printf("%s:", dir.fileName().c_str());
		if (dir.fileSize()) {
			File f = dir.openFile("r");
			Serial.println(f.size());
			f.close();
		}
	}
*/
  //df32 = SD.open("/df32_os8_4p.dsk", "r+");
  Serial.print("Free heap:");
  Serial.println(ESP.getMaxAllocHeap());
  tft.init();
  tft.initDMA();
  tft.setRotation(1);
  tft.fillScreen(0);
  tft.setTextColor(tft.color565(0, 255, 0));
  tft.setTextSize(1);
  tft.println("PDP-8 Online");
  spi.begin(SCK, MISO, MOSI, CS);
  if (!SD.begin(CS, spi)) {
      Serial.println("Card Mount Failed");
      return;
  }
  listDir(SD, "/", 3);
  rk05 = SD.open("/rk05.dsk", "r+");
  Serial.println("initialization done.");
  TStart();
  xTaskCreatePinnedToCore(
      TFTsetup,   /* Function to implement the task */
      "coreTask", /* Name of the task */
      10000,      /* Stack size in words */
      NULL,       /* Task input parameter */
      0,          /* Priority of the task */
      NULL,       /* Task handle. */
      0);  /* Core where the task should run */

  Serial.println("Task created...");

}

// the loop function runs over and over again forever
void loop() {
  xmain(3, NULL);
}


// Nano8.cpp
//

#define TTWAIT 4000

#define MEMSIZE 4096 * 8

short mem[MEMSIZE];
int pc, acc, ma, mq, tti, ttf, tto, intf, inst, ibus, pti, pto;
int ifl, dfl, ifr, dfr, svr, uflag, usint, intinh;
int kcnt = 0, tcnt = 0;
unsigned int dskrg, dskmem, dskfl, tm, i, tmp;
unsigned int dskad;
int vcflg,vcix,vciy;
int eaesc, eaemd, gtf;


void caf() {
  acc = mq = tti = tto = ttf = ibus = pto = 0;
  dfr = ifr = dfl = ifl = uflag = dskfl = 0;
  pti = -2;  // Flag clear until explicit fetch
  eaemd = eaesc = gtf = vcflg = clken = clkfl = 0;
}

void iot() {

  if (uflag == 3) {
    usint = 1;
    return;
  }
  switch (inst & 0770) {
    case 0:
      switch (inst & 0777) {
        case 000:
          if (intf)
            pc++;
          intf = intinh = 0;
          break;
        case 001:
          intinh |= 1;
          break;
        case 002:
          intf = intinh = 0;
          break;
        case 003:
          if (ibus)
            pc++;
          break;
        case 004:
          acc = (acc & 010000) ? 014000 : 0;
          if (intinh & 1)
            acc |= 0200;
          if (intf)
            acc |= 01000;
          if (gtf) acc |= 02000;
          acc |= svr;
          break;
        case 005:
          intinh = 3;
          acc &= 07777;
          if (acc & 04000)
            acc |= 010000;
          svr = acc & 0177;
          dfr = (svr & 07) << 3;
          dfl = dfr << 9;
          ifr = (svr & 070);
          if (svr & 0100)
            uflag = 1;
          gtf = acc & 02000;
          break;
        case 006:
            if (gtf)
                pc++;
            break;
        case 007:
          caf();
          break;
      }
      break;
    case 0200:
    case 0210:
    case 0220:
    case 0230:
    case 0240:
    case 0250:
    case 0260:
    case 0270:
      switch (inst & 0777) {
        case 0204:
          usint = 0;
          break;
        case 0254:
          if (usint)
            pc++;
          break;
        case 0264:
          uflag = 0;
          break;
        case 0274:
          uflag = 1;
          break;
        case 0214:
          acc |= dfr;
          break;
        case 0224:
          acc |= ifr;
          break;
        case 0234:
          acc |= svr;
          break;
        case 0244:
          dfr = (svr & 07) << 3;
          dfl = dfr << 9;
          ifr = (svr & 070);
          if (svr & 0100)
            uflag = 1;
          break;
      }
      switch (inst & 0707) {
        case 0201:
          dfr = inst & 070;
          dfl = dfr << 9;
          break;
        case 0202:
          ifr = inst & 070;
          intinh |= 2;
          break;
        case 0203:
          dfr = inst & 070;
          ifr = inst & 070;
          dfl = dfr << 9;
          intinh |= 2;
          break;
      }
      break;

    case 010:
      switch (inst & 07) {
        case 01:
          if (pti > -1)
            pc++;
          break;
        case 02:
          acc |= pti & 0377;
          pti = -2;
          break;
        case 04:
          pti = -1;
          break;
        case 06:
          acc |= pti & 0377;
          pti = -1;
          break;
      }
      break;
    case 020:
      switch (inst & 07) {
        case 1:
          if (pto >= TTWAIT)
            pc++;
          break;
        case 2:
          pto = 0;
          break;
        case 6:
        case 4:
          pto = acc & 0177;
          pto = 1;
          break;
      }
      break;
    case 030:
      switch (inst & 07) {
        case 1:
          if (ttf)
            pc++;
          break;
        case 2:
          ttf = 0;
          acc &= 010000;
          break;
        case 4:
          acc |= tti | 0200;
          break;
        case 6:
          acc &= 010000;
          acc |= tti | 0200;
          ttf = 0;
      }
      break;
    case 040:
      switch (inst & 07) {
        case 1:
          if (tto >= TTWAIT)
            pc++;
          break;
        case 2:
          tto = 0;
          break;
        case 6:
        case 4:
          Serial.write(acc & 0177);
          if (telnet.isConnected())
            telnet.write(acc & 0177);
          //tft.write(acc & 0177);
          tto = 1;
          break;
      }
      break;
				case 050:							// Minimalist VC8E implementation
					switch( inst&0777)
					{
					case 050:	vcflg=0;break;
					case 051:	vcflg=0;break;
					case 052:	if (vcflg>100) pc++;break;
					case 053:	vcix=(acc&01777)^01000;vcflg=1;break;
					case 054:	vciy=02000-((acc&01777)^01000);vcflg=1;break;
          case 055:	ndx = (int)(vcix / 4+40) + (int)(vciy/4)*320; bitmap[ndx] = 1; vcflg = 1; break;
					case 056:	acc&=010000;break;
					case 057:	acc=acc&010000;break;
					}
					break;
    case 0600:
    case 0610:
    case 0620:
      switch (inst & 0777) {
        case 0601:
          dskad = dskfl = 0;
          break;
        case 0605:
        case 0603:
          i = (dskrg & 070) << 9;
          dskmem = ((mem[07751] + 1) & 07777) | i; /* mem */
          tm = (dskrg & 03700) << 6;
          dskad = (acc & 07777) + tm; /* dsk */
          i = 010000 - mem[07750];
          p = (uint8_t *)&mem[dskmem];
          df32.seek(dskad * 2, SeekSet);
          if (inst & 2) {
            /*read */
            tmp = df32.read(p, i * 2);
            //Serial.printf("Read:%o>%o:%o,%d Len:%d\r\n", dskad, dskmem, dskrg, i, tmp);
          } else {
            tmp = df32.write(p, i * 2);
            //Serial.printf("Write:%o>%o:%o,%d Len:%d\r\n", dskad, dskmem, dskrg, i, tmp);
          }
          dskfl = 1;
          mem[07751] = 0;
          mem[07750] = 0;
          acc = acc & 010000;
          break;
        case 0611:
          dskrg = 0;
          break;
        case 0615:
          dskrg = (acc & 07777); /* register */
          break;
        case 0616:
          acc = (acc & 010000) | dskrg;
          break;
        case 0626:
          acc = (acc & 010000) + (dskad & 07777);
          break;
        case 0622:
          if (dskfl) pc++;
          break;
        case 0612: acc = acc & 010000;
        case 0621:
          pc++; /* No error */
          break;
      }
      break;
    case 0740:
      //      printf("Acc:%04o IOT:%04o\n",acc,xmd);
      switch (inst & 7) {
        case 0:
          break;
        case 1:
          if (rkdn) {
            pc++;
            rkdn = 0;
          }
          break;
        case 2:
          acc &= 010000;
          rkdn = 0;
          break;
        case 3:
          rkda = acc & 07777;
          //
          // OS/8 Scheme. 2 virtual drives per physical drive
          // Regions start at 0 and 6260 (octal).
          //
          acc &= 010000;
          if (rkcmd & 6) {
            printf("Unit error\n");
            return;
          }
          digitalWrite(B_LED, HIGH);
          switch (rkcmd & 07000) {
            case 0:
            case 01000:
              rkca |= (rkcmd & 070) << 9;
              rkwc = (rkcmd & 0100) ? 128 : 256;
              rkda |= (rkcmd & 1) ? 4096 : 0;
              //f_lseek(&rk05, rkda * 512);
              while (!rk05.seek(rkda * 512, SeekSet)) {
                  Serial.println("Seek fail");
                  rk05.close();
                  rk05 = SD.open("/rk05.dsk", "r+");
                  delay(500);
              }
              p = (uint8_t *)&mem[rkca];
              //f_read(&rk05, p, rkwc * 2, &bcnt);
              tmp = rk05.read(p, rkwc * 2);
              // Serial.printf("Read Mem:%04o Dsk:%04o Len:%d\r\n", rkca, rkda, tmp);
              rkca = (rkca + rkwc) & 07777;
              rkdn++;
              // printf(".");
              break;
            case 04000:
            case 05000:
              rkca |= (rkcmd & 070) << 9;
              rkwc = (rkcmd & 0100) ? 128 : 256;
              rkda |= (rkcmd & 1) ? 4096 : 0;
              //printf("Write Mem:%04o Dsk:%04o\n",rkca,rkda);
              //fseek(rk05,rkda*512,SEEK_SET);
              //f_lseek(&rk05, rkda * 512);
              rk05.seek(rkda * 512, SeekSet);
              p = (uint8_t *)&mem[rkca];
              //fwrite(p,2,rkwc,rk05);
              //f_write(&rk05, p, rkwc * 2, &bcnt);
              tmp = rk05.write(p, rkwc * 2);
              //rk05.flush();
              // Serial.printf("Write Mem:%04o Dsk:%04o Len:%d\r\n", rkca, rkda, tmp);
              rkca = (rkca + rkwc) & 07777;
              rkdn++;
              break;
            case 02000:
              break;
            case 03000:
              if (rkcmd & 0200) rkdn++;
              break;
          }
          digitalWrite(B_LED, LOW);
          break;
        case 4:
          rkca = acc & 07777;
          acc &= 010000;
          break;
        case 5:
          //        acc=(acc&010000)|(rkdn?04000:0);
          acc = (acc & 010000) | 04000;
          break;
        case 6:
          rkcmd = acc & 07777;
          acc &= 010000;
          break;
        case 7:
          printf("Not allowed...RK8E\n");
          break;
      }
      break;
				case 0130:
                    switch (inst & 0777)
                    {
                    case 0131:	clken = 1; clkcnt = 0; break;
                    case 0132:	clken = 0; break;
                    case 0133:	if (clkfl) {
                        clkfl = 0;
                        pc++;
                    }
                             break;
                    }
                    break;

  }
}

int group3(int xmd)
{
    int qtm, xtm = xmd & 07721, xtc, earg, farg, dphi;
    int tm1, tm2;

    if (xmd == 07431)					// Switch modes
    {
        eaemd = 1; return 0;
    }
    if (xmd == 07447 || xmd == 07777)
    {
        gtf = eaemd = 0; return 0;
    }

    if ((xmd == 07573 || xmd == 07575) && eaemd) {      // DPIC and DCM here
        if (xmd & 4) {
            mq = ~mq;                            // Complement
            acc = ~acc;
            mq &= 07777;
        }
        acc &= 07777;
        mq = mq + 1;
        if (mq & 010000)
            acc = acc + 1;
        mq &= 07777;
        acc &= 017777;
        return 0;
    }
    if (xmd & 0200)
        acc = acc & 010000;
    if ((xmd & 0120) == 0120) {
        qtm = acc;
        acc = mq | (acc & 010000);
        mq = qtm & 07777;
    }
    else {
        if (xmd & 020) {
            mq = acc & 07777;
            acc &= 010000;
        }
        if (xmd & 0100)
            acc |= mq;
    }
    if (xmd & 040 && !eaemd)
        acc |= eaesc;
    if (eaemd == 0) gtf = 0;
    if ((pc & 07770) == 010)
        mem[pc + ifl]++;
    earg = mem[pc + ifl] & 07777;
    //				if ( eaemd && ( earg&07770 )==010 ) 
    //                      mem[earg]++;

    xtc = eaemd ? xmd & 056 : xmd & 016;
    switch (xtc) {
    case 0:
        return 0;
        break;
    case 02:
        if (eaemd)
        {
            eaesc = acc & 037;
            acc &= 010000;
        }
        else {
            eaesc = (~earg) & 037;
            return 1;
        }
        return 0;
        break;
    case 04:                                        // MUY
        if (eaemd)
            earg = mem[earg + dfl];
        mq = mq * earg;
        xtm = ((mq) >> 12) & 07777;
        mq = ((acc & 07777) + mq) & 07777;
        acc = xtm;
        eaesc = 0;
        return 1;
        break;
    case 06:                                        // DVI
        if (eaemd)
            earg = mem[earg + dfl];
        if ((acc & 07777) >= earg) {               /* overflow? */
            acc = acc | 010000;                     /* set link */
            mq = ((mq << 1) + 1) & 07777;           /* rotate MQ */
            eaesc = 0;                                 /* no shifts */
        }
        else {
            xtm = ((acc & 07777) << 12) | mq;
            mq = xtm / earg;
            acc = xtm % earg;
            eaesc = 015;                               /* 13 shifts */
        }
        return 1;
        break;
    case 010:                                          // NMI
        xtm = (acc << 12) | mq;                    /* preserve link */
        for (eaesc = 0; ((xtm & 017777777) != 0) &&
            (xtm & 040000000) == ((xtm << 1) & 040000000); eaesc++)
            xtm = xtm << 1;
        acc = (xtm >> 12) & 017777;
        mq = xtm & 07777;
        if (eaemd && ((acc & 07777) == 04000) && (mq == 0))
            acc = acc & 010000;                     /* clr if 4000'0000 */
        return 0;
        break;
    case 012:                                          // SHL
        eaesc = eaemd ? 037 : 0;
        for (xtm = eaemd ? 0 : -1; xtm < (earg & 037); xtm++)
        {
            mq = mq << 1;
            acc = acc << 1;
            if (mq & 010000)
                acc += 1;
        }
        mq &= 07777;
        acc &= 017777;

        return 1;
        break;
    case 014:
        eaesc = eaemd ? 037 : 0;
        //printf("ACC:%05o MQ:%04o EARG:%04o\r\n",acc,mq,earg);
        if (acc & 04000)
            acc |= 010000;
        else
            acc &= 07777;
        for (xtm = eaemd ? 0 : -1; xtm < (earg & 037); xtm++)
        {
            gtf = mq & 1;
            mq = mq >> 1;
            if (acc & 04000)
                acc |= 030000;
            if (acc & 01)
                mq |= 04000;
            acc = acc >> 1;
        }
        mq &= 07777;
        acc &= 017777;
        return 1;
        break;
    case 016:
        eaesc = eaemd ? 037 : 0;
        acc &= 07777;
        for (xtm = eaemd ? 0 : -1; xtm < (earg & 037); xtm++)
        {
            gtf = mq & 1;
            mq = mq >> 1;
            if (acc & 01)
                mq |= 04000;
            acc = acc >> 1;
        }
        mq &= 07777;
        acc &= 07777;
        return 1;
        break;
    case 040:				// SCA
        if (eaemd) {
            acc &= 010000;
            acc |= eaesc;
        }
        return 0;
    case 042:
        dphi = mem[earg + dfl + 1];
        earg = mem[earg + dfl];
        acc &= 07777;
        mq = mq + earg;
        if (mq & 010000)
            acc = acc + 1;
        acc = (acc + dphi) & 017777;
        mq &= 07777;
        return 1;
    case 044:
        mem[dfl + earg++] = mq;
        mem[dfl + earg] = acc & 07777;
        return 1;
    case 050:				// DPSZ
        if ((acc & 07777) + mq == 0)
            return 1;
        break;
    case 056:               // SAM
        acc &= 07777;
        tm1 = (acc & 04000) ? acc - 010000 : acc;
        tm2 = (mq & 04000) ? mq - 010000 : mq;
        gtf = (tm1 <= tm2) ? 1 : 0;
        acc = mq - acc;
        acc &= 07777;
        acc |= (acc <= mq) ? 010000 : 0;
    default:
        return 0;
    }
    return 0;
}

void group2() {
  int state;

  state = 0;
  if (acc & 04000)
    state |= 0100;
  if ((acc & 07777) == 0)
    state |= 040;
  if (acc & 010000)
    state |= 020;
  if ((inst & 0160) == 0)
    state = 0;
  if (inst & 010) {
    if ((~state & inst) == inst)
      pc++;
  } else if (state & inst)
    pc++;
  if (inst & 0200)
    acc &= 010000;
  if (inst & 4)
    acc |= 0000;  //OSR
}

void group1() {

  if (inst & 0200)
    acc &= 010000;
  if (inst & 0100)
    acc &= 07777;
  if (inst & 040)
    acc ^= 07777;
  if (inst & 020)
    acc ^= 010000;
  if (inst & 1)
    acc++;
  acc &= 017777;
  switch (inst & 016) {
    case 2:
      tmp = (acc << 6) | ((acc >> 6) & 077);  // BSW .. v untidy!
      tmp &= 07777;
      if (acc & 010000) tmp |= 010000;
      acc = tmp;
      break;
    case 06:
      acc = acc << 1;
      if (acc & 020000)
        acc++;
    case 04:
      acc = acc << 1;
      if (acc & 020000)
        acc++;
      break;
    case 012:
      if (acc & 1)
        acc |= 020000;
      acc = acc >> 1;
    case 010:
      if (acc & 1)
        acc |= 020000;
      acc = acc >> 1;
      break;
  }
  acc &= 017777;
}

bool cycl(void) {
  if (intf && ibus) {
    mem[0] = pc & 07777;
    pc = 1;
    intf = intinh = 0;
    svr = (ifl >> 9) + (dfl >> 12);
    if (uflag == 3)
      svr |= 0100;
    dfr = ifr = dfl = ifl = uflag = 0;
  }

  ibus = ttf || (tto == TTWAIT) || usint || clkfl;
  inst = mem[pc + ifl];
  ma = ((inst & 0177) | ((inst & 0200) ? (pc & 07600) : 0)) + ifl;
  if (inst & 0400 && inst < 06000) {
    if ((ma & 07770) == 010)
      mem[ma]++;
    mem[ma] &= 07777;
    if (inst & 04000)
      ma = mem[ma] + ifl;
    else
      ma = mem[ma] + dfl;
  }
  //sprintf(bfr, "PC:%04o Inst:%04o MA:%04o Mem:%04o Acc:%05o\r\n", pc, inst, ma, mem[ma], acc);
  //Serial.print(bfr);
  pc++;
  if (kcnt++ > 2000) {
    Tloop();
    if (telnet.isConnected()) {
      if (telnet.peek() != -1 && !ttf) {
        tti = telnet.read();
        ttf = 1;
      }
    }
    if (Serial.available() && !ttf) {
      Serial.readBytes((char *)&tti, 1);
      ttf = 1;
    }
    kcnt = 0;
    if (tti == 5)  // Halt on keypress ^e
      return false;
  }
  if (tto && (tto < TTWAIT))
    tto++;
  if (pto && (pto < TTWAIT))
    pto++;
  if (vcflg && vcflg < 200)
      vcflg++;

  // 50 Hz clock ... set clock flag every 20000 cycles (av cycle=1us set by #cycles per call)

  if ((++clkcnt == 20000) && clken) {
      clkfl = 1;
      clkcnt = 0;
  }

  switch (inst & 07000) {
    case 0:  //AND
      acc &= mem[ma] | 010000;
      break;
    case 01000:  //TAD
      acc += mem[ma] & 07777;
      break;
    case 02000:
      if (!(mem[ma] = (mem[ma] + 1) & 07777))
        pc++;
      break;
    case 03000:  //DCA
      mem[ma] = acc & 07777;
      acc &= 010000;
      break;
    case 04000:  //JMS
      ifl = ifr << 9;
      ma = (ma & 07777) + ifl;
      mem[ma] = pc;
      pc = (ma + 1) & 07777;
      intinh &= 1;
      uflag |= 2;
      break;
    case 05000:  //JMP
      ifl = ifr << 9;
      pc = (ma & 07777);
      intinh &= 1;
      uflag |= 2;
      break;
    case 06000:  //IOT
      iot();
      break;
    case 07000:  //OPR
      if (inst & 0400) {
        if (inst & 1) {
          if (group3(inst))
              pc++;
          break;
        }
        if (inst & 2)
          return false;
        group2();
      } else
        group1();
      break;
  }
  acc &= 017777;
  if (intinh == 1 && inst != 06001)
    intf = 1;
  return true;
}

int xmain(int argc, char *args[]) {

  char bfr[128];
  short dms[] = {
    06603,
    06622,
    05201,
    05604,
    07600,
  };
  short os8[] = {
    07600,
    06603,
    06622,
    05352,
    05752,
  };


  if (argc != 3) {
    printf("Usage: nano8 <binfile> <start address (octal)>\n");
    exit(1);
  }

  for (i = 0; i < 4096; i++)
    mem[i] = 07402;
  mem[07750] = 07576;
  mem[07751] = 07576;
  memcpy(&mem[0200], dms, sizeof(dms));
  memcpy(&mem[07750], os8, sizeof(os8));
  mem[030] = 06743;
  mem[031] = 05031;
  caf();
  //sscanf(args[2],"%o",&pc);
  pc = 030;
  cntr = 0;
  Serial.printf("Run from: %04o\r\n", pc);
  while (1) {
    systime = time_ms();
    while (cycl()) {
    }
    rk05.close();
    // f_close(&ptwrite);
    //f_close(&ptread);
    Serial.println("Halt");
    sprintf(bfr, "PC:%04o Inst:%04o MA:%04o Mem:%04o Acc:%05o\r\n", pc, inst, ma, mem[ma], acc);
    Serial.print(bfr);
    delay(1000);
    Serial.print("Enter new PC (octal):");
    readline(bfr, 80);
    sscanf(bfr, "%o", &kcnt);  // Use kcnt as temp int
    rk05 = SD.open("/rk05.dsk", "r+");
    while (Serial.read() != -1)
      ;
    caf();
    pc = kcnt;
  }
  return 0;
}
