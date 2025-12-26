// ================================
// REQUIRED LIBRARIES
// ================================
// Install these libraries through Arduino IDE Library Manager:
// - PMS5003 (for air quality sensor)
// - Adafruit BME680, SSD1306, Sensor libraries
// - DHT sensor library
// - DS3231 (RTC module)
// - WiFiManager (for easy WiFi setup)
// - TimeLib (for time management)
#include <PMS5003.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_BME680.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <time.h>
#include <WiFiManager.h>
#include <DS3231.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// ================================
// HARDWARE PIN DEFINITIONS
// ================================
// Define which GPIO pins connect to which sensors/modules
// Change these if you wire components to different pins
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)
#define SD_CS 5
#define RX2 16
#define TX2 17
#define DHTPIN 14 // Digital pin connected to the DHT sensor
// #define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302)
// #define DHTTYPE    DHT21     // DHT 21 (AM2301)

// ================================
// DEVICE CONFIGURATION
// ================================
#define INFLUXDB_BUCKET "E_010" // CHANGE THIS: Unique device bucket ID (e.g. JDH_IITJ, ACRL_014)
#define S1 "AU_PMS_CAPSTONE_"   // CHANGE THIS: Device ID prefix for file naming
#define S0 "AGCP_009"           // CHANGE THIS: Short device name for CSV files
// #define INFLUXDB_MEASUREMENT "atmosphere_data"
// #define WIFI_CONNECT_TIMEOUT 60000 // 1 minute
// #define TZ_INFO "IST-5:30"

// ================================
// NETWORK TIME PROTOCOL (NTP) SETUP
// ================================
// NTP automatically syncs time from internet when WiFi is available
// Adjust GMT_OFFSET_SEC for your timezone (19800 = IST/India +5:30)
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 19800 // IST = UTC + 5:30
#define DAYLIGHT_OFFSET_SEC 0

// ================================
// DISPLAY CONFIGURATION
// ================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// ================================
// GLOBAL VARIABLES
// ================================
// These variables store sensor readings, time data, and system status
// Most are updated continuously in the main loop
String Date;
float rem = 0;
int i = 0;
int i2 = 0;
int Hour, Minute, Second, Day, Month, Year, pm25_aqi, oldDay, currentDate, oldDate;
float humidity, pressure, gas, altitudeBme, val1, val2, val3, val4, val5, val6, c_300, c_500, c_1000, c_2500, c_5000, c_10000, pms_temp, pms_h;
int rtci = 0;
float temperature = 0;
int bmei = 0;
float latitude = 23.038126;
float longitude = 72.552605;
float pms_fld;
char output[256];
int utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond;
int d, m, y, h, mm;

String dataMessage, lati, longi, atlt, noS, dateTime, dateTimeIf, aqi, filename, nowDay, olddate, f;

// WiFi and Time Management Variables
bool wifiConnected = false;                      // Tracks current WiFi connection status
bool rtcSynced = false;                          // True when RTC has valid time
bool ntpSynced = false;                          // True when NTP sync was successful
bool wifiConfigured = false;                     // True when WiFi credentials are saved
bool bmeAvailable = false;                       // True if BME680 sensor is connected
WiFiManager wm;                                  // Handles WiFi setup portal
time_t lastNtpSyncEpoch = 0;                     // When last NTP sync occurred
String lastFileDate = "";                        // Tracks current day for file creation
unsigned long lastWiFiAttempt = 0;               // Prevents WiFi spam attempts
const unsigned long WIFI_RETRY_INTERVAL = 30000; // Wait 30 seconds between WiFi retries
bool ntpSyncedThisSession = false;               // Prevents multiple NTP syncs per connection

// ================================
// HARDWARE OBJECTS
// ================================
// These objects represent physical sensors and modules
// They provide methods to read data and control hardware
Adafruit_BME680 bme; // I2C
GuL::PMS5003 pms(Serial2);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT_Unified dht(DHTPIN, DHTTYPE);
DS3231 rtcModule;

// ================================
// UTILITY FUNCTIONS
// ================================

/**
 * UTILITY: Calculate how many days are in a specific month
 * Handles leap years correctly for February
 * @param m Month (1=January, 2=February, ..., 12=December)
 * @param y Full year (e.g., 2025)
 * @return Number of days (28, 29, 30, or 31)
 */
