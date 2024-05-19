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
    uint8_t pinCurrentState = digitalRead(m_pin);   // ��ȡ������ƽ

    switch (m_debouneState) {
        case 0:
            if (pinCurrentState != m_pinStableState) { // �����ǰ״̬����һ��״̬��ͬ
                m_lastDebounceTime = millis(); // ����������ʱ��
                m_debouneState = 1;
            }
            break;

        case 1:                              
            if (pinCurrentState == m_pinStableState) {  // IO ��ƽ��֮ǰ�ĵ�ƽ��һ���ˣ�˵����ƽ��������
                m_debouneState = 0;                     // ���»ص�DEBOUNCE_IDLE
            } else if ((millis() - m_lastDebounceTime) > m_debounceTime) {  // �����ƽ״̬����ʱ�� > m_debounceTime
                m_pinStableState = pinCurrentState;                         // ���°�����ֵ
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


