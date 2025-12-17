// ================================
// LIBRARY INCLUDES
// ================================
//#include "Plantower_PMS7003.h"
// #include <Arduino.h>
// #include <string>
#include <PMS5003.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <Wire.h>
// Server Libraries
// #include <WiFi.h>
// #include <InfluxDbClient.h>
// #include <InfluxDbCloud.h>
// #include <WiFiManager.h>
#include <Adafruit_BME680.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "FS.h"
#include "SD.h"
#include <SPI.h>
//#include <string.h>
#include <TimeLib.h>
// Server Libraries
// #include <HTTPClient.h>
// const char* test_url = "http://www.google.com";
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
#define DHTPIN 14     // Digital pin connected to the DHT sensor 
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

// ================================
// DEVICE CONFIGURATION
// ================================
#define INFLUXDB_BUCKET "E_010"//Device ID CHange her e.g. JDH_IITJ # ACRL_014
#define S1 "AU_PMS_CAPSTONE_"//Device ID CHange here
// #define INFLUXDB_MEASUREMENT "atmosphere_data"
// #define WIFI_CONNECT_TIMEOUT 60000 // 1 minute
// #define TZ_INFO "IST-5:30"

// ================================
// DISPLAY CONFIGURATION
// ================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

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

// ================================
// HARDWARE OBJECTS
// ================================
Adafruit_BME680 bme;  // I2C
GuL::PMS5003 pms(Serial2);
TinyGPSPlus gps;
HardwareSerial GPS(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT_Unified dht(DHTPIN, DHTTYPE);
DS3231 rtcModule;

// Server Related
// const char* WIFI_SSID = "Yap";
// const char* WIFI_PASSWORD = "letstest";
// #define INFLUXDB_URL "http://103.233.171.34:8086"
// #define INFLUXDB_TOKEN "A-XmUWeZyP_5w8xTxfd1rImg90GPBizsQiQR2qx0ZWgxG-YecEWAz08K3kApq43uu56DPeAumVuqMARv-7ZO6w=="
// #define INFLUXDB_ORG "6ecfc9383074a24b"
// InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
// Point sensor(INFLUXDB_MEASUREMENT);

//ftpserver
// #include <FTPClient.h>
// #define FTP32_LOG FTP32_LOG_INFO
// #include "ftp32.h"
// const char* ftpuser = "SAS_Lab409";
// const char* ftpPass = "123";

// ================================
// UTILITY FUNCTIONS
// ================================
int daysInMonth(int m, int y) { 
  if (m == 4 || m == 6 || m == 9 || m == 11) return 30; 
  if (m == 2) return (isLeapYear(y) ? 29 : 28); 
  return 31; 
} 

bool isLeapYear(int y) { 
  if (y % 4 != 0) return false; 
  if (y % 100 != 0) return true; 
  if (y % 400 != 0) return false; 
  return true; 
}

// ================================
// TIME CONVERSION FUNCTIONS
// ================================
int convertUtcToIstdate(int utcYear, int utcMonth, int utcDay, int utcHour, int utcMinute, int utcSecond) {
    int year = utcYear;
    int month = utcMonth;
    int day = utcDay;
    int hour = utcHour + 5;  // India is 5 hours ahead of UTC
    int minute = utcMinute + 30; // Add 30 minutes for IST
    int second = utcSecond;

    // Handle minute overflow
    if (minute >= 60) {
        minute -= 60;
        hour += 1;
    }

    // Handle hour overflow
    if (hour >= 24) {
        hour -= 24;
        day += 1;
    }

    // Determine the number of days in the current month
    int daysInMonth;
    if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
        daysInMonth = 31;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        daysInMonth = 30;
    } else { // February
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            daysInMonth = 29; // Leap year
        } else {
            daysInMonth = 28;
        }
    }

    // Handle day overflow
    if (day > daysInMonth) {
        day -= daysInMonth;
        month += 1;
    }

    // Handle month overflow
    if (month > 12) {
        month -= 12;
        year += 1;
    }

    // Return the result in a composite integer format
    return (year * 100) + month;
}

