/** WiFi LEDs controlled by MQTT.

   Topics and messages:
     wifilights/system - System started notifications, identify which Node is active
     wifilights/AABBCCDDEEFF/control - Send control messages
            delay=100 - Set delay between pixel changes on wipe
            rdelay=500 - Set transition delay on rainbow mode
            off - Turn off all LEDs - Alternative to colour topic
     wifilights/AABBCCDDEEFF/colour - New colour.
            #rrggbb for RGB codes
            red|blue|green for named colours
            rainbow for rainbow sequence
            off to turn off all LEDs.

   TODO: Add Node heartbeat to show its still on and connected with current LED status
   TODO: Add CryptoAuth chip for signing messages - would need to go to JSON message format
   TODO: Proper license and header

   (c) 2015 Andrew D. Lindsay, Thing Innovations.
*/
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

#define BUTTON_PIN   0    // Digital IO pin connected to the button.  This will be
// driven with a pull-up resistor so the switch should
// pull the pin to ground momentarily.  On a high -> low
// transition the button press logic will execute.

#define PIXEL_PIN    2    // Digital IO pin connected to the NeoPixels.

// TODO: Better way to set these up
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// MQTT Server
char* server = "192.168.1.3";

// MQTT Topics to publish to and subscribe to
// SUB: Get new colours or control information
// PUB: Publish out status, e.g. starting, reconnected etc
#define MQTT_PUB_TOPIC "wifilights/status"
#define MQTT_BASE_TOPIC "wifilights/"
#define MQTT_COLOUR_TOPIC_PART "/colour"
#define MQTT_CONTROL_TOPIC_PART "/control"
// TODO: Use individual light specific topic.

// Hardware defines for this light
// TODO: Look at defining number of pixels remotely
#define PIXEL_COUNT    20

// Parameter 1 = number of pixels in strip,  neopixel stick has 8
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream, correct for neopixel stick
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip), correct for neopixel stick
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_RGB + NEO_KHZ400);

void callback(char* topic, byte* payload, unsigned int length);

// Wifi/MQTT objects
WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);

// Build the client name based on "WiFiLight-MAC-millis" to provide uniqueness
String clientName;
String ledIdStr;
String subTopic;
String colourTopic;
String controlTopic;

// If using rainbow then keep track so function doesnt take up time
#define RAINBOW_DELAY 50
#define WIPE_DELAY 20
boolean rainbowMode = true;
long rainbowMillis = 0L;
int rainbowOuter = 0;
long rainbowDelay = RAINBOW_DELAY;
long wipeDelay = WIPE_DELAY;

// For WiFi Star V1.0 LEDs 13 and 14 are swapped
void setPixelColor(uint16_t n, uint32_t c) {
  if ( n == 13 )
    n = 14;
  else if ( n == 14 )
    n = 13;
  strip.setPixelColor( n, c );
}


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

/** callback for MQTT subscription
   New messages on the subscribed topic are processed here.
   If wildcard topic is used, then individual topic code would need to handle their
   corresponding payloads.
   @param topic - Char pointer to topic name. 0 terminated
   @param payload - Pointer to payload, not 0 terminated, use length.
   @param length - of payload

*/
void callback(char* topic, byte* payload, unsigned int length) {
  char *pl = (char*)payload;
  char *tPtr = topic;
  // Is it a control topic or colour topic message
  //  if ( strncmp(tPtr, MQTT_CONTROL_TOPIC, strlen(MQTT_CONTROL_TOPIC)) == 0 ) {
  // TODO: Make into string comparisons instead of char str
  if ( strncmp(tPtr, (char*)controlTopic.c_str(), strlen((char*)controlTopic.c_str())) == 0 ) {
    // Control message is of form name=value or off
    char value[10];
    if (strncmp(pl, "delay=", 6) == 0) {
      // Wipe delay
      strncpy( value, &pl[6], length - 6);
      int tmp = atoi(value);
      wipeDelay = tmp;
    } else if (strncmp(pl, "rdelay=", 7) == 0) {
      // Rainbow delay
      strncpy( value, &pl[7], length - 7);
      int tmp = atoi(value);
      rainbowDelay = tmp;
    } else if (strncmp(pl, "off", 3) == 0) {
      // All off
      clearStrip();
      rainbowMode = false;
    }
    //  } else if ( strncmp(tPtr, MQTT_COLOUR_TOPIC, strlen(MQTT_COLOUR_TOPIC)) == 0 ) {
  } else if ( strncmp(tPtr, (char*)colourTopic.c_str(), strlen((char*)colourTopic.c_str())) == 0 ) {

    // Its a colour topic message, set next colour
    rainbowMode = false;
    if (strncmp(pl, "red", 3) == 0) {
      colorWipe(Color(255, 0, 0), wipeDelay);
    } else if (strncmp(pl, "green", 5) == 0) {
      colorWipe(Color(0, 255, 0), wipeDelay);
    } else if (strncmp(pl, "blue", 4) == 0) {
      colorWipe(Color(0, 0, 255), wipeDelay);
    } else if (strncmp(pl, "off", 3) == 0) {
      colorWipe(Color(0, 0, 0), 0);
    } else if (strncmp(pl, "#", 1) == 0) {
      // get colour hex value only if message long enough
      if ( length > 6 ) {
        char col[10];
        strncmp( col, &pl[1], length - 1);
        // Should be 6 hex characters
        int red = getColour( &pl[1] );
        int green = getColour( &pl[3] );
        int blue = getColour( &pl[5] );
        colorWipeIn(Color(red, green, blue), wipeDelay);
      }
    } else if (strncmp(pl, "rainbow", 7) == 0) {
      // Reset Rainbow mode
      rainbowMode = true;
      rainbowOuter = 0;
    } else {
      // Turn off
      rainbowMode = false;
      clearStrip();
    }
  }
}

