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
int daysInMonth(int m, int y)
{
  if (m == 4 || m == 6 || m == 9 || m == 11)
    return 30;
  if (m == 2)
    return (isLeapYear(y) ? 29 : 28);
  return 31;
}

bool isLeapYear(int y)
{
  if (y % 4 != 0)
    return false;
  if (y % 100 != 0)
    return true;
  if (y % 400 != 0)
    return false;
  return true;
}

bool syncTimeFromNTP()
{
  struct tm timeinfo;

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  if (!getLocalTime(&timeinfo, 10000))
  { // wait max 10 sec
    Serial.println("NTP sync failed");
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("NTP sync failed");
    // delay(500);
    return false;
  }

  Serial.println("Time synced from NTP");
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Time synced from NTP");
  // delay(500);
  return true;
}

bool setupWiFiFirstTime()
{

  wm.setConfigPortalTimeout(120); // 2 minutes
  wm.setConnectTimeout(5);
  wm.setDebugOutput(true);
  // wm.setBreakAfterConfig(true);

  if (!wm.autoConnect("ESP32-Time-Setup"))
  {
    Serial.println("WiFi setup skipped or failed");
    return false;
  }

  Serial.println("WiFi credentials saved");
  return true;
}

bool isRTCValid()
{
  bool centuryFlag;
  bool h12Flag, pmFlag;

  int y = rtcModule.getYear();
  int m = rtcModule.getMonth(centuryFlag);
  int d = rtcModule.getDate();
  int h = rtcModule.getHour(h12Flag, pmFlag);

  // Year >= 2024 and reasonable date
  return (y >= 24 && m >= 1 && m <= 12 && d >= 1 && d <= 31 && h <= 23);
}

// ================================
// RTC FUNCTIONS
// ================================
String getRTCDateTime()
{
  bool h12Flag, pmFlag;
  int rtcYear = rtcModule.getYear();
  int rtcMonth = rtcModule.getMonth(h12Flag);
  int rtcDay = rtcModule.getDate();
  int rtcHour = rtcModule.getHour(h12Flag, pmFlag);
  int rtcMinute = rtcModule.getMinute();
  int rtcSecond = rtcModule.getSecond();

  String dateTime = "";

  if (rtcDay < 10)
    dateTime += "0";
  dateTime += rtcDay;
  dateTime += "-";
  if (rtcMonth < 10)
    dateTime += "0";
  dateTime += rtcMonth;
  dateTime += "-";
  dateTime += (2000 + rtcYear);
  dateTime += " ";
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

String getRTCDate()
{
  bool h12Flag, pmFlag;
  int rtcYear = rtcModule.getYear();
  int rtcMonth = rtcModule.getMonth(h12Flag);
  int rtcDay = rtcModule.getDate();

  String nowDay = "";

  if (rtcDay < 10)
    nowDay += "0";
  nowDay += rtcDay;
  nowDay += "-";
  if (rtcMonth < 10)
    nowDay += "0";
  nowDay += rtcMonth;
  nowDay += "-";
  nowDay += (2000 + rtcYear);
  nowDay += " ";

  return nowDay;
}

void syncRTCFromSystemTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return;
  }

  rtcModule.setYear(timeinfo.tm_year - 100); // since 2000
  rtcModule.setMonth(timeinfo.tm_mon + 1);
  rtcModule.setDate(timeinfo.tm_mday);
  rtcModule.setHour(timeinfo.tm_hour);
  rtcModule.setMinute(timeinfo.tm_min);
  rtcModule.setSecond(timeinfo.tm_sec);

  rtcSynced = true;
  lastNtpSyncEpoch = now();
  Serial.println("RTC synced from NTP");
}

// ================================
// SENSOR FUNCTIONS
// ================================
void readSensors()
{
  // Read DHT sensor
  sensors_event_t event;
  // Serial.println("dht.begin();");
  dht.temperature().getEvent(&event);
  temperature = event.temperature;

  // Serial.println(temperature);
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  humidity = event.relative_humidity;
  // Serial.println(humidity);
  pressure = 9999;
  gas = 9999;
  altitudeBme = 9999;
  gas = 9999;
}

void readPMSSensor()
{
  pms.poll();
  pms.read();
  val1 = pms.getPM1_STD();
  val2 = pms.getPM2_5_STD();
  val3 = pms.getPM10_STD();
  val4 = pms.getPM1_ATM();
  val5 = pms.getPM2_5_ATM();
  val6 = pms.getPM10_ATM();
  c_300 = pms.getCntBeyond300nm();
  c_500 = pms.getCntBeyond500nm();
  c_1000 = pms.getCntBeyond1000nm();
  c_2500 = pms.getCntBeyond2500nm();
  c_5000 = pms.getCntBeyond5000nm();
  c_10000 = pms.getCntBeyond10000nm();
  pms_temp = pms.getTemperature();
  pms_h = pms.getHumidity();
  pms_fld = pms.getFormaldehydeConcentration();
  Serial.println("wait");
}

