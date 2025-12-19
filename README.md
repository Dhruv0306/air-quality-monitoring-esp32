# Air Quality Monitoring System

An ESP32-based environmental monitoring system that measures air quality parameters and weather conditions with RTC-based timing and local data logging.

## Features

- **Air Quality Monitoring**: PM1, PM2.5, PM10 particle measurements using PMS5003 sensor
- **Environmental Sensing**: Temperature and humidity via DHT22 sensor (BME680 optional)
- **Real-Time Clock**: DS3231 RTC module for accurate timekeeping
- **NTP Synchronization**: Automatic time sync via WiFi when available
- **Local Storage**: SD card logging with CSV format
- **Display**: OLED screen for real-time data visualization
- **WiFi Management**: Auto-connect with configuration portal on first boot
- **AQI Calculation**: Air Quality Index based on PM2.5 levels

## Hardware Requirements

### Core Components
- ESP32 Development Board
- PMS5003 Air Quality Sensor
- DHT22 Temperature/Humidity Sensor
- DS3231 RTC Module
- SSD1306 OLED Display (128x64)
- MicroSD Card Module
- MicroSD Card
- BME680 Environmental Sensor (optional)

### Pin Configuration
```
PMS5003:
- RX: GPIO 16 (RX2)
- TX: GPIO 17 (TX2)

DHT22:
- Data: GPIO 14

SD Card:
- CS: GPIO 5

I2C Devices (DS3231 RTC, BME680, OLED):
- SDA: GPIO 21
- SCL: GPIO 22
```

## Software Dependencies

### Arduino Libraries
```cpp
#include <PMS5003.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Adafruit_BME680.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <DHT_U.h>
#include <DS3231.h>
#include <TimeLib.h>
#include <SD.h>
```

## Configuration

### Device Identification
```cpp
#define S1 "AU_PMS_CAPSTONE_"  // Device ID prefix
#define INFLUXDB_BUCKET "E_010"  // Device bucket name
```

### NTP Configuration
```cpp
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 19800  // IST = UTC + 5:30
#define DAYLIGHT_OFFSET_SEC 0
```

### Hardware Pins
```cpp
#define SD_CS 5
#define RX2 16
#define TX2 17
#define DHTPIN 14
#define DHTTYPE DHT22
```

## Installation

1. **Hardware Setup**
   - Connect all sensors according to pin configuration
   - Insert formatted microSD card
   - Power the ESP32 via USB or external supply

2. **Software Setup**
   - Install Arduino IDE
   - Install ESP32 board package
   - Install required libraries via Library Manager
   - Update configuration parameters in code
   - Upload code to ESP32

3. **Initial Configuration**
   - On first boot, device creates WiFi hotspot "ESP32-Time-Setup"
   - Connect to configure WiFi credentials
   - Device will auto-connect on subsequent boots
   - RTC will sync with NTP time when WiFi is available

## First Boot Guide

### Step 1: Power On
- Connect ESP32 to power via USB or external supply
- Open Serial Monitor (9600 baud) to view status messages
- Device will display "First boot detected" if no WiFi credentials stored

### Step 2: WiFi Configuration
1. Device creates hotspot "ESP32-Time-Setup" (no password)
2. Connect your phone/computer to this hotspot
3. Browser should auto-open configuration page, or navigate to `192.168.4.1`
4. Select your WiFi network and enter password
5. Click "Save" - device will restart and connect to your WiFi

### Step 3: Time Synchronization
- If WiFi connects successfully, device syncs time from NTP server
- RTC module is automatically updated with correct time
- Display shows "Time synced from NTP" on successful sync
- If WiFi fails, device uses RTC time (may need manual setting)

### Step 4: Verify Operation
- OLED display shows sensor readings and "Using RTC Time" status
- Check Serial Monitor for "Data logged using RTC time" messages
- SD card should contain new CSV file with current date
- Display alternates between sensor data and AQI every 5 cycles

## Operation

### Data Collection
- Reads DHT22 sensor continuously for temperature/humidity
- PMS5003 data collected every 2 seconds
- Uses DS3231 RTC for accurate timekeeping
- Automatic NTP sync every 24 hours when WiFi available
- Calculates AQI based on PM2.5 levels
- Fixed GPS coordinates (23.038126, 72.552605) when GPS not available

