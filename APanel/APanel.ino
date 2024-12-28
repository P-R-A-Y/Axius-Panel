uint32_t onBootFreeHeap = ESP.getFreeHeap();

#include <AxiusSSD.h>
AxiusSSD axius;

#include "panel.h"
Panel panel;

void setup() {
  randomSeed(148854271337);
  Serial.begin(115200);
  Wire.begin(D5, D6);
  Wire.setClock(400000);

  axius.addMod(&panel);

  axius.setLockScreen(lockScreenRender);
  axius.setLastPreparation(lastPreparation);
  axius.setIconApplyer(applyIcons);
  axius.setLockScreenOverrideChecker(isLockScreenOverrided);
  axius.setIncomingPacketListener(onIncomingPacket);
  axius.setIncomingPayloadListener(onIncomingPayload);

  axius.begin("AxiusPanel v1", MemoryChip::c256, 1500.0f, D0, true, onBootFreeHeap);
}

void onIncomingPayload(float rssi, uint8_t sender, char* prefix, uint8_t payloadSize, uint8_t* payload) {
  
}

void lastPreparation() {
  axius.setButtons(false, D0, D0, D0, false, true);
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
  axius.tryForceSwitchToMod("panel_main_screen_mod1");
}

uint32_t lastSensorsCheck = 0;
void loop() {
  axius.tick();
  axius.endRender();
}

void applyIcons() {
  
}