void calculateAQI()
{
  // AQI calculation
  if (val2 >= 0 && val2 < 30)
  {
    pm25_aqi = 1;
  }
  else if (val2 >= 30 && val2 < 60)
  {
    pm25_aqi = 2;
  }
  else if (val2 >= 60 && val2 < 90)
  {
    pm25_aqi = 3;
  }
  else if (val2 >= 90 && val2 < 120)
  {
    pm25_aqi = 4;
  }
  else if (val2 >= 120 && val2 < 250)
  {
    pm25_aqi = 5;
  }
  else if (val2 >= 250)
  {
    pm25_aqi = 6;
  }
  else
  {
    pm25_aqi = 0;
  }

  // Max AQI converted to String
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
    aqi = "ERROR";
  }
}

// ================================
// DISPLAY FUNCTIONS
// ================================
void updateDisplay(String messge)
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(messge);
  display.print("Temperature: ");
  display.print(temperature);
  display.println(" *C");
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");
  // display.print("Pressure: ");
  // display.print(pressure);
  // display.println(" hPa");
  // display.print("Gas: ");
  // display.println(gas);
  // display.print("Altitude: ");
  // display.println(altitudeBme);
  display.print("PM1: ");
  display.print(val4);
  display.println(" ug/m^3");
  display.print("PM2.5: ");
  display.print(val5);
  display.println(" ug/m^3");
  display.print("PM10: ");
  display.print(val6);
  display.println(" ug/m^3");
  // display.print("LAT: ");
  // display.println(lati);
  // display.print("LONG: ");
  // display.println(longi);
  // display.print("DateTime: ");
  // display.println(dateTime);
  display.display();
}

void updateAQIDisplay()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);

  display.println(dateTime);
  display.setTextSize(1.2);
  display.println();
  display.println("PM2.5 based Air ");
  display.print("Quality-");
  display.println(aqi);
  display.display();
}

// ================================
// FILE FUNCTIONS
// ================================
void checkFileExists()
{
  bool h12Flag, pmFlag;
  int rtcYear = rtcModule.getYear();
  int rtcMonth = rtcModule.getMonth(h12Flag);
  int rtcDay = rtcModule.getDate();

  String date = "";

  // Use RTC time for file naming
  date += (2000 + rtcYear);
  date += "-";
  if (rtcMonth < 10)
    date += "0";
  date += rtcMonth;
  date += "-";
  if (rtcDay < 10)
    date += "0";
  date += rtcDay;

  filename = String(S1) + String(date) + ".csv";
  Serial.println(rtcDay);
  Serial.println(rtcMonth);
  Serial.println(2000 + rtcYear);
  Serial.println();
  File file = SD.open("/" + filename);

  if (!file)
  {
    Serial.println("File doesn't exist");
    Serial.println("Creating file...");
    // Serial.println("file old file nedds to uplod on ftp server.");

    String message = "DateTime,Temperature,Humidity,Pressure,Gas,Altitude,PM1,PM2.5,PM10,pm1atm,pm2.5atm,pm10atm,c_300,c_500,c_1000,c_2500,c_5000,c_10000,pms_temp,pms_humidity,pms_formaldihyde,Latitude,Longitude,Altitude_GPS,Satellite\r\n";

    File file = SD.open("/" + filename, FILE_WRITE);
    if (!file)
    {
      Serial.println("Failed to open file for writing");
      return;
    }
    if (file.print(message))
    {
      Serial.println("File written");
    }
    else
    {
      Serial.println("Write failed");
    }
    file.close();
  }
  else
  {
    Serial.println("File already exists");
    Serial.println(filename);
  }
  file.close();
}

