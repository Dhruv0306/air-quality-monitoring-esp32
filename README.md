# Air Quality Monitoring System

An ESP32-based environmental monitoring system that measures air quality parameters, weather conditions, and GPS location data with cloud connectivity and local data logging.

## Features

- **Air Quality Monitoring**: PM1, PM2.5, PM10 particle measurements using PMS5003 sensor
- **Environmental Sensing**: Temperature, humidity, pressure, and gas detection via BME680/DHT22
- **GPS Tracking**: Real-time location and altitude data
- **Cloud Integration**: Data upload to InfluxDB Cloud
- **Local Storage**: SD card logging with CSV format
- **Display**: OLED screen for real-time data visualization
- **WiFi Management**: Auto-connect with fallback configuration portal
- **FTP Upload**: Automated daily file transfer
- **AQI Calculation**: Air Quality Index based on PM2.5 levels

## Hardware Requirements

### Core Components
- ESP32 Development Board
- PMS5003 Air Quality Sensor
- BME680 Environmental Sensor (or DHT22 as fallback)
- GPS Module (compatible with TinyGPS++)
- SSD1306 OLED Display (128x64)
- MicroSD Card Module
- MicroSD Card

### Pin Configuration
```
GPS Module:
- RX: GPIO 26
- TX: GPIO 27

PMS5003:
- RX: GPIO 16 (RX2)
- TX: GPIO 17 (TX2)

DHT22:
- Data: GPIO 14

SD Card:
- CS: GPIO 5

I2C Devices (BME680, OLED):
- SDA: GPIO 21
- SCL: GPIO 22
```

## Software Dependencies

### Arduino Libraries
```cpp
#include <PMS5003.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <InfluxDbClient.h>
#include <WiFiManager.h>
#include <Adafruit_BME680.h>
#include <Adafruit_SSD1306.h>
#include <DHT_U.h>
#include <FTPClient.h>
```

## Configuration

### WiFi Settings
```cpp
const char* WIFI_SSID = "Yap";
const char* WIFI_PASSWORD = "letstest";
```

### InfluxDB Configuration
```cpp
#define INFLUXDB_URL "http://103.233.171.34:8086"
#define INFLUXDB_TOKEN "your_token_here"
#define INFLUXDB_ORG "6ecfc9383074a24b"
#define INFLUXDB_BUCKET "E_010"
```

### Device Identification
```cpp
#define S1 "PMS_MACX_"  // Device ID prefix
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
   - On first boot, device creates WiFi hotspot "E_010" (password: "12345678")
   - Connect to configure WiFi credentials
   - Device will auto-connect on subsequent boots

## Operation

### Data Collection
- Reads sensors every GPS update cycle
- PMS5003 data collected every 5th cycle for efficiency
- Converts UTC GPS time to IST (Indian Standard Time)
- Calculates AQI based on PM2.5 levels

### Data Storage
- **Local**: CSV files on SD card (daily files)
- **Cloud**: Real-time upload to InfluxDB
- **Display**: Live data on OLED screen

### File Management
- Daily CSV files named: `PMS_MACX_E_010_YYYYMMDD.csv`
- Automatic FTP upload at 18:25 IST
- System restarts at 03:20, 09:20, 15:20, 21:20 IST

### CSV Data Format
```
DateTime,Temperature,Humidity,Pressure,Gas,Altitude,PM1,PM2.5,PM10,
pm1atm,pm2.5atm,pm10atm,c_300,c_500,c_1000,c_2500,c_5000,c_10000,
pms_temp,pms_humidity,pms_formaldihyde,Latitude,Longitude,Altitude_GPS,Satellite
```

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
2. **GPS No Fix**: Ensure outdoor operation with clear sky view
3. **SD Card Error**: Verify card format (FAT32) and connections
4. **Sensor Reading Error**: Check wiring and power supply
5. **InfluxDB Upload Failed**: Verify network connectivity and credentials

### LED Indicators
- Monitor serial output (9600 baud) for diagnostic information
- OLED display shows current readings and connection status

## Maintenance

- **Daily**: Check OLED display for current readings
- **Weekly**: Verify SD card storage and file uploads
- **Monthly**: Clean sensors and check connections
- **Quarterly**: Update firmware and calibrate sensors

## License

This project is open source. Modify and distribute as needed for educational and research purposes.