/****************************************************************************************************************************
  ESP_MultiResetDetector.h
  For ESP8266 / ESP32 boards

  ESP_MultiResetDetector is a library for the ESP8266/Arduino platform
  to enable trigger configure mode by resetting ESP32 / ESP8266 twice.

  Based on and modified from
  1) DataCute    https://github.com/datacute/DoubleResetDetector
  2) Khoi Hoang  https://github.com/khoih-prog/ESP_DoubleResetDetector

  Built by Khoi Hoang https://github.com/khoih-prog/ESP_MultiResetDetector
  Licensed under MIT license
  Version: 1.1.1

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.1.1   K Hoang      30/12/2020 Initial coding to support Multiple Reset Detection. Sync with ESP_DoubleResetDetector v1.1.1
*****************************************************************************************************************************/

#pragma once

#ifndef ESP_MultiResetDetector_H
#define ESP_MultiResetDetector_H

#if defined(ARDUINO) && (ARDUINO >= 100)
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif

#define ESP_MULTI_RESET_DETECTOR_VERSION       "ESP_MultiResetDetector v1.1.1"
#define ESP_MULTIRESETDETECTOR_VERSION         ESP_MULTI_RESET_DETECTOR_VERSION

//#define ESP_MRD_USE_EEPROM      false
//#define ESP_MRD_USE_LITTLEFS    false
//#define ESP_MRD_USE_SPIFFS      false
//#define ESP8266_MRD_USE_RTC     false   //true

#ifdef ESP32
  #if (!ESP_MRD_USE_EEPROM && !ESP_MRD_USE_SPIFFS && !ESP_MRD_USE_LITTLEFS)
    #warning Neither EEPROM, SPIFFS nor LittleFS selected. Default to EEPROM
    #ifdef ESP_MRD_USE_EEPROM
      #undef ESP_MRD_USE_EEPROM
      #define ESP_MRD_USE_EEPROM      true
    #endif
  #endif
#endif

#ifdef ESP8266
  #if (!ESP8266_MRD_USE_RTC && !ESP_MRD_USE_EEPROM && !ESP_MRD_USE_SPIFFS && !ESP_MRD_USE_LITTLEFS)
    #warning Neither RTC, EEPROM, LITTLEFS nor SPIFFS selected. Default to EEPROM
    #ifdef ESP_MRD_USE_EEPROM
      #undef ESP_MRD_USE_EEPROM
      #define ESP_MRD_USE_EEPROM      true
    #endif
  #endif
#endif

//default to use EEPROM, otherwise, use LITTLEFS (higher priority), then SPIFFS
#if ESP_MRD_USE_EEPROM
  #include <EEPROM.h>

  #define  FLAG_DATA_SIZE     4

  #ifndef EEPROM_SIZE
    #define EEPROM_SIZE     512
  #endif

  #ifndef EEPROM_START
    #define EEPROM_START    256
  #endif

#elif ( ESP_MRD_USE_LITTLEFS || ESP_MRD_USE_SPIFFS )

#include <FS.h>

#ifdef ESP32

  #if ESP_MRD_USE_LITTLEFS
    // The library will be depreciated after being merged to future major Arduino esp32 core release 2.x
    // At that time, just remove this library inclusion
    #include <LITTLEFS.h>             // https://github.com/lorol/LITTLEFS
    #define FileFS   LITTLEFS
  #else
    #include "SPIFFS.h"
    // ESP32 core 1.0.4 still uses SPIFFS
    #define FileFS   SPIFFS
  #endif

#else
  // From ESP8266 core 2.7.1
  #include <LittleFS.h>

  #if ESP_MRD_USE_LITTLEFS
    #define FileFS    LittleFS
  #else
    #define FileFS   SPIFFS
  #endif

#endif    // #if ESP_MRD_USE_EEPROM

#define  MRD_FILENAME     "/mrd.dat"

#endif    //#if ESP_MRD_USE_EEPROM

#ifndef MULTIRESETDETECTOR_DEBUG
  #define MULTIRESETDETECTOR_DEBUG       false
