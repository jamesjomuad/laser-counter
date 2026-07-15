#include "wifi_setup.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

void clearWifiSettings() {
  WiFiManager wm;
  wm.resetSettings();
  Serial.println("WiFi credentials cleared via serial — restarting");
  delay(1000);
  ESP.restart();
}

void wifiSetup(int resetBtnPin) {
  WiFiManager wm;
  if (digitalRead(resetBtnPin) == LOW) {
    Serial.println("Reset button held – clearing WiFi credentials");
    wm.resetSettings();
  }
  wm.setConnectTimeout(10);
  wm.setConfigPortalTimeout(180);
  wm.setCaptivePortalEnable(false);
  if (!wm.autoConnect("FishCounter-Setup")) {
    Serial.println("WiFi not configured – running offline");
  } else {
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());
  }
}
