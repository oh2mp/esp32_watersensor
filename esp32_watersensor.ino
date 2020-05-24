/*
 * ESP32 based BLE beacon for liquid sensors
 *
 * See https://github.com/oh2mp/esp32_watersensor
 *
 */
 
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include <SPIFFS.h>

#define LED 2
#define SENS 36

int sensorbuff;
byte sensor;

char datastr[16];
char convtable[255];

BLEAdvertising *advertising;
std::string mfdata = "";

/* ----------------------------------------------------------------------------------
 * Set up data packet for advertising
 */ 
void set_beacon() {
    BLEBeacon beacon = BLEBeacon();
    BLEAdvertisementData advdata = BLEAdvertisementData();
    BLEAdvertisementData scanresponse = BLEAdvertisementData();
    
    advdata.setFlags(0x06); // BR_EDR_NOT_SUPPORTED 0x04 & LE General discoverable 0x02

    mfdata = "";
    mfdata += (char)0xE5; mfdata += (char)0x02;  // Espressif Incorporated Vendor ID = 0x02E5
    mfdata += (char)0xe9; mfdata += (char)0x48;  // Identifier for this sketch is 0xE948 (Oxygen)
    mfdata += (char)(sensor);                    // Raw (calculated) value from the sensor
    mfdata += convtable[sensor];                 // Table converted value
    mfdata += (char)0xBE; mfdata += (char)0xEF;  // Beef is always good nutriment
  
    advdata.setManufacturerData(mfdata);
    advertising->setAdvertisementData(advdata);
    advertising->setScanResponseData(scanresponse);
}

/* ----------------------------------------------------------------------------------
 * Read the conversion table from the file
 */

void readtable() {
    if (SPIFFS.exists("/table.txt")) {
        File file;
        file = SPIFFS.open("/table.txt", "r");
        char numstr[8];
        int inx;
        int val;
        while (file.available()) {
            memset(numstr,0,8);
            file.readBytesUntil(' ', numstr, 8);
            inx = (char)atoi(numstr);
            memset(numstr,0,8);
            file.readBytesUntil('\n', numstr, 8);
            val = (char)atoi(numstr);
            convtable[inx] = val;
        }
        file.close();
    }
}

/* ---------------------------------------------------------------------------------- */
void setup() {
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);   // LED off

    pinMode(SENS, INPUT);
    analogSetCycles(255);
    analogSetSamples(1);
    analogReadResolution(12);
    
    Serial.begin(115200);
    BLEDevice::init("ESP32+watersensor");
    advertising = BLEDevice::getAdvertising();
    SPIFFS.begin();
    readtable();
}
 
/* ---------------------------------------------------------------------------------- */
void loop() {
    // Read sensor and calculate value
    for (int i = 0; i < 50; i++) {
       sensorbuff += analogRead(SENS);
       delay(100);
    }
    sensor = int((float)sensorbuff/2000+0.5);
    
    sprintf(datastr, "%d = %d l\n",sensor,(int)convtable[sensor]);
    Serial.print(datastr);
    sensorbuff = 0;

    set_beacon();
    digitalWrite(LED, HIGH);   // LED on during the advertising
    advertising->start();
    delay(100);
    advertising->stop();
    digitalWrite(LED, LOW);   // LED off

    // Reboot once in hour to be sure    
    if (millis() > 3.6E+6) {
        ESP.restart();
    }
}

/* ---------------------------------------------------------------------------------- */
 
