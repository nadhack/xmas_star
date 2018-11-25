/*
    This sketch sends a request via HTTP GET requests to api.thingspeak.com

*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_NeoPixel.h>

#define PIN 2

const char* ssid = "YourSSID";
const char* password = "YourPassword";

const char* host = "api.thingspeak.com";

// Hardware defines for this light
#define NUM_PIXELS 20

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_RGB + NEO_KHZ800);


/** getColour - Get the integer colour value from a 2 hex digit characters
   @param val - The character pointer to hex values
   @return integer representation, 0-255 of the hex value
*/
int getColour( char *val ) {
  int col = 0;
  for ( int i = 0; i < 2; i++ ) {
    col *= 16;
    if ( *val >= 'a' && *val <= 'f')
      col += (*val - 'a' + 10);
    else if ( *val >= 'A' && *val <= 'F' )
      col += (*val - 'A' + 10);
    else
      col += (*val - '0');

    val++;
  }
  return col;
}

// For WiFi Star V1.0 LEDs 13 and 14 are swapped
void setPixelColor(uint16_t n, uint32_t c) {
  if( n == 13 )
    n = 14;
  else if( n == 14 )
    n = 13;
  strip.setPixelColor( n, c );
}

/** colorWipe - fill the dots one after the other with said color
   @param c - the colour value
   @param wait - Wait in mS between each pixel colour changes
*/
void colorWipe(uint32_t c, uint32_t wait) {
  if ( wait > 0 ) {
    for (int i = 0; i < strip.numPixels(); i++) {
      setPixelColor(i, c);
      strip.show();
      delay(wait);
    }
  } else {
    for (int i = 0; i < strip.numPixels(); i++)
      setPixelColor(i, c);
    strip.show();
  }
}

void colorWipeIn(uint32_t c, uint8_t wait) {
  // 5 arms, 4 LEDs/arm
  for(uint16_t i=0; i<4; i++) { // LEDs on each arm
    for( uint16_t j=0; j<5; j++) {  // Arm
      setPixelColor(4*j + i, c);
    }
    strip.show();
    delay(wait);
  }
}

/* Helper functions, taken from Adafruit WS2801 library example sketches */

/** clearStrip - reset the strip to all LEDs off
*/
void clearStrip() {
  for (int i = 0; i < strip.numPixels(); i++) {
    setPixelColor(i, 0L);
  }
  strip.show();
}

/** Colour - Create a 24 bit color value from R,G,B components
   @param r - Red component
   @param g - Green component
   @param b - Blue component
   @return 24 (32) bit encoded RGB value
*/
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

/** Wheel - Input a value 0 to 255 to get a color value.
   The colours are a transition r - g -b - back to r
   @param WheelPos - Position on the colour wheel
   @return 24 (32) bit encoded RGB value
*/
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
    return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


void setup() {
  // We start by connecting to a WiFi network
  strip.begin();
  // 0=darkest (off), 255=brightest.
  strip.setBrightness(50);
  strip.show(); // Initialize all pixels to 'off'

  // Use single LED to indicate connection and startup status
  colorWipeIn( Color(255, 0, 0), 100);  // Red
  strip.show();

  // Use single LED to indicate connection and startup status
  colorWipeIn( Color(0, 255, 0), 100);  // Blue
  strip.show();

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    // Flash LED Blue while waiting
    colorWipeIn(Color(0, 0, 255), 100 ); // Blue
    strip.show();
    delay(200);
    colorWipeIn(Color(0, 0, 0), 50);  // Off
    strip.show();
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status

    HTTPClient http;  //Declare an object of class HTTPClient

    http.begin("http://api.thingspeak.com/channels/1417/field/2/last.txt");  //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request

    if (httpCode > 0) { //Check the returning code

      String payload = http.getString();   //Get the request response payload
      char colour[10];
      payload.toCharArray(colour, 10);
      if (strncmp(colour, "#", 1) == 0) {
        // get colour hex value only if message long enough
        int length = payload.length();
        if ( length > 6 && length < 10) {
          char col[10];
          strncmp( col, &colour[1], length - 1);
          // Should be 6 hex characters
          int red = getColour( &colour[1] );
          int green = getColour( &colour[3] );
          int blue = getColour( &colour[5] );
          colorWipeIn(Color(red, green, blue), 250);
        }
      }
    }
    http.end();   //Close connection
  }

  delay(60000);

}
