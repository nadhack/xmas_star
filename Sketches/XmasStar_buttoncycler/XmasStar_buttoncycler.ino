// This is a demonstration on how to use an input device to trigger changes on your neo pixels.
// You should wire a momentary push button to connect from ground to a digital IO pin.  When you
// press the button it will change to a new pixel animation.  Note that you need to press the
// button once to start the first animation!

#include <Adafruit_NeoPixel.h>

#define BUTTON_PIN   0    // Digital IO pin connected to the button.  This will be
// driven with a pull-up resistor so the switch should
// pull the pin to ground momentarily.  On a high -> low
// transition the button press logic will execute.

#define PIXEL_PIN    2    // Digital IO pin connected to the NeoPixels.

#define PIXEL_COUNT 20

// Parameter 1 = number of pixels in strip,  neopixel stick has 8
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream, correct for neopixel stick
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip), correct for neopixel stick
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_RGB + NEO_KHZ800);

bool oldState = HIGH;
int showType = 0;

// For WiFi Star V1.0 LEDs 13 and 14 are swapped
void setPixelColor(uint16_t n, uint32_t c) {
  if ( n == 13 )
    n = 14;
  else if ( n == 14 )
    n = 13;
  strip.setPixelColor( n, c );
}

/** Set a LED based on arm number and led on arm
   @param arm - The star arm, 0 to 4 with 0 being top moving clockwise
   @param led - Led number on the arm, 0 to 3
*/
void setLedColor(uint16_t arm, uint16_t led, uint32_t c) {
  uint16_t pixel = arm * 4 + led;
  setPixelColor( pixel, c );
}


void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  strip.begin();
  // Set brighness, 0=darkest (off), 255=brightest.
  strip.setBrightness(50);
  colorWipe(strip.Color(0, 0, 0), 0);    // Black/off
  /*
    for( uint16_t arm = 0; arm < 5; arm++ ) {
      for( uint16_t led = 0; led < 4; led++ ) {
        setLedColor(arm, led, strip.Color(255, 0, 0) );
        strip.show();
        delay(250);
      }
    }
  */
}

/** buttonPress
 * 
 */
bool buttonChange( void ) {
  // Get current button state.
  bool newState = digitalRead(BUTTON_PIN);

  // Check if state changed from high to low (button press).
  if (newState == LOW && oldState == HIGH) {
    // Short delay to debounce button.
    delay(50);
    // Check if button is still low after debounce.
    newState = digitalRead(BUTTON_PIN);
    if (newState == LOW) {
      delay(50);
      
      // Wait for button release
      while( digitalRead(BUTTON_PIN) == LOW ) {
        delay(20);
      }
      
      return true;
    }
  }
  // Set the last button state to the old state.
  oldState = newState;

  return false;
}

bool buttonPress( void ) {
  // Get current button state.
  bool newState = digitalRead(BUTTON_PIN);

  // Check if state changed from high to low (button press).
  if (newState == LOW && oldState == HIGH) {
    // Short delay to debounce button.
    delay(50);
    // Check if button is still low after debounce.
    newState = digitalRead(BUTTON_PIN);
    if (newState == LOW) {
      return true;
    }
  }

  return false;
}


void loop() {
  if ( buttonChange() ) {
    // Delay before change
    delay(500);
    showType++;
    if (showType > 8)
      showType = 0;
  }
  switch (showType) {
    case 0: rainbowCycle(20);
      break;
    case 1:
      colorWipeIn(strip.Color(255, 0, 0), 500);  // Red
      delay(100);
      colorWipeIn(strip.Color(0, 0, 0), 500);  // Black/off
      break;
    case 2:
      colorWipeIn(strip.Color(0, 255, 0), 250);  // Green
      delay(100);
      colorWipeOut(strip.Color(0, 0, 0), 500);  // Black/off
      break;
    case 3:
      colorWipeIn(strip.Color(0, 0, 255), 100);  // Blue
      delay(100);
      colorWipeIn(strip.Color(0, 0, 0), 500);  // Black/off
      break;
    case 4: theaterChase(strip.Color(127, 127, 127), 50); // White
      break;
    case 5: theaterChase(strip.Color(127,   0,   0), 50); // Red
      break;
    case 6: theaterChase(strip.Color(  0,   0, 127), 50); // Blue
      break;
    case 7: rainbow(20);
      break;
    case 8: theaterChaseRainbow(50);
      break;
  }

}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    setPixelColor(i, c);
    strip.show();
    if (buttonPress())
      return;
    delay(wait);
  }
}

void colorWipeIn(uint32_t c, uint8_t wait) {
  // 5 arms, 4 LEDs/arm
  for (uint16_t i = 0; i < 4; i++) { // LEDs on each arm
    for ( uint16_t j = 0; j < 5; j++) { // Arm
      setPixelColor(4 * j + i, c);
    }
    strip.show();
    if (buttonPress())
      return;
    delay(wait);
  }
}

void colorWipeOut(uint32_t c, uint8_t wait) {
  // 5 arms, 4 LEDs/arm
  for (int16_t i = 3; i >= 0; i--) { // LEDs on each arm
    for ( uint16_t j = 0; j < 5; j++) { // Arm
      setPixelColor(4 * j + i, c);
    }
    strip.show();
    if (buttonPress())
      return;
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    if (buttonPress())
      return;
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    if (buttonPress())
      return;
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        setPixelColor(i + q, c);  //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        setPixelColor(i + q, 0);      //turn every third pixel off
      }
      if (buttonPress())
        return;
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        setPixelColor(i + q, 0);      //turn every third pixel off
      }
      if (buttonPress())
        return;
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
