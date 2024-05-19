#include "PushButton.h"
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

PushButton prevButton(32);
PushButton upButton(33);
PushButton downButton(25);
PushButton nextButton(26);

void OnPrevButtonPressed() {
    Serial.println("OnPrevButtonPressed");
}

void OnPrevButtonReleased() {
    Serial.println("OnPrevButtonReleased");
}

void OnUpButtonPressed() {
    Serial.println("OnUpButtonPressed");
}

void OnDownButtonPressed() {
    Serial.println("OnDownButtonPressed");
}

void OnNextButtonPressed() {
    Serial.println("OnNextButtonPressed");
}

void OnNextButtonReleased() {
    Serial.println("OnNextButtonReleased");
}

void setup() {
    Serial.begin(921600); 

    lcd.init();
    lcd.backlight();

    prevButton.Begin();
    upButton.Begin();
    downButton.Begin();
    nextButton.Begin();

    //prevButton.SetPressedCallback(OnPrevButtonPressed);
    //prevButton.SetReleasedCallback(OnPrevButtonReleased);

    prevButton.SetPressedCallback([]() { lcd.clear(); lcd.print("Prev BTN D"); });
    prevButton.SetReleasedCallback([]() { lcd.clear(); lcd.print("Prev BTN U"); });

    upButton.SetPressedCallback(OnUpButtonPressed);
    downButton.SetPressedCallback(OnDownButtonPressed);
    nextButton.SetPressedCallback(OnNextButtonReleased);
}

void loop() {
    prevButton.Run();
    upButton.Run();
    downButton.Run();
    nextButton.Run();
}