### Data Storage
- **Local**: CSV files on SD card (daily files)
- **Display**: Live data on OLED screen with alternating views

### File Management
- Daily CSV files named: `AU_PMS_CAPSTONE_YYYY-MM-DD.csv`
- Files created automatically based on RTC date
- Continuous data logging throughout operation

### CSV Data Format
```
DateTime,Temperature,Humidity,Pressure,Gas,Altitude,PM1,PM2.5,PM10,
pm1atm,pm2.5atm,pm10atm,c_300,c_500,c_1000,c_2500,c_5000,c_10000,
pms_temp,pms_humidity,pms_formaldihyde,Latitude,Longitude,Altitude_GPS,Satellite
```

**Note**: Pressure, Gas, and Altitude fields show 9999 when BME680 is not available. GPS fields show fixed coordinates when GPS module is not present.

## AQI Categories

| PM2.5 (μg/m³) | AQI Level | Description |
|---------------|-----------|-------------|
| 0-30 | 1 | Good |
| 30-60 | 2 | Satisfactory |
| 60-90 | 3 | Moderate |
| 90-120 | 4 | Poor |
| 120-250 | 5 | Very Poor |
| 250+ | 6 | Severe |

## Troubleshooting

### Common Issues
1. **WiFi Connection Failed**: Check credentials, use configuration portal
2. **RTC Time Invalid**: Ensure DS3231 module is connected and powered
3. **SD Card Error**: Verify card format (FAT32) and connections
4. **Sensor Reading Error**: Check wiring and power supply
5. **NTP Sync Failed**: Verify WiFi connectivity for time synchronization

### Status Indicators
- Monitor serial output (9600 baud) for diagnostic information
- OLED display alternates between sensor readings and AQI display
- Display shows "Using RTC Time" when operating with RTC
- "First boot detected" indicates WiFi setup needed
- "WiFi connected" confirms successful network connection
- "NTP sync failed" means time sync unsuccessful but RTC still works

## Maintenance

- **Daily**: Check OLED display for current readings
- **Weekly**: Verify SD card storage and RTC accuracy
- **Monthly**: Clean sensors and check connections
- **Quarterly**: Update firmware and replace RTC battery if needed

## Code Architecture

### System Overview
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   ESP32 MCU     │    │   Sensors       │    │   Storage       │
│                 │    │                 │    │                 │
│ • WiFi Manager  │◄──►│ • PMS5003       │    │ • SD Card       │
│ • NTP Client    │    │ • DHT22         │    │ • CSV Files     │
│ • RTC Manager   │    │ • DS3231 RTC    │◄──►│ • Daily Logs    │
│ • Display Ctrl  │    │ • BME680 (opt)  │    │                 │
│                 │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
           │                                            │
           ▼                                            ▼
┌─────────────────┐                          ┌─────────────────┐
│   OLED Display  │                          │   Serial Debug  │
│                 │                          │                 │
│ • Sensor Data   │                          │ • Status Info   │
│ • AQI Status    │                          │ • Error Logs    │
│ • Time Display  │                          │ • Data Output   │
└─────────────────┘                          └─────────────────┘
```

### Code Flow Diagram
```
┌─────────────┐
│   SETUP()   │
└──────┬──────┘
       │
       ▼
