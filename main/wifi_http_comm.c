

/* 
   This file implements functionality to act as a 
   WIFI station and connect to Wifi AP. 
   It also provides the HTTP server to which routes 
   can be added.

   Code in this file has been derived from the 
   ESP-IDF example named `Simple HTTP Server Example`.
   Its license statement below:

   The original example code is in the 
   Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <malloc.h>
#include "wpa2/utils/base64.h"

#include <esp_http_server.h>

#include "wifi_http_comm.h"

/*
 * We use simple WiFi configuration that you can set via
 * 'make menuconfig'.
 * If you'd rather not, just change the below entries to strings
 * with the config you want -
 * ie. #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

static const char *TAG = "WIFI_HTTP_COMM";

// singleton handle on the HTTP server instance
httpd_handle_t server = NULL;

// the handler function that should be called once a WASM module
// has been received and should be loaded to the Interpreter
wasmLoader_t moduleLoaderHandlerFunc = NULL;


/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "WAC API - 404 not found error ");
    return ESP_FAIL;
}

// FIXME
extern void parseAndRunWasm(unsigned char *bytes, size_t byte_count);

/* 
  HTTP POST handler
  receives a Binary WASM file from a client 
*/
esp_err_t wasm_post_handler(httpd_req_t *req)
{
    if (req->content_len <= 0)
    {
        ESP_LOGW(TAG, "wasm_post_handler: missing or invalid Content-Length.");

        /* send HTTP 411 Length Required response */
        httpd_resp_send_err(req, 411, "WAC API - 411 Content-Length missing!");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "wasm_post_handler: %i Bytes of base64 encoded WASM to be received ....",
             req->content_len);

    void *wasm_base64 = malloc(req->content_len);
    char buf[100];
    size_t ret = 0;
    int remaining = req->content_len;

    while (remaining > 0)
    {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                                  MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, " .%i ", (int)ret);
        memcpy((void *)(wasm_base64 + req->content_len - remaining), (void *)buf, ret);
        remaining -= ret;
    }

    ESP_LOGI(TAG, "decoding base64");
    size_t decoded_length;
    unsigned char *wasm_binary = base64_decode(wasm_base64, req->content_len, &decoded_length);
    free(wasm_base64);

    ESP_LOGI(TAG, "Decoding done, received %iBytes binary WASM", (int)decoded_length);

    // The following code line sends the binary WASM code back to the client
    // this can be useful to verify uploading and base64 decoding work ok.
    //httpd_resp_send(req, (char *)wasm_binary, decoded_length);

    /* Send back simple reply */
    char *msg = "OK";
    httpd_resp_send(req, msg, strlen(msg));

    (*moduleLoaderHandlerFunc)((unsigned char *)wasm_binary, decoded_length);

    // End response
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

httpd_uri_t route_run =
    {
        .uri = "/run",
        .method = HTTP_POST,
        .handler = wasm_post_handler,
        .user_ctx = NULL
    };


/* HTTP handler function for URL /hello
   its sole purpose is to make a friendly server
   who greets nicely when clients want to know wac 
   is alive. */
esp_err_t hello_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "hello_get_handler: starting ....");
    char *buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "user-agent") + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "user-agent", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => user-agent: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "url") + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "url", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => url: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            /*
            char param[32];
            // Get value of expected key from query string
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
            }
        }
        */
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "content-type", "text/plain");
    httpd_resp_set_hdr(req, "user-agent", "WAC Web Assembly v0.1");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char *resp_str = (const char *)"Hi, this is WAC, a Web Assembly interpreter!";
    httpd_resp_send(req, resp_str, strlen(resp_str));

    ESP_LOGI(TAG, "hello_get_handler: ... DONE");
    return ESP_OK;
}

httpd_uri_t route_hello = {
    .uri = "/hello",
    .method = HTTP_GET,
    .handler = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = "Hello World!"};

bool start_webserver(void)
{
    ESP_ERROR_CHECK(server != NULL);
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &route_hello);
        httpd_register_uri_handler(server, &route_run);
        return true;
    }

    ESP_LOGI(TAG, "Error starting server!");
    server = NULL;
    return false;
}

void stop_webserver(httpd_handle_t server)
{
    if (server == NULL)
        return;

    // Stop the httpd server
    httpd_stop(server);
    server = NULL;
}

esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    httpd_handle_t *server = (httpd_handle_t *)ctx;

    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        ESP_LOGI(TAG, "Got IP: '%s'",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

        /* Start the web server */
        // if (*server == NULL)
        // {
        start_webserver();
        // }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        ESP_ERROR_CHECK(esp_wifi_connect());

        /* Stop the web server */
        if (*server)
        {
            stop_webserver(*server);
            *server = NULL;
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

void initialiseWifiAndStartServer(wasmLoader_t moduleLoaderHandlerFunc1)
{
    // non-volatile storrage library (seems needed by Wifi)
    ESP_ERROR_CHECK(nvs_flash_init());

    moduleLoaderHandlerFunc = moduleLoaderHandlerFunc1;

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}
