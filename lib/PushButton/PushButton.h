#ifndef PUSH_BUTTON_h
#define PUSH_BUTTON_h
#include <Arduino.h>

class PushButton {
public:
    PushButton(uint8_t pin) : m_pin(pin) {}
    void Begin();
    uint8_t GetPin() { return m_pin; }
    void SetPressedCallback(void (*callback)());
    void SetReleasedCallback(void (*callback)());
    void Run();
    
private:
    void Debounce();
    void Callback();

private:
    uint8_t m_pin;
    uint16_t m_debounceTime = 50;
    uint16_t m_lastDebounceTime = 0;
    uint8_t m_pinStableState = HIGH;
    uint8_t m_pinPrevStableState = HIGH;
    uint8_t m_debouneState = 0;
    void (*m_pressedCallback)() = nullptr; 
    void (*m_releasedCallback)() = nullptr;
};

#endif