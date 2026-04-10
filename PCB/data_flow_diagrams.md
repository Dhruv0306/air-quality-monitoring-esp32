# Data-Flow Diagrams — Air Quality Monitor Firmware

> [!NOTE]
> All diagrams are derived from [PMS_DHT_2025_01_06.ino](file:///e:/CAPSTONE/PCB/PMS_DHT_2025_01_06.ino).
> Arrows show data movement; rounded boxes are processes; cylinders are stores; parallelograms are I/O.

---

## 1. `setup()` — System Startup

Runs **once** at power-on. Initialises every peripheral and establishes time.

```mermaid
flowchart TD
    subgraph Inputs
        PWR["Power-on / Reset"]
    end

    PWR --> SERIAL["Init Serial (9600 baud)<br>& Serial2 for PMS5003"]
    SERIAL --> I2C["Init I2C bus (Wire)<br>Set 400 kHz clock"]
    I2C --> WIFI_MODE["Set WiFi.mode(WIFI_STA)"]
    WIFI_MODE --> OLED["Init OLED SSD1306<br>(0x3C address)"]

    OLED --> CRED_CHK{"WiFi credentials<br>stored?"}
    CRED_CHK -- "No (first boot)" --> PORTAL["setupWiFiFirstTime()<br>Create AP portal"]
    CRED_CHK -- "Yes" --> AUTO["WiFi.begin()<br>Auto-connect"]

    PORTAL --> RTC_CHK
    AUTO --> RTC_CHK

    RTC_CHK{"isRTCValid()?"} -- "Yes" --> LOAD_RTC["setTime() from<br>RTC registers<br>rtcSynced ← true"]
    RTC_CHK -- "No" --> NTP_CHK

    LOAD_RTC --> NTP_CHK{"WiFi connected?"}
    NTP_CHK -- "Yes" --> NTP_SYNC["syncTimeFromNTP()<br>syncRTCFromSystemTime()<br>ntpSynced ← true"]
    NTP_CHK -- "No" --> SENSORS

    NTP_SYNC --> SENSORS["Init PMS5003 (passive)<br>Init DHT22<br>Try BME680"]
    SENSORS --> SD_INIT["SD.begin(SD_CS)"]

    SD_INIT --> DONE["Setup complete →<br>enter loop()"]

    subgraph Outputs / Side-Effects
        direction LR
        G1["Global flags:<br>wifiConnected<br>rtcSynced<br>ntpSynced<br>bmeAvailable"]
        G2["Hardware ready:<br>OLED, PMS, DHT,<br>BME680, SD card"]
    end
    DONE --> G1
    DONE --> G2
```

---

## 2. `loop()` — Main Program Loop

Runs **forever** after `setup()`. Orchestrates all subsystems with non-blocking `millis()` timing.

```mermaid
flowchart TD
    START(["loop() entry"]) --> MW["maintainWiFi()"]
    MW --> PNS["periodicNtpSync()"]
    PNS --> THROTTLE{"millis() − last<br>≥ 1 000 ms?"}
    THROTTLE -- "No" --> RETURN(["return"])
    THROTTLE -- "Yes" --> GPS["Set fixed GPS<br>coordinates"]

    GPS --> PMS_CHK{"millis() − lastPmsRead<br>≥ 2 000 ms?"}
    PMS_CHK -- "Yes" --> PMS_READ["readPMSSensor()"]
    PMS_CHK -- "No" --> DAY_CHK
    PMS_READ --> DAY_CHK

    DAY_CHK{"getRTCDate()<br>≠ lastFileDate?"}
    DAY_CHK -- "Yes" --> NEW_FILE["checkFileExists()<br>lastFileDate ← today"]
    DAY_CHK -- "No" --> SENSE
    NEW_FILE --> SENSE

    SENSE["readSensors()"] --> AQI["calculateAQI()"]
    AQI --> TIME["dateTime ← getRTCDateTime()<br>Date ← getRTCDate()"]

    TIME --> DISP_CHK{"millis() − lastDisplay<br>≥ 3 000 ms?"}
    DISP_CHK -- "Yes" --> DISP["updateDisplay()"]
    DISP_CHK -- "No" --> AQI_CHK
    DISP --> AQI_CHK

    AQI_CHK{"millis() − lastAqiDisp<br>≥ 5 000 ms?"}
    AQI_CHK -- "Yes" --> AQI_DISP["updateAQIDisplay()"]
    AQI_CHK -- "No" --> SD_CHK
    AQI_DISP --> SD_CHK

    SD_CHK{"millis() − lastSDLog<br>≥ 10 000 ms?"}
    SD_CHK -- "Yes" --> SD_LOG["logDataSdCard()"]
    SD_CHK -- "No" --> DBG
    SD_LOG --> DBG

    DBG["Serial.println<br>data cycle info"] --> RETURN
```

---

## 3. `readSensors()` — DHT22 + BME680 Reading

```mermaid
flowchart LR
    subgraph HW Inputs
        DHT["DHT22 sensor<br>(GPIO 14)"]
        BME["BME680 sensor<br>(I2C, optional)"]
    end

    DHT -->|"getEvent()"| TEMP["temperature ← event.temperature"]
    DHT -->|"getEvent()"| HUM["humidity ← event.relative_humidity"]

    BME --> BME_CHK{"bmeAvailable &&<br>bme.performReading()?"}
    BME_CHK -- "Yes" --> P["pressure ← bme.pressure / 100"]
    BME_CHK -- "Yes" --> G["gas ← bme.gas_resistance / 1000"]
    BME_CHK -- "Yes" --> A["altitudeBme ← bme.readAltitude()"]
    BME_CHK -- "No" --> DEF["pressure ← 9999<br>gas ← 9999<br>altitudeBme ← 9999"]

    subgraph Global Outputs
        TEMP
        HUM
        P
        G
        A
        DEF
    end
```

---

## 4. `readPMSSensor()` — PMS5003 Air Quality Reading

```mermaid
flowchart TD
    subgraph HW Input
        PMS["PMS5003<br>(Serial2: RX2/TX2)"]
    end

    PMS -->|"poll() + read()"| STD["Standard PM values<br>val1 ← PM1.0 STD<br>val2 ← PM2.5 STD<br>val3 ← PM10 STD"]
    PMS --> ATM["Atmospheric PM values<br>val4 ← PM1.0 ATM<br>val5 ← PM2.5 ATM<br>val6 ← PM10 ATM"]
    PMS --> CNT["Particle counts<br>c_300  (>0.3 μm)<br>c_500  (>0.5 μm)<br>c_1000 (>1.0 μm)<br>c_2500 (>2.5 μm)<br>c_5000 (>5.0 μm)<br>c_10000(>10 μm)"]
    PMS --> ENV["PMS environmental<br>pms_temp ← temperature<br>pms_h ← humidity<br>pms_fld ← formaldehyde"]

    subgraph Global Outputs
        STD
        ATM
        CNT
        ENV
    end
```

---

## 5. `calculateAQI()` — PM2.5 → Air Quality Index

```mermaid
flowchart TD
    IN["val2<br>(PM2.5 STD, μg/m³)"] --> R1{"0 – 30?"}
    R1 -- "Yes" --> L1["pm25_aqi ← 1<br>aqi ← 'Good'"]
    R1 -- "No" --> R2{"30 – 60?"}
    R2 -- "Yes" --> L2["pm25_aqi ← 2<br>aqi ← 'Satisfactory'"]
    R2 -- "No" --> R3{"60 – 90?"}
    R3 -- "Yes" --> L3["pm25_aqi ← 3<br>aqi ← 'Moderate'"]
    R3 -- "No" --> R4{"90 – 120?"}
    R4 -- "Yes" --> L4["pm25_aqi ← 4<br>aqi ← 'Poor'"]
    R4 -- "No" --> R5{"120 – 250?"}
    R5 -- "Yes" --> L5["pm25_aqi ← 5<br>aqi ← 'Very Poor'"]
    R5 -- "No" --> R6{"≥ 250?"}
    R6 -- "Yes" --> L6["pm25_aqi ← 6<br>aqi ← 'Severe'"]
    R6 -- "No" --> ERR["pm25_aqi ← 0<br>aqi ← 'ERROR'"]
```

---

## 6. `updateDisplay()` — OLED Sensor Screen

```mermaid
flowchart LR
    subgraph Global Inputs
        S0_["S0 device name"]
        T["temperature"]
        H["humidity"]
        PR["pressure"]
        V4["val4 (PM1 ATM)"]
        V5["val5 (PM2.5 ATM)"]
        V6["val6 (PM10 ATM)"]
    end

    S0_ --> RENDER
    T --> RENDER
    H --> RENDER
    PR --> RENDER
    V4 --> RENDER
    V5 --> RENDER
    V6 --> RENDER

    RENDER["Format text layout<br>clearDisplay()<br>setTextSize(1)"] --> OLED["OLED SSD1306<br>display.display()"]
```

---

## 7. `updateAQIDisplay()` — OLED AQI Screen

```mermaid
flowchart LR
    subgraph Global Inputs
        DT["dateTime string"]
        AQ["aqi string<br>(Good / Poor / …)"]
    end

    DT --> FMT["clearDisplay()<br>TextSize 2: dateTime<br>TextSize 1: AQI label"]
    AQ --> FMT
    FMT --> OLED["OLED SSD1306<br>display.display()"]
```

---

## 8. `checkFileExists()` — Daily CSV File Creation

```mermaid
flowchart TD
    subgraph Inputs
        RTC["RTC Module<br>(year, month, day)"]
        CFG["S0, S1<br>(device ID strings)"]
    end

    RTC --> FMT["Format date:<br>YYYY-MM-DD"]
    CFG --> FNAME["filename ←<br>S0 + '_' + S1 + date + '.csv'"]
    FMT --> FNAME

    FNAME --> OPEN{"SD.open('/' + filename)<br>File exists?"}
    OPEN -- "Yes" --> DONE["Log: file exists<br>Close file"]
    OPEN -- "No" --> CREATE["Create file<br>Write CSV header:<br>DateTime, Temperature,<br>Humidity, Pressure, Gas,<br>Altitude, PM1, PM2.5, PM10,<br>… 25 columns total"]
    CREATE --> CLOSE["Close file"]

    subgraph Side Effects
        SD[("SD Card<br>(new .csv file)")]
    end
    CREATE --> SD
```

---

## 9. `logDataSdCard()` — Append Sensor Row to CSV

```mermaid
flowchart TD
    subgraph Global Inputs
        DT["dateTime"]
        TEMP["temperature, humidity"]
        BME_D["pressure, gas, altitudeBme"]
        PM_STD["val1, val2, val3"]
        PM_ATM["val4, val5, val6"]
        PCNT["c_300 … c_10000"]
        PMS_E["pms_temp, pms_h, pms_fld"]
        GPS["lati, longi, atlt, noS"]
    end

    DT --> FMT
    TEMP --> FMT
    BME_D --> FMT
    PM_STD --> FMT
    PM_ATM --> FMT
    PCNT --> FMT
    PMS_E --> FMT
    GPS --> FMT

    RTC2["RTC Module<br>(year, month, day)"] --> FNAME["Build filename<br>S0_S1_YYYY-MM-DD.csv"]

    FMT["snprintf → dataMessage<br>(comma-separated row)"] --> WRITE

    FNAME --> OPEN["SD.open(filename, 'a+')"]
    OPEN --> WRITE["file.print(dataMessage)"]
    WRITE --> CLOSE["file.close()"]

    subgraph Output
        SD[("SD Card<br>(appended row)")]
    end
    WRITE --> SD
```

---

## 10. `syncTimeFromNTP()` — Internet Time Sync

```mermaid
flowchart TD
    subgraph Inputs
        NTP["NTP Server<br>(pool.ntp.org)"]
        CFG["GMT_OFFSET_SEC = 19800<br>(IST +5:30)"]
    end

    NTP --> CONFIG["configTime(<br>GMT_OFFSET,<br>DAYLIGHT_OFFSET,<br>NTP_SERVER)"]
    CFG --> CONFIG

    CONFIG --> GET{"getLocalTime(<br>&timeinfo, 10 s)?"}
    GET -- "Fail" --> FAIL["OLED: 'NTP sync failed'<br>return false"]
    GET -- "OK" --> OK["OLED: 'Time synced from NTP'<br>return true"]

    subgraph Outputs
        SYS["ESP32 system clock<br>updated to IST"]
    end
    OK --> SYS
```

---

## 11. `syncRTCFromSystemTime()` — Write System Clock → RTC

```mermaid
flowchart LR
    SYS["ESP32 system time<br>(struct tm)"] -->|"getLocalTime()"| CONV["Convert fields:<br>year: tm_year − 100<br>month: tm_mon + 1<br>date, hour, min, sec"]

    CONV --> RTC["DS3231 RTC Module<br>setYear / setMonth /<br>setDate / setHour /<br>setMinute / setSecond"]

    RTC --> FLAGS["rtcSynced ← true<br>lastNtpSyncEpoch ← now()"]
```

---

## 12. `setupWiFiFirstTime()` — WiFi Captive Portal

```mermaid
flowchart TD
    START(["Called on first boot"]) --> CFG["wm.setConfigPortalTimeout(120)<br>wm.setConnectTimeout(5)<br>wm.setDebugOutput(true)"]
    CFG --> NAME["S_name ← S0 + ' ESP32-Time-Setup'"]
    NAME --> AP["wm.autoConnect(S_name)<br>Create WiFi hotspot"]

    AP --> RES{"User configured<br>WiFi?"}
    RES -- "Yes" --> OK["return true<br>(credentials saved to flash)"]
    RES -- "No / timeout" --> FAIL["return false"]
```

---

## 13. `isRTCValid()` — RTC Sanity Check

```mermaid
flowchart LR
    RTC["DS3231 RTC<br>Registers"] -->|"getYear()<br>getMonth()<br>getDate()<br>getHour()"| CHK{"year ≥ 24<br>month 1-12<br>day 1-31<br>hour 0-23?"}

    CHK -- "All pass" --> T["return true"]
    CHK -- "Any fail" --> F["return false"]
```

---

## 14. `getRTCDateTime()` — Formatted Date-Time String

```mermaid
flowchart LR
    RTC["DS3231 RTC<br>Registers"] -->|"year, month, day,<br>hour, minute, second"| FMT["Format:<br>DD-MM-YYYY HH:MM:SS<br>(leading zeros added)"]

    FMT --> OUT["return String<br>e.g. '25-01-2025 14:30:15'"]
```

---

## 15. `getRTCDate()` — Date-Only String

```mermaid
flowchart LR
    RTC["DS3231 RTC<br>Registers"] -->|"year, month, day"| FMT["Format:<br>DD-MM-YYYY + trailing space"]

    FMT --> OUT["return String<br>e.g. '25-01-2025 '"]
```

---

## 16. `maintainWiFi()` — WiFi Connection Manager

```mermaid
flowchart TD
    START(["maintainWiFi()"]) --> CHK{"WiFi.status()<br>== WL_CONNECTED?"}

    CHK -- "Yes" --> FIRST{"wifiConnected<br>== false?"}
    FIRST -- "Yes" --> SET["wifiConnected ← true<br>ntpSyncedThisSession ← false"]
    FIRST -- "No" --> NTP_CHK
    SET --> NTP_CHK

    NTP_CHK{"ntpSyncedThisSession<br>== false?"}
    NTP_CHK -- "Yes" --> NTP["syncTimeFromNTP()<br>syncRTCFromSystemTime()<br>ntpSyncedThisSession ← true"]
    NTP_CHK -- "No" --> RET(["return"])
    NTP --> RET

    CHK -- "No" --> DISC["wifiConnected ← false"]
    DISC --> THROTTLE{"millis() − lastWiFiAttempt<br>< 30 000 ms?"}
    THROTTLE -- "Yes" --> RET2(["return (throttled)"])
    THROTTLE -- "No" --> UPD["lastWiFiAttempt ← millis()"]
    UPD --> SSID{"WL_NO_SSID_AVAIL?"}
    SSID -- "Yes" --> RET3(["return (no creds)"])
    SSID -- "No" --> RECONN["WiFi.begin()<br>(non-blocking)"]
    RECONN --> RET4(["return"])
```

---

## 17. `periodicNtpSync()` — 24-Hour Time Correction

```mermaid
flowchart TD
    START(["periodicNtpSync()"]) --> MIN{"millis() − lastCheck<br>< 60 000 ms?"}
    MIN -- "Yes" --> RET1(["return (too soon)"])
    MIN -- "No" --> UPD["lastCheck ← millis()"]

    UPD --> WIFI{"WiFi connected?"}
    WIFI -- "No" --> RET2(["return"])
    WIFI -- "Yes" --> TIME["currentTime ← now()"]

    TIME --> VALID{"currentTime<br>> 100 000?"}
    VALID -- "No" --> RET3(["return (invalid)"])
    VALID -- "Yes" --> ELAPSED{"currentTime − lastNtpSyncEpoch<br>≥ 86 400 s (24 h)?"}

    ELAPSED -- "No" --> RET4(["return"])
    ELAPSED -- "Yes" --> SYNC["syncTimeFromNTP()"]
    SYNC --> OK{"Success?"}
    OK -- "Yes" --> RTC_SYNC["syncRTCFromSystemTime()"]
    OK -- "No" --> FAIL["Log: sync failed"]

    RTC_SYNC --> RET5(["return"])
    FAIL --> RET5
```

---

## 18. Utility helpers: `daysInMonth()` / `isLeapYear()`

These are pure functions with no side-effects — included for completeness.

```mermaid
flowchart LR
    subgraph "isLeapYear(y)"
        Y["year"] --> D4{"y % 4 ≠ 0?"}
        D4 -- "Yes" --> NL["return false"]
        D4 -- "No" --> D100{"y % 100 ≠ 0?"}
        D100 -- "Yes" --> LY["return true"]
        D100 -- "No" --> D400{"y % 400 ≠ 0?"}
        D400 -- "Yes" --> NL2["return false"]
        D400 -- "No" --> LY2["return true"]
    end

    subgraph "daysInMonth(m, y)"
        M["month"] --> APR{"Apr/Jun/Sep/Nov?"}
        APR -- "Yes" --> T30["return 30"]
        APR -- "No" --> FEB{"February?"}
        FEB -- "Yes" --> LEAP["isLeapYear(y) ? 29 : 28"]
        FEB -- "No" --> T31["return 31"]
    end
```

---

## Global Data-Flow Overview

This final diagram ties all functions together showing the full data lifecycle:

```mermaid
flowchart TB
    subgraph "Hardware Layer"
        PMS5003["PMS5003<br>(UART)"]
        DHT22["DHT22<br>(GPIO 14)"]
        BME680["BME680<br>(I2C, opt.)"]
        DS3231["DS3231 RTC<br>(I2C)"]
        OLED["SSD1306 OLED<br>(I2C)"]
        SDCARD[("SD Card<br>(SPI)")]
        WIFI_HW["WiFi Radio"]
    end

    subgraph "Data Acquisition"
        readPMS["readPMSSensor()"]
        readS["readSensors()"]
        getRTC["getRTCDateTime()<br>getRTCDate()"]
    end

    subgraph "Processing"
        calcAQI["calculateAQI()"]
    end

    subgraph "Output"
        updDisp["updateDisplay()"]
        updAQI["updateAQIDisplay()"]
        logSD["logDataSdCard()"]
        chkFile["checkFileExists()"]
    end

    subgraph "Time Management"
        syncNTP["syncTimeFromNTP()"]
        syncRTC["syncRTCFromSystemTime()"]
        maintWiFi["maintainWiFi()"]
        perSync["periodicNtpSync()"]
    end

    PMS5003 -->|"PM, particles,<br>temp, humidity, HCHO"| readPMS
    DHT22 -->|"temperature,<br>humidity"| readS
    BME680 -->|"pressure, gas,<br>altitude"| readS
    DS3231 -->|"date & time<br>registers"| getRTC

    readPMS -->|"val1-6, c_*, pms_*"| calcAQI
    readPMS -->|"val1-6, c_*, pms_*"| logSD
    readS -->|"temperature,<br>humidity, pressure"| calcAQI
    readS -->|"temperature,<br>humidity, pressure"| updDisp
    readS -->|"temperature,<br>humidity, pressure"| logSD

    calcAQI -->|"pm25_aqi, aqi"| updAQI
    getRTC -->|"dateTime"| updAQI
    getRTC -->|"dateTime"| logSD
    getRTC -->|"date"| chkFile

    updDisp -->|"rendered frame"| OLED
    updAQI -->|"rendered frame"| OLED
    chkFile -->|"create CSV + header"| SDCARD
    logSD -->|"append CSV row"| SDCARD

    WIFI_HW <-->|"NTP packets"| syncNTP
    syncNTP -->|"system clock"| syncRTC
    syncRTC -->|"set registers"| DS3231
    maintWiFi -->|"reconnect"| WIFI_HW
    perSync -->|"trigger"| syncNTP
```