int daysInMonth(int m, int y)
{
  // April, June, September, November have 30 days
  if (m == 4 || m == 6 || m == 9 || m == 11)
    return 30;
  // February depends on leap year
  if (m == 2)
    return (isLeapYear(y) ? 29 : 28);
  // All other months have 31 days
  return 31;
}

/**
 * UTILITY: Determine if a year is a leap year
 * Leap years have 366 days (February has 29 days instead of 28)
 * Rules: Divisible by 4, except century years must be divisible by 400
 * @param y Full year to check (e.g., 2024)
 * @return true if leap year, false if regular year
 */
bool isLeapYear(int y)
{
  // Not divisible by 4 = not leap year
  if (y % 4 != 0)
    return false;
  // Divisible by 4 but not 100 = leap year
  if (y % 100 != 0)
    return true;
  // Divisible by 400 = leap year
  if (y % 400 != 0)
    return false;
  return true;
}

/**
 * TIME SYNC: Get accurate time from internet (NTP server)
 * This function connects to a time server and updates the ESP32's internal clock
 * Only works when WiFi is connected to internet
 * Automatically adjusts for Indian Standard Time (IST = UTC+5:30)
 * @return true if time was successfully retrieved, false if failed
 */
bool syncTimeFromNTP()
{
  struct tm timeinfo;

  // Configure NTP with IST timezone (GMT+5:30)
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  // Attempt to get time from NTP server (10 second timeout)
  if (!getLocalTime(&timeinfo, 10000))
  {
    Serial.println("NTP sync failed");
    // Display failure message on OLED
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("NTP sync failed");
    display.display();
    return false;
  }

  Serial.println("Time synced from NTP");
  // Display success message on OLED
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Time synced from NTP");
  display.display();
  return true;
}

/**
 * FIRST BOOT: Create WiFi setup portal for new devices
 * When device has no saved WiFi credentials, it creates a hotspot called "ESP32-Time-Setup"
 * Users connect to this hotspot and enter their WiFi network details through a web page
 * After setup, device will automatically connect to the saved network on future boots
 * @return true if user successfully configured WiFi, false if setup was skipped/failed
 */
bool setupWiFiFirstTime()
{
  // Set configuration portal timeout to 2 minutes
  wm.setConfigPortalTimeout(120);
  // Set connection attempt timeout to 5 seconds
  wm.setConnectTimeout(5);
  // Enable debug output for troubleshooting
  wm.setDebugOutput(true);

  // Create WiFi hotspot "ESP32-Time-Setup" for configuration
  String S_name = String(S0) + " " + "ESP32-Time-Setup";
  if (!wm.autoConnect(S_name.c_str()))
  {
    Serial.println("WiFi setup skipped or failed");
    return false;
  }

  Serial.println("WiFi credentials saved");
  return true;
}

/**
 * RTC VALIDATION: Check if the Real-Time Clock has sensible time
 * The RTC module keeps time even when ESP32 is powered off (has backup battery)
 * This function verifies the stored time isn't corrupted or uninitialized
 * Checks: Year >= 2024, month 1-12, day 1-31, hour 0-23
 * @return true if RTC time looks correct, false if time seems invalid
 */
bool isRTCValid()
{
  bool centuryFlag;
  bool h12Flag, pmFlag;

  // Get current RTC values
  int y = rtcModule.getYear(); // Years since 2000
  int m = rtcModule.getMonth(centuryFlag);
  int d = rtcModule.getDate();
  int h = rtcModule.getHour(h12Flag, pmFlag);

  // Validate: Year >= 2024 (24 since 2000) and reasonable date/time ranges
  return (y >= 24 && m >= 1 && m <= 12 && d >= 1 && d <= 31 && h <= 23);
}

// ================================
// RTC FUNCTIONS
// ================================

/**
 * RTC READ: Get current date and time as formatted string
 * Reads the current time from RTC module and formats it for display/logging
 * Format: DD-MM-YYYY HH:MM:SS (e.g., "25-01-2025 14:30:15")
 * Automatically adds leading zeros for single-digit values
 * @return Complete date-time string ready for CSV files or display
 */
