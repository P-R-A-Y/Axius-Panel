uint32_t onBootFreeHeap = ESP.getFreeHeap();

#include <AxiusSSD.h>
AxiusSSD axius;

#include "mainMod.h"
Dashboard dashboard(&axius);

void setup() {
  randomSeed(148854271337);
  Serial.begin(115200);
  Wire.begin(D5, D6);
  Wire.setClock(400000);

  axius.addMod(&dashboard);

  axius.setLockScreen(lockScreenRender);
  axius.setLastPreparation(lastPreparation);
  axius.setIconApplyer(applyIcons);
  axius.setLockScreenOverrideChecker(isLockScreenOverrided);
  axius.setIncomingPacketListener(onIncomingPacket);
  axius.setIncomingPayloadListener(onIncomingPayload);

  axius.begin("AxiusDashboard", MemoryChip::c256, 800.0f, D7, true, onBootFreeHeap);
}

void onIncomingPayload(float rssi, uint8_t sender, char* prefix, uint8_t payloadSize, uint8_t* payload) {
  
}

void lastPreparation() {
  axius.setButtons(false, D1, D2, D7, false, true);
}

void onIncomingPacket(esppl_frame_info *info) {
  
}

bool isLockScreenOverrided() {
  return true;
}

void lockScreenRender() {
  axius.showStatusBar = false;
  axius.drawText("AFK MODE", -1);
  axius.tomenu();
  axius.tryForceSwitchToMod("Dashboard_main_screen_mod1");
}

void loop() {
  axius.tick();
  axius.endRender();
}

void applyIcons() {
  dashboard.timeIcon();
}
