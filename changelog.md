## ESP_MultiResetDetector

[![arduino-library-badge](https://www.ardu-badge.com/badge/ESP_MultiResetDetector.svg?)](https://www.ardu-badge.com/ESP_MultiResetDetector)
[![GitHub release](https://img.shields.io/github/release/khoih-prog/ESP_MultiResetDetector.svg)](https://github.com/khoih-prog/ESP_MultiResetDetector/releases)
[![GitHub](https://img.shields.io/github/license/mashape/apistatus.svg)](https://github.com/khoih-prog/ESP_MultiResetDetector/blob/master/LICENSE)
[![contributions welcome](https://img.shields.io/badge/contributions-welcome-brightgreen.svg?style=flat)](#Contributing)
[![GitHub issues](https://img.shields.io/github/issues/khoih-prog/ESP_MultiResetDetector.svg)](http://github.com/khoih-prog/ESP_MultiResetDetector/issues)

---
---

## Table of Contents


* [Changelog](#changelog)
  * [Releases v1.2.1](#releases-v121)
  * [Releases v1.2.0](#releases-v120)
  * [Releases v1.1.2](#releases-v112)
  * [Releases v1.1.1](#releases-v111)
 
---
---

## Changelog

### Releases v1.2.1

1. Fix compile error for ESP32 core v1.0.5-

### Releases v1.2.0

1. Auto detect ESP32 core and use either built-in LittleFS or [LITTLEFS](https://github.com/lorol/LITTLEFS) library
2. Update `library.json` to use new `headers` for PIO
3. Update `platformio.ini`

### Releases v1.1.2

1. Update `platform.ini` and `library.json` to use original `khoih-prog` instead of `khoih.prog` after PIO fix

### Releases v1.1.1

1. Initial coding to support Multiple Reset Detection.
2. Sync with ESP_DoubleResetDetector v1.1.1


