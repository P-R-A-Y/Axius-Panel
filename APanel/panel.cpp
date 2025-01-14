#include "panel.h"

void Panel::tick() {
  if (state == 0) {
    axius.updateScreen = true;
    axius.drawText("Hi.", -1);
    axius.drawText("Im trying to connect to", 0);
    axius.drawText("WIFI \"11\" w/ pass \"91809791\"", 1);

    if (substate == 0) {
      WiFi.disconnect(true);
      WiFi.config(customIP, INADDR_NONE, INADDR_NONE);
      WiFi.macAddress(customMac);
      WiFi.mode(WIFI_STA);
      WiFi.hostname("AxiusPanel");
      WiFi.begin("11", "91809791");
      connectTimeout = millis();
      substate = 1;
      wifiConnectAttempt++;
    } else if (substate == 1) {
      axius.drawText("Attempt: "+String(wifiConnectAttempt), 2);
      if (WiFi.status() == WL_CONNECTED) {
        delay(500);
        timeClient.begin();
        state = 1;
        substate = 0;
      } else if (WiFi.status() == WL_CONNECT_FAILED) {
        axius.drawText("Password is incorrect!", 3);
        axius.drawText("I will try again in 15s", 4);
        if (millis() - connectTimeout > 15000) substate = 0;
        axius.drawLoadingLine(millis() - connectTimeout, 15000, 5);
      } else {
        if (millis() - connectTimeout > 20000) substate = 0;
        axius.drawText("Waiting for response...", 3);
        axius.drawLoadingLine(millis() - connectTimeout, 20000, 4);
        axius.drawText("Status: "+String(WiFi.status()), 5);
      }
    }
  } else if (state == 1) {
    // code soon
    state = 2;
    loopingTimeout = millis();
    loopState = 0;
    axius.showStatusBar = true;

  //-----------looping trough panels
  } else if (state == 2) {
    if (WiFi.status() == WL_DISCONNECTED) {
      state = 0;
      substate = 0;
      return;
    }
    if (millis() - loopingTimeout > 4000) {
      loopState++;
      substate = 0;
      loopingTimeout = millis();
      axius.updateScreen = true;
    }
    if (loopState == 0) { // ----------------------------------------------- TIME TEMERATURE HUMIDITY -----------------------------------------------
      axius.updateScreen = true;
      if (substate == 0) {
        int err = dht11.read(&temperature, &humidity, NULL);
        if (err != SimpleDHTErrSuccess) {
          STemperature = String(temperature);
          SHumidity    = String(humidity   );
        } else {
          STemperature = "~";
          SHumidity    = "~";
        }
        substate++;
      } else {
        axius.display.setCursor(0, 25);
        axius.display.print(STemperature);
      }
      timeClient.update();
      unsigned long epochTime = timeClient.getEpochTime();
      setTime(epochTime);
      
      axius.display.setFont(&BigNums);
      
      char formattedTime[6];
      sprintf(formattedTime, (blinkNow(1000)?"%02d:%02d":"%02d %02d"), hour(), minute());
      
      int16_t x1, y1;
      uint16_t textWidth, textHeight;
      axius.display.getTextBounds(formattedTime, 0, 0, &x1, &y1, &textWidth, &textHeight);

      int16_t x = (128 - textWidth) / 2;

      axius.display.setCursor(x, 28);
      axius.display.print(formattedTime);

      axius.resetFont();
      
      uint8_t maxsize = 0;
      for (uint8_t i=0;i<7;i++) {
        uint8_t temp = getTextWidth(week[i]) - 1;
        if (temp > maxsize) 
          maxsize = temp;
      }
      uint8_t spaceForEveryDay = 128 / 7;
      int currentWeekday = weekday();
      for (uint8_t i = 0; i < 7; i++) {
        uint8_t dayMinX = (i * spaceForEveryDay);
        uint8_t dayCenter = dayMinX + (spaceForEveryDay / 2);
        if (currentWeekday-1 == i && blinkNow(500)) {
          axius.display.drawFastHLine(dayMinX, 63, spaceForEveryDay, WHITE);
          axius.display.drawFastHLine(dayMinX, 45, spaceForEveryDay, WHITE);
          axius.display.drawFastVLine(dayMinX, 46, 17, WHITE);
          axius.display.drawFastVLine(dayMinX+spaceForEveryDay, 45, 19, WHITE);
        }
        uint8_t x = dayCenter - (maxsize / 2);
        axius.display.setCursor(x, 64-12);
        axius.display.print(week[i]);
        uint32_t dayEpoch = epochTime + (i - (currentWeekday - 1)) * 86400;
        setTime(dayEpoch);
        x = dayCenter - (getTextWidth(String(day())) / 2);
        axius.display.setCursor(x, 64-3);
        axius.display.print(String(day()));
      }

      setTime(epochTime);
      axius.display.drawFastHLine(0,   43, 128, WHITE);
      axius.display.drawFastHLine(0,   41, 128, WHITE);
      axius.display.drawPixel    (0,   42,      WHITE);
      axius.display.drawPixel    (127, 42,      WHITE);
      axius.display.drawFastHLine(1,   42, uint8_t(127.0*(float(day()) / float(getDaysInMonth(month(), year())))), WHITE);
      axius.display.setCursor(0, 39);
      axius.display.print(months[month()-1]);
      uint8_t nextMonthId = (month() + 1 == 13 ? 0 : month());
      axius.display.setCursor(128 - getTextWidth(months[nextMonthId]), 39);
      axius.display.print(months[nextMonthId]);
    } else if (loopState == 1) { // ----------------------------------------------- Local WebSite -----------------------------------------------
      if (substate == 0) {
        HTTPClient http;
        http.setTimeout(800);
        http.begin("http://192.168.0.72/server-info");
        httpCode = http.GET();
        if (httpCode == 200) {
          payload = http.getString();
          DeserializationError error = deserializeJson(doc, payload);
          if (error) {
            parsed = false;
            jsonParseError = error.c_str();
          } else {
            parsed = true;
            total_memory = convertUnits(doc["total_memory"]);
            free_memory  =  convertUnits(doc["free_memory"]);
            used_memory  =  convertUnits(doc["used_memory"]);
            uint64_t buff;
            buff = doc["disk_total"];
            disk_total = convertUnits64(buff);
            buff = doc["disk_free"];
            disk_free = convertUnits64(buff);
            buff = doc["disk_used"];
            disk_used = convertUnits64(buff);
          }
        }
        http.end();
        yield();
        substate++;
      } else {
        axius.updateScreen = true;
        if (httpCode == 200) {
          if (parsed) {
            axius.drawText("Server is working!", 0);
            axius.drawText("CPU Usage: "+String(float(doc["cpu_usage_percent"]))+"%", 1);
            axius.drawText("RAM Total: "+total_memory+" Free: "+free_memory, 2);
            axius.drawLoadingLine((uint32_t(doc["used_memory"]) / 1024.0), (uint32_t(doc["total_memory"]) / 1024.0), 3);
            axius.drawText("DISK Total: "+disk_total+" Free: "+disk_free, 4);
            axius.drawLoadingLine(float(uint64_t(doc["disk_used"]) / 1073741824.0), float(uint64_t(doc["disk_total"]) / 1073741824.0), 5);
          } else {
            axius.drawText("Server is working!", 0);
            axius.drawText("But resource usage data", 1);
            axius.drawText("cannot be parsed.", 2);
            axius.drawText("Error: "+jsonParseError, 3);
          }
        } else {
          axius.drawText("Website is unreachable!", 0);
          axius.drawText("No data on resource usage.", 1);
          axius.drawText("HTTP code: "+String(httpCode), 2);
        }
      }
    } else if (loopState == 2) {
      if (substate == 0) {
        lastRouterRequest = millis();
        maxSumbyte = 0.0;
        HTTPClient http;
        http.setTimeout(2000);
        http.begin("http://192.168.0.1:81/rci/");
        httpCode = http.POST("[{\"show\": {\"ip\": {\"hotspot\": {\"chart\": {\"attributes\": \"sumbytes\",\"detail\": 0,\"items\": \"others\"}}}}}]");
        if (httpCode == 200) {
          payload = http.getString();
          DeserializationError error = deserializeJson(docRouter, payload, DeserializationOption::NestingLimit(200));
          if (error) {
            parsed = false;
            jsonParseError = error.c_str();
          } else {
            parsed = true;
            routerTrafficData.clear();
            JsonArray sumbytesArray = docRouter[0]["show"]["ip"]["hotspot"]["chart"]["bar"][0]["bars"][0]["data"]; //ddx
            for (uint8_t i = 0; i < sumbytesArray.size()/2; i++) { // 64 trunc to 32
              int curSumbyte = sumbytesArray[i]["v"];
              routerTrafficData.push_back(curSumbyte);
              if (curSumbyte > maxSumbyte) maxSumbyte = curSumbyte;
            }
            docRouter.clear();
          }
        }
        http.end();
        yield();
        substate++;
      } else {
        axius.updateScreen = true;
        if (millis() - lastRouterRequest > 1000) {
          substate = 0;
        }
        if (httpCode == 200) {
          if (parsed) {
            axius.drawText("Router traffic diagram "+String(routerTrafficData.size()), 0);
            axius.display.drawFastHLine(0, 20, 128, WHITE);

            uint8_t x = 0;
            for (int8_t i = routerTrafficData.size(); --i >= 0;) {
              x++;
              uint8_t height = ceil(30 * (routerTrafficData[i] / maxSumbyte));
              axius.display.drawFastVLine(x++, 52-height, height, WHITE);
              axius.display.drawFastVLine(x++, 52-height, height, WHITE);
              x++;
            }

            axius.display.drawFastHLine(0, 53, 128, WHITE);
            axius.drawText("Peak: "+String(convertUnits(maxSumbyte)), 6);
          } else {
            axius.drawText("Error when parsing -", 0);
            axius.drawText(" - router traffic data", 1);
            axius.drawText("Error: "+jsonParseError, 2);
          }
        } else {
          axius.drawText("Router backend -", 0);
          axius.drawText("    - is unreachable!", 1);
          if (blinkNow(500)) axius.drawText("NO DATA NO DATA ((", 2);
          axius.drawText("HTTP code: "+String(httpCode), 3);
        }
      }
    } else if (loopState == 3) {
      if (substate == 0) {
        lastRouterRequest = millis();
        HTTPClient http;
        http.setTimeout(2000);
        http.begin("http://192.168.0.1:81/rci/");
        httpCode = http.POST("[{\"show\": {\"interface\": {}}}]");
        if (httpCode == 200) {
          //-----------getting payload
          WiFiClient& stream = http.getStream();
          String payload = "{\"show\": {\"interface\": {";

          uint32_t starttime = millis();
          bool caching = false;
          const uint8_t BUFFERSIZE = 128;
          int startIndex = 0 - BUFFERSIZE;
          while (stream.available()) {
            char buffer[BUFFERSIZE+1];
            size_t len = stream.readBytes(buffer, BUFFERSIZE);
            buffer[len] = '\0';
            
            String chunk(buffer);

            if (!caching) {
              int index = chunk.indexOf("n1\":");
              if (index > 0) {
                chunk = chunk.substring(chunk.indexOf("        \"GigabitEthernet0/Vlan1\": {"));
                caching = true;
                payload += chunk;
                startIndex += BUFFERSIZE;
              }
            } else {
              payload += chunk;
              
              int index = payload.indexOf("r0\":", startIndex + BUFFERSIZE - 25);
              if (index > 0) {
                payload = payload.substring(0, payload.indexOf("        \"WifiMaster0\": {", startIndex + BUFFERSIZE - 25) - 2);
                caching = false;
                payload += "}}}";
                stream.stop();
                break;
              }

              startIndex += BUFFERSIZE;
            }
            yield();
          }
          //-----------getting payload
          DeserializationError error = deserializeJson(docRouter, payload, DeserializationOption::NestingLimit(200));
          if (error) {
            parsed = false;
            jsonParseError = error.c_str();
          } else {
            parsed = true;
            for (uint8_t i = 0; i < VLANPorts; i++) {
              JsonObject vlanPort = docRouter["show"]["interface"]["GigabitEthernet0/Vlan"+String(i)];
              if (vlanPort["address"].is<String>()) {
                hasIP = true;
                ipv4Status = vlanPort["summary"]["layer"]["ipv4"].as<String>();
                ipv6Status = vlanPort["summary"]["layer"]["ipv6"].as<String>();
                ip4 = vlanPort["address"].as<String>();
                ip6 = "undefined";
                if (vlanPort["ipv6"].is<JsonObject>()) {
                  if (vlanPort["ipv6"]["addresses"].size() > 0) {
                    ip6 = vlanPort["ipv6"]["addresses"][0]["address"].as<String>();
                  }
                }
                mac = vlanPort["mac"].as<String>();
                uptime = vlanPort["uptime"];
                break;
              } else hasIP = false;
            }
            docRouter.clear();
          }
        }
        http.end();
        yield();
        substate++;
      } else {
        axius.updateScreen = true;

        if (millis() - lastRouterRequest > 1000) {
          substate = 0;
        }

        if (httpCode == 200) {
          if (parsed) {
            if (hasIP) {
              axius.drawText("IPv4 "+ipv4Status, 0);
              axius.drawText("Addr.: "+ip4, 1);
              axius.drawText("IPv6 "+ipv6Status, 2);
              axius.drawText("Addr.: "+ip6, 3);
              axius.drawText("Connection uptime: ", 4);
              axius.drawText("     "+formatTime(uptime), 5);
              axius.drawText("MAC: "+mac, 6);
            } else {
              axius.drawText("Ethernet is not connected", 0);
            }
          } else {
            axius.drawText("Error when parsing -", 0);
            axius.drawText(" - Ethernet connection data", 1);
            axius.drawText("Error: "+jsonParseError, 2);
          }
        } else {
          axius.drawText("Router backend -", 0);
          axius.drawText("    - is unreachable!", 1);
          if (blinkNow(500)) axius.drawText("NO DATA NO DATA ((", 2);
          axius.drawText("HTTP code: "+String(httpCode), 3);
        }
      }
    } else loopState = 0;
  }
}