#endif

///////////////////
// Default values if not specified in sketch

#ifndef MRD_TIMES
  #define MRD_TIMES         3
#endif

#ifndef MRD_TIMEOUT
  #define MRD_TIMEOUT       10
#endif

#ifndef MRD_ADDRESS
  #define MRD_ADDRESS       0
#endif

///////////////////

// Flag clear to 0xFFFE0001 if no MRD within MRD_TIMEOUT. Flag will increase 1 for each reset within MRD_TIMEOUT
// So MULTIRESETDETECTOR_FLAG_SET is not necessary.
// Will use upper 2 bytes to verify if corrupted data. 

#define USING_INVERTED    true

#if USING_INVERTED
  #define MULTIRESETDETECTOR_FLAG_BEGIN  0xFFFE0001     // Used when beginning a new cycle
  #define MULTIRESETDETECTOR_FLAG_CLEAR  0x00000000     // Used when data corrupted, such as reformat LittleFS/SPIFFS
#else
  #define MULTIRESETDETECTOR_FLAG_BEGIN  0x00010001     // Used when beginning a new cycle
  #define MULTIRESETDETECTOR_FLAG_CLEAR  0x00000000     // Used when data corrupted, such as reformat LittleFS/SPIFFS
#endif

class MultiResetDetector
{
  public:
    MultiResetDetector(int timeout, int address)
    {
      mrd_times = MRD_TIMES;
      
#if ESP_MRD_USE_EEPROM
#if (MULTIRESETDETECTOR_DEBUG)
      Serial.printf("EEPROM size = %d, start = %d\n", EEPROM_SIZE, EEPROM_START);
#endif

      EEPROM.begin(EEPROM_SIZE);
#elif ( ESP_MRD_USE_LITTLEFS || ESP_MRD_USE_SPIFFS )
      // LittleFS / SPIFFS code. Format FileFS if not yet
  #ifdef ESP32
      if (!FileFS.begin(true))
  #else
      if (!FileFS.begin())
  #endif        
      {
#if (MULTIRESETDETECTOR_DEBUG)

#if ESP_MRD_USE_LITTLEFS
        Serial.println(F("LittleFS failed!. Please use SPIFFS or EEPROM."));
#else
        Serial.println(F("SPIFFS failed!. Please use LittleFS or EEPROM."));
#endif

#endif
        multiResetDetectorFlag = MULTIRESETDETECTOR_FLAG = 0;
      }
#else
#ifdef ESP8266
      //RTC only for ESP8266
#endif
#endif

      this->timeout = timeout * 1000;
      this->address = address;
      multiResetDetected = false;
      waitingForMultiReset = false;
    };

    bool detectMultiReset()
    {
      multiResetDetected = detectRecentlyResetFlag();

      if (multiResetDetected)
      {
#if (MULTIRESETDETECTOR_DEBUG)
        Serial.printf("multiResetDetected, number of times = %d\n", MRD_TIMES);
#endif

        clearRecentlyResetFlag();
      }
      else
      {
#if (MULTIRESETDETECTOR_DEBUG)
        Serial.printf("No multiResetDetected, number of times = %d\n", (uint16_t) (multiResetDetectorFlag & 0x0000FFFF) );
#endif

        setRecentlyResetFlag();
        waitingForMultiReset = true;
      }

      return multiResetDetected;

    };

    void loop()
    {
      if (waitingForMultiReset && millis() > timeout)
      {
#if (MULTIRESETDETECTOR_DEBUG)
        Serial.println(F("Stop multiResetDetecting"));
#endif

        stop();
      }
    };

    void stop()
    {
      clearRecentlyResetFlag();
      waitingForMultiReset = false;
    };

    bool multiResetDetected;


  private:
  
    uint32_t MULTIRESETDETECTOR_FLAG;
    
    unsigned long mrd_times;
    
    unsigned long timeout;
    
    int address;
    bool waitingForMultiReset;
    