String getRTCDateTime()
{
  bool h12Flag, pmFlag;
  // Read current time from RTC module
  int rtcYear = rtcModule.getYear(); // Years since 2000
  int rtcMonth = rtcModule.getMonth(h12Flag);
  int rtcDay = rtcModule.getDate();
  int rtcHour = rtcModule.getHour(h12Flag, pmFlag);
  int rtcMinute = rtcModule.getMinute();
  int rtcSecond = rtcModule.getSecond();

  String dateTime = "";

  // Format date: DD-MM-YYYY (with leading zeros)
  if (rtcDay < 10)
    dateTime += "0";
  dateTime += rtcDay;
  dateTime += "-";
  if (rtcMonth < 10)
    dateTime += "0";
  dateTime += rtcMonth;
  dateTime += "-";
  dateTime += (2000 + rtcYear); // Convert to full year
  dateTime += " ";

  // Format time: HH:MM:SS (with leading zeros)
  if (rtcHour < 10)
    dateTime += "0";
  dateTime += rtcHour;
  dateTime += ":";
  if (rtcMinute < 10)
    dateTime += "0";
  dateTime += rtcMinute;
  dateTime += ":";
  if (rtcSecond < 10)
    dateTime += "0";
  dateTime += rtcSecond;

  return dateTime;
}

/**
 * RTC READ: Get current date only (no time)
 * Used for creating daily CSV files and checking if day has changed
 * Format: DD-MM-YYYY with trailing space (e.g., "25-01-2025 ")
 * @return Date string for file naming and day change detection
 */
String getRTCDate()
{
  bool h12Flag, pmFlag;
  // Read current date from RTC module
  int rtcYear = rtcModule.getYear(); // Years since 2000
  int rtcMonth = rtcModule.getMonth(h12Flag);
  int rtcDay = rtcModule.getDate();

  String nowDay = "";

  // Format date: DD-MM-YYYY (with leading zeros and trailing space)
  if (rtcDay < 10)
    nowDay += "0";
  nowDay += rtcDay;
  nowDay += "-";
  if (rtcMonth < 10)
    nowDay += "0";
  nowDay += rtcMonth;
  nowDay += "-";
  nowDay += (2000 + rtcYear); // Convert to full year
  nowDay += " ";              // Trailing space for consistency

  return nowDay;
}

/**
 * RTC UPDATE: Save internet time to RTC module
 * After getting accurate time from NTP server, this function stores it in the RTC
 * The RTC will keep this time even when ESP32 loses power (backup battery)
 * This ensures accurate timekeeping for data logging
 * Called automatically after successful NTP sync
 */
void syncRTCFromSystemTime()
{
  struct tm timeinfo;
  // Get current system time (should be NTP-synced)
  if (!getLocalTime(&timeinfo))
  {
    return; // Exit if system time not available
  }

  // Update RTC module with system time
  rtcModule.setYear(timeinfo.tm_year - 100); // Convert from years since 1900 to years since 2000
  rtcModule.setMonth(timeinfo.tm_mon + 1);   // Convert from 0-11 to 1-12
  rtcModule.setDate(timeinfo.tm_mday);
  rtcModule.setHour(timeinfo.tm_hour);
  rtcModule.setMinute(timeinfo.tm_min);
  rtcModule.setSecond(timeinfo.tm_sec);

  // Update sync status flags
  rtcSynced = true;
  lastNtpSyncEpoch = now(); // Record when sync occurred
  Serial.println("RTC synced from NTP");
}

// ================================
// SENSOR FUNCTIONS
// ================================

/**
 * SENSOR READ: Get temperature and humidity from DHT22
 * DHT22 is the primary environmental sensor (temperature + humidity)
 * BME680 is optional - if not connected, values are set to 9999 (indicates "no data")
 * Temperature in Celsius, humidity as percentage (0-100%)
 * Updates global variables: temperature, humidity, pressure, gas, altitudeBme
 */
void readSensors()
{
  sensors_event_t event;

  // Read temperature from DHT22 sensor
  dht.temperature().getEvent(&event);
  temperature = event.temperature;

  // Read humidity from DHT22 sensor
  dht.humidity().getEvent(&event);
  humidity = event.relative_humidity;

  // Set BME680 values to 9999 when sensor not available
  // These values indicate "no data" in the CSV output
  // ---- BME680 ----
  if (bmeAvailable && bme.performReading())
  {
    pressure = bme.pressure / 100.0;   // Convert Pa → hPa
    gas = bme.gas_resistance / 1000.0; // Ohms → kOhms
    altitudeBme = bme.readAltitude(SEALEVELPRESSURE_HPA);
  }
  else
  {
    pressure = 9999;
    gas = 9999;
    altitudeBme = 9999;
  }
}

