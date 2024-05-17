#include "KeyScan.h"
#include <Arduino.h>

// ���尴��������
#define NUM_KEYS 4
const int keyPins[NUM_KEYS] = {32, 33, 25, 26}; // �����������

// ���尴��״̬
enum KeyState { 
    KEY_STATE_PRESS = 0, 
    KEY_STATE_HOLD,
    KEY_STATE_RELEASE 
};

enum DebounceState { 
    DEBOUNCE_IDLE, 
    DEBOUNCE_BEGIN 
};

// ���尴���ṹ��
struct Key {
    int pin;
    KeyState state;
    uint8_t value;
    DebounceState debouneState;
    unsigned long lastDebounceTime;
};

// ������������
static Key keys[NUM_KEYS];

// ���尴������ʱ��
#define DEBOUNCE_TIME 50

void debounceKey(Key &key);
void checkKeyState(Key &key);

// ��ʼ������������
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
    // ��ȡ������ƽ
    uint8_t keyCurrentState = digitalRead(key.pin);

    switch (key.debouneState) {
        case DEBOUNCE_IDLE:
            if (keyCurrentState != key.value) { // �����ǰ״̬����һ��״̬��ͬ
                key.lastDebounceTime = millis(); // ����������ʱ��
                key.debouneState = DEBOUNCE_BEGIN;
            }
            break;

        case DEBOUNCE_BEGIN:                              
            if (keyCurrentState == key.value) { 
                key.debouneState = DEBOUNCE_IDLE;   // �����ͣ�������ص�DEBOUNCE_IDLE������ȷ�ϰ���״̬�Ƿ����仯
            } else if ((millis() - key.lastDebounceTime) > DEBOUNCE_TIME) { // ����������㹻��ʱ�䣬ȷ�ϰ����ȶ�
                key.value = keyCurrentState; // ���°�����ֵ
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
                key.state = KEY_STATE_HOLD; // �����������¼�
                Serial.print("Key ");
                Serial.print(key.pin);
                Serial.println(" Pressed");
            }
            break;

        case KEY_STATE_PRESS:
            if (key.value == HIGH) {
                key.state = KEY_STATE_RELEASE; // �������ɿ��¼�
                Serial.print("Key ");
                Serial.print(key.pin);
                Serial.println(" Released");
            }
            break;

        default:
            // ����״̬����
            break;
    }
}