    uint32_t multiResetDetectorFlag;
    
    bool readRecentlyResetFlag()
    {
#if (ESP_MRD_USE_EEPROM)
      EEPROM.get(EEPROM_START, MULTIRESETDETECTOR_FLAG);
      multiResetDetectorFlag = MULTIRESETDETECTOR_FLAG;

  #if (MULTIRESETDETECTOR_DEBUG)
      Serial.printf("EEPROM Flag read = 0x%08X\n", MULTIRESETDETECTOR_FLAG);
  #endif
#elif ( ESP_MRD_USE_LITTLEFS || ESP_MRD_USE_SPIFFS )
      // LittleFS / SPIFFS code
      if (FileFS.exists(MRD_FILENAME))
      {
        // if config file exists, load
        File file = FileFS.open(MRD_FILENAME, "r");

        if (!file)
        {
  #if (MULTIRESETDETECTOR_DEBUG)
          Serial.println(F("Loading config file failed"));
  #endif
          return false;
        }

        file.readBytes((char *) &MULTIRESETDETECTOR_FLAG, sizeof(MULTIRESETDETECTOR_FLAG));
        multiResetDetectorFlag = MULTIRESETDETECTOR_FLAG;

  #if (MULTIRESETDETECTOR_DEBUG)

    #if ESP_MRD_USE_LITTLEFS
        Serial.printf("LittleFS Flag read = 0x%08X\n", MULTIRESETDETECTOR_FLAG);
    #else
        Serial.printf("SPIFFS Flag read = 0x%08X\n", MULTIRESETDETECTOR_FLAG);
    #endif

  #endif

        file.close();
      }
#else
  #ifdef ESP8266
      //RTC only for ESP8266
      ESP.rtcUserMemoryRead(address, &multiResetDetectorFlag, sizeof(multiResetDetectorFlag));
  #endif
#endif

      return true;
    }

    bool detectRecentlyResetFlag()
    {
      if (!readRecentlyResetFlag())
        return false;

      //multiResetDetected = (multiResetDetectorFlag == MULTIRESETDETECTOR_FLAG_SET);
      // Check lower 2 bytes is > 0 and upper 2 bytes agrees
           
#if USING_INVERTED      
      uint16_t upperBytes = ~(multiResetDetectorFlag >> 16);
#else
      uint16_t upperBytes = multiResetDetectorFlag >> 16;
#endif
     
      uint16_t lowerBytes = multiResetDetectorFlag & 0x0000FFFF;
      
#if (MULTIRESETDETECTOR_DEBUG)
      Serial.printf("multiResetDetectorFlag = 0x%08X\n", multiResetDetectorFlag);
      Serial.printf("lowerBytes = 0x%04X, upperBytes = 0x%04X\n", lowerBytes, upperBytes);
#endif      
   
      if ( ( lowerBytes >= MRD_TIMES ) && ( lowerBytes == upperBytes )  )   
      {
        multiResetDetected = true;
      }
      else
      {
        multiResetDetected = false;
        
        if (lowerBytes != upperBytes)     
        {
          // To reset if data corrupted
          multiResetDetectorFlag = MULTIRESETDETECTOR_FLAG_CLEAR;
#if (MULTIRESETDETECTOR_DEBUG)
          Serial.printf("lowerBytes = 0x%04X, upperBytes = 0x%04X\n", lowerBytes, upperBytes);
          Serial.println(F("detectRecentlyResetFlag: Data corrupted. Reset to 0"));
#endif
        }
      }
      
      return multiResetDetected;
    };