/**
 * SENSOR READ: Get air quality data from PMS5003 sensor
 * PMS5003 measures particulate matter (PM1, PM2.5, PM10) in micrograms per cubic meter
 * Also counts particles by size and provides temperature/humidity/formaldehyde readings
 * Standard vs Atmospheric: Standard = lab conditions, Atmospheric = real-world conditions
 * Updates global variables: val1-val6 (PM values), c_300-c_10000 (particle counts), pms_temp, pms_h, pms_fld
 */
void readPMSSensor()
{
  // Poll sensor for new data
  pms.poll();
  pms.read();

  // Read PM concentrations (Standard conditions)
  val1 = pms.getPM1_STD();   // PM1.0 μg/m³
  val2 = pms.getPM2_5_STD(); // PM2.5 μg/m³
  val3 = pms.getPM10_STD();  // PM10 μg/m³

  // Read PM concentrations (Atmospheric conditions)
  val4 = pms.getPM1_ATM();   // PM1.0 μg/m³ (atmospheric)
  val5 = pms.getPM2_5_ATM(); // PM2.5 μg/m³ (atmospheric)
  val6 = pms.getPM10_ATM();  // PM10 μg/m³ (atmospheric)

  // Read particle counts by size
  c_300 = pms.getCntBeyond300nm();     // Particles > 0.3μm
  c_500 = pms.getCntBeyond500nm();     // Particles > 0.5μm
  c_1000 = pms.getCntBeyond1000nm();   // Particles > 1.0μm
  c_2500 = pms.getCntBeyond2500nm();   // Particles > 2.5μm
  c_5000 = pms.getCntBeyond5000nm();   // Particles > 5.0μm
  c_10000 = pms.getCntBeyond10000nm(); // Particles > 10μm

  // Read environmental data from PMS sensor
  pms_temp = pms.getTemperature();              // Temperature from PMS
  pms_h = pms.getHumidity();                    // Humidity from PMS
  pms_fld = pms.getFormaldehydeConcentration(); // Formaldehyde concentration

  Serial.println("PMS sensor read complete");
}

/**
 * AQI CALCULATION: Convert PM2.5 reading to Air Quality Index
 * AQI is a standardized way to communicate air pollution levels to the public
 * Based on Indian AQI standards using PM2.5 concentration:
 * 0-30 μg/m³ = Good, 30-60 = Satisfactory, 60-90 = Moderate
 * 90-120 = Poor, 120-250 = Very Poor, 250+ = Severe
 * Updates global variables: pm25_aqi (numeric 1-6), aqi (text description)
 */
void calculateAQI()
{
  // Calculate AQI level based on PM2.5 concentration (val2)
  if (val2 >= 0 && val2 < 30)
  {
    pm25_aqi = 1; // Good (0-30 μg/m³)
  }
  else if (val2 >= 30 && val2 < 60)
  {
    pm25_aqi = 2; // Satisfactory (30-60 μg/m³)
  }
  else if (val2 >= 60 && val2 < 90)
  {
    pm25_aqi = 3; // Moderate (60-90 μg/m³)
  }
  else if (val2 >= 90 && val2 < 120)
  {
    pm25_aqi = 4; // Poor (90-120 μg/m³)
  }
  else if (val2 >= 120 && val2 < 250)
  {
    pm25_aqi = 5; // Very Poor (120-250 μg/m³)
  }
  else if (val2 >= 250)
  {
    pm25_aqi = 6; // Severe (250+ μg/m³)
  }
  else
  {
    pm25_aqi = 0; // Error/Invalid reading
  }

  // Convert AQI level to descriptive string
  if (pm25_aqi == 1)
  {
    aqi = "Good";
  }
  else if (pm25_aqi == 2)
  {
    aqi = "Satisfactory";
  }
  else if (pm25_aqi == 3)
  {
    aqi = "Moderate";
  }
  else if (pm25_aqi == 4)
  {
    aqi = "Poor";
  }
  else if (pm25_aqi == 5)
  {
    aqi = "Very Poor";
  }
  else if (pm25_aqi == 6)
  {
    aqi = "Severe";
  }
  else
  {
    aqi = "ERROR"; // Invalid reading
  }
}

