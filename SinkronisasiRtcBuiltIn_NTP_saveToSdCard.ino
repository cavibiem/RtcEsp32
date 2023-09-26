#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Time.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

const char* ssid = "idoenx";
const char* password = "88888888";

const long utcOffsetInSeconds = 25200;

const char* ntpServer = "id.pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);
ESP32Time rtc;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcOffsetInSeconds);

// Days of week dalam bahasa indonesia (hari)
String hari[7] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jum'at", "Sabtu"};

// Month dalam bahasa indonesia (bulan)
String bulan[12] = {"Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli", "Agustus", "September", "Oktober", "November", "Desember"};

// File untuk menyimpan waktu terakhir di kartu SD
File waktuTerakhir;

void setup() {
  Serial.begin(9600);
  
  // Inisialisasi kartu SD
  if (!SD.begin(5)) {
    Serial.println("Gagal inisialisasi kartu SD.");
    return;
  }

  // Coba membuka file waktu terakhir
  waktuTerakhir = SD.open("/waktu_terakhir.txt");
  if (waktuTerakhir) {
    // Baca waktu terakhir dari file
    String waktuTerakhirStr = waktuTerakhir.readStringUntil('\n');
    waktuTerakhir.close();
    
    // Konversi waktu terakhir ke struct tm
    struct tm waktuTerakhirTM;
    sscanf(waktuTerakhirStr.c_str(), "%04d-%02d-%02d %02d:%02d:%02d",
           &waktuTerakhirTM.tm_year, &waktuTerakhirTM.tm_mon, &waktuTerakhirTM.tm_mday,
           &waktuTerakhirTM.tm_hour, &waktuTerakhirTM.tm_min, &waktuTerakhirTM.tm_sec);
    
    // Set RTC dengan waktu terakhir
    rtc.setTimeStruct(waktuTerakhirTM);
  } else {
    Serial.println("Gagal membaca file waktu terakhir.");
  }

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print("Created by...");
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("Idoenx Cavibiem");
  delay(4000);
  lcd.clear();

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  lcd.setCursor(0, 0);
  lcd.print("Connecting");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    for (int i = 10; i < 16; i++) {
      lcd.setCursor(i, 0);
      lcd.print(".");
      Serial.print(".");
      delay(500);
    }
  }
  Serial.println("Berhasil terhubung WiFi");
  lcd.setCursor(2, 1);
  lcd.print("Connected");
  delay(2000);
  lcd.clear();

  // Ambil data waktu NTP
  timeClient.begin();
  timeClient.update();
  Serial.print("Waktu NTP: ");
  Serial.println(timeClient.getFormattedTime());

  /*---------set RTC with NTP---------------*/
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    rtc.setTimeStruct(timeinfo);
  }
  Serial.print("Waktu RTC: ");
  Serial.println(rtc.getTime("%A, %d %B %Y %H:%M:%S"));
}

void loop() {
  rtc.getTime();
  struct tm timeinfo = rtc.getTimeStruct();

  Serial.print("Tanggal: ");
  Serial.print(hari[timeinfo.tm_wday]);
  Serial.print(", ");
  Serial.print(timeinfo.tm_mday);
  Serial.print(" ");
  Serial.print(bulan[timeinfo.tm_mon]);
  Serial.print(" ");
  Serial.print(timeinfo.tm_year + 1900); // Tambah 1900 karena tm_year adalah tahun sejak 1900
  Serial.print("  Jam: ");
  Serial.print(timeinfo.tm_hour);
  Serial.print(":");
  Serial.print(timeinfo.tm_min);
  Serial.print(":");
  Serial.println(timeinfo.tm_sec);
  delay(2000);

  // Simpan waktu terakhir ke kartu SD
  if (SD.exists("/waktu_terakhir.txt")) {
    SD.remove("/waktu_terakhir.txt");
  }
  waktuTerakhir = SD.open("/waktu_terakhir.txt", FILE_WRITE);
  if (waktuTerakhir) {
    waktuTerakhir.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                         timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    waktuTerakhir.close();
  } else {
    Serial.println("Gagal membuat file waktu terakhir.");
  }

  // Tunggu sebentar sebelum mengambil waktu dari NTP lagi
  delay(1000); // 1 menit
}
