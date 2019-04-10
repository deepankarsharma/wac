
#ifndef WIFI_HTTP_H

#define WIFI_HTTP_H

void initialiseWifiAndStartServer(esp_event_handler_t moduleLoaderHandlerFunc)
#ifdef ESP_TARGET
;
#else
{ 
    /* empty method unless on ESP32 */
}
#endif

#endif