void logDataSdCard()
{
  bool h12Flag, pmFlag;
  int rtcYear = rtcModule.getYear();
  int rtcMonth = rtcModule.getMonth(h12Flag);
  int rtcDay = rtcModule.getDate();

  String date = "";

  // Use RTC time for file naming
  date += (2000 + rtcYear);
  if (rtcMonth < 10)
    date += "0";
  date += rtcMonth;
  if (rtcDay < 10)
    date += "0";
  date += rtcDay;

  filename = String(S1) + String(date) + ".csv";
  File file = SD.open("/" + filename, "a+");
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  // val4, val5, val6, c_300 , c_500, c_1000, c_2500, c_5000, c_10000, pms_temp, pms_h;
  char dataMessage[1024];
  snprintf(dataMessage, sizeof(dataMessage),
           "%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%s,%s,%s,%s\r\n",
           dateTime.c_str(),
           temperature,
           humidity,
           pressure,
           gas,
           altitudeBme,
           (float)val1, (float)val2, (float)val3, (float)val4, (float)val5, (float)val6,
           (float)c_300, (float)c_500, (float)c_1000, (float)c_2500, (float)c_5000, (float)c_10000,
           (float)pms_temp, (float)pms_h, (float)pms_fld,
           lati.c_str(), longi.c_str(),
           atlt.c_str(), noS.c_str());
  if (file.print(dataMessage))
  {
    Serial.print("Data saved to SD card: ");
    Serial.println(dataMessage);
    // Serial.println(dateTimeIf);
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
void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RX2, TX2);
  Serial.println("final_gps_rtc_bme_dht_20241110");

  // Initialize Display
  Wire.begin();
  Wire.setClock(400000); // Set I2C frequency to 400kHz
  // rtcModule.begin();
  WiFi.mode(WIFI_STA); // <-- REQUIRED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    while (true)
      ;
  }
  // Initialize RTC
  Serial.println("RTC initialized");

  // NTP Time
  if (WiFi.SSID().length() == 0)
  {
    Serial.println("First boot detected");
    wifiConfigured = setupWiFiFirstTime();
  }
  else
  {
    WiFi.begin(); // auto connect
  }

  if (isRTCValid())
  {
    bool centuryFlag;
    bool h12Flag, pmFlag;

    setTime(
        rtcModule.getHour(h12Flag, pmFlag),
        rtcModule.getMinute(),
        rtcModule.getSecond(),
        rtcModule.getDate(),
        rtcModule.getMonth(centuryFlag),
        2000 + rtcModule.getYear());
    rtcSynced = true;
    lastNtpSyncEpoch = now();
    Serial.println("RTC time loaded");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi connected");

    if (syncTimeFromNTP())
    {
      syncRTCFromSystemTime();
      ntpSynced = true;
      rtcSynced = true;
    }
  }

  // pms7003.init(&Serial2);
  pms.setToPassiveReporting();

  // Server Setup
  // connectToWifi();

  dht.begin();
  if (!bme.begin())
  {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");

    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    Serial.println(F("------------------------------------"));
    Serial.println(F("Temperature Sensor"));
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
    // Print humidity sensor details.
    dht.humidity().getSensor(&sensor);
    Serial.println(F("Humidity Sensor"));
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

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);

  SD.begin(SD_CS);
}

// ================================
// NTP PERIODIC SYNC FUNCTION
// ================================
void maintainWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!wifiConnected)
    {
      Serial.println("WiFi connected");
      wifiConnected = true;
      ntpSyncedThisSession = false; // allow NTP sync again
    }

    // Sync NTP once per connection
    if (!ntpSyncedThisSession)
    {
      if (syncTimeFromNTP())
      {
        syncRTCFromSystemTime();
        ntpSyncedThisSession = true;
        ntpSynced = true;
      }
    }
    return;
  }

  // ---- WiFi NOT connected ----
  wifiConnected = false;

  if (millis() - lastWiFiAttempt < WIFI_RETRY_INTERVAL)
    return;

  lastWiFiAttempt = millis();

  if (WiFi.status() == WL_NO_SSID_AVAIL)
  {
    Serial.println("No WiFi credentials stored");
    return;
  }

  Serial.println("Attempting WiFi reconnect...");
  WiFi.begin(); // non-blocking
}

void periodicNtpSync()
{
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 60000)
    return;
  lastCheck = millis();

  if (WiFi.status() != WL_CONNECTED)
    return;

  time_t currentTime = now(); // RTC-based

  if (currentTime < 100000)
    return;

  if (currentTime - lastNtpSyncEpoch >= 24UL * 60UL * 60UL)
  {
    Serial.println("24h elapsed, syncing RTC from NTP");

    if (syncTimeFromNTP())
    {
      syncRTCFromSystemTime();
    }
  }
}

// ================================
// MAIN LOOP
// ================================
void loop()
{
  // i = i +1 ;
  // pms7003.updateFrame();
  // Serial.print("1");

  // delay(20);
  // String datetime = "";
  maintainWiFi();
  periodicNtpSync();

  // GPS not available, use RTC as fallback
  Serial.println("GPS not available, using RTC");

  // Set default GPS coordinates when GPS is not available
  latitude = 23.038126;
  longitude = 72.552605;
  lati = String(latitude, 6);
  longi = String(longitude, 6);
  atlt = "0";
  noS = "0";

  static unsigned long lastPmsRead = 0;
  if (millis() - lastPmsRead >= 2000)
  {
    readPMSSensor();
    lastPmsRead = millis();
  }

  String today = getRTCDate();
  if (today != lastFileDate)
  {
    checkFileExists();
    lastFileDate = today;
  }
  readSensors();
  calculateAQI();

  // Use RTC for date and time
  dateTime = getRTCDateTime();
  Date = getRTCDate();

  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate >= 3000)
  {
    updateDisplay("Using RTC Time");
    lastDisplayUpdate = millis();
  }

  static unsigned long lastAqiDisplayUpdate = 0;
  if (millis() - lastAqiDisplayUpdate >= 5000)
  {
    updateAQIDisplay();
    lastAqiDisplayUpdate = millis();
  }

  logDataSdCard();

  Serial.println("Data logged using RTC time: " + dateTime);
}