    void setRecentlyResetFlag()
    {
      // Add 1 every time detecting a reset
      // To read first, increase and update 2 checking bytes
      readRecentlyResetFlag();
      
#if USING_INVERTED
      // 2 lower bytes
      MULTIRESETDETECTOR_FLAG = (MULTIRESETDETECTOR_FLAG & 0x0000FFFF) + 1;
      
      // 2 upper bytes
      uint16_t upperBytes = ~MULTIRESETDETECTOR_FLAG;
      MULTIRESETDETECTOR_FLAG = (upperBytes << 16) | MULTIRESETDETECTOR_FLAG;
#else   
      // 2 lower bytes
      MULTIRESETDETECTOR_FLAG = (MULTIRESETDETECTOR_FLAG & 0x0000FFFF) + 1;
      
      // 2 upper bytes
      MULTIRESETDETECTOR_FLAG = (MULTIRESETDETECTOR_FLAG << 16) | MULTIRESETDETECTOR_FLAG;
#endif

      multiResetDetectorFlag  = MULTIRESETDETECTOR_FLAG;
      
#if (ESP_MRD_USE_EEPROM)
      EEPROM.put(EEPROM_START, MULTIRESETDETECTOR_FLAG);
      EEPROM.commit();

#if (MULTIRESETDETECTOR_DEBUG)
      delay(1000);
      EEPROM.get(EEPROM_START, MULTIRESETDETECTOR_FLAG);

      Serial.printf("SetFlag write = 0x%08X\n", MULTIRESETDETECTOR_FLAG);
#endif
#elif ( ESP_MRD_USE_LITTLEFS || ESP_MRD_USE_SPIFFS )
      // LittleFS / SPIFFS code
      File file = FileFS.open(MRD_FILENAME, "w");
#if (MULTIRESETDETECTOR_DEBUG)
      Serial.println(F("Saving config file..."));
#endif

      if (file)
      {
        file.write((uint8_t *) &MULTIRESETDETECTOR_FLAG, sizeof(MULTIRESETDETECTOR_FLAG));
        file.close();
#if (MULTIRESETDETECTOR_DEBUG)
        Serial.println(F("Saving config file OK"));
#endif
      }
      else
      {
#if (MULTIRESETDETECTOR_DEBUG)
        Serial.println(F("Saving config file failed"));
#endif
      }
#else
#ifdef ESP8266
      //RTC only for ESP8266
      ESP.rtcUserMemoryWrite(address, &multiResetDetectorFlag, sizeof(multiResetDetectorFlag));
#endif
#endif
    };


    void clearRecentlyResetFlag()
    {
      multiResetDetectorFlag = MULTIRESETDETECTOR_FLAG_BEGIN;
      MULTIRESETDETECTOR_FLAG = MULTIRESETDETECTOR_FLAG_BEGIN;

#if (ESP_MRD_USE_EEPROM)
      //MULTIRESETDETECTOR_FLAG = MULTIRESETDETECTOR_FLAG_BEGIN;
      EEPROM.put(EEPROM_START, MULTIRESETDETECTOR_FLAG);
      EEPROM.commit();

#if (MULTIRESETDETECTOR_DEBUG)
      delay(1000);
      EEPROM.get(EEPROM_START, MULTIRESETDETECTOR_FLAG);

      Serial.printf("ClearFlag write = 0x%08X\n", MULTIRESETDETECTOR_FLAG);
#endif
#elif ( ESP_MRD_USE_LITTLEFS || ESP_MRD_USE_SPIFFS )
      // LittleFS / SPIFFS code
      File file = FileFS.open(MRD_FILENAME, "w");
#if (MULTIRESETDETECTOR_DEBUG)
      Serial.println(F("Saving config file..."));
#endif

      if (file)
      {
        file.write((uint8_t *) &MULTIRESETDETECTOR_FLAG, sizeof(MULTIRESETDETECTOR_FLAG));
        file.close();
#if (MULTIRESETDETECTOR_DEBUG)
        Serial.println(F("Saving config file OK"));
#endif
      }
      else
      {
#if (MULTIRESETDETECTOR_DEBUG)
        Serial.println(F("Saving config file failed"));
#endif
      }

#else
#ifdef ESP8266
      //RTC only for ESP8266
      ESP.rtcUserMemoryWrite(address, &multiResetDetectorFlag, sizeof(multiResetDetectorFlag));
#endif
#endif
    };
};
#endif // ESP_MultiResetDetector_H
