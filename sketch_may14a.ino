// SPDX-FileCopyrightText: 2023 Limor Fried for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <Arduino_GFX_Library.h>

#include <AnimatedGIF.h>
#define GIF_NAME rick
#include "../test_images/earth_128x128.h"
#include "rick.h"

uint8_t *pTurboBuffer;

AnimatedGIF gif;
int iOffX, iOffY;

Arduino_XCA9554SWSPI *expander = new Arduino_XCA9554SWSPI(
    PCA_TFT_RESET, PCA_TFT_CS, PCA_TFT_SCK, PCA_TFT_MOSI,
    &Wire, 0x3F);
    
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    TFT_DE, TFT_VSYNC, TFT_HSYNC, TFT_PCLK,
    TFT_R1, TFT_R2, TFT_R3, TFT_R4, TFT_R5,
    TFT_G0, TFT_G1, TFT_G2, TFT_G3, TFT_G4, TFT_G5,
    TFT_B1, TFT_B2, TFT_B3, TFT_B4, TFT_B5,
    1 /* hsync_polarity */, 50 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,
    1 /* vsync_polarity */, 16 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 18 /* vsync_back_porch */
//    ,1, 30000000
    );

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
// 2.1" 480x480 round display
    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    expander, GFX_NOT_DEFINED /* RST */, TL021WVC02_init_operations, sizeof(TL021WVC02_init_operations));

void setup(void)
{  
  Serial.begin(115200);
  // while (!Serial) delay(100);
  
#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  Serial.println("Beginning");
  // Init Display

  Wire.setClock(1000000); // speed up I2C 
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }

  Serial.println("Initialized!");

  gfx->fillScreen(BLACK);

  expander->pinMode(PCA_TFT_BACKLIGHT, OUTPUT);
  expander->digitalWrite(PCA_TFT_BACKLIGHT, HIGH);

  gif.begin(GIF_PALETTE_RGB565_LE); // Set the cooked output type we want (compatible with SPI LCDs)
}

void GIFDraw(GIFDRAW *pDraw)
{
    int y;
    uint16_t *d;

    auto *buffer = gfx->getFramebuffer();

    y = pDraw->iY + pDraw->y + iOffY; // current line
    d = &buffer[iOffX+ pDraw->iX + (y * 480)];
    memcpy(d, pDraw->pPixels, pDraw->iWidth * 2);
} /* GIFDraw() */

static void *GIFAlloc(uint32_t u32Size) {
    return heap_caps_malloc(u32Size, MALLOC_CAP_SPIRAM);
} /* GIFAlloc() */


void loop()
{
  long lTime;
  int iFrames, iFPS;

// Allocate a buffer to enable Turbo decoding mode (~50% faster)
// it requires enough space for the full "raw" canvas plus about 32K workspace for the decoder
  pTurboBuffer = (uint8_t *)heap_caps_malloc(TURBO_BUFFER_SIZE + (480*480), MALLOC_CAP_8BIT);


  while (1) { // loop forever
     // The GIFDraw callback is optional if you use Turbo mode (pass NULL to disable it). You can either
     // manage the transparent pixels + palette conversion yourself or provide a framebuffer for the 'cooked'
     // version of the canvas size (setDrawType to GIF_DRAW_FULLFRAME).
      if (gif.open((uint8_t *)GIF_NAME, sizeof(GIF_NAME), GIFDraw)) {
      Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
      gif.setDrawType(GIF_DRAW_COOKED); // We want the library to generate ready-made pixels
      gif.setTurboBuf(pTurboBuffer); // Set this before calling playFrame()
      gif.allocFrameBuf(GIFAlloc);

      iOffX = (480 - gif.getCanvasWidth())/2; // center on the display
      iOffY = (480 - gif.getCanvasHeight())/2;
      lTime = micros();
      // Decode frames until we hit the end of the file
      // false in the first parameter tells it to return immediately so we can test the decode speed
      // Change to true if you would like the animation to run at the speed programmed into the file
      iFrames = 0;
      while (gif.playFrame(false, NULL)) {
        iFrames++;
        gfx->flush();
        if(gif.getLastError() != 0){
          Serial.printf("Gif Error %i\n", gif.getLastError());
          while(1){}
        }
      }
      lTime = micros() - lTime;
      iFPS = (iFrames * 10000000) / lTime; // get 10x FPS to make an integer fraction of 1/10th 
      Serial.printf("total decode time for %d frames = %d us, %d.%d FPS\n", iFrames, (int)lTime, iFPS/10, iFPS % 10);
      gif.close(); // You can also use gif.reset() instead of close() to start playing the same file again
    }
  } // while (1)
}