/** nodeIdToStr - Convert ESP8266 MAC address in array to string representation. Called Node ID
   here to avoid confusion with Message Authentication Codes (MAC) in CryptoAuth.
   @param mac - pointer to WiFi MAC address array
   @param incColon - Include : between octets if true
   @return String representation of MAC address
*/
String nodeIdToStr(const uint8_t* mac, bool incColon)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (incColon && i < 5)
      result += ':';
  }
  return result;
}

/** setup - Perform various setup functions of hardware, connect to WiFi
*/
void setup() {

  strip.begin();
  clearStrip();

  // Set brighness, 0=darkest (off), 255=brightest.
  strip.setBrightness(50);

  colorWipe(strip.Color(0, 0, 0), 0);    // Black/off

  // Connect to WiFi network
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  colorWipe(strip.Color(0, 255, 0), 50);
  while (WiFi.status() != WL_CONNECTED) {
    // Flash LED Blue while waiting
    colorWipeIn(strip.Color(0, 0, 255), 100 ); // Blue
    strip.show();
    delay(200);
    colorWipeIn(strip.Color(0, 0, 0), 50);  // Off
    strip.show();
  }

  colorWipe(strip.Color(0, 0, 0), 0);    // Black/off

  // Generate client name based on Product, ESP8266 MAC address and last 8 bits of microsecond counter
  clientName += "XmasStar-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  ledIdStr = nodeIdToStr(mac, false);

  clientName += ledIdStr;
  clientName += "-";
  clientName += String(micros() & 0xff, 16);

  // Setup colour and control topic names
  colourTopic = MQTT_BASE_TOPIC + ledIdStr + MQTT_COLOUR_TOPIC_PART;
  controlTopic = MQTT_BASE_TOPIC + ledIdStr + MQTT_CONTROL_TOPIC_PART;
  subTopic =  MQTT_BASE_TOPIC + ledIdStr + "/#";

  if (client.connect((char*) clientName.c_str())) {
    String startMsg = clientName + " Started";
    if (client.publish(MQTT_PUB_TOPIC, (char*)startMsg.c_str())) {
      // Published
      setPixelColor(0, Color(0, 255, 0));    // Green
      strip.show();
    }
  }
  else {
    // Restart whole device and try to re-connect
    setPixelColor(0, Color(255, 0, 0));  // Red
    strip.show();
    delay(1000);
    clearStrip();
    abort();
  }

  client.subscribe((char*)subTopic.c_str());    //MQTT_SUB_TOPIC);
  setPixelColor(0, Color(128, 128, 128));    // 1/2 white for 500mS
  strip.show();
  delay(500);
  setPixelColor(0, Color(0, 0, 0));    // Off
  strip.show();

  // Ready!
}

/** loop - main loop where processing happens
   Look for incoming subscribed messages, if rainbow mode, determine if its
   time to move the sequence on
*/
void loop() {
  client.loop();

  if ( rainbowMode ) {
    if ( millis() > rainbowMillis + rainbowDelay) {
      rainbowMillis = millis();
      rainbowStrip( rainbowOuter ) ;
      if ( ++rainbowOuter > 255 ) {
        rainbowOuter = 0;
      }
    }
  }
}

/** rainbowStrip - Re-draw strip based on outer loop value
   @param outer - The outer loop value, range 0-255
*/
void rainbowStrip(int outer) {
  for (int i = 0; i < strip.numPixels(); i++) {
    setPixelColor(i, Wheel( (i + outer) % 255));
  }
  strip.show();   // write all the pixels out
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
  for (uint16_t i = 0; i < 4; i++) { // LEDs on each arm
    for ( uint16_t j = 0; j < 5; j++) { // Arm
      int pixelNum = 4 * j + i;
      setPixelColor(pixelNum, c);
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