int convertUtcToIsttime(int utcYear, int utcMonth, int utcDay, int utcHour, int utcMinute, int utcSecond) {
    int year = utcYear;
    int month = utcMonth;
    int day = utcDay;
    int hour = utcHour + 5;  // India is 5 hours ahead of UTC
    int minute = utcMinute + 30; // Add 30 minutes for IST
    int second = utcSecond;

    // Handle minute overflow
    if (minute >= 60) {
        minute -= 60;
        hour += 1;
    }

    // Handle hour overflow
    if (hour >= 24) {
        hour -= 24;
        day += 1;
    }

    // Determine the number of days in the current month
    int daysInMonth;
    if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
        daysInMonth = 31;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        daysInMonth = 30;
    } else { // February
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            daysInMonth = 29; // Leap year
        } else {
            daysInMonth = 28;
        }
    }

    // Handle day overflow
    if (day > daysInMonth) {
        day -= daysInMonth;
        month += 1;
    }

    // Handle month overflow
    if (month > 12) {
        month -= 12;
        year += 1;
    }

    // Return the result in a composite integer format
    return (hour * 100) + minute;
}

int convertUtcToIstday(int utcYear, int utcMonth, int utcDay, int utcHour, int utcMinute, int utcSecond) {
    int year = utcYear;
    int month = utcMonth;
    int day = utcDay;
    int hour = utcHour + 5;  // India is 5 hours ahead of UTC
    int minute = utcMinute + 30; // Add 30 minutes for IST
    int second = utcSecond;

    // Handle minute overflow
    if (minute >= 60) {
        minute -= 60;
        hour += 1;
    }

    // Handle hour overflow
    if (hour >= 24) {
        hour -= 24;
        day += 1;
    }

    // Determine the number of days in the current month
    int daysInMonth;
    if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
        daysInMonth = 31;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        daysInMonth = 30;
    } else { // February
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            daysInMonth = 29; // Leap year
        } else {
            daysInMonth = 28;
        }
    }

    // Handle day overflow
    if (day > daysInMonth) {
        day -= daysInMonth;
        month += 1;
    }

    // Handle month overflow
    if (month > 12) {
        month -= 12;
        year += 1;
    }

    // Return the result in a composite integer format
    return day;
}

// ================================
// RTC FUNCTIONS
// ================================
String getRTCDateTime() {
    bool h12Flag, pmFlag;
    int rtcYear = rtcModule.getYear();
    int rtcMonth = rtcModule.getMonth(h12Flag);
    int rtcDay = rtcModule.getDate();
    int rtcHour = rtcModule.getHour(h12Flag, pmFlag);
    int rtcMinute = rtcModule.getMinute();
    int rtcSecond = rtcModule.getSecond();
    
    String dateTime = "";
    
    if (rtcDay < 10) dateTime += "0";
    dateTime += rtcDay;
    dateTime += "-";
    if (rtcMonth < 10) dateTime += "0";
    dateTime += rtcMonth;
    dateTime += "-";
    dateTime += (2000 + rtcYear);
    dateTime += " ";
    if (rtcHour < 10) dateTime += "0";
    dateTime += rtcHour;
    dateTime += ":";
    if (rtcMinute < 10) dateTime += "0";
    dateTime += rtcMinute;
    dateTime += ":";
    if (rtcSecond < 10) dateTime += "0";
    dateTime += rtcSecond;
    
    return dateTime;
}

String getRTCDate() {
    bool h12Flag, pmFlag;
    int rtcYear = rtcModule.getYear();
    int rtcMonth = rtcModule.getMonth(h12Flag);
    int rtcDay = rtcModule.getDate();
    
    String nowDay = "";
    
    if (rtcDay < 10) nowDay += "0";
    nowDay += rtcDay;
    nowDay += "-";
    if (rtcMonth < 10) nowDay += "0";
    nowDay += rtcMonth;
    nowDay += "-";
    nowDay += (2000 + rtcYear);
    nowDay += " ";
    
    return nowDay;
}

