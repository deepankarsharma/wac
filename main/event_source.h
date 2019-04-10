
#ifndef EVENT_SOURCE_H_
#define EVENT_SOURCE_H_

#include "esp_event.h"
#include "esp_event_loop.h"


// Defime and Declare an event base
ESP_EVENT_DECLARE_BASE(WAC_HTTPD_EVENTS);

enum {                             
    WAC_HTTPD_INIT_MODULE_EVENT,        
    WAC_HTTPD_CALL_FUNC_EVENT
};


#endif // #ifndef EVENT_SOURCE_H_