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

### Arduino Libraries (Install via Library Manager)
```cpp
// Core sensor libraries
#include <PMS5003.h>          // Air quality sensor
#include <DHT.h>              // Temperature/humidity sensor
#include <DHT_U.h>            // DHT unified sensor library
#include <DS3231.h>           // Real-time clock module

// Display and storage
#include <Adafruit_SSD1306.h> // OLED display
#include <Adafruit_BME680.h>  // Optional environmental sensor
#include <SD.h>               // SD card storage

// Networking and time
#include <WiFi.h>             // ESP32 WiFi
#include <WiFiManager.h>      // Easy WiFi setup
#include <TimeLib.h>          // Time management
```

**Installation Steps**:
1. Open Arduino IDE
2. Go to Tools → Manage Libraries
3. Search and install each library listed above
4. Restart Arduino IDE after installation

## Configuration

### Device Identification (MUST CHANGE)
```cpp
#define S1 "AU_PMS_CAPSTONE_"  // CHANGE THIS: Device ID prefix for file naming
#define INFLUXDB_BUCKET "E_010"  // CHANGE THIS: Unique device bucket ID
#define S0 "O_015"              // CHANGE THIS: Short device name for CSV files
```
**Important**: Update these values for each device deployment to ensure unique identification.

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
│ • WiFi Manager  │◄──►│ • PMS5003 (Air) │    │ • SD Card       │
│ • NTP Client    │    │ • DHT22 (Temp)  │    │ • CSV Files     │
│ • RTC Manager   │    │ • DS3231 (RTC)  │◄──►│ • Daily Logs    │
│ • Display Ctrl  │    │ • BME680 (Opt)  │    │ • Auto Headers  │
│ • AQI Calculator│    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
           │                     │                       │
           ▼                     ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   OLED Display  │    │   WiFi Portal   │    │   Serial Debug  │
│                 │    │                 │    │                 │
│ • Live Readings │    │ • First Boot    │    │ • Status Info   │
│ • AQI Status    │    │ • "ESP32-Setup" │    │ • Error Logs    │
│ • Date/Time     │    │ • 192.168.4.1   │    │ • Data Preview  │
│ • Auto Refresh  │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### Startup and Main Loop Flow
```
┌─────────────┐
│   SETUP()   │ ← Runs once on power-on/reset
└──────┬──────┘
       │
       ▼
┌─────────────────────────────────────────────────────────────┐
│ HARDWARE INITIALIZATION                                     │
│ 1. Serial (9600 baud) + I2C (400kHz) + OLED Display       │
│ 2. Check WiFi Credentials → Create "ESP32-Time-Setup"      │
│    hotspot if first boot (192.168.4.1)                    │
│ 3. Validate RTC time → Load if valid (year >= 2024)       │
│ 4. Auto-connect WiFi → NTP sync → Update RTC              │
│ 5. Initialize sensors: PMS5003, DHT22, (BME680 optional)  │
│ 6. Initialize SD card → Ready for CSV logging             │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────┐          ┌─────────────────────────────────────┐
│   LOOP()    │◄─────────│ CONTINUOUS OPERATION                │
└──────┬──────┘          │ Runs forever after setup           │
       │                 └─────────────────────────────────────┘
       ▼
┌─────────────────────────────────────────────────────────────┐
│ TIMED OPERATIONS (Non-blocking with millis() timing)       │
│                                                             │
│ Every 1s:  WiFi maintenance + GPS coordinates              │
│ Every 2s:  Read PMS5003 air quality sensor                │
│ Every 3s:  Update OLED with sensor readings               │
│ Every 5s:  Update OLED with AQI display (alternating)     │
│ Every 10s: Log all data to SD card CSV file               │
│ Every 24h: Automatic NTP time sync (if WiFi connected)    │
│ Daily:     Create new CSV file when date changes          │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          └─── Loop Continues Indefinitely
```

### Time Management Strategy
```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ First Boot  │    │ Normal Boot │    │ Running     │
│ (No WiFi)   │    │ (Has WiFi)  │    │ (24/7)      │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       ▼                  ▼                  ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ Create WiFi │    │ Auto Connect│    │ Monitor     │
│ Hotspot:    │    │ to Saved    │    │ Connection  │
│"ESP32-Setup"│    │ Network     │    │ Status      │
│192.168.4.1  │    │             │    │             │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       ▼                  ▼                  ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ User Setup  │    │ Check RTC   │    │ Auto NTP    │
│ → NTP Sync  │    │ → NTP Sync  │    │ Sync Every  │
│ → Save RTC  │    │ → Save RTC  │    │ 24 Hours    │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       └──────────────────┼──────────────────┘
                          ▼
                   ┌─────────────┐
                   │ RTC Keeps   │
                   │ Accurate    │
                   │ Time Always │
                   │ (Battery)   │
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

### OLED Display Management (128x64 pixels)
```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ Main Loop   │    │ 3 Second    │    │ 5 Second    │
│ Continuous  │    │ Timer       │    │ Timer       │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       └──────────────────┼──────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│ ALTERNATING DISPLAY MODES                           │
│                                                     │
│ MODE 1 (Every 3s): SENSOR READINGS                 │
│ ┌─────────────────────────────────────────────────┐ │
│ │ Using RTC Time                                  │ │
│ │ Temperature: 23.5 °C                           │ │
│ │ Humidity: 65.2 %                               │ │
│ │ PM1: 12.3 μg/m³                                │ │
│ │ PM2.5: 25.7 μg/m³                              │ │
│ │ PM10: 35.1 μg/m³                               │ │
│ └─────────────────────────────────────────────────┘ │
│                                                     │
│ MODE 2 (Every 5s): AIR QUALITY INDEX               │
│ ┌─────────────────────────────────────────────────┐ │
│ │ 25-01-2025 14:30:15                            │ │
│ │                                                 │ │
│ │ PM2.5 based Air                                 │ │
│ │ Quality: Good                                   │ │
│ └─────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