// ================================
// DISPLAY FUNCTIONS
// ================================

/**
 * DISPLAY UPDATE: Show current sensor data on OLED screen
 * Updates the small OLED display with live environmental readings
 * Shows: status message, temperature, humidity, PM1/PM2.5/PM10 values
 * Called every 3 seconds to refresh the display
 * @param messge Status text to show at top (e.g., "Using RTC Time")
 */
void updateDisplay(String messge)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);

  // Display status message (e.g., "Using RTC Time")
  display.println(S0);

  // Display environmental readings
  display.print("Temperature: ");
  display.print(temperature);
  display.println(" *C");
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");

  // BME680 readings commented out (sensor not available)
  display.print("Pressure: ");
  display.print(pressure);
  display.println(" hPa");

  // Display PM concentrations (atmospheric conditions)
  display.print("PM1: ");
  display.print(val4);
  display.println(" ug/m^3");
  display.print("PM2.5: ");
  display.print(val5);
  display.println(" ug/m^3");
  display.print("PM10: ");
  display.print(val6);
  display.println(" ug/m^3");

  // GPS coordinates and datetime commented out (space constraints)
  // display.print("LAT: ");
  // display.println(lati);

  display.display(); // Update physical display
}

/**
 * DISPLAY UPDATE: Show Air Quality Index on OLED screen
 * Alternates with sensor display every 5 seconds
 * Shows current date/time in large font and AQI status (Good/Poor/etc.)
 * Provides quick visual indication of air quality for users
 */
void updateAQIDisplay()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);

  // Display current date and time in large font
  display.println(dateTime);

  // Switch to smaller font for AQI info
  display.setTextSize(1);
  display.println();
  display.println("PM2.5 based Air ");
  display.print("Quality: ");
  display.println(aqi); // Display AQI category (Good, Poor, etc.)

  display.display(); // Update physical display
}

// ================================
// FILE FUNCTIONS
// ================================

/**
 * FILE MANAGEMENT: Ensure today's CSV file exists
 * Creates a new CSV file each day with proper headers
 * File naming: [S0]_[S1]YYYY-MM-DD.csv (e.g., "O_015_AU_PMS_CAPSTONE_2025-01-25.csv")
 * Headers include all sensor data columns for proper CSV format
 * Called when day changes to start fresh daily log
 */
void checkFileExists()
{
  bool h12Flag, pmFlag;
  // Get current date from RTC
  int rtcYear = rtcModule.getYear();
  int rtcMonth = rtcModule.getMonth(h12Flag);
  int rtcDay = rtcModule.getDate();

  String date = "";

  // Format date as YYYY-MM-DD for filename
  date += (2000 + rtcYear);
  date += "-";
  if (rtcMonth < 10)
    date += "0";
  date += rtcMonth;
  date += "-";
  if (rtcDay < 10)
    date += "0";
  date += rtcDay;

  // Create filename with device ID prefix
  filename = String(S0) + "_" + String(S1) + String(date) + ".csv";

  // Debug: Print current date components
  Serial.println("Current date: " + String(rtcDay) + "/" + String(rtcMonth) + "/" + String(2000 + rtcYear));

  // Check if file exists
  File file = SD.open("/" + filename);

  if (!file)
  {
    // File doesn't exist, create new file with CSV headers
    Serial.println("File doesn't exist");
    Serial.println("Creating file: " + filename);

    // CSV header with all data columns
    String message = "DateTime,Temperature,Humidity,Pressure,Gas,Altitude,PM1,PM2.5,PM10,pm1atm,pm2.5atm,pm10atm,c_300,c_500,c_1000,c_2500,c_5000,c_10000,pms_temp,pms_humidity,pms_formaldihyde,Latitude,Longitude,Altitude_GPS,Satellite\r\n";

    // Create and write header to new file
    File file = SD.open("/" + filename, FILE_WRITE);
    if (!file)
    {
      Serial.println("Failed to open file for writing");
      return;
    }
    if (file.print(message))
    {
      Serial.println("CSV file created with headers");
    }
    else
    {
      Serial.println("Failed to write headers");
    }
    file.close();
  }
  else
  {
    // File already exists
    Serial.println("File already exists: " + filename);
  }
  file.close();
}

