#ifndef __CAMERA_WEBSERVER_H__
#define __CAMERA_WEBSERVER_H__

#include "esp_event_loop.h"
#include "esp_http_server.h"

#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"

#include "camera-config.h"

/* An HTTP GET handler */
esp_err_t hello_get_handler(httpd_req_t *req);

esp_err_t jpg_stream_httpd_handler(httpd_req_t *req);

static const httpd_uri_t stream = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = jpg_stream_httpd_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


httpd_handle_t start_webserver(void);

void stop_webserver(httpd_handle_t server);

void disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data);

void connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data);


#endif