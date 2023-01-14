#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"

#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"

#include "camera-config.h"
#include "camera-s3-upload.h"
#include "camera-webserver.h"

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* CA Root certificate, device ("Thing") certificate and device
 * ("Thing") key.

   Example can be configured one of two ways:

   "Embedded Certs" are loaded from files in "certs/" and embedded into the app binary.

   "Filesystem Certs" are loaded from the filesystem (SD card, etc.)

   See example README for more details.
*/
#if defined(CONFIG_EXAMPLE_EMBEDDED_CERTS)

extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");
extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_pem_crt_end[] asm("_binary_certificate_pem_crt_end");
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[] asm("_binary_private_pem_key_end");

#elif defined(CONFIG_EXAMPLE_FILESYSTEM_CERTS)

static const char * DEVICE_CERTIFICATE_PATH = CONFIG_EXAMPLE_CERTIFICATE_PATH;
static const char * DEVICE_PRIVATE_KEY_PATH = CONFIG_EXAMPLE_PRIVATE_KEY_PATH;
static const char * ROOT_CA_PATH = CONFIG_EXAMPLE_ROOT_CA_PATH;

#else
#error "Invalid method for loading certs"
#endif

#define BUTTON GPIO_NUM_12
#define GREEN_LED_GPIO 15
#define RED_LED_GPIO 13
#define YELLOW_LED_GPIO 14

uint8_t base_mac_addr[6] = {0};
uint32_t aws_host_port = AWS_IOT_MQTT_PORT;
char aws_host_address[255] = AWS_IOT_MQTT_HOST;
char t_filename[41] = "";

bool buttonPressedDown = false;
bool buttonTrigger = false;
bool urlTrigger = false;

/**
 * Function signatures
 * */
static void init_app();
static void init_wifi(void);
void subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData);
void disconnect_callback_handler(AWS_IoT_Client *pClient, void *data);
void aws_mqtt_task(void *param);

static esp_err_t event_handler(void *ctx, system_event_t *event);
char** str_split(char* a_str, const char a_delim);
int get_led_to_turn(char *payload);

/** 
 * Description: Setting initial vars with zeros. Reading out MAC Address for device identification
 * */
static void init_app() 
{
    gpio_pad_select_gpio(BUTTON);
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    gpio_pulldown_en(BUTTON);
    gpio_pullup_dis(BUTTON);

    gpio_pad_select_gpio(RED_LED_GPIO);
    gpio_pad_select_gpio(YELLOW_LED_GPIO);
    gpio_pad_select_gpio(GREEN_LED_GPIO);
    /* Set the GPIO as a push/pull output for LED */
    gpio_set_direction(RED_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(YELLOW_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED_GPIO, GPIO_MODE_OUTPUT);
        
    esp_err_t ret = ESP_OK;
    esp_efuse_mac_get_default(base_mac_addr);

    ret = esp_efuse_mac_get_default(base_mac_addr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get base MAC address from EFUSE BLK0. (%s)", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Aborting");
        abort();
    } else {
        ESP_LOGI(TAG, "Base MAC Address read from EFUSE BLK0.");
        esp_log_buffer_hexdump_internal(TAG, base_mac_addr, 6, ESP_LOG_INFO);
    }    
}


/**
 * Description: Wifi init
 * */
static void init_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS
        },
    };

    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

/**
 * Description: Handler for system events
 * */
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

/**
 * Description: Callback for MQTT url subscription
 * */
void subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData) {
    ESP_LOGI(TAG, "Subscribe callback");
    //ESP_LOGI(TAG, "%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)params->payload);

    char* t_topicName = malloc (sizeof(char) * topicNameLen+1);
    strlcpy(t_topicName, topicName, topicNameLen+1);
    ESP_LOGI(TAG, "Topic: %s", t_topicName);

    char* t_payload = malloc (sizeof(char) * (int) params->payloadLen+1);
    strlcpy(t_payload, (char *)params->payload, (int) params->payloadLen+1);

    if (strcmp(t_topicName, "esp32/sub/url") == 0) {
        char** tokens;
        tokens = str_split(t_payload, '/');

        
        char* t_url = malloc(128 * sizeof(char));
        char* t_params = malloc(1280 * sizeof(char));

        if (tokens)
        {
            int i;
            for (i = 0; *(tokens + i); i++)
            {
                if (i == 0) {
                    strlcpy(t_filename, *(tokens + i), strlen(*(tokens + i))+1);
                    ESP_LOGI(TAG, "Filename: %s", t_filename);
                }
                if (i == 1) {
                    strlcpy(t_url, *(tokens + i), strlen(*(tokens + i))+1);
                    ESP_LOGI(TAG, "URL: %s", t_url);
                }
                if (i == 2) {
                    strlcpy(t_params, *(tokens + i), strlen(*(tokens + i))+1);
                    ESP_LOGI(TAG, "Params: %s", t_params);
                }
                free(*(tokens + i));
            }
            free(tokens);

            if (i == 3) {
                if (upload_image_to_s3(t_url, t_params) == ESP_OK) {
                    urlTrigger = true;
                }
            }
        }

        free(t_url);
        free(t_params);
    } else if (strcmp(t_topicName, "esp32/sub/data") == 0) {
        ESP_LOGI(TAG, "Detected results: %s", t_payload);
        gpio_set_level(get_led_to_turn(t_payload), 1);
    } 

    free(t_topicName);
    free(t_payload);
}

