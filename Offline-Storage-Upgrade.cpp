#include "LSM6DS3.h"
#include "Wire.h"
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h> 

using namespace Adafruit_LittleFS_Namespace;

bool DEBUG_MODE = false; 

LSM6DS3 myIMU(I2C_MODE, 0x6A);

BLEService padelService(0x181A);
BLECharacteristic dataChar(0x2A6E); 
BLECharacteristic cmdChar(0x2A6F);

BLEBas blebas; // Standardne Bluetoothi akuteenus

#ifndef PIN_VBAT
#define PIN_VBAT 32 // XIAO aku mõõtmise viik
#endif

unsigned long lastBatteryCheck = 0;

// Funktsioon, mis arvutab pingest protsendi (XIAO plaadi loogika)
uint8_t readBatteryLevel() {
  // 1. Loeme toore analoogväärtuse (0-1023)
  int adcCount = analogRead(PIN_VBAT);
  
  // 2. Arvutame pinge kiibi viigul (nRF52 kasutab 3.6V viidet)
  float pinVoltage = adcCount * (3.6 / 1024.0); 
  
  // 3. KORRIGEERIMINE: Pingejaguri matemaatika (1M ja 510k takistid).
  // Korrutame pinge tagasi üles, et saada tegelik aku pinge!
  float voltage = pinVoltage * (1510.0 / 510.0); 
  
  int percent = 0;
  
  // Tõeline Li-Po tühjenemiskõver
  if (voltage >= 4.20) {
    percent = 100;
  } else if (voltage >= 4.00) {
    percent = 80 + (voltage - 4.00) * 100; 
  } else if (voltage >= 3.80) {
    percent = 40 + (voltage - 3.80) * 200; 
  } else if (voltage >= 3.70) {
    percent = 20 + (voltage - 3.70) * 200; 
  } else if (voltage > 3.30) {
    percent = (voltage - 3.30) * 50;       
  } else {
    percent = 0;                           
  }

  // Turvalisuse piirid
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  
  return percent;
}

// === KALIBREERIMINE ===
const float IMPACT_THRESHOLD = 4.5; 
const float REKETI_PIKKUS = 0.35;    
const float ROLL_THRESHOLD = 75.0;  
const float MINspeedKMH = 6.0;
const float MAX_TAP_FORCE = 8.0;

unsigned long lastTapTime = 0;
int tapCount = 0;

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, HIGH);

  InternalFS.begin();
  Serial.println("Internal Flash Memory Initialized.");
  //parameetrid güroskoobi ja kiirendusanduri jaoks
  myIMU.settings.accelEnabled = 1;
  myIMU.settings.accelRange = 16; 
  myIMU.settings.accelSampleRate = 416;
  myIMU.settings.gyroEnabled = 1;
  myIMU.settings.gyroRange = 2000;  
  myIMU.settings.gyroSampleRate = 416;

  if (myIMU.begin() != 0) {
    Serial.println("IMU Error!");
    while(1);
  }
  
  Bluefruit.begin();
  Bluefruit.autoConnLed(false); 
  Bluefruit.setName("Smart Padel Sensor"); //Ühendamisel tuvastatav nimi
  padelService.begin();
  
  dataChar.setProperties(CHR_PROPS_NOTIFY);
  dataChar.begin();

  // Bulletproof Write characteristic
  cmdChar.setProperties(CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  cmdChar.setWriteCallback(onCommandReceived); 
  cmdChar.begin();

  // AKU TEENUSE KÄIVITAMINE
  blebas.begin();
  blebas.write(readBatteryLevel()); // Saadame kohe algse aku taseme

  Bluefruit.Advertising.addService(padelService);
  Bluefruit.Advertising.start(0);

  Serial.println("System Ready! Waiting for hits...");
}

