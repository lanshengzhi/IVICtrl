#include "KeyScan.h"
#include <Arduino.h>

// 定义按键的引脚
#define NUM_KEYS 4
const int keyPins[NUM_KEYS] = {32, 33, 25, 26}; // 按键引脚序号

// 定义按键状态
enum KeyState { 
    KEY_STATE_PRESS = 0, 
    KEY_STATE_HOLD,
    KEY_STATE_RELEASE 
};

enum DebounceState { 
    DEBOUNCE_IDLE, 
    DEBOUNCE_BEGIN 
};

// 定义按键结构体
struct Key {
    int pin;
    KeyState state;
    uint8_t value;
    DebounceState debouneState;
    unsigned long lastDebounceTime;
};

// 创建按键数组
static Key keys[NUM_KEYS];

// 定义按键消抖时间
#define DEBOUNCE_TIME 50

void debounceKey(Key &key);
void checkKeyState(Key &key);

// 初始化按键的引脚
void keyscanBegin() {
    for (int i = 0; i < NUM_KEYS; ++i) {
        pinMode(keyPins[i], INPUT_PULLUP);
        keys[i].pin = keyPins[i];
        keys[i].state = KEY_STATE_RELEASE;
        keys[i].value = HIGH;
        keys[i].debouneState = DEBOUNCE_IDLE;
        keys[i].lastDebounceTime = 0;
    }
}

void keyscanLoop() {
    for (int i = 0; i < NUM_KEYS; ++i) {
        debounceKey(keys[i]);
        checkKeyState(keys[i]);
    }
}

void debounceKey(Key &key) {
    // 读取按键电平
    uint8_t keyCurrentState = digitalRead(key.pin);

    switch (key.debouneState) {
        case DEBOUNCE_IDLE:
            if (keyCurrentState != key.value) { // 如果当前状态和上一次状态不同
                key.lastDebounceTime = millis(); // 重置消抖计时器
                key.debouneState = DEBOUNCE_BEGIN;
            }
            break;

        case DEBOUNCE_BEGIN:                              
            if (keyCurrentState == key.value) { 
                key.debouneState = DEBOUNCE_IDLE;   // 如果不停抖动，回到DEBOUNCE_IDLE，重新确认按键状态是否发生变化
            } else if ((millis() - key.lastDebounceTime) > DEBOUNCE_TIME) { // 如果经过了足够的时间，确认按键稳定
                key.value = keyCurrentState; // 更新按键的值
                key.debouneState = DEBOUNCE_IDLE;
            }
            break;
            
        default:
            break;
    }
}

void checkKeyState(Key &key) {
    switch (key.state) {
        case KEY_STATE_RELEASE:
            if (key.value == LOW) {
                key.state = KEY_STATE_HOLD; // 处理按键按下事件
                Serial.print("Key ");
                Serial.print(key.pin);
                Serial.println(" Pressed");
            }
            break;

        case KEY_STATE_PRESS:
            if (key.value == HIGH) {
                key.state = KEY_STATE_RELEASE; // 处理按键松开事件
                Serial.print("Key ");
                Serial.print(key.pin);
                Serial.println(" Released");
            }
            break;

        default:
            // 其他状态处理
            break;
    }
}