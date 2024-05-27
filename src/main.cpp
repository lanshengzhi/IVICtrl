#include "PushButton.h"
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <SimpleTimer.h>

// WiFI
static const char wifi_ssid[]       = "YOU WIFI SSID";
static const char wifi_password[]   = "YOU WIFI PASSWORD";

// MQTT broker
static const char mqtt_broker_ip[]  = "YOU MQTT Broker's IP address";
const int  mqtt_broker_port         = 1883;
static const char mqtt_broker_username[] = "mqtt_broker_username";
static const char mqtt_broker_password[] = "mqtt_broker_password";

// MQTT topic
static const char topic_publish[]   = "ESP32/data";
static const char topic_subscribe[] = "ESP32/command";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

LiquidCrystal_I2C lcd(0x27, 16, 2);

DHTesp dht;
SimpleTimer timer;

PushButton prevButton(32);
PushButton upButton(33);
PushButton downButton(25);
PushButton nextButton(26);

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

#define MSG_BUFFER_SIZE	(50)
static char publish_msg[MSG_BUFFER_SIZE];

void TimerCallback() {
    TempAndHumidity tempAndHum = dht.getTempAndHumidity();
    Serial.printf("Temperature: %f\n", tempAndHum.temperature);
    Serial.printf("Humidity: %f\n",  tempAndHum.humidity);
    snprintf(publish_msg, MSG_BUFFER_SIZE, "temp: %f, Humidiy: %f", tempAndHum.temperature, tempAndHum.humidity);
    mqttClient.publish(topic_publish, publish_msg);
}

// 
void MQTTCallback(const char* topic, uint8_t* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }

    Serial.println();
}

void setup() {
    Serial.begin(921600); 

    dht.setup(27, DHTesp::DHT11);

    WiFi.begin(wifi_ssid, wifi_password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(2000);
        Serial.println("Connecting to the wifi......");
    }

    Serial.println("WiFI connected");

    mqttClient.setServer(mqtt_broker_ip, mqtt_broker_port);

    while (!mqttClient.connected()) {
        String client_id = "ESP32-TEST-1";
        if (mqttClient.connect(client_id.c_str(), mqtt_broker_username, mqtt_broker_password)) {
            Serial.println("MQTT server connected");
        } else {
            Serial.println("MQTT server connect fail: " + String(mqttClient.state()));
        }
        delay(2000);
    }

    // Subscribe the topic
    mqttClient.subscribe(topic_subscribe);
    mqttClient.setCallback(MQTTCallback);

    lcd.init();
    lcd.backlight();

    prevButton.Begin();
    upButton.Begin();
    downButton.Begin();
    nextButton.Begin();

    prevButton.SetPressedCallback([]() { lcd.clear(); lcd.print("Prev BTN D"); });
    prevButton.SetReleasedCallback([]() { lcd.clear(); lcd.print("Prev BTN U"); });

    upButton.SetPressedCallback(OnUpButtonPressed);
    downButton.SetPressedCallback(OnDownButtonPressed);
    nextButton.SetPressedCallback(OnNextButtonReleased);

    timer.setInterval(2000, TimerCallback);
}

void loop() {
    prevButton.Run();
    upButton.Run();
    downButton.Run();
    nextButton.Run();
    mqttClient.loop();
    timer.run();
}