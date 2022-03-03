//************LIBRARIES USED**************
// include the Bounce library for 'de-bouncing' switches -- removing electrical chatter as contacts settle
#include <Bounce2.h>
//usbMIDI.h library is added automatically when code is compiled as a MIDI device
#include <Adafruit_NeoPixel.h>
// for timing
#include <Chrono.h>

// autosave interval
const long interval = 2500;   //in millis
// number of led
const int numberOfLeds = 22;
// led states
int ledStates[numberOfLeds];
// order for startup led
const int ledOrder[numberOfLeds] = {5, 4, 3, 2, 1, 10, 9, 8, 7, 6, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};
// initial led brightness
int ledBrightness = 20;
bool up = 0;
bool down = 0;

//neoPixel schtuff
const int ledPin = 13; //pin the led strip is connected to
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numberOfLeds, ledPin, NEO_GRB + NEO_KHZ800);

// analog variables
const int A_PINS = 14; // number of Analog PINS
int data[A_PINS];
int dataLag[A_PINS];
int mapVal;
const int A = 25;
const int B = 26;
const int C = 27;

// midi variables
const int channel = 1; // MIDI channel
const int CCID[A_PINS] = {31, 32, 33, 34, 35, 36, 41, 42, 43, 44, 45, 46, 47, 48}; // analog control change id's

// digital variables
const int D_PINS = 27; // number of pins
const int DIGITAL_PINS[D_PINS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 32, 31, 30, 29, 33, 34, 35, 36, 37, 38, 39, 40, 41, 28}; // pins buttons are connected to
const int buttons[D_PINS] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27}; // ccid for buttons
const int BOUNCE_TIME = 50; // 5 ms is usually sufficient // note: for this midi controller 50 is still responsive enough
const boolean toggled = true;

// digital objects
Bounce digital[] =   {
  Bounce(DIGITAL_PINS[0], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[1], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[2], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[3], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[4], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[5], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[6], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[7], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[8], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[9], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[10], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[11], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[12], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[13], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[14], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[15], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[16], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[17], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[18], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[19], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[20], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[21], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[22], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[23], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[24], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[25], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[26], BOUNCE_TIME),
};

unsigned long previousMillis = 0;
unsigned long previousTime = 0;

void setup() {                                                                          // setup
  Serial.begin(115200);

  // set all led states to off
  for (int i = 0; i <= numberOfLeds - 1; i++) {
    if (i == 0 || i == 1 || i == 9 || i == 8 || i == 14 || i == 13 || i == 15 || i == 16) {
      ledStates[i] = true;
    }
    else {
      ledStates[i] = false;
    }
  }
  pixels.begin();
  pixels.setBrightness(ledBrightness);
  //ledOff();
  ledWipe();
  // pullup button pins
  
  for (int i = 0; i < D_PINS; i++) {
    pinMode(DIGITAL_PINS[i], INPUT_PULLUP);
  }

  pinMode(A, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(C, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);

  sendLed();
}

void loop() {                                                                           // loop
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= (interval / 2)) {
    previousMillis = currentMillis;
    usbMIDI.sendControlChange(127, 127, channel);
    ledOff();
    sendLed();
  }

  Brightness();
  getAnalogData();
  getDigitalData();
  //debug();

  if (usbMIDI.read()) {
    processMIDI();
  }
}

void getAnalogData() {
  for (int i = 0; i < A_PINS; i ++) {
    data[i] = dataLag[i];
  }

  for (int i = 0; i < 8; i++) {
    digitalWrite(A, HIGH && (i & B00000001));
    digitalWrite(B, HIGH && (i & B00000010));
    digitalWrite(C, HIGH && (i & B00000100));
    delayMicroseconds(200);
    data[i] = analogRead(A0) >> 3;
  }
  for (int i = 0; i < 6; i++) {
    digitalWrite(A, HIGH && (i & B00000001));
    digitalWrite(B, HIGH && (i & B00000010));
    digitalWrite(C, HIGH && (i & B00000100));
    delayMicroseconds(200);
    data[i + 8] = analogRead(A1) >> 3;
  }

  for (int i = 0; i < A_PINS; i++) {
    if ( abs(data[i] - dataLag[i]) > 1) {
      dataLag[i] = data[i];
      if ( data[i] < 2 ) {
        data[i] = 0;
      }
      if ( data[i] > 125 ) {
        data[i] = 127;
      }
      mapVal = (( 1.6 * data[13] ) - .0047 * (data[13] * data[13]));

      if ( i != 13) {
        usbMIDI.sendControlChange(CCID[i], data[i], channel);
      }
      else {
        usbMIDI.sendControlChange(CCID[i], mapVal, channel);
      }
    }
  }
}

void debug() {
  for (int i = 0; i < 13; i++) {
    Serial.print(data[i]);
    Serial.print(",");
  }
  Serial.print(mapVal);
  Serial.println();
}

//************DIGITAL SECTION**************
void getDigitalData() {
  for (int i = 0; i < D_PINS; i++) {
    digital[i].update();
    if (digital[i].fallingEdge()) {
      usbMIDI.sendControlChange(buttons[i], 1, channel);
    }
    // Note Off messages when each button is released
    if (digital[i].risingEdge()) {
      usbMIDI.sendControlChange(buttons[i], 1, channel);
    }
  }
}

