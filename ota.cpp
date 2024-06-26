// OTA Firmware Update
//
// March 24, 2024
// Tetsu Nishimura
//
//
// Description
// - Check new firmware every 24 hours.
// - If a new firmware is found, set status to ST_PUDATE_ONGOING, then reboot.
// - Download the new firmware and update it.
// - Once firmware update finished, set status to ST_CHECK_UPDATE, then go back to normal operation.

#include <Arduino.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Preferences.h>
#include <Ticker.h>
#include "Mqtt.h"
#include "ota.h"
#include "serverInfo/serverInfo.h"

#define UPDATE_CHECK_INTERVAL   (24 * 3600)   // 24 hours

// Status flags
#define ST_CHECK_UPDATE     0
#define ST_PUDATE_ONGOING   1

Ticker updateMonitorTicker;
extern Preferences prefs;

char firmwareHash[HASH_SIZE + 1];

void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
  // Firmware update finished. Set status flag to ST_CHECK_UPDATE
  prefs.begin("firmware", false);
  prefs.putUShort("status", ST_CHECK_UPDATE);
  prefs.end();
  // System reset occurs
}

void update_progress(int cur, int total) {
  // Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
  Serial.print(".");
}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

void updateFirmware(void)
{
  WiFiClient client;
  t_httpUpdate_return ret;

  Serial.print("Start firmware update ");
  WiFi.begin();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");

  httpUpdate.onStart(update_started);
  httpUpdate.onEnd(update_finished);
  httpUpdate.onProgress(update_progress);
  httpUpdate.onError(update_error);

  ret = httpUpdate.update(client, URL_FIRMWARE_IMAGE);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}

// Get firmware info from web server
String getFirmwareInfo(void)
{
  HTTPClient http;
  String payload;
  int httpCode;

  http.begin(URL_FIRMWARE_INFO);
  httpCode = http.GET();

  if(httpCode > 0) {  // httpCode will be negative on error
      if(httpCode == HTTP_CODE_OK) {
          payload = http.getString();
      }
  } else {
      Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();

  return payload.substring(0, HASH_SIZE);
}

// Periodically monitor new firmware
void updateMonitorHandler(void)
{
  Serial.println("Check new firmware...");

  if (isWifiConnected()) {
    String payload = getFirmwareInfo();
    Serial.print("Server: ");
    Serial.println(payload);
    Serial.print("Local : ");
    Serial.println(firmwareHash);
    
    if (memcmp(payload.c_str(), firmwareHash, HASH_SIZE) == 0) {
      Serial.println("Firmware is up to date");
    }
    else {
      // Set status flag to ST_PUDATE_ONGOING and restart ESP32
      prefs.begin("firmware", false);
      prefs.putUShort("status", ST_PUDATE_ONGOING);
      prefs.putBytes("hash", payload.c_str(), HASH_SIZE);
      prefs.end();

      Serial.println("Restart");
      ESP.restart();
    }
  }
}

void otaInit(void)
{
  uint16_t status;

  prefs.begin("firmware", true);
  status  = prefs.getUShort("status");
  prefs.getBytes("hash", firmwareHash, HASH_SIZE);
  prefs.end();

  if (status == ST_CHECK_UPDATE) {
    updateMonitorTicker.attach(UPDATE_CHECK_INTERVAL, updateMonitorHandler);
  }
  else if (status == ST_PUDATE_ONGOING) {
    updateFirmware();
  }
}
