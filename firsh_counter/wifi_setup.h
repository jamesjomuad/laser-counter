#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

// WiFiManager uses ESP8266WebServer internally — keep it in its own
// compilation unit so the enum doesn't clash with ESPAsyncWebServer.

void wifiSetup(int resetBtnPin);
void clearWifiSettings();

#endif