// ================================
// GPS FUNCTIONS
// ================================
String getDateFromGPS(TinyGPSDate &date, TinyGPSTime &time) {
  utcHour = time.hour();
  utcMinute = time.minute();
  utcSecond = time.second();
  utcDay = date.day();
  utcMonth = date.month();
  utcYear = date.year();
  Serial.print(utcMonth);
  
  int istTime = convertUtcToIsttime(utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond);
  int istdate = convertUtcToIstdate(utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond);
  int istday = convertUtcToIstday(utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond);

  int year = istdate / 100;
  int month = istdate % 100;
  int day = istday;
  int hour = istTime / 100;
  int minute = istTime % 100;
  
  //int Hour = Time / 100;
  //int Minute = Time % 100;
  int Second = utcSecond;
  //Month = Month ;
  //Year = 0;
  //Month = 0;

    // Call the conversion function with sample data
  //  convertUtcToIst(2023, 5, 21, 23, 45, 0);

    // Print the result
    //printf("IST Year: %d, IST Month: %d\n", Year, Month);
  //int Day = utcDay;
  //int Month = utcMonth;
  //int Year = utcYear;
  //setTime(Hour, Minute, Second, Day, Month, Year);
  //adjustTime(19800);
  
  //const int offset = 5;

  String nowDay= "";
  
  if (Day < 10) nowDay += "0";
  nowDay += Day;
  nowDay += "-";
  if (Month < 10) nowDay += "0";
  nowDay += Month;
  nowDay += "-";
  nowDay += Year;
  nowDay += " ";

  return nowDay = nowDay;
  
  //filename = 
  //return dateTimeIf;
  Serial.println(nowDay);
  Serial.print("now day =");
}

String getDateTimeFromGPS(TinyGPSDate &date, TinyGPSTime &time) {
  utcHour = time.hour();
  utcMinute = time.minute();
  utcSecond = time.second();
  utcDay = date.day();
  utcMonth = date.month();
  utcYear = date.year();
  Serial.print(utcMonth);
  int istTime = convertUtcToIsttime(utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond);
  int istdate = convertUtcToIstdate(utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond);
  int istday = convertUtcToIstday(utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond);

  int Year = istdate / 100;
  int Month = istdate % 100;
  int Day = istday;
  int Hour = istTime / 100;
  int Minute = istTime % 100;
  
  //int Hour = Time / 100;
  //int Minute = Time % 100;
  int Second = utcSecond;

  //Month = Month ;
  //Year = 0;
  //Month = 0;

    // Call the conversion function with sample data
  //  convertUtcToIst(2023, 5, 21, 23, 45, 0);

    // Print the result
    //printf("IST Year: %d, IST Month: %d\n", Year, Month);
  //int Day = utcDay;
  //int Month = utcMonth;
  //int Year = utcYear;
  //setTime(Hour, Minute, Second, Day, Month, Year);
  //adjustTime(19800);
  
  //const int offset = 5;

  String dateTime = "";
  
  if (Day < 10) dateTime += "0";
  dateTime += Day;
  dateTime += "-";
  if (Month < 10) dateTime += "0";
  dateTime += Month;
  dateTime += "-";
  dateTime += Year;
  dateTime += " ";
  if (Hour < 10) dateTime += "0";
  dateTime += Hour;
  dateTime += ":";
  if (Minute < 10) dateTime += "0";
  dateTime += Minute;
  dateTime += ":";
  if (Second < 10) dateTime += "0";
  dateTime += Second;
  return dateTime = dateTime;
}

// ================================
// SENSOR FUNCTIONS
// ================================
void readSensors() {
    // Read DHT sensor
    sensors_event_t event;
    Serial.println("dht.begin();");
    dht.temperature().getEvent(&event);
    temperature = event.temperature;
    
    Serial.println(temperature);
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    humidity = event.relative_humidity;
    Serial.println(humidity);
    pressure = 9999;
    gas = 9999;
    altitudeBme = 9999;
    gas = 9999;
}

