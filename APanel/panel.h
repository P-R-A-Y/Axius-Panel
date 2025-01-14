#include <Arduino.h>

#include <AxiusSSD.h>
extern AxiusSSD axius;

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include "BigNums.h"
#include <TimeLib.h>
#include <globalstructures.h>
#include <vector>
#include <SimpleDHT.h>

#define VLANPorts 3

extern "C" {
  #include "user_interface.h"
  typedef void (*freedom_outside_cb_t)(uint8 status);
  int  wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
  void wifi_unregister_send_pkt_freedom_cb(void);
  int  wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

class Panel : public Mod {
public:
  Panel() : timeClient(ntpUDP, "europe.pool.ntp.org", 3600*3, 300000), customIP(192, 168, 0, 80), dht11(D4) {};
  void tick() override;
  void firsttick() override {
    axius.showStatusBar = false;
    axius.stopPacketListening();
    wifi_promiscuous_enable(false);
    wifi_fpm_close();
    wifi_set_opmode(STATION_MODE);
    wifi_station_disconnect();
  };
  void setup() override {};
  String getName() override {return "panel_main_screen_mod1";}
private:
  WiFiUDP ntpUDP;
  NTPClient timeClient;
  SimpleDHT11 dht11;
  //web
  const char* serverIP = "http://192.168.0.72";

  uint8_t state = 0, substate = 0, wifiConnectAttempt = 0, loopState = 0;
  uint32_t connectTimeout = 0, loopingTimeout = 0;
  bool localServerAvaliable = false, parsed = false;

  int httpCode = 0;
  String payload;
  StaticJsonDocument<300> doc;
  StaticJsonDocument<10240> docRouter;
  std::vector<int> routerTrafficData;
  String total_memory, free_memory, used_memory, disk_total, disk_free, disk_used;

  String jsonParseError;
  float maxSumbyte = 0;
  uint32_t lastRouterRequest = 0;

  bool hasIP = false;
  uint32_t uptime = 0;
  String ip4, ip6, mac, ipv4Status, ipv6Status;

  uint8_t customMac[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
  IPAddress customIP;

  String week[7] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};
  String months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

  uint8_t temperature, humidity;
  String STemperature, SHumidity;


public:
  String convertUnits(uint32_t bytes) {
    if (bytes >= 1099511627776) { // 1 TB
      return String(bytes / 1099511627776.0, 2) + "T";
    } else if (bytes >= 1073741824) { // 1 GB
      return String(bytes / 1073741824.0, 2) + "G";
    } else if (bytes >= 1048576) { // 1 MB
      return String(bytes / 1048576.0, 2) + "M";
    } else if (bytes >= 1024) { // 1 KB
      return String(bytes / 1024.0, 2) + "K";
    } else {
      return String(bytes) + "B";
    }
  }

  String uint64_tToString(uint64_t ll) {
    String result = "";
    do {
      result = int(ll % 10) + result;
      ll /= 10;
    } while (ll != 0);
    return result;
  }

  String convertUnits64(uint64_t bytes) {
    if (bytes >= 1099511627776ULL) { // 1 TB
      return uint64_tToString(bytes / 1099511627776.0) + "T";
    } else if (bytes >= 1073741824ULL) { // 1 GB
      return uint64_tToString(bytes / 1073741824.0) + "G";
    } else if (bytes >= 1048576ULL) { // 1 MB
      return uint64_tToString(bytes / 1048576.0) + "M";
    } else if (bytes >= 1024ULL) { // 1 KB
      return uint64_tToString(bytes / 1024.0) + "K";
    } else {
      return uint64_tToString(bytes) + "B";
    }
  }

  uint16_t getTextWidth(String text) {
    int16_t buff;
    uint16_t textWidth, ubuff;
    axius.display.getTextBounds(text, 0, 0, &buff, &buff, &textWidth, &ubuff);
    return textWidth;
  }

  int getDaysInMonth(int currentMonth, int currentYear) {
    if (currentMonth == 1 || currentMonth == 3 || currentMonth == 5 || currentMonth == 7 || currentMonth == 8 || currentMonth == 10 || currentMonth == 12) {
      return 31;
    } else if (currentMonth == 4 || currentMonth == 6 || currentMonth == 9 || currentMonth == 11) {
      return 30;
    } else if (currentMonth == 2) {
      if ((currentYear % 4 == 0 && currentYear % 100 != 0) || (currentYear % 400 == 0)) {
        return 29; 
      } else {
        return 28;
      }
    }
    return 0;
  }

  String formatTime(uint32_t seconds) {
    uint32_t secondsLeft = seconds % 60;      // Остаток секунд
    uint32_t totalMinutes = seconds / 60;

    uint32_t minutes = totalMinutes % 60;    // Остаток минут
    uint32_t totalHours = totalMinutes / 60;

    uint32_t hours = totalHours % 24;        // Остаток часов
    uint32_t totalDays = totalHours / 24;

    uint32_t days = totalDays % 30;          // Остаток дней (условно 30 дней в месяце)
    uint32_t months = totalDays / 30;        // Месяцы (условно)

    String result = "";
    if (months > 0) result += "M" + String(months) + " ";
    if (days > 0) result += "D" + String(days) + " ";
    if (hours > 0) result += "H" + String(hours) + " ";
    if (minutes > 0) result += "M" + String(minutes) + " ";
    if (secondsLeft > 0) result += "S" + String(secondsLeft) + " ";

    return result;
  }
};