/**
 * Description: Handler if mqtt disconnects and attempt reconnecting
 * */
void disconnect_callback_handler(AWS_IoT_Client *pClient, void *data) {
    ESP_LOGW(TAG, "MQTT Disconnect");
    IoT_Error_t rc = FAILURE;

    if(NULL == pClient) {
        return;
    }

    if(aws_iot_is_autoreconnect_enabled(pClient)) {
        ESP_LOGI(TAG, "Auto Reconnect is enabled, Reconnecting attempt will start now");
    } else {
        ESP_LOGW(TAG, "Auto Reconnect not enabled. Starting manual reconnect...");
        rc = aws_iot_mqtt_attempt_reconnect(pClient);
        if(NETWORK_RECONNECTED == rc) {
            ESP_LOGW(TAG, "Manual Reconnect Successful");
        } else {
            ESP_LOGW(TAG, "Manual Reconnect Failed - %d", rc);
        }
    }
}

/**
 * Description: Main task for handling MQTT transport
 * */
void aws_mqtt_task(void *param) {
    char cPayload[128];

    IoT_Error_t rc = FAILURE;

    AWS_IoT_Client client;
    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

    IoT_Publish_Message_Params paramsQOS0;

    ESP_LOGI(TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    mqttInitParams.enableAutoReconnect = false; // We enable this later below
    mqttInitParams.pHostURL = aws_host_address;
    mqttInitParams.port = aws_host_port;

#if defined(CONFIG_EXAMPLE_EMBEDDED_CERTS)
    mqttInitParams.pRootCALocation = (const char *)aws_root_ca_pem_start;
    mqttInitParams.pDeviceCertLocation = (const char *)certificate_pem_crt_start;
    mqttInitParams.pDevicePrivateKeyLocation = (const char *)private_pem_key_start;

#elif defined(CONFIG_EXAMPLE_FILESYSTEM_CERTS)
    mqttInitParams.pRootCALocation = ROOT_CA_PATH;
    mqttInitParams.pDeviceCertLocation = DEVICE_CERTIFICATE_PATH;
    mqttInitParams.pDevicePrivateKeyLocation = DEVICE_PRIVATE_KEY_PATH;
#endif

    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnect_callback_handler;
    mqttInitParams.disconnectHandlerData = NULL;

#ifdef CONFIG_EXAMPLE_SDCARD_CERTS
    ESP_LOGI(TAG, "Mounting SD card...");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 3,
    };
    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card VFAT filesystem. Error: %s", esp_err_to_name(ret));
        abort();
    }