// --- UPGRADED SYNC FUNCTION ---
void onCommandReceived(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (data && len > 0 && data[0] == 'S') { 
    Serial.println("\n--- SYNC REQUESTED ---");
    
    File file = InternalFS.open("/shots.csv", FILE_O_READ); 
    if (file) file.seek(0); // Kerib lugemisjärje kindluse mõttes faili algusesse!

    if (file) {
      Serial.println("File found. Sending data to Web App...");
      
      while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim(); // CRITICAL: Eemaldab nähtamatud vead, mis graafikuid lõhuvad!
        
        if (line.length() > 2) {
          Serial.print("Sending: "); Serial.println(line);
          dataChar.notify(line.c_str(), line.length());
          delay(50); // Paus, et Bluetooth puhver üle ei kuumeneks
        }
      }
      file.close();
      Serial.println("Sync Complete! Wiping memory for next game.");
      InternalFS.remove("/shots.csv"); 
      
    } else {
      Serial.println("Sync Failed: No saved data found in memory.");
    }
    Serial.println("----------------------\n");
  }
}

void loop() {
  float ax = myIMU.readFloatAccelX();
  float ay = myIMU.readFloatAccelY();
  float az = myIMU.readFloatAccelZ();
  float totalAcc = sqrt(ax*ax + ay*ay + az*az);

  float gx = myIMU.readFloatGyroX();
  float gy = myIMU.readFloatGyroY();
  float gz = myIMU.readFloatGyroZ();

  unsigned long now = millis();

  // 1. SECRET GESTURE (Mälu tühjendamine)
  if (totalAcc > 1.5 && totalAcc < MAX_TAP_FORCE && az > 0.5 && abs(gx) < 150 && abs(gy) < 150 && abs(gz) < 150) {
    if (now - lastTapTime > 300) { 
      if (now - lastTapTime < 1500) {
        tapCount++;
      } else {
        tapCount = 1; 
      }
      lastTapTime = now;
      
      if (tapCount > 0 && tapCount < 3) {
        digitalWrite(LED_RED, LOW);  
        delay(200);               
        digitalWrite(LED_RED, HIGH); 
      }
      
      if (tapCount == 3) {
        Serial.println("=== SECRET GESTURE: MANUAL MEMORY WIPE ===");
        InternalFS.remove("/shots.csv"); 
        for(int i=0; i<3; i++) {
          digitalWrite(LED_GREEN, LOW);  
          delay(300);
          digitalWrite(LED_GREEN, HIGH); 
          delay(300);
        }
        tapCount = 0; 
        delay(1000); 
      }
    }
  }

  // 2. HIT DETECTION
  if (totalAcc > IMPACT_THRESHOLD) { 
    float swingRotationDPS = sqrt(gx*gx + gy*gy); 
    float omegaRad = swingRotationDPS * (PI / 180.0);
    float speedMS = omegaRad * REKETI_PIKKUS;
    float speedKMH = speedMS * 3.6;

   // Offcenter v Sweetspot
    String accuracy = "Sweet";
    if (abs(gz) > 600) { 
      accuracy = "Off";
    }
    // Löögitüüp
    String swingType = "";
    if (gz > ROLL_THRESHOLD) {
      swingType = "Top";
    } else if (gz < -ROLL_THRESHOLD) {
      swingType = "Slice";
    } else {
      swingType = "Flat";
    }
    
    if (speedKMH > MINspeedKMH) {  
      String result = swingType + ";" + accuracy + ";" + String(speedKMH, 0);
      
      // LFS_O_APPEND (Lisab uued löögid vana faili lõppu)
      File writeFile = InternalFS.open("/shots.csv", FILE_O_WRITE);
      if (writeFile) {
        writeFile.println(result);
        writeFile.close();
        Serial.print("EDUKAS - Salvestati mällu: "); Serial.println(result);
      } else {
        Serial.println("VIGA: Mälu ei saanud avada!");
      }

      if (Bluefruit.connected()) {
        dataChar.notify(result.c_str(), result.length());
      }
    }
    // AKU level
    if (now - lastBatteryCheck > 10000) {
    uint8_t battLvl = readBatteryLevel();
    blebas.write(battLvl); // Uuendame Bluetoothi aku näitu
    lastBatteryCheck = now;
    }  
    delay(600); 
  }
}