┌─────────────────────────────────────────────────────────────┐
│ 1. Initialize Hardware (Serial, I2C, WiFi, Display)        │
│ 2. Check WiFi Credentials → Setup Portal if First Boot     │
│ 3. Validate RTC → Load System Time if Valid                │
│ 4. Connect WiFi → Sync NTP → Update RTC                    │
│ 5. Initialize Sensors (PMS5003, DHT22, BME680)             │
│ 6. Initialize SD Card for Data Logging                     │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────┐          ┌─────────────────────────────────────┐
│   LOOP()    │◄─────────│ Main Program Loop (Continuous)     │
└──────┬──────┘          └─────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────────────────────────┐
│ 1. maintainWiFi() → Check Connection → NTP Sync if Needed  │
│ 2. periodicNtpSync() → 24hr Sync Check                     │
│ 3. Set Fixed GPS Coordinates (23.038126, 72.552605)        │
│ 4. readPMSSensor() → Every 2 seconds                       │
│ 5. checkFileExists() → Create Daily CSV if New Day        │
│ 6. readSensors() → DHT22 Temperature/Humidity              │
│ 7. calculateAQI() → Based on PM2.5 Levels                 │
│ 8. getRTCDateTime() → Current Timestamp                    │
│ 9. updateDisplay() → OLED Every 3 seconds                  │
│ 10. updateAQIDisplay() → OLED Every 5 seconds              │
│ 11. logDataSdCard() → Append to CSV File                   │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          └─── Loop Continues
```

### Time Management Flow
```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ First Boot  │    │ Normal Boot │    │ Running     │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       ▼                  ▼                  ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ WiFi Setup  │    │ Auto Connect│    │ Maintain    │
│ Portal      │    │ to Saved    │    │ Connection  │
│             │    │ Network     │    │             │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       ▼                  ▼                  ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ NTP Sync    │    │ Load RTC    │    │ Periodic    │
│ → Update    │    │ Time First  │    │ NTP Sync    │
│ RTC         │    │ → NTP Sync  │    │ (24 hours)  │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       └──────────────────┼──────────────────┘
                          ▼
                   ┌─────────────┐
                   │ Use RTC for │
                   │ All Timing  │
                   │ Operations  │
                   └─────────────┘
```

### Data Logging Process
```
┌─────────────┐
│ New Day?    │
└──────┬──────┘
       │ Yes
       ▼
┌─────────────────────────────────────┐
│ Create New CSV File:                │
│ AU_PMS_CAPSTONE_YYYY-MM-DD.csv      │
│ With Headers                        │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ Collect Sensor Data:                │
│ • DateTime (RTC)                    │
│ • Temperature, Humidity (DHT22)     │
│ • PM1, PM2.5, PM10 (PMS5003)       │
│ • Particle Counts (PMS5003)        │
│ • Environmental Data (PMS5003)     │
│ • Fixed GPS Coordinates             │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ Format CSV Row:                     │
│ DateTime,Temp,Humidity,Pressure,... │
│ 25-01-2025 14:30:15,23.5,65.2,...  │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ Append to SD Card File              │
│ • Open in append mode               │
│ • Write data row                    │
│ • Close file                        │
└─────────────────────────────────────┘
```

### AQI Calculation Logic
```
┌─────────────┐
│ Read PM2.5  │
│ from PMS    │
└──────┬──────┘
       │
       ▼
┌─────────────────────────────────────┐
│ PM2.5 Range Check:                  │
│                                     │
│ 0-30 μg/m³    → AQI 1 (Good)       │
│ 30-60 μg/m³   → AQI 2 (Satisfactory)│
│ 60-90 μg/m³   → AQI 3 (Moderate)   │
│ 90-120 μg/m³  → AQI 4 (Poor)       │
│ 120-250 μg/m³ → AQI 5 (Very Poor)  │
│ 250+ μg/m³    → AQI 6 (Severe)     │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│ Display AQI Status:                 │
│ • Numeric Level (1-6)               │
│ • Text Description                  │
│ • Update OLED Display               │
│ • Log to CSV File                   │
└─────────────────────────────────────┘
```

### Display Management
```
┌─────────────┐    ┌─────────────┐
│ Main Loop   │    │ Timer Check │
│ Running     │    │ Every 3s    │
└──────┬──────┘    └──────┬──────┘
       │                  │
       └──────────────────┘
                          │
                          ▼
┌─────────────────────────────────────┐
│ updateDisplay("Using RTC Time"):    │
│ • Status Message                    │
│ • Temperature: XX.X °C              │
│ • Humidity: XX.X %                  │
│ • PM1: XX.X μg/m³                   │
│ • PM2.5: XX.X μg/m³                 │
│ • PM10: XX.X μg/m³                  │
└─────────────────┬───────────────────┘
                  │
                  ▼
┌─────────────┐    ┌─────────────┐
│ Timer Check │    │ AQI Display │
│ Every 5s    │    │ Update      │
└──────┬──────┘    └──────┬──────┘
       │                  │
       └──────────────────┘
                          │
                          ▼
┌─────────────────────────────────────┐
│ updateAQIDisplay():                 │
│ • Current Date/Time (Large Font)    │
│ • "PM2.5 based Air Quality"         │
│ • AQI Status (Good/Poor/etc.)       │
└─────────────────────────────────────┘
```

## License

This project is open source. Modify and distribute as needed for educational and research purposes.