void readPMSSensor() {
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

void calculateAQI() {
    //AQI calculation
    if (val2 >= 0 && val2 < 30) {
        pm25_aqi = 1;
    } else if (val2 >= 30 && val2 < 60) {
        pm25_aqi = 2;
    } else if (val2 >= 60 && val2 < 90) {
        pm25_aqi = 3;
    } else if (val2 >= 90 && val2 < 120) {
        pm25_aqi = 4;
    } else if (val2 >= 120 && val2 < 250) {
        pm25_aqi = 5;
    } else if (val2 >= 250) {
        pm25_aqi = 6;
    } else {
        pm25_aqi = 0;
    }
 
    //Max AQI converted to String
    if (pm25_aqi == 1) {
        aqi = "Good";
    } else if (pm25_aqi == 2) {
        aqi = "Satisfactory";
    } else if (pm25_aqi == 3) {
        aqi = "Moderate";
    } else if (pm25_aqi == 4) {
        aqi = "Poor";
    } else if (pm25_aqi == 5) {
        aqi = "Very Poor";
    } else if (pm25_aqi == 6) {
        aqi = "Severe";
    } else {
        aqi = "ERROR";
    }
}

// ================================
// DISPLAY FUNCTIONS
// ================================
void updateDisplay(String messge) {
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

void updateAQIDisplay() {
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
void checkFileExists() {
    bool h12Flag, pmFlag;
    int rtcYear = rtcModule.getYear();
    int rtcMonth = rtcModule.getMonth(h12Flag);
    int rtcDay = rtcModule.getDate();
    
    String date = "";
    
    // Use RTC time for file naming
    date += (2000 + rtcYear);
    if (rtcMonth < 10) date += "0";
    date += rtcMonth;
    if (rtcDay < 10) date += "0";
    date += rtcDay;
    
    filename = String(S1) + String(date) + ".csv";
    Serial.println(rtcDay);
    Serial.println(rtcMonth);
    Serial.println(2000 + rtcYear);
    Serial.println();
    File file = SD.open("/" + filename);

    if (!file) {
        Serial.println("File doesn't exist");
        Serial.println("Creating file...");
        //Serial.println("file old file nedds to uplod on ftp server.");
        
        String message = "DateTime,Temperature,Humidity,Pressure,Gas,Altitude,PM1,PM2.5,PM10,pm1atm,pm2.5atm,pm10atm,c_300,c_500,c_1000,c_2500,c_5000,c_10000,pms_temp,pms_humidity,pms_formaldihyde,Latitude,Longitude,Altitude_GPS,Satellite\r\n";
        
        File file = SD.open("/" + filename, FILE_WRITE );
        if (!file) {
            Serial.println("Failed to open file for writing");
            return;
        }
        if (file.print(message)) {
            Serial.println("File written");
        } else {
            Serial.println("Write failed");
        }
        file.close();
    } else {
        Serial.println("File already exists");
        Serial.println(filename);
    }
    file.close();
}

void logDataSdCard() {
    bool h12Flag, pmFlag;
    int rtcYear = rtcModule.getYear();
    int rtcMonth = rtcModule.getMonth(h12Flag);
    int rtcDay = rtcModule.getDate();
    
    String date = "";
    
    // Use RTC time for file naming
    date += (2000 + rtcYear);
    if (rtcMonth < 10) date += "0";
    date += rtcMonth;
    if (rtcDay < 10) date += "0";
    date += rtcDay;
 
    filename = String(S1) + String(date) + ".csv";
    File file = SD.open("/" + filename, "a+");
    if (!file) {
        Serial.println ("Failed to open file for writing");
        return;
    }
    //val4, val5, val6, c_300 , c_500, c_1000, c_2500, c_5000, c_10000, pms_temp, pms_h;
    String dataMessage = dateTime + "," + String(temperature) + "," + String(humidity) + "," + String(pressure) + "," + String(gas) + "," + String(altitudeBme) + "," +
                        String(val1) + "," + String(val2) + "," + String(val3) + "," + String(val4) + "," +String(val5) + "," + String(val6) + "," + String(c_300) + "," +String(c_500) + "," +String(c_1000) + "," +
                        String(c_2500) + "," + String(c_5000) + "," +String(c_10000) + "," +String(pms_temp) + "," +String(pms_h) + "," +String(pms_fld) + "," +
                        String(lati) + "," + String(longi) + "," + atlt + "," + noS + "\r\n";

    if (file.print(dataMessage)) {
        Serial.print("Data saved to SD card: ");
        Serial.println(dataMessage);
        //Serial.println(dateTimeIf);
    } else {
        Serial.println("Error writing to SD card");
    }
    file.close();
}

// ================================
// SERVER FUNCTIONS (COMMENTED)
// ================================
// Server Functions
// void readAndUploadCSVgps(String filename) {
//   File file = SD.open("/" +filename);
//   if (!file) {
//     Serial.println("Failed to open file!");
//     return;
//   }
//   int lineNum = 0;
//   while (file.available()) {
//     String line = file.readStringUntil('\n');
//     String remoteFilePath = "/line_" + String(lineNum) + ".txt"; // Change the remote file path as needed
//     FTP32 ftp("103.233.171.34", 8087);
//     //size_t dataSize = strlen(line);
//     Serial.println("File reading started");
//     if( ftp.connectWithPassword("SAS_Lab409", "Aqiguj@700") ){

//       Serial.println("Login unsuccessful");
//       Serial.printf("Exited with code: %d %s\n", ftp.getLastCode(), ftp.getLastMsg());
//       while(true){}
//     }
//     if (ftp.uploadSingleshot(filename.c_str(), (uint8_t*) line.c_str(), line.length() ,FTP32::OpenType::CREATE_REPLACE)) {
//       Serial.println("Upload failed");
//       Serial.printf("Exited with code: %d %s\n", ftp.getLastCode(), ftp.getLastMsg().c_str());
//       break;
//     } else {
//       Serial.printf("Uploaded line %d successfully: %s\n", lineNum, line.c_str());
//     }
//     lineNum++;
//   }

//   file.close();
// }

// Server Callbacks
// void readMacAddress(){
//   uint8_t baseMac[6];
//   esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
//   if (ret == ESP_OK) {
//     Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
//                   baseMac[0], baseMac[1], baseMac[2],
//                   baseMac[3], baseMac[4], baseMac[5]);
//   } else {
//     Serial.println("Failed to read MAC address");
//   }
// }

// Server Callback Functions
// void saveConfigCallback() {
//   Serial.println("Saving WiFi configuration");
//   //delayMicroseconds(5000000);
//   //delayMicroseconds(5000000);
// }
// void connectToWifi() {
//   // Check if WiFi is already connected
//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.println("Already connected to WiFi!");
//     Serial.println("Wait 20s");
//     delay(2000);
//     return;
//   }
  
//   // Attempt to connect using saved credentials for 20 seconds
//   unsigned long wifiStartTime = millis(); // Record the start time of WiFi connection attempt
//   while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 200) { // Try for 20 seconds
//     WiFi.begin(); // Attempt to reconnect using saved credentials
//     delay(500);
//   }

//   // If WiFi connection is established, display IP address and exit the function
//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.println("Connected to WiFi!");
//     Serial.print("IP Address: ");
//     Serial.println(WiFi.localIP());
//     return;
//   }
//   // Start the WiFi configuration portal and run it for 1 minute
//   WiFiManager wifiManager;
//   wifiManager.setTimeout(1); // 60 seconds
//   wifiManager.setSaveParamsCallback([]() {
//     Serial.println("Save new WiFi credentials...");
//   });

//   if (!wifiManager.startConfigPortal(INFLUXDB_BUCKET, "12345678")) {
//     Serial.println("Failed to connect and hit timeout");
//     // Handle if the connection attempt fails or times out
//     return;
//   }
//   // If the control reaches here, the connection was successful or the portal timed out
//   Serial.println("Connected to WiFi");
//   //Ptinting IP address
//   Serial.print("IP Address: ");
//   Serial.println(WiFi.localIP());
//   //WiFi.status();
// }

// ================================
// SETUP FUNCTION
// ================================
void setup() {
    Serial.begin(9600);
    Serial2.begin(9600);
    Serial2.begin(9600, SERIAL_8N1, RX2, TX2);
    Serial.println("final_gps_rtc_bme_dht_20241110");
    GPS.begin(9600, SERIAL_8N1, 26, 27);
    
    //pms7003.init(&Serial2);
    pms.setToPassiveReporting();

    // Initialize RTC
    Wire.begin();
    Serial.println("RTC initialized");

    // Server Setup
    // connectToWifi();

    if (!bme.begin()) {
        Serial.println("Could not find a valid BME680 sensor, check wiring!");
        dht.begin();

        sensor_t sensor;
        dht.temperature().getSensor(&sensor);
        Serial.println(F("------------------------------------"));
        Serial.println(F("Temperature Sensor"));
        Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
        Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
        Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
        Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
        Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
        Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
        Serial.println(F("------------------------------------"));
        // Print humidity sensor details.
        dht.humidity().getSensor(&sensor);
        Serial.println(F("Humidity Sensor"));
        Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
        Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
        Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
        Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
        Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
        Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
        Serial.println(F("------------------------------------"));
    }

    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (true);
    }

    SD.begin(SD_CS);

    // Server Setup
    // readMacAddress();
    //checkFileExists();
    // saveConfigCallback();

    //wifi setup for first time 
    // if (WiFi.status() == WL_CONNECTED) {
    //   Serial.println("Already connected to WiFi!");
    //   Serial.println("Wait 20s");
    //   delay(20000);
    //   return;
    // }
    
    // Attempt to connect using saved credentials for 20 seconds
    // unsigned long wifiStartTime = millis(); // Record the start time of WiFi connection attempt
    // while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 20000) { // Try for 20 seconds
    //   WiFi.begin(); // Attempt to reconnect using saved credentials
    //   delay(500);
    // }

    // // If WiFi connection is established, display IP address and exit the function
    // if (WiFi.status() == WL_CONNECTED) {
    //   Serial.println("Connected to WiFi!");
    //   Serial.print("IP Address: ");
    //   Serial.println(WiFi.localIP());
    //   return;
    // }

    // // Start the WiFi configuration portal and run it for 1 minute
    // WiFiManager wifiManager;
    // wifiManager.setTimeout(60); // 60 seconds
    // wifiManager.setSaveParamsCallback([]() {
    //   Serial.println("Save new WiFi credentials...");
    // });

    // if (!wifiManager.startConfigPortal(INFLUXDB_BUCKET, "12345678")) {
    //   Serial.println("Failed to connect and hit timeout");
    //   // Handle if the connection attempt fails or times out
    //   return;
    // }

    // // If the control reaches here, the connection was successful or the portal timed out
    // Serial.println("Connected to WiFi");

    // //Ptinting IP address
    // Serial.print("IP Address: ");
    // Serial.println(WiFi.localIP());
    // Serial.println("final_ist");
    // //checkFileExists();
}

// ================================
// MAIN LOOP
// ================================
void loop() {
    //i = i +1 ;
    //pms7003.updateFrame();
    //Serial.print("1");
    
    //delay(20);
    //String datetime = "";

    if (GPS.available()) {
        if (gps.encode(GPS.read())) {
            Serial.println("GPS Data Available");
            i = i+1;
            latitude = gps.location.lat();
            longitude = gps.location.lng();
            atlt = String(gps.altitude.meters());
            noS = String(gps.satellites.value());
            //Serial.println(n);
            latitude = gps.location.lat();
            lati = String(latitude, 6);
            longitude = gps.location.lng();
            longi = String(longitude, 6);
            atlt = gps.altitude.meters();
            noS = gps.satellites.value();
            //ESP.restart();
            
            //dateTime = getDateTimeFromGPS(gps.date, gps.time);
            
            //delay(10);
            rem = i%5;
            if(rem==0) {
                readPMSSensor();
            }
            
            checkFileExists();
            readSensors();

            // Server Data Preparation
            // sensor.clearFields();
            // sensor.addField("temperature", temperature);
            // sensor.addField("humidity", humidity);
            // sensor.addField("pressure", pressure);
            // sensor.addField("gas", gas);
            // sensor.addField("altitude", altitudeBme);
            // sensor.addField("pm1", val1);
            // sensor.addField("pm2.5", val2);
            // sensor.addField("pm10", val3);
            // sensor.addField("lat", lati);
            // sensor.addField("lng", longi);
            // sensor.addField("altitude_gps_1", atlt);
            // sensor.addField("Satellite_1", noS);
            // sensor.addField("g0.3", c_300);
            // sensor.addField("g0.5", c_500);
            // sensor.addField("g1.0", c_1000);
            // sensor.addField("g2.5", c_2500);
            // sensor.addField("g5.0", c_5000);
            // sensor.addField("g10.0", c_10000);
            // sensor.addField("pms_temp", pms_temp);
            // sensor.addField("pms_h", pms_h);
            // sensor.addField("pms_fld", pms_fld);
            // sensor.addField("dateTime", dateTime);
            // sensor.addField("pm1_atm", val4);
            // sensor.addField("pm2.5_atm", val5);
            // sensor.addField("pm10_atm", val6);
            // Serial.print("Writing: ");
            // Serial.println(sensor.toLineProtocol());
            //ESP.restart();
            // Save data to SD card regardless of WiFi connection status    ` `
            
            calculateAQI();
            updateDisplay("Using GPS Time");
            
            //Serial.println(dateTime);
            //Serial.print("now day =");
            //Serial.println(nowDay);
            //Serial.println(nowDay);
            //String x = strtok(dateTime," ").tostr();
            Date = getDateFromGPS(gps.date, gps.time);
            //String dateTime = "";
            dateTime = getDateTimeFromGPS(gps.date, gps.time);
            //Date =  x[0];
            Serial.println("1");
            Serial.println(Date);
            Serial.println("1");
            Serial.println(Day);
            Serial.println(Month);
            Serial.println(Year);

            rem = i%5;
            if(rem==0) {
                updateAQIDisplay();
                //readAndUploadCSVgps(filename);
            }

            // Server Data Upload
            // saveConfigCallback();

            checkFileExists();
            logDataSdCard();
            
            //oldDay = Day;

            // Server WiFi Connection Management
            // if (WiFi.status() == WL_CONNECTED) {
            //   HTTPClient http;
            //   http.begin(test_url); // Send HTTP GET request int httpCode = http.GET();
            //   int httpCode = http.GET();
            //   if(httpCode > 0){
            //     http.end();
            //     if (!client.writePoint(sensor)) {
            //     Serial.print("InfluxDB Cloud write failed: ");
            //     Serial.println(client.getLastErrorMessage());
            //   }
            //   int delimiterIndex = dateTime.indexOf(' ');

            //   String Dated = dateTime.substring(0, delimiterIndex); 
            //   //Serial.println(part1); // Extract the second part of the string 
            //   String Timet = dateTime.substring(delimiterIndex + 1); 
            //   Serial.println("Dated");
            //   Serial.println(Dated);
            //   Serial.println(Timet);

            //   //Serial.println(part2);
            //   int delimiter1Index = Timet.indexOf(':');
            //   String hh = Timet.substring(0, delimiter1Index); 
            //   //Serial.println(part1); // Extract the second part of the string 
            //   String mm = Timet.substring(1,delimiter1Index);
            //   //Serial.println(typeof(mm));
            //   //Serial.println(TypeOf("16"));
            //   //ESP.restart();
            //   //ESP.reset();
            //   //p
            //   Serial.println("hh,mm");
            //   Serial.println(mm);
            //   Serial.println(hh);
            //   Serial.println("hh,mm finished");
            //   //readAndUploadCSVgps(filename);
            //   //if (hh == String(15)) ESP.restart();
            //   if ((hh == "18" )&&  (mm == "25")){
            //     //FTP32 ftp("10.90.18.2", 8087);
            //     int istdate = convertUtcToIstdate(utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond);

            //     int istday = convertUtcToIstday(utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond);

            //     int y = istdate / 100;
                
            //     int m = istdate % 100; 
            //     int d = istday;
            //     d -= 1; // Subtract one day // Handle month transition 
            //     if (d == 0) { 
            //       m -= 1; 
            //       if (m == 0) { 
            //         m = 12; 
            //         y -= 1; // Handle year transition
            //       }
            //       d = daysInMonth(m, y); // Get the last day of the previous month 
            //     } 
            //     if (d < 10) dateTime += "0";
            //     dateTime += d;
            //     dateTime += "-";
            //     if (m < 10) dateTime += "0";
            //     dateTime += m;
            //     dateTime += "-";
            //     dateTime += y;
                

            //     Serial.print("Yesterday's Date: "); 
            //     Serial.print(d); 
            //     Serial.print("/"); 
            //     Serial.print(m); 
            //     Serial.print("/"); 
            //     Serial.println(y);
            //     filename = String(S1) + String(INFLUXDB_BUCKET) + "_" + String(dateTime) + ".csv";
            //     readAndUploadCSVgps(filename);
            //     //uploadonftp(olddate);
            //     //          Ser
            //     //ESP.restart();
            //   }
            //   if ((hh == "03" )&&  (mm == "20")){
            //     ESP.restart();
            //   }
            //   if ((hh == "09" )&&  (mm == "20")){
            //     ESP.restart();
            //   }
            //   if ((hh == "15" )&&  (mm == "20")){
            //     ESP.restart();
            //   }
            //   if ((hh == "21" )&&  (mm == "20")){
            //     ESP.restart();
            //   }
            
            //   }
            
            // } else {
            //   HTTPClient http;
            //   http.begin(test_url);
            //   int httpCode = http.GET();
            //   if(httpCode > 0){
            //     http.end();
            //     connectToWifi();
            //     if (!client.writePoint(sensor)) {
            //       Serial.print("InfluxDB Cloud write failed: ");
            //       Serial.println(client.getLastErrorMessage());
            //     }
            //     int delimiterIndex = dateTime.indexOf(' ');

            //     String Dated = dateTime.substring(0, delimiterIndex); 
            //     //Serial.println(part1); // Extract the second part of the string 
            //     String Timet = dateTime.substring(delimiterIndex + 1); 
            //     Serial.println("Dated");
            //     Serial.println(Dated);
            //     Serial.println(Timet);

            //     //Serial.println(part2);
            //     int delimiter1Index = Timet.indexOf(':');
            //     String hh = Timet.substring(0, delimiter1Index); 
            //     //Serial.println(part1); // Extract the second part of the string 
            //     String mm = Timet.substring(1,delimiter1Index);
            //     //Serial.println(typeof(mm));
            //     //Serial.println(TypeOf("16"));
            //     //ESP.restart();
            //     //ESP.reset();
            //     //p
            //     Serial.println("hh,mm");
            //     Serial.println(mm);
            //     Serial.println(hh);
            //     Serial.println("hh,mm finished");
            //     //if (hh == String(15)) ESP.restart();
            //     if (hh == "18" && mm == "25"){
            //       //FTP32 ftp("10.90.18.2", 8087);
            //       int istdate = convertUtcToIstdate(utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond);

            //       int istday = convertUtcToIstday(utcYear, utcMonth, utcDay, utcHour, utcMinute, utcSecond);

            //       int y = istdate / 100;
                
            //       int m = istdate % 100; 
            //       int d = istday;
            //       d -= 1; // Subtract one day // Handle month transition 
            //       if (d == 0) { 
            //         m -= 1; 
            //         if (m == 0) { 
            //           m = 12; 
            //           y -= 1; // Handle year transition
            //         }
            //         d = daysInMonth(m, y); // Get the last day of the previous month 
            //       } 
            //       if (d < 10) dateTime += "0";
            //       dateTime += d;
            //       dateTime += "-";
            //       if (m < 10) dateTime += "0";
            //       dateTime += m;
            //       dateTime += "-";
            //       dateTime += y;
                

            //       Serial.print("Yesterday's Date: "); 
            //       Serial.print(d); 
            //       Serial.print("/"); 
            //       Serial.print(m); 
            //       Serial.print("/"); 
            //       Serial.println(y);
            //       filename = String(S1) + String(INFLUXDB_BUCKET) + "_" + String(dateTime) + ".csv";
            //       readAndUploadCSVgps(filename);
            //       //uploadonftp(olddate);
            //       //          Ser
            //       //ESP.restart();
                
            //     }

            //     if ((hh == "03" )&&  (mm == "20")){
            //       ESP.restart();
            //     }
            //     if ((hh == "09" )&&  (mm == "20")){
            //       ESP.restart();
            //     }
            //     if ((hh == "15" )&&  (mm == "20")){
            //       ESP.restart();
            //     }
            //     if ((hh == "21" )&&  (mm == "20")){
            //       ESP.restart();
            //     }
            //   }
            // }
        } 
    } else {
        // GPS not available, use RTC as fallback
        Serial.println("GPS not available, using RTC");
        
        i = i+1;
        
        // Set default GPS coordinates when GPS is not available
        latitude = 23.038126;
        longitude = 72.552605;
        lati = String(latitude, 6);
        longi = String(longitude, 6);
        atlt = "0";
        noS = "0";
        
        rem = i%5;
        if(rem==0) {
            readPMSSensor();
        }
        
        checkFileExists();
        readSensors();
        calculateAQI();
        
        // Use RTC for date and time
        dateTime = getRTCDateTime();
        Date = getRTCDate();
        
        updateDisplay("Using RTC Time");
        
        rem = i%5;
        if(rem==0) {
            updateAQIDisplay();
        }
        
        checkFileExists();
        logDataSdCard();
        
        Serial.println("Data logged using RTC time: " + dateTime);
    } 
}