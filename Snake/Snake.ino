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


uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);

int controllerX = 0;
int controllerY = 0;

int moveThreshold = 30;

struct snakeLink
{
  int x;
  int y;
  snakeLink *next;
};

struct snake
{
  int xSpeed = 0;
  int ySpeed = 0;
  boolean ateAppleLastTurn = false;
  boolean alive = true;
  snakeLink *head;
};

snake *player;

void initSnake(int snakeLength, int x, int y) {
  snakeLink *current;
  snakeLink *previous = NULL;
  for (int i = x - snakeLength + 1; i <= x; i++) {
    current = new snakeLink;
    current->x = i;
    current->y = y;
    current->next = previous;
    previous = current;
  }

  player = new snake;
  player->head = current;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
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

  //Serial.println("Snake init start");
  initSnake(10, 32, 32);

  //Serial.println("Snake init done");
}

int16_t x = 0, dx = 1;

void readControllerInput()
{
  boolean success = nchuk.update();  // Get new data from the controller

  if (!success) {  // Ruh roh
    Serial.println("Controller disconnected!");
    delay(1000);
  }
  else {

    if (nchuk.buttonC()) {
      initSnake(10, 32, 32);
    }

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
  }
}

void calculateSnakeMovement() {

  if (controllerX != 0 || controllerY != 0) {
    //We have some update
    if (player->xSpeed != 0) {
      // Snake is moving on the X

      // We only care about y movements from the controller
      // as the snake will continue forward on the X anyways
      // and it can't change direction on the X

      if (controllerY != 0)
      {
        player->xSpeed = 0;
        player->ySpeed = controllerY;
      }
    } else if (player->ySpeed != 0) {
      // Snake is moving on the Y
      if (controllerX != 0)
      {
        player->ySpeed = 0;
        player->xSpeed = controllerX;
      }
    } else {
      // Snake is not moving
      if (controllerX == 1)
      {
        //-1 instantly killed the snake!
        player->xSpeed = controllerX;
      } else {
        // will just be 0 if it's not
        // being pressed.
        player->ySpeed = controllerY;
      }
    }
  }
}

void processSnake() {

  if (player->alive && (player->xSpeed != 0 || player->ySpeed != 0))
  {
    int newHeadX = player->head->x + player->xSpeed;
    int newHeadY = player->head->y + player->ySpeed;

    if(newHeadX > 63){
      newHeadX = 0;
    } else if(newHeadX < 0){
      newHeadX = 63;
    }

    if(newHeadY > 63){
      newHeadY = 0;
    } else if(newHeadY < 0){
      newHeadY = 63;
    }

    snakeLink *current;
    snakeLink *previous;
    current = player->head;
    boolean collision = false;
    while (current->next != NULL)
    {
      collision = collision || ((current->x == newHeadX) && (current->y == newHeadY));
      previous = current;
      current = current->next;
    }

    player->alive = !collision;

    if (player->ateAppleLastTurn) {
      // Snake ate apple last turn, so we create a new
      // link as the head.
      player->ateAppleLastTurn = false;
      snakeLink *newHead = new snakeLink;
      newHead->x = newHeadX;
      newHead->y = newHeadY;
      newHead->next = player->head;
      player->head = newHead;
    } else {
      // Move tail to the head, and make the second
      // last link the tail

      //current is the old tail
      current->x = newHeadX;
      current->y = newHeadY;
      current->next = player->head;
      player->head = current;

      //previous is the new tail
      previous->next = NULL;
    }

    //Serial.print("Count: ");
    //Serial.println(count);
  } else {
    //Serial.print("No movement yet");
  }

}

void drawSnake() {
  snakeLink *tmp = player->head;
  boolean head = true;
  uint16_t snakeColour = player->alive ? myRED : myMAGENTA;
  while (tmp != NULL) {
    display.drawPixel(tmp->x, tmp->y, snakeColour);
    tmp = tmp->next;
  }
  //adding a different colour for the head
  display.drawPixel(player->head->x, player->head->y, myGREEN);
}

void loop() {

  readControllerInput();
  calculateSnakeMovement();
  processSnake();


  //  display.clearDisplay();
  display.fillScreen(myBLACK);

  drawSnake();
  display.showBuffer();
  delay(10);
}
