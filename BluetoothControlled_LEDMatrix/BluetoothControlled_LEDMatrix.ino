#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "config.h"

//#if SOFTWARE_SERIAL_AVAILABLE
//  #include <SoftwareSerial.h>
//#endif

// Libraries for the LEDs
#include <FastLED.h>
#include "font.h" // Include the font library

// Define LED matrix dimensions
#define NUM_LEDS                       160 // 8 rows * 20 columns
#define DATA_PIN                       6   // Pin connected to the LED strip

CRGB leds[NUM_LEDS];

// Create the bluefruit object hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

// LED matrix dimensions
int BRIGHTNESS = 64;
const int MATRIX_WIDTH = 20;
const int MATRIX_HEIGHT = 8;
CRGB currentColour = CRGB::Red;  // Variable to store the current color

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}


// Map a row-column coordinate to the LED index
int getLEDIndex(int x, int y) {
    if (y % 2 == 0) {
        // Even rows go left to right
        return y * MATRIX_WIDTH + x;
    } else {
        // Odd rows go right to left
        return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
    }
}

// Function to draw a character from the font
void drawChar(char c, int xOffset, CRGB color) {
    if (c < 32 || c > 127) return; // Skip unsupported characters
    byte charData[5];
    memcpy(charData, font[c - 32], 5); // Load font data for the character

    for (int col = 0; col < 5; col++) {
        if (xOffset + col < 0 || xOffset + col >= MATRIX_WIDTH) continue; // Skip if outside visible area
        byte column = charData[col];
        for (int row = 0; row < 8; row++) {
            int ledIndex = getLEDIndex(xOffset + col, row);
            if (column & (1 << row)) {
                leds[ledIndex] = color; // Set LED to the desired color
            } else {
                leds[ledIndex] = CRGB::Black; // Turn off LED
            }
        }
    }
}

// Function to scroll text across the matrix
void scrollText(const char* text, CRGB color, int scrollSpeed) {
    int textLength = strlen(text);
    int totalScrollWidth = textLength * 6; // Each char is 5 columns + 1 space
    for (int offset = MATRIX_WIDTH; offset > -totalScrollWidth; offset--) {
        FastLED.clear(); // Clear the display
        for (int i = 0; i < textLength; i++) {
            drawChar(text[i], offset + i * 6, color); // Draw each character
        }
        FastLED.show();
        delay(scrollSpeed);
    }
}

void printHelp(void)
{
  ble.println("This is the help page");
  ble.println("To change a configuration, enter a message starting with !");
  ble.println("The currently accepted configurations are the following:");
  ble.println("!h to print the help page");
  ble.println("!r to set the colour of the text to red");
  ble.println("!g to set the colour of the text to green");
  ble.println("!b to set the colour of the text to blue");
  ble.println("!0 to set the brightness of the text to off");
  ble.println("!1 to set the brightness of the text to the lowest setting");
  ble.println("!5 to set the brightness of the text to the highest setting");
  ble.println("!2, !3, !4 to set the brightness of the text to increasing settings");
}

void configureColour(char colour)
{
  switch(colour){
    case 'r':
      currentColour = CRGB::Red;
      break;
    case 'g':
      currentColour = CRGB::Green;
      break;
    case 'b':
      currentColour = CRGB::Blue;
      break;
  }
}

void configureBrightness(char brightness)
{
  switch(brightness){
    case '0':
      BRIGHTNESS = 0;
      break;
    case '1':
      BRIGHTNESS = 12;
      break;
    case '2':
      BRIGHTNESS = 25;
      break;
    case '3':
      BRIGHTNESS = 38;
      break;
    case '4':
      BRIGHTNESS = 51;
      break;
    case '5':
      BRIGHTNESS = 64;
      break;
  }
}

void setup(void)
{
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  delay(500);

  Serial.begin(115200);

  /* Initialise the module */
  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  //Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  /* Print Bluefruit information */
  ble.info();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

//  ble.println("Ready for data");
}

void loop(void)
{
  // Check for incoming characters from Bluefruit
  ble.println("AT+BLEUARTRX");
  ble.readline();
  if (strcmp(ble.buffer, "OK") == 0) {
    // no data
    return;
  }
  if(ble.buffer[0] == '!') {
     if(ble.buffer[1] == 'h'){
//        printHelp();
     }
     else if(isAlpha(ble.buffer[1])){
        configureColour(ble.buffer[1]);
     }
      else if(isDigit(ble.buffer[1])){
        configureBrightness(ble.buffer[1]);
        FastLED.setBrightness(BRIGHTNESS);
      }
  }
  else{
    scrollText(ble.buffer, currentColour, 100);
  }
  ble.waitForOK();
}
