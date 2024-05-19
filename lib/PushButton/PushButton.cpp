#include "PushButton.h"

void PushButton::Begin() {
    pinMode(m_pin, INPUT_PULLUP);
}

void PushButton::SetPressedCallback(void (*callback)()) {
    m_pressedCallback = callback;
}

void PushButton::SetReleasedCallback(void (*callback)()) {
    m_releasedCallback = callback;
}

void PushButton::Run() {
    Debounce();
    Callback();
}

void PushButton::Debounce() {
    uint8_t pinCurrentState = digitalRead(m_pin);   // 读取按键电平

    switch (m_debouneState) {
        case 0:
            if (pinCurrentState != m_pinStableState) { // 如果当前状态和上一次状态不同
                m_lastDebounceTime = millis(); // 重置消抖计时器
                m_debouneState = 1;
            }
            break;

        case 1:                              
            if (pinCurrentState == m_pinStableState) {  // IO 电平与之前的电平又一样了，说明电平发生抖动
                m_debouneState = 0;                     // 重新回到DEBOUNCE_IDLE
            } else if ((millis() - m_lastDebounceTime) > m_debounceTime) {  // 如果电平状态保持时间 > m_debounceTime
                m_pinStableState = pinCurrentState;                         // 更新按键的值
                m_debouneState = 0;
            }
            break;
            
        default:
            break;
    }
}

void PushButton::Callback() {
    if (m_pinPrevStableState == m_pinStableState) {
        return;
    }

    if (m_pinStableState == LOW) {
        if (m_pressedCallback != nullptr) {
            m_pressedCallback();
        }
    } else {
        if (m_releasedCallback != nullptr) {
            m_releasedCallback();
        }
    }

    m_pinPrevStableState = m_pinStableState;
}


