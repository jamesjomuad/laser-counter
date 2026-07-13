#ifndef WIFI_DASHBOARD_H
#define WIFI_DASHBOARD_H

#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

void wifiSetup(int resetBtnPin);
void webServerSetup(int &count, bool &fishInGate, bool &running, int &lastSensorVal, void (*updateDisplay)(int));
void addLog(const char *msg);
void clearWifiSettings();

#endif
