#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "esp_camera.h"

#include "camera-config.h"

esp_err_t upload_image_to_s3(char *webserver, char *url)
{
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return ESP_FAIL;
    }

    if(fb->format != PIXFORMAT_JPEG){
        ESP_LOGE(TAG, "Non jpeg format");
        return ESP_FAIL;
    } 

    esp_camera_fb_return(fb);

    char s3Request[2048]= {0};
    sprintf(s3Request,"PUT /%s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n", url, webserver, fb->len);

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s/*, r */;

    int err = getaddrinfo(webserver, "80", &hints, &res);

    if(err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    /* Code to print the resolved IP.

        Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */

    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGE(TAG, "... Failed to allocate socket.");
        freeaddrinfo(res);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "... allocated socket");

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "... connected");
    freeaddrinfo(res);

    if (write(s, s3Request, strlen(s3Request)) < 0) {
        close(s);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        ESP_LOGE(TAG, "... socket send failed #1");
    }
    
    if (write(s, (const char *)fb->buf, fb->len) < 0) {
        ESP_LOGE(TAG, "... socket send failed #2");
        close(s);
    }
    
    char *footer = "\r\n";
    if (write(s, footer, strlen(footer)) < 0) {
        ESP_LOGE(TAG, "... socket send failed #3");
        close(s);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "... socket send success");

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout)) < 0) {
        ESP_LOGE(TAG, "... failed to set socket receiving timeout");
        close(s);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "... set socket receiving timeout success");

    /* Read HTTP response */
    /*do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);
        for(int i = 0; i < r; i++) {
            putchar(recv_buf[i]);
        }
    } while(r > 0);

    ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);*/
    close(s);
    
    return ESP_OK;
}