/*
*	SMART FlowerPot
*	smartControl
*	__________________
*
*	Code by Vladislav Yasnetski'
*/

/* ----- Libraries ----- */
#include <Wire.h>
#include <DS3231.h>
#include <ArduinoJson.h>

/* ---- Pins ----- */
#define VCC_MOISTURE	A4
#define GND_MOISTURE	A5
#define MOISTURE		A7

#define PUMP			9

#define VCC_CLOCK		19
#define GND_CLOCK		18

#define WiFi			Serial3

/* ----- Setup ----- */
#define pumpPower		20
#define waterDelay		3000

/* ----- Sensors ----- */
DS3231 clock;
RTCDateTime dt;

/* ----- Sysytem vars ----- */
boolean canTransmitt = false;
boolean watered = false;
unsigned long WiFitimer = 0;
int Mode = 0;
int Hou = 0;
int Min = 0;
int MoistureSetup = 100;
unsigned long ActionTimer = 0;

void setup()
{
	Serial.begin(9600);
	WiFi.begin(115200);

	PinsInit();
	delay(1000);

	clock.begin();
}

void loop()
{
	TWaction();
	updateFlowerPot();
}

void PinsInit() {
	pinMode(VCC_CLOCK, OUTPUT);
	pinMode(GND_CLOCK, OUTPUT);
	pinMode(VCC_MOISTURE, OUTPUT);
	pinMode(GND_MOISTURE, OUTPUT);
	pinMode(PUMP, OUTPUT);

	digitalWrite(VCC_CLOCK, 1);
	digitalWrite(GND_CLOCK, 0);
	digitalWrite(VCC_MOISTURE, 1);
	digitalWrite(GND_MOISTURE, 0);
}

/* -------- Enable pump, Checking Timers  -------- */
void updateFlowerPot() {
	if (millis() - ActionTimer >= 20) {
		if (Mode != 1) {
			if (map(analogRead(MOISTURE), 0, 1024, 0, 100) > MoistureSetup) {
				analogWrite(PUMP, pumpPower);
			}
			else {
				analogWrite(PUMP, 0);
			}
		}
		else {
			dt = clock.getDateTime();
			if (!watered && dt.hour == Hou && dt.minute == Min) {
				watered = true;
				analogWrite(PUMP, pumpPower);
				delay(waterDelay);
				analogWrite(PUMP, 0);
			}
			else
				analogWrite(PUMP, 0);
			if (dt.hour != Hou || dt.minute != Min)
				watered = false;
		}

		ActionTimer = millis();
	}
}

/* -------- Data exchange with Thingworx Server  -------- */
void TWaction() {
	if (canTransmitt && millis() - WiFitimer >= 5000) {
		dt = clock.getDateTime();								//Read time

		WiFi.print("Moisture=");
		WiFi.print(map(analogRead(MOISTURE), 0, 1024, 0, 100));
		WiFi.print("&Hours=");
		WiFi.print(dt.hour);
		WiFi.print("&Minutes=");
		WiFi.print(dt.minute);

		Serial.println("Sended!");
		canTransmitt = false;
		WiFitimer = millis();
	}

	String message = "";
	if (WiFi.available()) {						//Waiting for '>' symbol
		message = WiFi.readStringUntil('\n');

		if (message.startsWith(">"))
			canTransmitt = true;

		else if (message.startsWith("{")) {
			Serial.print("Recieved JSON: ");
			Serial.println(message);

			StaticJsonBuffer<600> jsonBuffer;
			JsonObject& root = jsonBuffer.parseObject(message);

			if (!root.success())
				Serial.println("JSON paring failed");
			else {
				MoistureSetup = (float)root["e"];
				Hou = (float)root["h"];
				Min = (float)root["m"];
				Mode = (float)root["d"];

				Serial.println("Parsed from JSON: ");
				Serial.print("m: ");
				Serial.print(MoistureSetup);
				Serial.print(" hou: ");
				Serial.print(Hou);
				Serial.print(" min: ");
				Serial.print(Min);
				Serial.print(" mode: ");
				Serial.println(Mode);
			}
		}
		else
			Serial.println(message);
	}
}