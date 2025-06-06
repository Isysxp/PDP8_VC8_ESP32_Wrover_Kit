# An implementation of a PDP8/E processor with a VC8 display for the ESP32-Wrover kit.
<br>
<br>
This project is an evaluation of the ESP-32 Wrover kit. The application is a simulated DEC PDP8/E with a VC8 display.
The system configuration is a PDP8 processor with an EAE and 32KW core. This includes a single KL8 serial port
which is interfaced to the default serial port of the ESP32 which is linked to a USB adapter and appears as a
COM port on the host system running at 115200 baud. In addition, the KL8 is interfaces to a telnet compatible
server listening on TCP/IP port 23 with a DNS registered name esppdp8. The USB and telnet channels run in parallel such
that either interface may be used. The simulation also provides a VC8 point plot display which is visible via the 320x240
LCD provided with the Wrover kit. Finally, the simulation contains an RK05 disk drive handler which is connected to
a disk image named rk05.dsk which should be copied to an SD card and plugged into the card slot of the kit. This disk
image is in the repo and is Simh compatible.
<br>
<br>
Hardware changes:
<br>
<br>
1. R117 should be removed from the Wrover kit PC. This allows the SD card and the LCD display to be used together.
<br>
<br>
Functional overview:
<br>
<br>
The PDP8 simulation software will not be described here in detail as the primary purpose of this project is to
test the viability of a CRT 'like' display on an LCD. The VC8 display is simply an XY display connected to 2x10 bit
DACs giving a max resolution of 1024x1024. There is no hardware refresh such that the processor must maintain the
display. In this case, a given pixel must be set and subsequently cleared by some process. There are many types of CRT
that may be used for an XY display varying is size and more importantly, the phosphor target on the screen. In order
to simulate a CRT display where a given pixel is briefly illuminated is to arrange for the luminance and possibly
the color of the pixel to gradually decay to black. The code for this is in TFTDriver.ino. The pixel data is stored
in 3 arrays. The decay array is initially filled with a number of RGB565 colors which decay exponentially from a start color
to black. Each of the individual RGB colors may be decayed at different rates. In the current configuration, the initial
color is white and the Blue component is set to rapidly decay with a constant of 0.6 while the Red and Green components
decay slower with a constant of 0.9. The second (bitmap) array is a 320x240 array allocated in PSRAM. This array
is used to contains a set of indices into the decay array. When a given index is 0, the corresponding entry in the decay
array is 0 and this is the default (black) state. When the index for the given pixel is set to 1, the fade function detects
this an uses the decay array value at index 1. After the pixel has been updated with the value from the decay array
this index in the bitmap array incremented. After a number of cycles, the RGB565 value from the decay array will be zero
and the pixel will then be cleared. In this case, the fade function will skip over this pixel until it is set by the VC8 interface
again. Finally, the RGB565 value is written into the bmp array which is 320x240 of uint16. This array is then written to the LCD.
This process takes about 70ms. and is limited by the SPI clock to the LCD. The SPI transfer takes about 40 ms. The fade function
timing is between 0 to 30 ms. and depends on the number of active pixels. As the PDP8 write pixel rate is dependant upon the
programme and at best there may be anywhere between 1000 and 5000 active pixels. The fastest write speed is using the PDP8
app in M.PA which is a translation of the classic Munching Squares PDP1 app: https://www.masswerk.at/minskytron/. The current
phosphor setting matches the P7 used in the PDP1 Type 30 display. This decay pattern being defined as above in conjunction with
the timer callback rate which is 100ms.
<br>
<br>
Building the app:
<br>
<br>
For simplicity, this is an Arduino app. I would note that the ESP-IDF build environment is an option but is very complex.
A number of libraries are required. These may be determined from the include files.
My library source list in the Arduino preferences is:
<br>
<br>
https://adafruit.github.io/arduino-board-index/package_adafruit_index.json<br>
https://espressif.github.io/arduino-esp32/package_esp32_index.json<br>
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
<br>
<br>
The rk05 disk image:
<br>
<br>
There is a substantial number of files on this disk which even hard bitten PDP8 programmers might not recognise.
As a first try after the system boots to the monitor '.':
Type EXE M<return><br>
If you want to be more adventurous:
EXE CCR<br>
at the > prompt, type in RDR.C<return>
And, yes this is a PDP8 running OS/8 and there is a C compiler!<br>
Check https://tangentsoft.com/pidp8i/wiki?name=Home for more details.
<br>
<br>
Ian schofield June 2023<br>
<br>
This app has now been updated to follow updates in the Arduino environment:<br>
Board: esp32 by Espressif systems V 3.1.1<br>
Lib: LovyanGFX V 1.2.7<br>
  NB Set INFRA_SSID and INFRA_PSWD in ESP_Telnet.ino to match your router.<br>
<br>
Ian Schofield May 2025<br>



