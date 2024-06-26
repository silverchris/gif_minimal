#include <AnimatedGIF.h>
#define GIF_NAME rick
#include "../test_images/earth_128x128.h"
#include "rick.h"

uint8_t *pTurboBuffer;

AnimatedGIF gif;
int iOffX, iOffY;

void setup(void)
{  
  Serial.begin(115200);
  // while (!Serial) delay(100);

  Serial.println("Beginning");
  // Init Display

  gif.begin(GIF_PALETTE_RGB565_LE); // Set the cooked output type we want (compatible with SPI LCDs)
}

void GIFDraw(GIFDRAW *pDraw)
{
} /* GIFDraw() */

void loop()
{
  long lTime;
  int iFrames, iFPS;

// Allocate a buffer to enable Turbo decoding mode (~50% faster)
// it requires enough space for the full "raw" canvas plus about 32K workspace for the decoder
  pTurboBuffer = (uint8_t *)heap_caps_malloc(TURBO_BUFFER_SIZE + (480*480), MALLOC_CAP_INTERNAL);


  while (1) { // loop forever
     // The GIFDraw callback is optional if you use Turbo mode (pass NULL to disable it). You can either
     // manage the transparent pixels + palette conversion yourself or provide a framebuffer for the 'cooked'
     // version of the canvas size (setDrawType to GIF_DRAW_FULLFRAME).
      if (gif.open((uint8_t *)GIF_NAME, sizeof(GIF_NAME), GIFDraw)) {
      Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
      gif.setDrawType(GIF_DRAW_COOKED); // We want the library to generate ready-made pixels
      gif.setTurboBuf(pTurboBuffer); // Set this before calling playFrame()

      iOffX = (480 - gif.getCanvasWidth())/2; // center on the display
      iOffY = (480 - gif.getCanvasHeight())/2;
      lTime = micros();
      // Decode frames until we hit the end of the file
      // false in the first parameter tells it to return immediately so we can test the decode speed
      // Change to true if you would like the animation to run at the speed programmed into the file
      iFrames = 0;
      while (gif.playFrame(false, NULL)) {
        iFrames++;
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