#ifndef WIFI_DASHBOARD_H
#define WIFI_DASHBOARD_H

#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;
extern AsyncEventSource events;

void webServerSetup(int &count, bool &fishInGate, bool &running, int &lastSensorVal, void (*updateDisplay)(int));
void addLog(const char *msg);
void handleSSEClients();

#endif