#endif

    rc = aws_iot_mqtt_init(&client, &mqttInitParams);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "aws_iot_mqtt_init returned error : %d ", rc);
        abort();
    }

    /* Wait for WiFI to show as connected */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);

    connectParams.keepAliveIntervalInSec = 10;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    /* Client ID is set in the menuconfig of the example */
    connectParams.pClientID = CONFIG_AWS_EXAMPLE_CLIENT_ID;
    connectParams.clientIDLen = (uint16_t) strlen(CONFIG_AWS_EXAMPLE_CLIENT_ID);
    connectParams.isWillMsgPresent = false;

    ESP_LOGI(TAG, "Connecting to AWS...");
    do {
        rc = aws_iot_mqtt_connect(&client, &connectParams);
        if(SUCCESS != rc) {
            ESP_LOGE(TAG, "Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    } while(SUCCESS != rc);

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "Unable to set Auto Reconnect to true - %d", rc);
        abort();
    }

    const char *TOPIC_SUB = "esp32/sub/+";
    const int TOPIC_SUB_LEN = strlen(TOPIC_SUB);

    const char *TOPIC_PUB_URL = "esp32/pub/url";
    const int TOPIC_PUB_URL_LEN = strlen(TOPIC_PUB_URL);

    const char *TOPIC_PUB_DATA = "esp32/pub/data";
    const int TOPIC_PUB_DATA_LEN = strlen(TOPIC_PUB_DATA);

    ESP_LOGI(TAG, "Subscribing...");
    rc = aws_iot_mqtt_subscribe(&client, TOPIC_SUB, TOPIC_SUB_LEN, QOS0, subscribe_callback_handler, NULL);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "Error subscribing : %d ", rc);
        abort();
    }


    while((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc)) {

        //Max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(&client, 100);
        if(NETWORK_ATTEMPTING_RECONNECT == rc) {
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }

        if (buttonTrigger) {
            ESP_LOGI(TAG, "Button pressed");
            gpio_set_level(RED_LED_GPIO, 0);
            gpio_set_level(GREEN_LED_GPIO, 0);
            sprintf(cPayload, "{\"id\": \"%s\"}",  CONFIG_AWS_EXAMPLE_CLIENT_ID);
            paramsQOS0.qos = QOS0;
            paramsQOS0.payload = (void *) cPayload;
            paramsQOS0.isRetained = 0;
            paramsQOS0.payloadLen = strlen(cPayload);

            // ESP_LOGI(TAG, "Stack remaining for task '%s' is %d bytes", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));
            rc = aws_iot_mqtt_publish(&client, TOPIC_PUB_URL, TOPIC_PUB_URL_LEN, &paramsQOS0);

            if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
                ESP_LOGW(TAG, "QOS0 publish ack not received.");
                rc = SUCCESS;
            }
            buttonTrigger = false;
        }

        if (urlTrigger) {
            sprintf(cPayload, "{\"id\": \"%s\", \"payload\": \"%s\"}", CONFIG_AWS_EXAMPLE_CLIENT_ID, t_filename);
            paramsQOS0.qos = QOS0;
            paramsQOS0.payload = (void *) cPayload;
            paramsQOS0.isRetained = 0;
            paramsQOS0.payloadLen = strlen(cPayload);

            // ESP_LOGI(TAG, "Stack remaining for task '%s' is %d bytes", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));
            rc = aws_iot_mqtt_publish(&client, TOPIC_PUB_DATA, TOPIC_PUB_DATA_LEN, &paramsQOS0);

            if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
                ESP_LOGW(TAG, "QOS0 publish ack not received.");
                rc = SUCCESS;
            }
            urlTrigger = false;
        }

        vTaskDelay(5000 / portTICK_RATE_MS);
    }

    ESP_LOGE(TAG, "An error occurred in the main loop.");
    abort();
}

/**
 * Cheater's way of checking if assigned button pin is high or low.
 */ 
void button_pressed_task() {
    while (1) {
        if (gpio_get_level(BUTTON) && !buttonPressedDown) {
            buttonPressedDown = true;
            buttonTrigger = false;
            vTaskDelay(10 / portTICK_PERIOD_MS);
        } 
        
        if (!gpio_get_level(BUTTON) && buttonPressedDown) {
            buttonPressedDown = false;
            buttonTrigger = true;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/**
 * Helper function to split the string into tokens. This is for parsing responses back from AWS
 */
char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

/**
 * Just checks the payload response and maps LED color to object we are looking for.
 * Red for animals
 * Green for humans
 * Yellow for everything else
 */ 
int get_led_to_turn(char *payload) {
    char** tokens;
    tokens = str_split(payload, ';');
    int retval = YELLOW_LED_GPIO;

    if (tokens)
    {
        int i;
        for (i = 0; *(tokens + i); i++)
        {
            if (strcmp(*(tokens + i), "Animal") == 0) {
                retval = RED_LED_GPIO;
            }
            if (strcmp(*(tokens + i), "Human") == 0) {
                retval = GREEN_LED_GPIO;
            }
            free(*(tokens + i));
        }
        free(tokens);
    }

    return retval;
}

/**
 * Description: Main method
 * */
void app_main()
{
    static httpd_handle_t server = NULL;
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    init_app();
    init_wifi();

    esp32_camera_init();

    xTaskCreate(&aws_mqtt_task, "aws_mqtt_task", 18432, NULL, 5, NULL);
    xTaskCreate(&button_pressed_task, "button_pressed_task", 4096, NULL, 4, NULL);
    
    // Enable webserver to check the stream. Just open the http://your-esp32-cam-ip-address/ in the browser.
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    server = start_webserver();
}
