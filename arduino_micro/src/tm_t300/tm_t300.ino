// Include necessary libraries
#include <SPI.h>
#include <Joystick.h>

// Define constants
#define AMOUNTOFBYTES 4
#define WHEELIDBITS 6
#define WHEELBUTTONBITS (AMOUNTOFBYTES * 8 - WHEELIDBITS)

// Create a Joystick object with default report ID, of type gamepad, with 21 buttons and 1 hat switch
Joystick_ Joystick(
    JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD, 13, 1, // 21 buttons, 1 hatswitch
    false, false, false, false, false, false,
    false, false, false, false, false);

// Define constants, arrays and variables
const int slaveSelectPin = 7;                                  // This is the pin used to select the slave device (the wheel in this case)
byte pos[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}; // Array used for bit operations to identify button presses
byte currBit[AMOUNTOFBYTES * 8];
byte prevBit[AMOUNTOFBYTES * 8];                                                                                                                  // Current reading from the wheel
int wheelbyte, fourthbyte, fifthbyte, wheelID;                                                                                                    // Variables used to store decoded wheel information
bool btnState, joyBtnState, prevJoyBtnState, buttonsreset, wheelIdentified;                                                                       // Variables used to store button states and wheel identification status
const bool debugging = true;                                                                                                                      // Enable or disable debugging
int bit2btn[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}; // Working array of buttons
// These arrays hold the mapping between bit positions and actual button functions for different wheel types
int T300Btn[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 6, 7, 9, 3, 1, 8, 5,
    2, 10, 33, 0, 32, 34, 31, 4,
    -1, -1, -1, -1, -1, -1, -1, -1};
int F599Btn[AMOUNTOFBYTES * 8] = {-1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, 12, 33, 32, 34, 31, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}; // Button numbers for 599xx wheel
int R383Btn[AMOUNTOFBYTES * 8 - WHEELIDBITS] = {-1, 5, 1, 4, 8, -1, 6, 12, 7, -1, 2, 9, 34, 31, 32, 33, 3, 0, -1, -1, -1, -1, -1, -1, -1, -1};           // Button numbers for R383 wheel
int F1Btn[AMOUNTOFBYTES * 8] = {4, -1, -1, -1, 0, 1, 2, 3, 12, 5, 6, 7, 8, 9, 10, 11, 17, 33, 32, 34, 31, 18, 19, 20, -1, 15, 13, 14, 16, -1, -1, -1};   // Button numbers for F1 wheel
int flag = 1;

// T300
int firstByteID[WHEELIDBITS] = {1, 1, 0, 1, 0, 0};
int buttonMap[AMOUNTOFBYTES * 8 - WHEELIDBITS] = {
    -1,   // 0
    -1,   // 1
    -1,   // 2
    5,    // 3
    8,    // 4
    6,    // 5
    9,    // 6
    2,    // 7
    7,    // 8
    10,   // 9
    3,    // 10
    10,   // 11
    -90,  // 12
    1,    // 13
    -270, // 14
    -90,  // 15
    -360, // 16
    4,    // 17
    -1,   // 18
    -1,   // 19
    -1,   // 20
    -1,   // 21
    -1,   // 22
    -1,   // 23
    -1,   // 24
    -1,   // 25
};

bool release_next_cycle = false;

void print_byte(byte currBit[], int size)
{
  for (int i = 0; i < size; i++)
  {
    Serial.print(currBit[i]);
  }
  Serial.println();
}

void get_wheel_data(byte *currBit)
{
  digitalWrite(slaveSelectPin, LOW);
  delayMicroseconds(40);
  for (int i = 0; i < AMOUNTOFBYTES; i++)
  {
    byte byteValue = SPI.transfer(0x00);
    for (int j = 0; j < 8; j++)
    {
      currBit[i * 8 + j] = (byteValue & pos[j]) ? 1 : 0;
    }
    delayMicroseconds(40);
  }
  digitalWrite(slaveSelectPin, HIGH);
  delayMicroseconds(40);
  buttonsreset = false;
}

bool check_wheel_type(byte *currBit)
{
  bool isWheel = true;
  for (int j = 0; j < WHEELIDBITS; j++)
  {
    if (currBit[j] != firstByteID[j])
    {
      isWheel = false;
      break;
    }
  }
  if (isWheel)
  {
    if (debugging)
    {
      Serial.println("Wheel identified");
    }
    return true;
  }
  return false;
}

bool up;
bool down;
bool left;
bool right;

void setButton(int button, int state)
{
  if (button >= 0)
  {
    Joystick.setButton(button, state);
  }
  else if (state == -90)
  {
    up = true;
  }
  else if (state == -180)
  {
    down = true;
  }
  else if (state == -270)
  {
    left = true;
  }
  else if (state == -360)
  {
    right = true;
  }
}

void setHatSwitch()
{
  if (!(up | down | left | right))
  {
    Joystick.setHatSwitch(0, -1);
  }
  else if (up)
  {
    Joystick.setHatSwitch(0, 0);
  }
  else if (right)
  {
    Joystick.setHatSwitch(0, 90);
  }
  else if (down)
  {
    Joystick.setHatSwitch(0, 180);
  }
  else if (left)
  {
    Joystick.setHatSwitch(0, 270);
  }
  up = false;
  down = false;
  left = false;
  right = false;
}

void setup()
{
  // Initialize communication with wheel
  Serial.begin(9600);
  SPI.beginTransaction(SPISettings(40000, MSBFIRST, SPI_MODE0));
  SPI.begin();
  pinMode(slaveSelectPin, OUTPUT);
  wheelIdentified = false;
  // Initialize joystick communication
  Joystick.begin();
  delay(1000);

  get_wheel_data(currBit);
  wheelIdentified = check_wheel_type(currBit);
}

void loop()
{
  Joystick.setHatSwitch(0, -1);
  get_wheel_data(currBit);
  if (!wheelIdentified) // If the wheel is not identified, wait 1 second and try again
  {
    Serial.println("Wheel not identified, waiting 1 second");
    wheelIdentified = check_wheel_type(currBit);
    if (debugging)
    {
      print_byte(currBit, AMOUNTOFBYTES * 8);
    }
    delay(1000);
    return;
  }

  for (int i = 6; i < sizeof(currBit); i++)
  {
    if (currBit[i] < prevBit[i])
    {
      setButton(buttonMap[i - 6], 1);
      if (debugging)
      {
        Serial.print("Index: ");
        Serial.println(i - 6);
        Serial.print("Button ");
        Serial.print(buttonMap[i - 6]);
        Serial.println(" pressed");
      }
    }
    else if (currBit[i] > prevBit[i])
    {
      setButton(buttonMap[i - 6], 0);
      if (debugging)
      {
        Serial.print("Index: ");
        Serial.println(i - 6);
        Serial.print("Button ");
        Serial.print(buttonMap[i - 6]);
        Serial.println(" released");
      }
    }
  }
  // Hatswitch handling
  setHatSwitch();

  // Store current button state
  for (int i = 0; i < sizeof(currBit); i++)
  {
    prevBit[i] = currBit[i];
  }
}