//************RGB mapRGB SECTION**************
int mapRGB(int x) {
  return -(x - 255);
}

//************LED BRIGHTNESS///////////
void Brightness() {
  unsigned long currentTime = millis();
  down = digital[21].read();
  up = digital[22].read();

  if (!up) {
    if (currentTime > previousTime + 100) {
      previousTime = currentTime;
      if (ledBrightness < 245) {
        ledBrightness = ledBrightness + 10;
        pixels.setBrightness(ledBrightness);
      }
    }
  }

  if (!down) {
    if (currentTime > previousTime + 100) {
      previousTime = currentTime;
      if (ledBrightness >= 10) {
        ledBrightness = ledBrightness - 10;
        pixels.setBrightness(ledBrightness);
      }
    }
  }

  if (ledBrightness == 10) {
    sendLed();
  }
  pixels.show();
  //Serial.println(ledBrightness);
}

void sendLed() {
  for (int j = 0; j < numberOfLeds; j++) {
    if (ledStates[j] == 0) {
      //leds[j].setRGB(mapRGB(0), mapRGB(0), mapRGB(0)); // leds off
      pixels.setPixelColor(j, pixels.Color(0, 0, 0));
    }
    else {
      if (j == 4 || j == 5 || j == 10 || j == 19 || j == 20 || j == 21) {
        //leds[j].setRGB(mapRGB(255), mapRGB(0), mapRGB(0));       //red
        pixels.setPixelColor(j, pixels.Color(255, 0, 0));
      }

      else if (j == 3 || j == 6 || j == 11 || j == 18) {
        //leds[j].setRGB(mapRGB(255), mapRGB(170), mapRGB(0));     //orange
        pixels.setPixelColor(j, pixels.Color(255, 170, 0));
      }

      else {
        //leds[j].setRGB(mapRGB(55), mapRGB(249), mapRGB(160));    //blue
        pixels.setPixelColor(j, pixels.Color(55, 249, 160));
      }
    }
  }
  //FastLED.show();
  pixels.show();
}

//**************LED BOOTUP********************
void ledOff() {
  // turn leds off for bootup
  for (int i = 0; i < numberOfLeds; i++) {
    //leds[i] = CRGB(255, 255, 255);
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  //FastLED.show();
  pixels.show();
}

void ledWipe() {
  ledOff();
  for (int i = 0; i < numberOfLeds; i++) {
    pixels.setPixelColor(ledOrder[i] - 1, pixels.Color(100, 0, 50));
    //FastLED.show();
    pixels.show();
    delay(5);
    for (int i = 0; i < numberOfLeds; i++) {
      pixels.setPixelColor(ledOrder[i] - 1, pixels.Color(0, 0, 0));
      //FastLED.show();
      pixels.show();
      //delay(20);
    }
  }
  for (int i = numberOfLeds; i > 1; i--) {
    pixels.setPixelColor(ledOrder[i] - 1, pixels.Color(100, 0, 50));
    //FastLED.show();
    pixels.show();
    delay(5);
    for (int i = numberOfLeds; i > 0; i--) {
      pixels.setPixelColor(ledOrder[i] - 1, pixels.Color(0, 0, 0));
      //FastLED.show();
      pixels.show();
      //delay(20);
    }
  }
  delay(100);
  sendLed();
}

//void nextLed() {
//  pixels.clear();
//
//  for (int i = 0; i <= 4; i++) {
//    pixels.setPixelColor(ledOrder[i]-1, pixels.Color(157,16,222));
//  }
//  pixels.show();
//  delay(30);
//  pixels.clear();
//  for (int i = 5; i <= 9; i++) {
//    pixels.setPixelColor(ledOrder[i]-1, pixels.Color(157,16,222));
//  }
//  pixels.show();
//  delay(30);
//  pixels.clear();
//  for (int i = 10; i <= 14; i++) {
//    pixels.setPixelColor(ledOrder[i]-1, pixels.Color(157,16,222));
//  }
//  pixels.show();
//  delay(30);
//  pixels.clear();
//  for (int i = 15; i <= 19; i++) {
//    pixels.setPixelColor(ledOrder[i]-1, pixels.Color(157,16,222));
//  }
//  pixels.show();
//  delay(30);
//  pixels.clear();
//
//
//  delay(100);
//  okToSendLed = true;
//}

//************LED INPUT DATA SECTION***************
void processMIDI(void) {
  byte type, channel, data1, data2, cable;

  // fetch the MIDI message, defined by these 5 numbers (except SysEX)
  //
  type = usbMIDI.getType();       // which MIDI message, 128-255
  channel = usbMIDI.getChannel(); // which MIDI channel, 1-16
  data1 = usbMIDI.getData1();     // first data byte of message, 0-127
  data2 = usbMIDI.getData2();     // second data byte of message, 0-127
  cable = usbMIDI.getCable();     // which virtual cable with MIDIx8, 0-7
  ledStates[data1 - 1] = !ledStates[data1 - 1];   // change led states from macro midi
  sendLed();  // call led function
}
