/*******************************************************************
    WIP: Snake game using a 64x64 RGB LED Matrix, 
    an ESP32 and a Wii Nunchuck.

    Parts:
    ESP32 D1 Mini * - http://s.click.aliexpress.com/e/_s4fXSx
    ESP32 Matrix Shield (From my Tindie) = https://www.tindie.com/products/brianlough/esp32-matrix-shield-mini-32/

 *  * = Affilate

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/


    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/
// This needs to be above the PxMatrix include.
#define double_buffer

// ----------------------------
// Standard Libraries
// ----------------------------

#include <Wire.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <NintendoExtensionCtrl.h>
// This library is for interfacing with the Nunchuck

// Can be installed from the library manager
// https://github.com/dmadison/NintendoExtensionCtrl

#include <PxMatrix.h>
// The library for controlling the LED Matrix
//
// Can be installed from the library manager
//
// https://github.com/2dom/PxMatrix

// Adafruit GFX library is a dependancy for the PxMatrix Library
// Can be installed from the library manager
// https://github.com/adafruit/Adafruit-GFX-Library

// Pin definition for the Matrix
#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 2

// I2C pins (pxMatrix uses the default ones, so we need to use others)
#define ONE_SDA 27
#define ONE_SCL 33

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;


Nunchuk nchuk;   // Controller on bus #1

PxMATRIX display(64, 64, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

void IRAM_ATTR display_updater() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(30);
  portEXIT_CRITICAL_ISR(&timerMux);
}


uint16_t textColor = display.color565(0, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);
uint16_t lineColor = display.color565(255, 0, 0);
uint16_t backgroundColor = display.color565(0, 0, 0);

int snakeX = 32;
int snakeY = 32;

int controllerX = 0;
int controllerY = 0;

int snakeXSpeed = 0;
int snakeYSpeed = 0;

int moveThreshold = 30;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  display.begin(32);
  display.flushDisplay();

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &display_updater, true);
  timerAlarmWrite(timer, 2000, true);
  timerAlarmEnable(timer);

  Wire.begin(ONE_SDA, ONE_SCL);
  while (!nchuk.connect()) {
    Serial.println("Nunchuk on bus #1 not detected!");
    delay(1000);
  }

}

int16_t x = 0, dx = 1;

void loop() {

  boolean success = nchuk.update();  // Get new data from the controller

  if (!success) {  // Ruh roh
    Serial.println("Controller disconnected!");
    delay(1000);
  }
  else {
    // Read a joystick axis (0-255, X and Y)
    int joyY = nchuk.joyY();
    int joyX = nchuk.joyX();

    if (joyX > 127 + moveThreshold) {
      controllerX = 1;
    } else if (joyX < 127 - moveThreshold) {
      controllerX = -1;
    } else {
      controllerX = 0;
    }

    if (joyY > 127 + moveThreshold) {
      controllerY = -1;
    } else if (joyY < 127 - moveThreshold) {
      controllerY = 1;
    } else {
      controllerY = 0;
    }

    if (controllerX != 0 || controllerY != 0) {
      //We have some update
      if (snakeXSpeed != 0) {
        // Snake is moving on the X
        if (controllerY != 0)
        {
          snakeXSpeed = 0;
          snakeYSpeed = controllerY;
        }
      } else if (snakeYSpeed != 0) {
        // Snake is moving on the Y
        if (controllerX != 0)
        {
          snakeYSpeed = 0;
          snakeXSpeed = controllerX;
        }
      } else {
        // Snake is not moving
        if (controllerX != 0)
        {
          snakeXSpeed = controllerX;
        } else {
          // will just be 0 if it's not
          // being pressed.
          snakeYSpeed = controllerY;
        }
      }

    }

    snakeX += snakeXSpeed;
    snakeY += snakeYSpeed;

  }

  //  display.clearDisplay();
  display.fillScreen(backgroundColor);

  display.drawPixel(snakeX, snakeY, lineColor);
  display.showBuffer();
  delay(10);
}
