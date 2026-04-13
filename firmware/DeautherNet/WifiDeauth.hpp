#ifndef _WIFI_DEAUTH_H
#define _WIFI_DEAUTH_H

#include <string>
#include "esp_wifi.h"

class WifiDeauth {
   public:
    WifiDeauth(std::string targetSsid) : targetSsid(targetSsid) {};
    ~WifiDeauth() { close(); };

    bool open();
    bool sendDeauth();
    void close();

   private:
    std::string targetSsid;
    uint16_t seq_num = 0;
    uint8_t spoofed_bssid[6];
};

#endif  //_WIFI_DEAUTH_H
