/*
 * Arduino interface for the use of WS2812 strip LEDs
 * Uses Adalight protocol and is compatible with Boblight, Prismatik etc...
 * "Magic Word" for synchronisation is 'Ada' followed by LED High, Low and Checksum
 * @author: Wifsimster <wifsimster@gmail.com> 
 * @library: FastLED v3.001
 * @date: 11/22/2015
 */
#include "FastLED.h"
#define NUM_LEDS 111
#define DATA_PIN 6
#define SENSOR_PIN A10

//uint8_t valSensor = 255;
fract8 smoothing = 255;
bool updateReady = true;

// Baudrate, higher rate allows faster refresh rate and more LEDs (defined in /etc/boblight.conf)
#define serialRate 256000

// Adalight sends a "Magic Word" (defined in /etc/boblight.conf) before sending the pixel data
uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;

// Initialise LED-array
CRGB leds[NUM_LEDS];
CRGB ledBuffer[NUM_LEDS];

void setup() {
  // Use NEOPIXEL to keep true colors
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  
  // Initial RGB flash
  LEDS.showColor(CRGB(255, 0, 0));
  delay(500);
  LEDS.showColor(CRGB(0, 255, 0));
  delay(500);
  LEDS.showColor(CRGB(0, 0, 255));
  delay(500);
  LEDS.showColor(CRGB(0, 0, 0));
  
  Serial.begin(serialRate);
  // Send "Magic Word" string to host
  Serial.print("Ada\n");
}

void loop() { 
  // Wait for first byte of Magic Word
  for(i = 0; i < sizeof prefix; ++i) {
    waitLoop: //while (!Serial.available()) ;;
    if (!Serial.available() ) goto updateArray;
    
    // Check next byte in Magic Word
    if(prefix[i] == Serial.read()) continue;
    // otherwise, start over
    i = 0;
    goto waitLoop;
  }
  
  // Hi, Lo, Checksum  
  while (!Serial.available()) ;;
  hi=Serial.read();
  while (!Serial.available()) ;;
  lo=Serial.read();
  while (!Serial.available()) ;;
  chk=Serial.read();
  
  // If checksum does not match go back to wait
  if (chk != (hi ^ lo ^ 0x55)) {
    i=0;
    goto waitLoop;
  }

//  memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));
  // Read the transmission data and set LED values
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    byte r, g, b;    
    while(!Serial.available());
    r = Serial.read();
    while(!Serial.available());
    g = Serial.read();
    while(!Serial.available());
    b = Serial.read();
    
    ledBuffer[i].r = r;
    ledBuffer[i].g = g;
    ledBuffer[i].b = b;
  }
  updateReady = true;

  updateArray:
  if (updateReady) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      leds[i].r = lerp8by8( ledBuffer[i].r, leds[i].r, smoothing);
      leds[i].g = lerp8by8( ledBuffer[i].g, leds[i].g, smoothing);
      leds[i].b = lerp8by8(  ledBuffer[i].b,leds[i].b, smoothing);
    }
    FastLED.show();
  }

  EVERY_N_MILLISECONDS(100) {checkIn();}
}

void checkIn() {
  updateReady = true;
  float sensorValue = analogRead(SENSOR_PIN);

  //most of the effect of smoothing can be seen in the higher values, with the lower values seeming very similar.
  //So instead of linear input, I'm scaling it quadraticaly
//  smoothing = ease8InOutQuad( map( analogRead(SENSOR_PIN), 0, 1023, 0, 240) );

  if (sensorValue == 0) {
    smoothing = 0;
  } else {
    smoothing = 0 - ( sensorValue / 1023 - 1 ) * ( sensorValue / 1023 - 1 ) *255 + 254;
  }
  
//  Serial.print("Sensor: ");
//  Serial.println(sensorValue);
//  Serial.print("Smoothing: ");
//  Serial.println(smoothing);
}
