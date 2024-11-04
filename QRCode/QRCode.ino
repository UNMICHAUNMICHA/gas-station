#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

const char *ssid = "NSSS";
const char *password = "Numchai1.";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

LiquidCrystal_I2C lcd1(0x27, 16, 2);
LiquidCrystal_I2C lcd2(0x26, 16, 2);
LiquidCrystal_I2C lcd3(0x25, 16, 2);
LiquidCrystal_I2C lcd4(0x24, 16, 2);
LiquidCrystal_I2C lcd5(0x23, 16, 2);

#define SwitchPin D5
#define PumpPin   D6
#define ON HIGH
#define OFF LOW

float receivedLiters = 0;
float totalOilInTank = 200;  
String fuelName = "";
String unit = "";
float amount = 0;
bool isWaitingForButton = false; 

void setup() {
    Serial.begin(115200);

    lcd1.begin();
    lcd2.begin();
    lcd3.begin();
    lcd4.begin();
    lcd5.begin();

    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 30000) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi");
        return; 
    }

    timeClient.begin();
    pinMode(SwitchPin, INPUT_PULLUP);
    pinMode(PumpPin, OUTPUT);
    digitalWrite(PumpPin, OFF);
}

void loop() {
    timeClient.update(); 
    if (!isWaitingForButton) {
        fetchData();
    }
    LCD2();
    LCD3();
    LCD4();
    LCD5();

    int SwitchValue = digitalRead(SwitchPin);
    if (SwitchValue == LOW && receivedLiters > 0) {
        digitalWrite(PumpPin, ON);
        float currentLiters = receivedLiters; 

        unsigned long fillTime = (unsigned long)(receivedLiters * 28 * 1000); 

        unsigned long startTime = millis();
        while (millis() - startTime < fillTime) {
            LCD4();
            currentLiters = receivedLiters - ((millis() - startTime) / 1000.0) / 28.0;
            currentLiters = max(currentLiters, 0.0);
            
            // แสดงค่าใน Serial Monitor
            Serial.print("Current Liters: ");
            Serial.println(currentLiters);

            displayLitersIncrementally(currentLiters);
        }

        digitalWrite(PumpPin, OFF);
        totalOilInTank -= receivedLiters;  
        receivedLiters = 0; 
        isWaitingForButton = false; 
    } else if (receivedLiters > 0) {
        isWaitingForButton = true; 
    }
    LCD1();
    delay(2000);
}

void displayLitersIncrementally(float currentLiters) {
    lcd4.clear(); 
    lcd4.setCursor(0, 0);
    lcd4.print("Filled: ");
    lcd4.setCursor(8, 0); 
    lcd4.print(currentLiters, 2); 
    lcd4.print(" L     ");
    delay(1000);
}

void fetchData() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client; 
        client.setInsecure(); 
        HTTPClient http;
        http.begin(client, "https://webpoo.ncwjjdx.work/data"); 
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.GET(); 
        if (httpResponseCode > 0) {
            String response = http.getString(); 
            Serial.println(httpResponseCode);
            Serial.println(response);
            
            DynamicJsonDocument doc(1024);
            auto error = deserializeJson(doc, response);
            if (!error) {
                fuelName = doc["fuel_name"].isNull() ? "" : doc["fuel_name"].as<String>();
                unit = doc["unit"].isNull() ? "" : doc["unit"].as<String>();
                receivedLiters = doc["liters"].isNull() ? 0 : doc["liters"];
                amount = doc["amount"].isNull() ? 0 : doc["amount"];

                // แสดงค่าที่ได้รับใน Serial Monitor
                Serial.print("Fuel Name: "); Serial.println(fuelName);
                Serial.print("Unit: "); Serial.println(unit);
                Serial.print("Received Liters: "); Serial.println(receivedLiters);
                Serial.print("Amount: "); Serial.println(amount);
            } else {
                Serial.println("JSON deserialization failed");
            }
        } else {
            Serial.print("Error on sending GET: ");
            Serial.println(httpResponseCode);
        }

        http.end(); 
    } else {
        Serial.println("WiFi not connected");
    }
}

void LCD1() {
    lcd1.clear();
    lcd1.setCursor(0, 0);
    
    time_t epochTime = timeClient.getEpochTime() + 7 * 3600; 
    struct tm *timeInfo = localtime(&epochTime);
    
    lcd1.print("Time: ");
    lcd1.print(timeInfo->tm_hour < 10 ? "0" : ""); 
    lcd1.print(timeInfo->tm_hour);
    lcd1.print(":");
    lcd1.print(timeInfo->tm_min < 10 ? "0" : ""); 
    lcd1.print(timeInfo->tm_min);
    lcd1.print(":");
    lcd1.print(timeInfo->tm_sec < 10 ? "0" : ""); 
    lcd1.print(timeInfo->tm_sec);
    
    lcd1.setCursor(0, 1);
    lcd1.print("Date: ");
    lcd1.print(timeInfo->tm_mday);
    lcd1.print("/");
    lcd1.print(timeInfo->tm_mon + 1); 
    lcd1.print("/");
    lcd1.print(timeInfo->tm_year + 1900); 
}

void LCD2() {
    lcd2.clear();
    lcd2.setCursor(0, 0);
    lcd2.print("Fuel: ");
    lcd2.print(fuelName);
    lcd2.setCursor(0, 1);
    lcd2.print("Unit: ");
    lcd2.print(unit);
}

void LCD3() {
    lcd3.clear();
    lcd3.setCursor(0, 0);
    lcd3.print("Liters: ");
    lcd3.print(receivedLiters);
    lcd3.setCursor(0, 1);
    lcd3.print("Amount: ");
    lcd3.print(amount);
}

void LCD4() {
    lcd4.clear();
    lcd4.setCursor(0, 0);
    lcd4.print("Filled: ");
    lcd4.setCursor(15, 0);
}

void LCD5() {
    lcd5.clear();
    lcd5.setCursor(3, 0);
    lcd5.print("Oil in Tank");
    lcd5.setCursor(5, 1);
    lcd5.print(totalOilInTank);
    lcd5.print(" L");
}
