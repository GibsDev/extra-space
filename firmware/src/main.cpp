#include <Arduino.h>
#include <avr/pgmspace.h>
#include <Adafruit_NeoPixel.h>

#include <EEPROM.h>
#include <HID-Project.h>

#include "keycodes.h"

#define NUMBER_OF_BUTTONS 4

// EEPROM base address
int BIND_ADDR = 0;

// Pins for buttons
int PINS[] = { 9, 8, 7, 6 };
// Previous state of buttons from last loop
int keys_last[] = { 0, 0, 0, 0 };
unsigned long keys_last_time[] = { 0, 0, 0, 0 };
// The current state of buttons 1 for pressed
int keys[] = { 0, 0, 0, 0 };
// Current binds
KeyboardKeycode binds[] = {
	KEY_RESERVED,
	KEY_RESERVED,
	KEY_RESERVED,
	KEY_RESERVED
};
// Default binds
const KeyboardKeycode DEFAULT_BINDS[] = {
	KEY_MINUS,
	KEY_EQUAL,
	KEY_LEFT_BRACE,
	KEY_RIGHT_BRACE
};

uint32_t BUTTON_COLOR = 0xff0000;
uint32_t BUTTON_DOWN_COLOR = 0xFFFFFF;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(4, 10, NEO_RGB + NEO_KHZ800);

const int CMD_SIZE = 32;
char* commandBuffer;
int cmd_length = 0;

char* keynameBuffer;

unsigned long loopstart = 0;
unsigned long debounce = 5;

void updateBinds();
void printKeys ();
void onKeyPressed(int);
void onKeyReleased(int);
void onCommand(char*);
void onGet(int);
void onSet(int, char*);
void onReset();
void onList();
void printBind(int key_index);

void setup() {
	// LEDS
	strip.begin();
	strip.show();
	strip.setBrightness(1);
	strip.setPixelColor(0, BUTTON_COLOR);
	strip.setPixelColor(1, BUTTON_COLOR);
	strip.setPixelColor(2, BUTTON_COLOR);
	strip.setPixelColor(3, BUTTON_COLOR);
	strip.show();
	// Disable constant TX LED while running
	pinMode(LED_BUILTIN_TX,INPUT);
	pinMode(LED_BUILTIN_RX,INPUT);
	// Keyboard stuff
	Serial.begin(9600);
	NKROKeyboard.begin();
	updateBinds();
	for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
		pinMode( PINS[i], INPUT_PULLUP);
	}
	commandBuffer = (char*) malloc(CMD_SIZE * sizeof(char));
	commandBuffer[0] = '\0';
	keynameBuffer = (char*) malloc(32 * sizeof(char));
	keynameBuffer[0] = '\0';
}

void updateBinds() {
	NKROKeyboard.releaseAll();
	for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
		int bind = EEPROM.read(BIND_ADDR + i);
		if (bind == 255) {
			binds[i] = DEFAULT_BINDS[i];
		} else {
			binds[i] = (KeyboardKeycode) EEPROM.read(BIND_ADDR + i);
		}
	}
}


char buffer[32];

void loop() {
	if (millis() % 1000 == 0) {
		// DEBUG
	}
	loopstart = millis();
	// Process serial data
	while (Serial.available() > 0) {
		char next = Serial.read();
		if(next == '\n') {
			onCommand(commandBuffer);
			commandBuffer[0] = '\0';
			cmd_length = 0;
		} else if(cmd_length < CMD_SIZE) {
			commandBuffer[cmd_length] = next;
			commandBuffer[cmd_length + 1] = '\0';
			cmd_length++;
		}
	}
	// Update button state
	for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
		keys[i] = !digitalRead(PINS[i]);
		if (keys_last[i] != keys[i] && loopstart - keys_last_time[i] >= debounce) {
			if (keys[i]) {
				onKeyPressed(i);
			} else {
				onKeyReleased(i);
			}
			keys_last[i] = keys[i];
			keys_last_time[i] = loopstart;
		}
	}
	delay(1);
}

void printKeys () {
	String output = "";
	for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
		output += i;
		output += ": ";
		output += keys[i];
		output += ", ";
	}
	Serial.println(output);
}

void onKeyPressed(int key_index) {
	NKROKeyboard.press(binds[key_index]);
	strip.setBrightness(1);
	strip.setPixelColor(key_index, BUTTON_DOWN_COLOR);
	strip.show();
}

void onKeyReleased(int key_index) {
	NKROKeyboard.release(binds[key_index]);
	strip.setBrightness(1);
	strip.setPixelColor(key_index, BUTTON_COLOR);
	strip.show();
}

// Commands:
// SET <index(1-4)> KEYNAME
// GET <index(1-4)>
// RESET
// LIST
void onCommand(char* command) {
	int len = 0;
	char** split = (char**) malloc(sizeof(char*) * CMD_SIZE);
	if (!split) {
		Serial.print("No more memory...");
	}
	char* token = strtok(command, " ");
	while (token != NULL && len < (CMD_SIZE / 2) - 1) {
		split[len] = token;
		len++;
		token = strtok(NULL, " ");
	}
	if (!strcmp(split[0], "PING")) {
		Serial.println("PONG");
	}
	if (!strcmp(split[0], "SET")) {
		int key = atoi(split[1]);
		if (key == 0 || key > 4) {
			Serial.println("Invalid key");
			return;
		}
		onSet(key - 1, split[2]);
	} else if (!strcmp(split[0], "GET")) {
		int key = atoi(split[1]);
		if (key == 0 || key > 4) {
			Serial.println("Invalid key");
			return;
		}
		onGet(key - 1);
	} else if (!strcmp(split[0], "RESET")) {
		onReset();
	} else if (!strcmp(split[0], "LIST")) {
		onList();
	}
	free(split);
}

void onGet(int key_index) {
	printBind(key_index);
}

void onSet(int key_index, char* value) {
	Serial.print("Setting ");
	Serial.print(key_index);
	Serial.print(" to ");
	Serial.println(value);
	EEPROM.write(BIND_ADDR + key_index, getKeycodeFromName(value));
	updateBinds();
	printBind(key_index);
}

void onReset() {
	for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
		EEPROM.write(BIND_ADDR + i, DEFAULT_BINDS[i]);
	}
	updateBinds();
	Serial.println("Reset complete");
}

void onList() {
	printKeynames();
}

void printBind(int key_index) {
	Serial.print("Key ");
	Serial.print(key_index + 1);
	Serial.print(" -> (");
	Serial.print(binds[key_index]);
	Serial.print(") ");
	getNameFromKeycode(binds[key_index], keynameBuffer);
	Serial.println(keynameBuffer);
}
