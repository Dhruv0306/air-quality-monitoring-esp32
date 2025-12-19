// ================================
// LIBRARY INCLUDES
// ================================
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
// HARDWARE DEFINITIONS
// ================================
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
#define INFLUXDB_BUCKET "E_010" // Device ID CHange her e.g. JDH_IITJ # ACRL_014
#define S1 "AU_PMS_CAPSTONE_"   // Device ID CHange here
// #define INFLUXDB_MEASUREMENT "atmosphere_data"
// #define WIFI_CONNECT_TIMEOUT 60000 // 1 minute
// #define TZ_INFO "IST-5:30"

// ================================
// NTP CONFIGURATION
// ================================
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

bool wifiConnected = false; // Global variable to track WiFi connection status
bool rtcSynced = false;
bool ntpSynced = false;
bool wifiConfigured = false;
WiFiManager wm;
time_t lastNtpSyncEpoch = 0;
String lastFileDate = "";
unsigned long lastWiFiAttempt = 0;
const unsigned long WIFI_RETRY_INTERVAL = 30000; // 30 seconds
bool ntpSyncedThisSession = false;

// ================================
// HARDWARE OBJECTS
// ================================
Adafruit_BME680 bme; // I2C
GuL::PMS5003 pms(Serial2);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT_Unified dht(DHTPIN, DHTTYPE);
DS3231 rtcModule;

// ================================
// UTILITY FUNCTIONS
// ================================

/**
 * Calculate number of days in a given month and year
 * @param m Month (1-12)
 * @param y Year
 * @return Number of days in the month
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
 * Check if a year is a leap year
 * @param y Year to check
 * @return true if leap year, false otherwise
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
 * Synchronize system time with NTP server
 * Configures timezone and attempts to get current time from NTP
 * @return true if sync successful, false otherwise
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
 * Setup WiFi connection for first-time boot
 * Creates configuration portal for user to enter WiFi credentials
 * @return true if WiFi configured successfully, false otherwise
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
  if (!wm.autoConnect("ESP32-Time-Setup"))
  {
    Serial.println("WiFi setup skipped or failed");
    return false;
  }

  Serial.println("WiFi credentials saved");
  return true;
}

/**
 * Validate if RTC has reasonable time values
 * Checks if year is >= 2024 and date/time values are within valid ranges
 * @return true if RTC time appears valid, false otherwise
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
 * Get formatted date and time string from RTC
 * Format: DD-MM-YYYY HH:MM:SS
 * @return Formatted datetime string
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
 * Get formatted date string from RTC
 * Format: DD-MM-YYYY (with trailing space)
 * @return Formatted date string
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
 * Synchronize RTC module with system time (after NTP sync)
 * Updates RTC hardware with current system time from NTP
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
 * Read temperature and humidity from DHT22 sensor
 * Sets BME680 values to 9999 when sensor not available
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
  pressure = 9999;
  gas = 9999;
  altitudeBme = 9999;
}

/**
 * Read all data from PMS5003 air quality sensor
 * Includes PM values, particle counts, and environmental data from sensor
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
 * Calculate Air Quality Index (AQI) based on PM2.5 concentration
 * Uses Indian AQI standards for categorization
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
 * Update OLED display with sensor readings
 * Shows temperature, humidity, and PM concentrations
 * @param messge Status message to display at top
 */
void updateDisplay(String messge)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);

  // Display status message (e.g., "Using RTC Time")
  display.println(messge);

  // Display environmental readings
  display.print("Temperature: ");
  display.print(temperature);
  display.println(" *C");
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");

  // BME680 readings commented out (sensor not available)
  // display.print("Pressure: ");
  // display.print(pressure);
  // display.println(" hPa");

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
 * Update OLED display with AQI information
 * Shows current date/time and air quality status
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
 * Check if daily CSV file exists, create with headers if not
 * File naming format: AU_PMS_CAPSTONE_YYYY-MM-DD.csv
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
  filename = String(S1) + String(date) + ".csv";

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
 * Log sensor data to SD card CSV file
 * Appends new data row with timestamp and all sensor readings
 */
void logDataSdCard()
{
  bool h12Flag, pmFlag;
  // Get current date from RTC for filename
  int rtcYear = rtcModule.getYear();
  int rtcMonth = rtcModule.getMonth(h12Flag);
  int rtcDay = rtcModule.getDate();

  String date = "";

  // Format date as YYYYMMDD for filename
  date += (2000 + rtcYear);
  if (rtcMonth < 10)
    date += "0";
  date += rtcMonth;
  if (rtcDay < 10)
    date += "0";
  date += rtcDay;

  // Generate filename and open for append
  filename = String(S1) + String(date) + ".csv";
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
 * Initialize all hardware components and establish connections
 * Sets up sensors, display, WiFi, RTC, and SD card
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
 * Maintain WiFi connection and handle reconnection attempts
 * Performs NTP sync once per connection session
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
 * Perform periodic NTP synchronization (every 24 hours)
 * Ensures RTC stays accurate over long periods
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
 * Main program loop - runs continuously
 * Handles sensor readings, display updates, and data logging
 */
void loop()
{
  // Maintain WiFi connection and perform periodic NTP sync
  maintainWiFi();
  periodicNtpSync();

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

  // Log all sensor data to SD card
  logDataSdCard();

  // Debug output
  Serial.println("Data cycle completed - Time: " + dateTime);
}