/**
 * DATA LOGGING: Save all sensor readings to SD card
 * Appends one row of data to today's CSV file every 10 seconds
 * Includes: timestamp, temperature, humidity, all PM values, particle counts,
 * GPS coordinates (fixed), and environmental data from PMS sensor
 * Data is preserved even if device loses power - stored on SD card
 */
void logDataSdCard()
{
  bool h12Flag, pmFlag;
  // Get current date from RTC for filename
  int rtcYear = rtcModule.getYear();
  int rtcMonth = rtcModule.getMonth(h12Flag);
  int rtcDay = rtcModule.getDate();

  String date = "";

  // Format date as YYYY-MM-DD for filename
  date += (2000 + rtcYear);
  date += "-";
  if (rtcMonth < 10)
    date += "0";
  date += rtcMonth;
  date += "-";
  if (rtcDay < 10)
    date += "0";
  date += rtcDay;

  // Generate filename and open for append
  filename = String(S0) + "_" + String(S1) + String(date) + ".csv";
  File file = SD.open("/" + filename, "a+"); // Append mode
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Format data row with all sensor readings
  char dataMessage[1024];
  snprintf(dataMessage, sizeof(dataMessage),
           "%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%s,%s,%s,%s\r\n",
           dateTime.c_str(),                          // Current date/time
           temperature,                               // DHT22 temperature
           humidity,                                  // DHT22 humidity
           pressure,                                  // BME680 pressure (9999 if not available)
           gas,                                       // BME680 gas (9999 if not available)
           altitudeBme,                               // BME680 altitude (9999 if not available)
           (float)val1, (float)val2, (float)val3,     // PM standard concentrations
           (float)val4, (float)val5, (float)val6,     // PM atmospheric concentrations
           (float)c_300, (float)c_500, (float)c_1000, // Particle counts
           (float)c_2500, (float)c_5000, (float)c_10000,
           (float)pms_temp, (float)pms_h, (float)pms_fld, // PMS environmental data
           lati.c_str(), longi.c_str(),                   // GPS coordinates (fixed)
           atlt.c_str(), noS.c_str());                    // GPS altitude and satellites

  // Write data to file
  if (file.print(dataMessage))
  {
    Serial.print("Data logged: ");
    Serial.println(dataMessage);
  }
  else
  {
    Serial.println("Error writing to SD card");
  }
  file.close();
}

// ================================
// SETUP FUNCTION
// ================================
/**
 * SYSTEM STARTUP: Initialize all hardware and establish connections
 * This function runs once when ESP32 powers on or resets
 * Sets up: Serial communication, I2C bus, WiFi, sensors, display, SD card
 * Handles first-boot WiFi setup and time synchronization
 * If anything fails during setup, error messages are printed to Serial Monitor
 */
