#include "mainMod.h"

void Dashboard::tick() {
  if (state == 0) {
    axius->updateScreen = true;
    axius->drawText("Hi.", -1);
    axius->drawText("Im trying to connect to WIFI", 0);
    axius->drawText("\"kv301\" p/ \"6130prayxdd0801\"", 1);

    if (substate == 0) {
      WiFi.disconnect(true);
      WiFi.config(customIP, INADDR_NONE, INADDR_NONE);
      WiFi.macAddress(customMac);
      WiFi.mode(WIFI_STA);
      WiFi.hostname("AxiusDashboard");
      WiFi.begin("kv301", "6130prayxdd0801"); //да я вот так просто палю пароль на гитхабе, а какая разница, вы всеравно не знаете где я живу)
      connectTimeout = millis();
      substate = 1;
      wifiConnectAttempt++;
    } else if (substate == 1) {
      axius->drawText("Attempt: " + String(wifiConnectAttempt), 2);
      if (WiFi.status() == WL_CONNECTED) {
        delay(500);
        timeClient.begin();
        state = 1;
        substate = 0;
      } else if (WiFi.status() == WL_CONNECT_FAILED) {
        axius->drawText("Password is incorrect!", 3);
        axius->drawText("I will try again in 15s", 4);
        if (millis() - connectTimeout > 15000) substate = 0;
        axius->drawLoadingLine(millis() - connectTimeout, 15000, 5);
      } else {
        if (millis() - connectTimeout > 20000) substate = 0;
        axius->drawText("Waiting for response...", 3);
        axius->drawLoadingLine(millis() - connectTimeout, 20000, 4);
        axius->drawText("Status: " + String(WiFi.status()), 5);
      }
    }
  } else if (state == 1) {
    // code soon
    state = 2;
    loopingTimeout = millis();
    loopState = 0;
    axius->showStatusBar = true;

    //-----------looping trough Boards
  } else if (state == 2) {
    if (WiFi.status() == WL_DISCONNECTED) {
      state = 0;
      substate = 0;
      return;
    }
    if (millis() - loopingTimeout > 2000) {
      loopState++;
      substate = 0;
      loopingTimeout = millis();
      axius->updateScreen = true;
    }
    if (loopState == 0) {  // ----------------------------------------------- TIME TEMERATURE HUMIDITY -----------------------------------------------
      axius->updateScreen = true;
      if (substate == 0) {
        int err = dht11.read(&temperature, &humidity, NULL);
        if (err == SimpleDHTErrSuccess) {
          STemperature = String(temperature) + "`C";  //this ` symbol replaced to ° in my font
          SHumidity = String(humidity) + " F";
        } else {
          Serial.println(err);
          STemperature = "~";
          SHumidity = "~";
        }
        substate++;
      } else {
        uint8_t width;

        width = getTextWidth(STemperature);
        axius->display.drawRoundRect(0, 16, 31, 14, 3, WHITE);
        axius->display.setCursor(15 - width / 2, 25);
        axius->display.print(STemperature);

        width = getTextWidth(SHumidity);
        axius->display.drawRoundRect(128 - 31, 16, 31, 14, 3, WHITE);
        axius->display.setCursor(128 - 15 - width / 2, 25);
        axius->display.print(SHumidity);
      }
      timeClient.update();
      unsigned long epochTime = timeClient.getEpochTime();
      setTime(epochTime);

      axius->display.setFont(&BigNums);

      char formattedTime[6];
      sprintf(formattedTime, (blinkNow(1000) ? "%02d:%02d" : "%02d %02d"), hour(), minute());

      int16_t x1, y1;
      uint16_t textWidth, textHeight;
      axius->display.getTextBounds(formattedTime, 0, 0, &x1, &y1, &textWidth, &textHeight);

      int16_t x = (128 - textWidth) / 2;

      axius->display.setCursor(x, 28);
      axius->display.print(formattedTime);

      axius->resetFont();

      uint8_t maxsize = 0;
      for (uint8_t i = 0; i < 7; i++) {
        uint8_t temp = getTextWidth(week[i]) - 1;
        if (temp > maxsize)
          maxsize = temp;
      }
      uint8_t spaceForEveryDay = 128 / 7;
      int currentWeekday = weekday();
      for (uint8_t i = 0; i < 7; i++) {
        uint8_t dayMinX = (i * spaceForEveryDay);
        uint8_t dayCenter = dayMinX + (spaceForEveryDay / 2);
        if (currentWeekday - 1 == i && blinkNow(500)) {
          axius->display.drawFastHLine(dayMinX, 63, spaceForEveryDay, WHITE);
          axius->display.drawFastHLine(dayMinX, 45, spaceForEveryDay, WHITE);
          axius->display.drawFastVLine(dayMinX, 46, 17, WHITE);
          axius->display.drawFastVLine(dayMinX + spaceForEveryDay, 45, 19, WHITE);
        }
        uint8_t x = dayCenter - (maxsize / 2);
        axius->display.setCursor(x, 64 - 12);
        axius->display.print(week[i]);
        uint32_t dayEpoch = epochTime + (i - (currentWeekday - 1)) * 86400;
        setTime(dayEpoch);
        x = dayCenter - (getTextWidth(String(day())) / 2);
        axius->display.setCursor(x, 64 - 3);
        axius->display.print(String(day()));
      }

      setTime(epochTime);
      axius->display.drawFastHLine(0, 43, 128, WHITE);
      axius->display.drawFastHLine(0, 41, 128, WHITE);
      axius->display.drawPixel(0, 42, WHITE);
      axius->display.drawPixel(127, 42, WHITE);
      axius->display.drawFastHLine(1, 42, uint8_t(127.0 * (float(day()) / float(getDaysInMonth(month(), year())))), WHITE);
      axius->display.setCursor(0, 39);
      axius->display.print(months[month() - 1]);
      uint8_t nextMonthId = (month() + 1 == 13 ? 0 : month());
      axius->display.setCursor(128 - getTextWidth(months[nextMonthId]), 39);
      axius->display.print(months[nextMonthId]);
    } else if (loopState == 1) {  // ----------------------------------------------- Local WebSite -----------------------------------------------
      if (substate == 0) {
        HTTPClient http;
        http.setTimeout(800);
        http.begin(serverIP+"/server-info");
        httpCode = http.GET();
        if (httpCode == 200) {
          payload = http.getString();
          DeserializationError error = deserializeJson(sharedBuffer, payload);
          if (error) {
            parsed = false;
            jsonParseError = error.c_str();
          } else {
            parsed = true;
            uint64_t buff;
            buff = sharedBuffer["total_memory"];
            total_memory = convertUnits(buff);
            buff = sharedBuffer["free_memory"];
            free_memory = convertUnits(buff);
            buff = sharedBuffer["used_memory"];
            used_memory = convertUnits(buff);

            buff = sharedBuffer["disk_total"];
            disk_total = convertUnits64(buff);
            buff = sharedBuffer["disk_free"];
            disk_free = convertUnits64(buff);
            buff = sharedBuffer["disk_used"];
            disk_used = convertUnits64(buff);
          }
        }
        http.end();
        yield();
        substate++;
      } else {
        axius->updateScreen = true;
        if (httpCode == 200) {
          if (parsed) {
            axius->drawText("Server is working!", 0);
            axius->drawText("CPU Usage: " + String(float(sharedBuffer["cpu_usage_percent"])) + "%", 1);
            axius->drawText("RAM Total: " + total_memory + " Free: " + free_memory, 2);
            axius->drawLoadingLine((uint64_t(sharedBuffer["used_memory"]) / 1024), (uint64_t(sharedBuffer["total_memory"]) / 1024), 3);
            axius->drawText("DISK Total: " + disk_total + " Free: " + disk_free, 4);
            axius->drawLoadingLine(uint64_t(sharedBuffer["disk_used"]) / uint64_t(1073741824), uint64_t(sharedBuffer["disk_total"]) / uint64_t(1073741824), 5);
          } else {
            axius->drawText("Server is working!", 0);
            axius->drawText("But resource usage data", 1);
            axius->drawText("cannot be parsed.", 2);
            axius->drawText("Error: " + jsonParseError, 3);
          }
        } else {
          axius->drawText("Website is unreachable!", 0);
          axius->drawText("No data on resource usage.", 1);
          axius->drawText("HTTP code: " + String(httpCode), 2);
        }
      }
    } else if (loopState == 2) {  // ----------------------------------------------- TRAFFIK -----------------------------------------------
      if (substate == 0) {
        lastRouterRequest = millis();
        maxSumbyte = 0;
        HTTPClient http;
        http.setTimeout(1000);
        http.begin("http://192.168.0.1:81/rci/");
        httpCode = http.POST("[{\"show\": {\"ip\": {\"hotspot\": {\"chart\": {\"attributes\": \"sumbytes\",\"detail\": 0,\"items\": \"others\"}}}}}]");
        if (httpCode == 200) {
          payload = http.getString();
          DeserializationError error = deserializeJson(sharedBuffer, payload, DeserializationOption::NestingLimit(200));
          if (error) {
            parsed = false;
            jsonParseError = error.c_str();
          } else {
            parsed = true;
            routerTrafficData.clear();
            JsonArray sumbytesArray = sharedBuffer[0]["show"]["ip"]["hotspot"]["chart"]["bar"][0]["bars"][0]["data"];  //xdd
            for (uint8_t i = 0; i < sumbytesArray.size() / 2; i++) {
              int curSumbyte = sumbytesArray[i]["v"];
              routerTrafficData.push_back(curSumbyte);
              if (curSumbyte > maxSumbyte) maxSumbyte = curSumbyte;
            }
            sharedBuffer.clear();
          }
        }
        http.end();
        yield();
        substate++;
      } else {
        axius->updateScreen = true;
        if (millis() - lastRouterRequest > 1000) {
          substate = 0;
        }
        if (httpCode == 200) {
          if (parsed) {
            axius->drawText("Router traffic diagram " + String(routerTrafficData.size()), 0);
            axius->display.drawFastHLine(0, 20, 128, WHITE);
          
            uint8_t x = 0;
            if (maxSumbyte != 0) {
              for (int8_t i = routerTrafficData.size(); --i >= 0;) {
                x++;
                uint8_t height = ceil(30 * (routerTrafficData[i] / maxSumbyte));
                axius->display.drawFastVLine(x++, 52 - height, height, WHITE);
                axius->display.drawFastVLine(x++, 52 - height, height, WHITE);
                x++;
              }
            }

            axius->display.drawFastHLine(0, 53, 128, WHITE);
            axius->drawText("Peak: " + String(convertUnits(maxSumbyte)), 6);
          } else {
            axius->drawText("Error when parsing -", 0);
            axius->drawText(" - router traffic data", 1);
            axius->drawText("Error: " + jsonParseError, 2);
          }
        } else {
          axius->drawText("Router backend -", 0);
          axius->drawText("    - is unreachable!", 1);
          if (blinkNow(500)) axius->drawText("NO DATA NO DATA ((", 2);
          axius->drawText("HTTP code: " + String(httpCode), 3);
        }
      }
    } else if (loopState == 3) {  // ----------------------------------------------- CONNECTION INFO -----------------------------------------------
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
          const uint8_t BUFFERSIZE = 130;
          int startIndex = 0 - BUFFERSIZE;
          while (stream.available()) {
            char buffer[BUFFERSIZE + 1];
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
          DeserializationError error = deserializeJson(sharedBuffer, payload, DeserializationOption::NestingLimit(200));
          if (error) {
            parsed = false;
            jsonParseError = error.c_str();
          } else {
            parsed = true;
            for (uint8_t i = 0; i < VLANPorts; i++) {
              JsonObject vlanPort = sharedBuffer["show"]["interface"]["GigabitEthernet0/Vlan" + String(i)];
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
            sharedBuffer.clear();
          }
        }
        http.end();
        yield();
        substate++;
      } else {
        axius->updateScreen = true;

        if (millis() - lastRouterRequest > 1000) {
          substate = 0;
        }

        if (httpCode == 200) {
          if (parsed) {
            if (hasIP) {
              axius->drawText("IPv4 " + ipv4Status, 0);
              axius->drawText("Addr.: " + ip4, 1);
              axius->drawText("IPv6 " + ipv6Status, 2);
              axius->drawText("Addr.: " + ip6, 3);
              axius->drawText("Connection uptime: ", 4);
              axius->drawText("     " + formatTime(uptime), 5);
              axius->drawText("MAC: " + mac, 6);
            } else {
              axius->drawText("Ethernet is not connected", 0);
            }
          } else {
            axius->drawText("Error when parsing -", 0);
            axius->drawText(" - Ethernet connection data", 1);
            axius->drawText("Error: " + jsonParseError, 2);
          }
        } else {
          axius->drawText("Router backend -", 0);
          axius->drawText("    - is unreachable!", 1);
          if (blinkNow(500)) axius->drawText("NO DATA NO DATA ((", 2);
          axius->drawText("HTTP code: " + String(httpCode), 3);
        }
      }
    } else if (loopState == 4) {  // ----------------------------------------------- MINECRAFT SERVER INFO -----------------------------------------------
      if (substate == 0) {
        //-------------------------------------------------------------------
        WiFiClient client;

        if (client.connect(minecraftServerIP, minecraftServerPORT)) {
          uint8_t handshakeBuff[64];
          uint8_t hssize = 1;

          /*uint8_t cut = minecraftServerIP.length() % 10;
          uint8_t bytedLength = ((minecraftServerIP.length() - cut) / 10) << 4 | cut;*/
          //handshakeBuff[hssize++] = bytedLength; // size
          handshakeBuff[hssize++] = 0x00; // protocol
          handshakeBuff[hssize++] = 0x04; // protocol
          handshakeBuff[hssize++] = minecraftServerIP.length(); // ip length
          memcpy(&handshakeBuff[hssize], minecraftServerIP.c_str(), minecraftServerIP.length()); // ip bytearray
          hssize += minecraftServerIP.length();
          handshakeBuff[hssize++] = uint8_t(minecraftServerPORT >> 8); // port
          handshakeBuff[hssize++] = uint8_t(minecraftServerPORT & 0xFF); // port
          handshakeBuff[hssize++] = 0x01; // next state

          handshakeBuff[0] = hssize-1; //first packet size

          handshakeBuff[hssize++] = 0x01; // packet size
          handshakeBuff[hssize++] = 0x00; // packet id

          client.write(handshakeBuff, hssize); // write 2 packets simultaneously

          uint32_t startTime = millis();
          while (client.available() == 0) {
            if (millis() - startTime > 300) {
              MCServerPingError = 1;
              substate++;
              return;
            }
          }
          
          String data_string = "";
          bool saving = false;
          while (client.available() > 0) {
            /*uint8_t BUFFSIZE = uint8_t(min(64, client.available()));

            char buffer[BUFFSIZE + 1];
            size_t len = client.readBytes(buffer, BUFFSIZE);
            buffer[len] = '\0';
            String chunk(buffer);

            //Serial.print(chunk);
            data_string += chunk;*/
            // somehow its laggy as fuck, idk what  is the problems

            char c = char(client.read());
            if (!saving && c == '{') saving = true;
            if (saving) data_string += c;
          }
          //Serial.println(data_string);
          client.stop();

          DeserializationError error = deserializeJson(sharedBuffer, data_string, DeserializationOption::NestingLimit(20));
          if (error) {
            MCServerPingError = 2;
          } else {
            if (sharedBuffer["description"].is<JsonObject>() && sharedBuffer["description"]["text"].is<String>())
              MOTDName = sharedBuffer["description"]["text"].as<String>();
            else
              MOTDName = "Hidden name";

            strversion = sharedBuffer["version"]["name"].as<String>();
            maxPlayers = sharedBuffer["players"]["max"].as<uint16_t>();
            curPlayers = sharedBuffer["players"]["online"].as<uint16_t>();
            players.clear();
            if (sharedBuffer["players"]["sample"].is<JsonArray>()) {
              JsonArray samples = sharedBuffer["players"]["sample"].as<JsonArray>();
              for (uint8_t index = 0; index < samples.size(); index++) {
                players.push_back(samples[index]["name"].as<String>());
              }
            }
          }

          MCServerPingError = 0;
        } else {
          MCServerPingError = 3;
        }

        loopingTimeout = millis();
        //-------------------------------------------------------------------
        substate++;
      } else {
        axius->updateScreen = true;
      }

      axius->display.drawRoundRect(0, 14, 61, 11, 4, WHITE);
      axius->display.setFont();
      axius->display.setCursor(4, 16);
      if (MCServerPingError == 0) axius->display.print("MINECRAFT  Players(" + String(curPlayers) + "):");
      else axius->display.print("MINECRAFT");
      axius->resetFont();

      if (MCServerPingError == 0) {
        if (players.size() == 0 && curPlayers > 0) {
          axius->display.setCursor(70, 31);
          axius->display.print("[ Hidden - ");
          axius->display.setCursor(70, 38);
          axius->display.print("- player list ]");
        } else {
          for (uint8_t i = 0; i < players.size(); i++) {
            axius->display.setCursor(70, 31 + 7 * i);
            if (i == 4) {
              axius->display.print("+ " + String(players.size() - i - 1) + " more");
              break;
            } else {
              axius->display.print(players[i]);
            }
          }
        }

        axius->drawText("Max players: " + String(maxPlayers), 2);
        axius->drawText("MOTD Name: ", 3);
        if (getTextWidth(MOTDName) >= 63) {
          String tempMOTDName = String(MOTDName);
          while (true) {
            tempMOTDName = tempMOTDName.substring(0, tempMOTDName.length() - 1);
            if (getTextWidth(tempMOTDName) < 63) break;
          }
          tempMOTDName += "...";
          axius->drawText(tempMOTDName, 4);
        } else {
          axius->drawText(MOTDName, 4);
        }
        axius->drawText("Version: ", 5);
        axius->drawText(strversion, 6);
      } else if (MCServerPingError == 1) {
        axius->drawText(":(", 3);
        axius->drawText("The server is active", 4);
        axius->drawText("but not responding", 5);
      } else if (MCServerPingError == 2) {
        axius->drawText(":(", 3);
        axius->drawText("The server responded w/", 4);
        axius->drawText("unexpecable data", 5);
      } else if (MCServerPingError == 3) {
        axius->drawText(":(", 3);
        axius->drawText("The server is unreachable", 4);
      }
    } else loopState = 0;
  }
}