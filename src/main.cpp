#include "KeyScan.h"
#include <Arduino.h>

void setup() {
    // put your setup code here, to run once:
    Serial.begin(921600); 
    keyscan_begin();
}

void loop() {
    // put your main code here, to run repeatedly:
    keyscan_loop();
}