# An implementation of a PDP8/E processor with a VC8 display for the ESP32-Wrover kit.
<br>
<br>
This project is an evaluation of the ESP-32 Wrover kit. The application is a simulated DEC PDP8/E with a VC8 display.
The system configuration is a PDP8 processor with an EAE and 32KW core. This includes a single KL8 serial port
which is interfaced to the default serial port of the ESP32 which is linked to a USB adapter and appears as a
COM port on the host system running at 115200 baud. In addition, the KL8 is interfaces to a telnet compatible
server listening on TCP/IP port 23 with a dns registered name esppdp8. The USB and telnet channels run in parallel such
that eithe interface may be used. The simulation also provides a VC8 point plot display which is visible via the 32x240
LCD provided with the Wrover kit. Finally, the simulation contains an RK05 disk drive handler which is connected to
a disk image name rk05.dsk which should be copied to an SD card and plugged into the card slot of the kit.
<br>
Hardware changes:

1. R117 should be removed from the Wrover kit PC. This allows the SD card and the LCD display to be used together.

Functional overview:

The PDP8 simulation sofware will not be described here in detail as the primary purpose of this project is to
test the viability of a CRT 'like' display on an LCD. The VC8 display is simply an XY display connected to 2x10 bit
DACs giving a max resolution of 1024x1024. There is no hardware refresh such that the processor must maintain the
display. In this case, a given pixel must be set and subsquently cleared by some process. There are many types of CRT
that may be used for an XY display varying is size and more importantly, the phosphor target on the screen. In order
to simulate a CRT display where a given pixel is briefly illuminated is to arrange for the luminance and possibly
the color of the pixel to gradually decay to black. The code for this is in TFTDriver.ino. The pixel data is stored
in 3 arrays. The decay array is initally filled with a number of RGB565 colors which decay exponentially from a start color
to black. Each of the individual RGB colors may be decayed at different rates. In the current configuration, the initial
color is white and the Blue component is set to rapidly decay which a constant of 0.6 while the Red and Green components
decay slower with a constant of 0.9. The second (bitmap) array is a 320x240 array allocated in PSRAM. This array
is used to contains a set of indices into the decay array. When a given index is 0, the corresponding entry in the decay
array is 0 and this is the default (black) state. Whne the index for the given pixel is set to 1, the fade function detects
this an uses the dacy array value at index 1. After the pixel has been updated with the value from the decay array
this index in the bitamp array incremented. After a number of cycle, the RGB565 value from the decay array will be zero
and the pixel will thne be cleared. In this case, the fade functon will skip over this pixel until it is set by the VC8 interface
again. Finally, the RGB565 value is written into the bmp array which is 320x240 of uint16. This array is then written to the LCD.
This process takes about 70ms. and is limited by the SPI clock to the LCD. The SPI transfer takes about 40 ms. The fade function
timing is between 0 to 30 ms. and depends on the number of active pixels. As the PDP8 write pixel rate is dependant upon the
programme and at best there may be aywhere between 1000 and 5000 active pixels. The fastest write speed is using the PDP8
app in M.PA which is a translation of the classic Munching Squares PDP1 app: https://www.masswerk.at/minskytron/. The current
phosphor setting matches the P7 used in the Type 30 display.
