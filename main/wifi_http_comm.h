
#ifndef WIFI_HTTP_H

#define WIFI_HTTP_H

typedef void (*wasmLoader_t) (unsigned char* wasmBytes, size_t wasmLength);

void initialiseWifiAndStartServer(wasmLoader_t moduleLoaderHandlerFunc)
#ifdef ESP_TARGET
;
#else
{ 
    /* empty method unless on ESP32 */
}
#endif

#endif
