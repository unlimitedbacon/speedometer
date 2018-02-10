// Speedometer
// ==========
//
// TODO:
// * Acceleration
// * Other units
// * Non-volotile Odometer
// * MPG

#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "mirror.h"

// Digital pins
const int pulserPin = 4;
const int modePin = 5;
const int mirrorPin = 8;
const int odoStepPin = 6;
const int odoDirPin = 7;
const int odoEnPin = 2;
// Analog pins
const int brightnessPin = 0;

// Dynamic Brightness Settings
const int maxBrightness = 600;
const int minBrightness = 100;
const unsigned long CHECKINTERVAL = 500000;

int speed = 0;					// Speed in 1/10ths of an MPH
//int odoCount = 0;
bool pulserState = LOW;
bool lastPulserState = LOW;
//bool odoStepState = LOW;
bool odoEnState = HIGH;
unsigned long pulseTimes[4] = {0,0,0,0};	// Duration of last 4 pulses in microseconds
unsigned long oldTime = 0;
unsigned long newTime = 0;
unsigned long odometer = 0;			// Distance traveled since startup
unsigned long miles = 0;

int mode = 0;					// 0: Speed, 1: Odometer
int brightness = 0x0F;
unsigned long lastCheckTime = 0;
bool mirror = false;

Mirror_AlphaNum4 display = Mirror_AlphaNum4();

void setup() {
	// Setup IO
	Serial.begin(115200);
	pinMode(pulserPin, INPUT);
	pinMode(modePin, INPUT_PULLUP);
	pinMode(mirrorPin, INPUT_PULLUP);
  pinMode(odoStepPin, OUTPUT);
  pinMode(odoDirPin, OUTPUT);
  pinMode(odoEnPin, OUTPUT);
  digitalWrite(odoEnPin, odoEnState);
  digitalWrite(odoDirPin, LOW);
  digitalWrite(odoStepPin, LOW);
	pulserState = digitalRead(pulserPin);
	lastPulserState = pulserState;
	mode = !digitalRead(modePin);
	mirror = digitalRead(mirrorPin);

	// Initialize display
	display.begin(0x70);			// I2C Address

	// Play an animation
	display.playAnim();
	display.clear();
	display.writeDisplay();

	// Display speed unit
	display.writeDigitMirror(2, 'm', false);
	display.writeDigitMirror(1, 'p', false);
	display.writeDigitMirror(0, 'h', false);
	display.writeDisplay();
	delay(500);

	updateScreen();

	Serial.println("I am a speedometer");
}

void loop() {
	pulserState = digitalRead(pulserPin);
	newTime = micros();

  // Rotate odometer motor
  if (odoEnState == HIGH) {
    odoEnState = LOW;
    digitalWrite(odoEnPin, odoEnState);
  }
  digitalWrite(odoStepPin,pulserState);
  
	// Detect rising edge of signal
	// and calculate speed
	if (pulserState == HIGH and lastPulserState == LOW) {
		// 4 pulses / revolution
		// 1026 revolutions / mile
		// = 4104 pulses / mile
		//
		// At 140 mph
		// 574560 pulses / hour
		// 159.6 pulses / second
		// 6.27 ms
    //
    // At 100 mph
    // 410400 pulses / hour
    // 114 pulses / second
    // 8.772 ms
		pulseTimes[3] = pulseTimes[2];
		pulseTimes[2] = pulseTimes[1];
		pulseTimes[1] = pulseTimes[0];
		pulseTimes[0] = newTime - oldTime;
		oldTime = newTime;
		//speed = 8771930 / pulseTimes[0];
		// Average over 4 pulses (1 rotation of VSS gear)
		speed = 35087719 / sum(pulseTimes);
		odometer++;
		miles = odometer*10 / 4104;
    // Step the odometer motor
    // 513 motor steps / revolution
    // 1026 pulses / mile
    // 8 pulses / motor step
    // HOLY SHIT THATS LUCKY
    //odoCount++;
    //if (odoCount >= 4) {
    //  odoCount = 0;
    //  odoStepState = !odoStepState;
    //  digitalWrite(odoStepPin, odoStepState);
    //}
		updateScreen();
   
	// If no new pulses for a while,
	// assume we are at 0 mph
	} else if (newTime - oldTime > 4385965) {
		// 0.2 mph
		// 4.385965 seconds
		speed = 0;
    odoEnState = HIGH;
    digitalWrite(odoEnPin, odoEnState);
		updateScreen();
	}
 
	lastPulserState = pulserState;
  
	// Dynamic brightness adjustment
	if (newTime >= lastCheckTime+CHECKINTERVAL) {
		checkBrightness();
		lastCheckTime = newTime;
	}
 
	// Check for mode switch
	mode = !digitalRead(modePin);
}


// Shows information based on current mode
void updateScreen() {
	switch (mode) {
		// Speed mode
		case 0:
			showNum(speed);
			break;
		// Odometer mode
		case 1:
			showNum(miles);
			break;
	}
}


// Displays a number with one decimal
void showNum(int speed) {
	int digit;
	char charBuffer[4] = {'X','X','X','X'};
	// Get each digit of speed
	for ( int i=0; i<4; i++ ) {
		digit = speed % 10;
		charBuffer[3-i] = char(digit+48);		// Convert to ASCII
		speed /= 10;
	}
	// Chop leading 0s
	if (charBuffer[0] == '0' and charBuffer[1] == '0') {
		charBuffer[0] = ' ';
		charBuffer[1] = ' ';
	} else if (charBuffer[0] == '0') {
		charBuffer[0] = ' ';
	}
	// Display
	if (mirror) {
		display.writeDigitMirror(3, charBuffer[0], false);
		display.writeDigitMirror(2, charBuffer[1], false);
		display.writeDigitMirror(1, charBuffer[2], false);	// Put decimal here
		display.writeDigitMirror(0, charBuffer[3], true);
	} else {
		display.writeDigitAscii(0, charBuffer[0], false);
		display.writeDigitAscii(1, charBuffer[1], false);
		display.writeDigitAscii(2, charBuffer[2], true);	// Put decimal here
		display.writeDigitAscii(3, charBuffer[3], false);
	}
	display.writeDisplay();
}


void checkBrightness() {
	brightness = analogRead(brightnessPin);
	//Serial.print(brightness);
	//Serial.print(' ');
	if (brightness > minBrightness) {
		brightness = map( brightness, minBrightness, maxBrightness, 0, 16 );
	} else {
		brightness = 0;
	}
	//Serial.println(brightness);
	display.setBrightness(brightness);
}

unsigned long sum(unsigned long list[4]) {
	return list[0]+list[1]+list[2]+list[3];
}
