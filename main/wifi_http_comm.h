
#ifndef WIFI_HTTP_H

#define WIFI_HTTP_H

void initialise_wifi(void *arg)
#ifdef ESP_TARGET
;
#else
{ 
    /* empty method unless on ESP32 */
}
#endif

#endif
