#include <string.h>
#include "keycodes.h"
#include <Arduino.h>
#include <HID-Project.h>

char namebuff[32];

void getNameFromKeycode(byte keycode, char* dest) {
    for (int i = 0; i < 109; i++) {
        KeyboardKeycode code = (KeyboardKeycode) pgm_read_byte(&(KEYNAMES[i].code));
        if (code == keycode) {
            strcpy_P(dest, (char*) pgm_read_word(&(KEYNAMES[i].name)));
            return;
        }
    }
    strcpy(dest, "UNKNOWN");
}

KeyboardKeycode getKeycodeFromName(char* name) {
    for (int i = 0; i < 109; i++) {
        strcpy_P(namebuff, (char*) pgm_read_word(&(KEYNAMES[i].name)));
        if (strcmp(name, namebuff) == 0) {
            KeyboardKeycode code = (KeyboardKeycode) pgm_read_byte(&(KEYNAMES[i].code));
            return code;
        }
    }
    return (KeyboardKeycode) 255;
}

void printKeynames() {
    for (int i = 0; i < 109; i++) {
        strcpy_P(namebuff, (char*) pgm_read_word(&(KEYNAMES[i].name)));
        Serial.println(namebuff);
    }
}