## Developer Guide

### Code Structure Overview
The code is organized into clear functional sections with comprehensive comments:

- **Library Includes**: All required Arduino libraries with installation notes
- **Hardware Definitions**: Pin assignments and sensor configurations
- **Device Configuration**: Unique identifiers that MUST be changed per device
- **Global Variables**: Sensor data, timing, and system status variables
- **Utility Functions**: Date/time calculations and validation
- **RTC Functions**: Real-time clock management and formatting
- **Sensor Functions**: Data reading from PMS5003, DHT22, and BME680
- **Display Functions**: OLED screen updates and alternating displays
- **File Functions**: SD card CSV file creation and data logging
- **WiFi Functions**: Connection management and NTP synchronization
- **Main Functions**: setup() and loop() with timing controls

### Key Function Categories

#### TIME MANAGEMENT
- `syncTimeFromNTP()`: Get accurate time from internet
- `syncRTCFromSystemTime()`: Save internet time to RTC hardware
- `getRTCDateTime()`: Read formatted time from RTC
- `isRTCValid()`: Validate RTC has reasonable time

#### SENSOR OPERATIONS
- `readSensors()`: DHT22 temperature and humidity
- `readPMSSensor()`: PMS5003 air quality data
- `calculateAQI()`: Convert PM2.5 to Air Quality Index

#### DATA MANAGEMENT
- `checkFileExists()`: Create daily CSV files with headers
- `logDataSdCard()`: Append sensor data to CSV files
- `updateDisplay()`: Show live readings on OLED
- `updateAQIDisplay()`: Show air quality status

#### NETWORK OPERATIONS
- `setupWiFiFirstTime()`: Create configuration portal
- `maintainWiFi()`: Keep connection alive and sync time
- `periodicNtpSync()`: Daily time corrections

### Timing Strategy
The system uses non-blocking timing with `millis()` to handle multiple tasks:

```cpp
// Example timing pattern used throughout the code
static unsigned long lastAction = 0;
if (millis() - lastAction >= INTERVAL) {
    performAction();
    lastAction = millis();
}
```

### Error Handling
- **WiFi Failures**: System continues with RTC time
- **Sensor Errors**: Invalid readings logged as error values
- **SD Card Issues**: Errors logged to Serial Monitor
- **RTC Problems**: Falls back to system time when possible

### Customization Points
1. **Device Identity**: Update S0, S1, INFLUXDB_BUCKET defines
2. **Timing Intervals**: Modify timing constants in loop()
3. **GPS Coordinates**: Change fixed latitude/longitude values
4. **AQI Thresholds**: Adjust PM2.5 ranges in calculateAQI()
5. **Display Content**: Modify updateDisplay() functions

### Debugging Tips
- Monitor Serial output at 9600 baud for status messages
- Check OLED display for real-time system status
- Verify SD card files are being created with correct timestamps
- Use WiFi portal (192.168.4.1) for network troubleshooting

## License

This project is open source. Modify and distribute as needed for educational and research purposes.

---

## Quick Reference

### Essential Files
- `PMS_DHT_2025_01_06.ino`: Main Arduino code with comprehensive comments
- `README.md`: This documentation file
- CSV files: Auto-created daily on SD card as `[S0]_[S1]YYYY-MM-DD.csv`

### Critical Configuration
```cpp
// CHANGE THESE FOR EACH DEVICE:
#define S0 "O_015"              // Short device name
#define S1 "AU_PMS_CAPSTONE_"   // Device ID prefix  
#define INFLUXDB_BUCKET "E_010" // Unique bucket ID
```

### Default Credentials
- **WiFi Hotspot**: "ESP32-Time-Setup" (no password)
- **Configuration Portal**: 192.168.4.1
- **Serial Monitor**: 9600 baud
- **I2C Address**: 0x3C (OLED display)

### File Naming Convention
- **Format**: `[S0]_[S1]YYYY-MM-DD.csv`
- **Example**: `O_015_AU_PMS_CAPSTONE_2025-01-25.csv`
- **Location**: Root directory of SD card
- **Headers**: Auto-added to new files

### Troubleshooting Checklist
1. ✅ Power supply stable (USB or 5V external)
2. ✅ SD card formatted as FAT32
3. ✅ All sensor connections secure
4. ✅ RTC battery installed (CR2032)
5. ✅ WiFi credentials configured via portal
6. ✅ Serial Monitor shows "Setup complete"
7. ✅ OLED display shows live readings
8. ✅ CSV files appear on SD card with data