void setup()
{
  // Initialize serial communications
  Serial.begin(9600);                        // Debug serial
  Serial2.begin(9600, SERIAL_8N1, RX2, TX2); // PMS5003 sensor serial
  Serial.println("Air Quality Monitor - Starting Setup");

  // Initialize I2C bus for RTC, BME680, and OLED
  Wire.begin();
  Wire.setClock(400000); // Set I2C frequency to 400kHz for better performance

  // Set WiFi to station mode (required for WiFiManager)
  WiFi.mode(WIFI_STA);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    while (true)
      ; // Halt if display fails
  }
  Serial.println("OLED display initialized");

  // WiFi and NTP Time Setup
  if (WiFi.SSID().length() == 0)
  {
    // First boot - no WiFi credentials stored
    Serial.println("First boot detected - starting WiFi setup");
    wifiConfigured = setupWiFiFirstTime();
  }
  else
  {
    // Credentials exist - attempt auto-connect
    WiFi.begin();
  }

  // Load time from RTC if valid
  if (isRTCValid())
  {
    bool centuryFlag;
    bool h12Flag, pmFlag;

    // Set system time from RTC
    setTime(
        rtcModule.getHour(h12Flag, pmFlag),
        rtcModule.getMinute(),
        rtcModule.getSecond(),
        rtcModule.getDate(),
        rtcModule.getMonth(centuryFlag),
        2000 + rtcModule.getYear());
    rtcSynced = true;
    lastNtpSyncEpoch = now();
    Serial.println("RTC time loaded into system");
  }

  // Sync with NTP if WiFi connected
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi connected - attempting NTP sync");

    if (syncTimeFromNTP())
    {
      syncRTCFromSystemTime(); // Update RTC with NTP time
      ntpSynced = true;
      rtcSynced = true;
    }
  }

  // Initialize PMS5003 air quality sensor
  pms.setToPassiveReporting(); // Set to passive mode for controlled readings
  Serial.println("PMS5003 sensor initialized");

  // Initialize DHT22 temperature/humidity sensor
  dht.begin();
  Serial.println("DHT22 sensor initialized");

  // Try to initialize BME680 (optional sensor)
  if (!bme.begin())
  {
    Serial.println("BME680 sensor not found - using DHT22 only");
    bmeAvailable = false;

    // Print DHT22 sensor details for debugging
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    Serial.println(F("------------------------------------"));
    Serial.println(F("Temperature Sensor Details:"));
    Serial.print(F("Sensor Type: "));
    Serial.println(sensor.name);
    Serial.print(F("Driver Ver:  "));
    Serial.println(sensor.version);
    Serial.print(F("Unique ID:   "));
    Serial.println(sensor.sensor_id);
    Serial.print(F("Max Value:   "));
    Serial.print(sensor.max_value);
    Serial.println(F("°C"));
    Serial.print(F("Min Value:   "));
    Serial.print(sensor.min_value);
    Serial.println(F("°C"));
    Serial.print(F("Resolution:  "));
    Serial.print(sensor.resolution);
    Serial.println(F("°C"));
    Serial.println(F("------------------------------------"));

    // Print humidity sensor details
    dht.humidity().getSensor(&sensor);
    Serial.println(F("Humidity Sensor Details:"));
    Serial.print(F("Sensor Type: "));
    Serial.println(sensor.name);
    Serial.print(F("Driver Ver:  "));
    Serial.println(sensor.version);
    Serial.print(F("Unique ID:   "));
    Serial.println(sensor.sensor_id);
    Serial.print(F("Max Value:   "));
    Serial.print(sensor.max_value);
    Serial.println(F("%"));
    Serial.print(F("Min Value:   "));
    Serial.print(sensor.min_value);
    Serial.println(F("%"));
    Serial.print(F("Resolution:  "));
    Serial.print(sensor.resolution);
    Serial.println(F("%"));
    Serial.println(F("------------------------------------"));
  }
  else
  {
    bmeAvailable = true;
    // Configure BME680 if available
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320°C for 150ms
    Serial.println("BME680 sensor configured");
  }

  // Initialize SD card for data logging
  if (SD.begin(SD_CS))
  {
    Serial.println("SD card initialized successfully");
  }
  else
  {
    Serial.println("SD card initialization failed");
  }

  Serial.println("Setup complete - starting main loop");
}

// ================================
// WIFI MANAGEMENT FUNCTIONS
// ================================

/**
 * WIFI MANAGEMENT: Keep WiFi connected and sync time when possible
 * Monitors WiFi status and attempts reconnection if disconnected
 * Performs NTP time sync once per WiFi connection session (prevents spam)
 * Throttles reconnection attempts to avoid network spam (30 second intervals)
 * Updates global status flags: wifiConnected, ntpSyncedThisSession
 * Called continuously from main loop for robust network management
 */
void maintainWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    // WiFi is connected
    if (!wifiConnected)
    {
      Serial.println("WiFi connected");
      wifiConnected = true;
      ntpSyncedThisSession = false; // Allow NTP sync for this session
    }

    // Perform NTP sync once per connection session
    if (!ntpSyncedThisSession)
    {
      if (syncTimeFromNTP())
      {
        syncRTCFromSystemTime();
        ntpSyncedThisSession = true;
        ntpSynced = true;
        Serial.println("NTP sync completed for this session");
      }
    }
    return;
  }

  // WiFi is NOT connected
  wifiConnected = false;

  // Throttle reconnection attempts to avoid spam
  if (millis() - lastWiFiAttempt < WIFI_RETRY_INTERVAL)
    return;

  lastWiFiAttempt = millis();

  // Check if credentials are available
  if (WiFi.status() == WL_NO_SSID_AVAIL)
  {
    Serial.println("No WiFi credentials stored");
    return;
  }

  // Attempt non-blocking reconnection
  Serial.println("Attempting WiFi reconnect...");
  WiFi.begin();
}

