
#include <TinyWireM.h>		        //ATtiny (e.g. Adafruit Trinket, Gemma) I2C library
#include <Adafruit_TinyLSM303.h>	//ATtiny LSM303DLHC accelerometer/magnetometer library
#include <Adafruit_NeoPixel.h>		//Adafruit Led Strip library

/* Constants */

// NEO_PIXEL
#define PIXELS_PIN		1			// Pixels are connected to digital 1
#define NUM_PIXELS		6			// amount of pixels in Strip
#define MAX_BRIGHTNESS 	        127			// maximum brightness between 0 <-> 255
#define COLOR_INTERVAL 	        1000		        // milliseconds of no motion till color change ( small change )		
// MIC breakout board
#define MIC_PIN 		2			// MIC is on analog 2
#define SAMPLE_WINDOW 	        50			// Sample window width in mS (50 mS = 20Hz)
#define NOISE 			100			// Noise/hum/interference in mic signal
// ACCELEROMETER
#define MOVE_THRESHOLD 	6000		// amount of motion till a shakle is tracked
#define PARTY_INTERVAL 	1000		// interval in which we expect a motion

/* Gloabal Variables */

// Neo_Pixel 
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, PIXELS_PIN, NEO_GRB + NEO_KHZ800);
byte brightness	 		= 180;		// initial brightness between 0 <-> 255
uint32_t color;	  				// var to hold colors
uint8_t color_timer 	        = 0;		// counter for COLOR_INTERVAL ( 32_t )
uint8_t  spectrum_part 	        = 0;		// switch for current light spectrum 
uint8_t color_idx 		= 0;		// current color can be between 0 - 255 ( 32_t )
uint8_t curr_color_granularity	= 1;// granularity, is amount of color change

// Accelerometer
Adafruit_TinyLSM303 	lsm;			
int8_t storedVector 			= 0;	// last motionVector ( 32_t )
int8_t newVector 			= 0;	// motion Vector ( 32_t )	
uint8_t nr_shackes			= 0;	// counter for shackes 
uint8_t last_shacke			= 0;	// time since last shacke ( 32_t )

void setup() {
  pixels.begin();
  pixels.setBrightness(brightness);

  // if lsm ( accelerometer) can't be initialised turn all red and hold
  // else turn green for 1 second
  if (!lsm.begin()) {
    setColor(pixels.Color(255, 0, 0));
    while (1);
  } 
  else {
    setColor(pixels.Color(0, 255, 0));
    delay(1000);
  }

}

void loop() {

  // set brigtness 
  brightness = getMicLevel(); 		// first get it from mic levels
  pixels.setBrightness(brightness);	// set brightness on pixels
  // get color change
  getColor();

}

/* check for motion and decide on color change */
uint32_t getColor() {

  // get motion vector
  lsm.read();
  newVector = lsm.accelData.x*lsm.accelData.x;
  newVector += lsm.accelData.y*lsm.accelData.y;
  newVector += lsm.accelData.z*lsm.accelData.z;
  newVector = sqrt(newVector);

  curr_color_granularity = 1;				// allwayes reset granularity to 1

    // if motion vector - last motion vector is bigger than MOVE_TRESHOLD we register a shackle
  if (abs(newVector - storedVector) > MOVE_THRESHOLD) {
    curr_color_granularity = 20;
    nextColor();
    last_shacke = millis(); 
    nr_shackes++;
  } 


  // if nothing moved as long as PARTY_INTERVAL shakes become 0
  if((millis() - last_shacke) >  PARTY_INTERVAL){
    nr_shackes = 0;
  } 

  storedVector = newVector; 				// save last motion vector

    // if shackes are bigger than 8 do somthing wonderfull ???
  if(nr_shackes > 8) {							
    //sparkel();
    nr_shackes = 0;	// reset shackes counter
  }

  // if nothing has happend since switch color a bit 
  if ((millis() - color_timer) > COLOR_INTERVAL) {
    nextColor();
    color_timer = millis();
  }

  setColor(color);

}

/* loop through color spectrum */
void nextColor ()
{
  switch (spectrum_part)
  {
  case 0 :  // spectral wipe from red to blue
    {
      color = Adafruit_NeoPixel::Color(255-color_idx,color_idx,0);
      color_idx += curr_color_granularity;
      if (color_idx > 255) 
      {
        spectrum_part = 1;
        color_idx = 0;
      }
      break;
    }
  case 1 :  // spectral wipe from blue to green
    {
      color = Adafruit_NeoPixel::Color(0,255-color_idx,color_idx);
      color_idx += curr_color_granularity;
      if (color_idx > 255) 
      {
        spectrum_part = 2;
        color_idx = 0;
      }
      break;
    }
  case 2 :  // spectral wipe from green to red
    {
      color = Adafruit_NeoPixel::Color(color_idx,0,255-color_idx);
      color_idx += curr_color_granularity;
      if (color_idx > 255) 
      {
        spectrum_part = 0;
        color_idx = 0;
      }
      break;
    }

  }
}


/*	get mic levels over a time of SAMPLE_WINDOW and calculate the 
 	average, than map it between 0 and 255 and map it again 
 	to the MAX_BRIGHTNESS 
 */
byte getMicLevel() {
  unsigned long startMillis= millis();	// Start of sample window
  unsigned int peakToPeak = 0;   			// peak-to-peak level

  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;

  unsigned int sample;

  byte level = 0;					// return value

  while (millis() - startMillis < SAMPLE_WINDOW) {
    sample = analogRead(MIC_PIN); 		// read the mic level

    sample = sample - NOISE;			// remove noise

    if (sample < 1024) {				// toss out spurious readings

      if (sample > signalMax) {
        signalMax = sample;  // save just the max levels
      } 
      else if (sample < signalMin) {
        signalMin = sample;  // save just the min levels
      }
    }
  }

  peakToPeak = signalMax - signalMin; 	// calc the average signal strength

  level = map(peakToPeak, 0, 1023, 0, 255 );              // map betwenn 0<->255
  level = map(brightness, 0, 255, 0, MAX_BRIGHTNESS);	// use max brightness to dimm
  return level;
}

/*	iterate over all pixels ( NUM_PIXELS ) and set a color */
void setColor(uint32_t color){
  // iterate over all pixels and set the color	
  for (int i = 0; i < NUM_PIXELS; i++) pixels.setPixelColor(i,color);
  pixels.show();
}