/**
 * TIME MAINTENANCE: Sync with internet time every 24 hours
 * Even though RTC keeps good time, it can drift slightly over days/weeks
 * This function automatically corrects RTC time once per day when WiFi is available
 * Helps maintain accurate timestamps for data logging over long periods
 * Only runs when WiFi is connected to internet
 */
void periodicNtpSync()
{
  static unsigned long lastCheck = 0;

  // Check only once per minute to reduce overhead
  if (millis() - lastCheck < 60000)
    return;
  lastCheck = millis();

  // Skip if WiFi not connected
  if (WiFi.status() != WL_CONNECTED)
    return;

  time_t currentTime = now(); // Get current RTC-based time

  // Skip if time seems invalid (less than ~1970 + some years)
  if (currentTime < 100000)
    return;

  // Check if 24 hours have passed since last NTP sync
  if (currentTime - lastNtpSyncEpoch >= 24UL * 60UL * 60UL)
  {
    Serial.println("24 hours elapsed - performing periodic NTP sync");

    if (syncTimeFromNTP())
    {
      syncRTCFromSystemTime();
      Serial.println("Periodic NTP sync completed");
    }
    else
    {
      Serial.println("Periodic NTP sync failed");
    }
  }
}

// ================================
// MAIN LOOP
// ================================
/**
 * MAIN PROGRAM: Continuous operation loop with enhanced timing control
 * This function runs forever after setup() completes
 * Manages all ongoing tasks with non-blocking millis() timing:
 * - Maintain WiFi connection and perform periodic NTP sync
 * - Read sensors at optimal intervals (PMS every 2s, others as needed)
 * - Update display alternately (sensor data every 3s / AQI info every 5s)
 * - Log comprehensive data to SD card every 10 seconds
 * - Create new daily CSV files when date changes
 * - Throttle main loop to 1 second intervals for system stability
 * Uses advanced timing controls to prevent system overload
 */
void loop()
{
  // Maintain WiFi connection and perform periodic NTP sync
  maintainWiFi();
  periodicNtpSync();
  static unsigned long lastMainLoopAttempt = 0;
  if (millis() - lastMainLoopAttempt < 1000)
    return;
  lastMainLoopAttempt = millis();
  // Set fixed GPS coordinates (GPS module not available)
  // Location: Ahmedabad, Gujarat, India
  latitude = 23.038126;
  longitude = 72.552605;
  lati = String(latitude, 6); // Convert to string with 6 decimal places
  longi = String(longitude, 6);
  atlt = "0"; // Altitude not available
  noS = "0";  // Number of satellites not available

  // Read PMS5003 sensor every 2 seconds (sensor limitation)
  static unsigned long lastPmsRead = 0;
  if (millis() - lastPmsRead >= 2000)
  {
    readPMSSensor();
    lastPmsRead = millis();
  }

  // Check if new day started - create new CSV file if needed
  String today = getRTCDate();
  if (today != lastFileDate)
  {
    checkFileExists(); // Create new daily file
    lastFileDate = today;
  }

  // Read environmental sensors (DHT22)
  readSensors();

  // Calculate Air Quality Index based on PM2.5
  calculateAQI();

  // Get current date/time from RTC
  dateTime = getRTCDateTime();
  Date = getRTCDate();

  // Update main display every 3 seconds
  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate >= 3000)
  {
    updateDisplay("Using RTC Time");
    lastDisplayUpdate = millis();
  }

  // Update AQI display every 5 seconds (alternates with main display)
  static unsigned long lastAqiDisplayUpdate = 0;
  if (millis() - lastAqiDisplayUpdate >= 5000)
  {
    updateAQIDisplay();
    lastAqiDisplayUpdate = millis();
  }

  // Log all sensor data to SD card every 10 seconds
  static unsigned long lastSDLog = 0;
  if (millis() - lastSDLog >= 10000) // Log every 10 seconds
  {
    logDataSdCard();
    lastSDLog = millis();
  }

  // Debug output
  Serial.println("Data cycle completed - Time: " + dateTime);
}