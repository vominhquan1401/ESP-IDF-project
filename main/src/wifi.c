#include "config.h"
#include "wifi.h"


/* STATICS VARIABLEs */
static const char *TAG = "WIFI_TASK";
static const char *TAG1 = "WIFI_HANDLER";

/* wifi task */
WifiSetUpState_t state = WIFI_STATE_LOAD_CONFIG;
saved_ap_t ap_list[MAX_AP_COUNT];
int ap_number = 0;
TaskHandle_t wifiHandlerHandle;
static bool mock_button_pressed = false;
bool wifi_handler_created = false;

/* wifi handler */
WifiHandlerState_t handler_state = WIFI_HANDLER_STATE_WATING;
QueueHandle_t wifiCmdQueue;
bool done_check = true;
bool get_new_list = false;

/* LED task */
LED_status_t led_status = LED_STATUS_DISCONNECTED;



/* INTERNAL FUNCTIONs */
// Kiểm tra AP lưu có trong danh sách scan
static bool is_ap_in_scan(const char *ssid, int scan_count, wifi_ap_record_t *scan_list)
{
    for (int i = 0; i < scan_count; i++)
    {
        if (strncmp(ssid, (char *)scan_list[i].ssid, sizeof(scan_list[i].ssid)) == 0)
        {
            return true;
        }
    }
    return false;
}

// đồng bộ thời gian
static void initialize_sntp(void)
{

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}
static void obtain_time(void)
{
    initialize_sntp();

    // Chờ đồng bộ
    int retry = 0;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < 10)
    {
        vTaskDelay(pdMS_TO_TICKS(2000));
        retry++;
    }
    ESP_LOGI(TAG, "SYN successful");
}



/* INTERNAL STATE */
static inline bool wait_button_to_run_web_server(WifiSetUpState_t *state)
{
    if(mock_button_pressed)
    {
        //suspend task if done check wifi (wifi handler)
        if(wifi_handler_created && done_check)
        {
            //suspend task:
            ESP_LOGW(TAG, "Suspend task WifiHandler\n");
            vTaskSuspend(wifiHandlerHandle);
            mock_button_pressed = false;
            *state = WIFI_STATE_WEB_CONFIG;
            return true;
        }
        // iff wifi handler isn't existed => just switch state
        else if(!wifi_handler_created)
        {
            mock_button_pressed = false;
            *state = WIFI_STATE_WEB_CONFIG;
            return true;
        }

    }
    return false;
}

void get_info_from_nvs(WifiSetUpState_t *state)
{
    ap_number = wifi_nvs_get_all_saved_ap(ap_list, MAX_AP_COUNT);
    if(ap_number == 0)
    {
        *state = WIFI_STATE_WEB_CONFIG;
        ESP_LOGW(TAG, "Change to WEB SERVER");
    }
    else
    {
        *state = WIFI_STATE_CONNECT_WIFI;
        ESP_LOGW(TAG, "Change to CONNECT WIFI");
    }
        
}

void connect_to_wifi(WifiSetUpState_t *state)
{
    //scan to get list
    uint16_t ap_num = 0;
    wifi_ap_record_t *scan_list = NULL;
    scan_list = wifi_scan(&ap_num);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGW(TAG, "check Wifi Status");
    /* Check what wifi is active  */
    for(int i = 0; i < ap_number; i++)
    {
        if(is_ap_in_scan(ap_list->ssid, ap_num, scan_list)){
            // if active => update info and connect
            wifi_manager_update_sta_creds(ap_list[i].ssid, ap_list[i].pass);
            wifi_connect_sta();
            //check status
            while(1)
            {
                vTaskDelay(pdMS_TO_TICKS(500));
                ESP_LOGW(TAG, "check Wifi Status");

                if(wifi_status == WIFI_STATUS_CONNECTED){
                    ESP_LOGW(TAG, "change to TIME SYN");
                    *state = WIFI_STATE_TIME_SYN;
                    //free list
                    free(scan_list);
                    return;
                }
                //check button if pressed to change to web
                if(wait_button_to_run_web_server(state))
                {
                    //free list
                    free(scan_list);
                    return;
                }
                    
            }  
        }
        // if can't connect to wifi => change to web
    }
    
    ESP_LOGW(TAG, "change to WEB SERVER");
    *state = WIFI_STATE_WEB_CONFIG;
    //free list
    free(scan_list);
       
}

void web_server_run(WifiSetUpState_t *state)
{
    led_status = LED_STATUS_WEBSERVER;
    if (wifi_config_portal_start() == ESP_OK)
    {
        ESP_LOGI(TAG, "Portal started, waiting for user to connect...");
        // Chờ người dùng nhập SSID/PASS và ESP chuyển qua STA
        
        while (wifi_status != WIFI_STATUS_CONNECTED)
        {
            if(wifi_status == WIFI_STATUS_CONNECTING)
                led_status = LED_STATUS_CONNECTING;
            else if(wifi_status == WIFI_STATUS_IDLE)
                led_status = LED_STATUS_WEBSERVER;
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        ESP_LOGI(TAG, "✅ Wi-Fi connected via portal!");
        ESP_LOGW(TAG, "change to TIME SYN");
        *state = WIFI_STATE_TIME_SYN;
    }
    else
    {
        ESP_LOGE(TAG, "❌ Failed to start config portal");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void time_syn(WifiSetUpState_t *state)
{
    // SYN:
    static bool done = false;
    if(!done)
    {
        obtain_time();
        done = true;
    }
    
    *state = WIFI_STATE_CONTROL_WIFI_HANDLER;
    
}



/* TASKs */
void vtaskWifiSetup(void *pvParameters) 
{
    wifiCmdQueue = xQueueCreate(5, sizeof(wifi_cmd_t));
    while(1)
    {
        switch(state)
        {
            /**
             * Get WIFI info from NVS:
             *      - No info:      Switch to WIFI_STATE_WEB_CONFIG
             *      - info exists:  Switch to WIFI_STATE_CONNECT_WIFI
             * Note: 
             *      - Change led_status = LED_STATUS_DISCONNECTED
             */
            case WIFI_STATE_LOAD_CONFIG:
                ESP_LOGI(TAG, "Get WIFI info from nvs\n");
                led_status = LED_STATUS_DISCONNECTED;
                get_info_from_nvs(&state);
                break;
            /**
             * Try to connect to WIFI:
             *      - Failed:      Switch to WIFI_STATE_WEB_CONFIG
             *      - Successful:  Switch to WIFI_STATE_TIME_SYN
             * Note: 
             *      - Change led_status = LED_STATUS_CONNECTING
             */
            case WIFI_STATE_CONNECT_WIFI:
                ESP_LOGI(TAG, "Preparing to connect to WIFI\n");
                led_status = LED_STATUS_CONNECTING;
                connect_to_wifi(&state);
                break;
            /**
             * Try to connect to Server:
             *      - Failed:      Switch to WIFI_STATE_CONNECT_WIFI
             *      - Successful:  Switch to WIFI_STATE_CONTROL_WIFI_HANDLER
             * Note: 
             *      - Change led_status = LED_STATUS_CONNECTED
             */
            case WIFI_STATE_TIME_SYN:
            /* MOCK: auto successful*/
                ESP_LOGI(TAG, "Time SYN");
                led_status = LED_STATUS_CONNECTED;
                time_syn(&state);
                break;
            /**
             * wait user enter SSID & Pass in WEB mode and try to connect:
             *      - Failed:      remain WIFI_STATE_WEB_CONFIG state
             *      - Successful:  Switch to WIFI_STATE_CONTROL_WIFI_HANDLER
             * Note: 
             *      - Change led_status = LED_STATUS_WEBSERVER
             */
            case WIFI_STATE_WEB_CONFIG:
                led_status = LED_STATUS_CONNECTING;
                web_server_run(&state);
                break;
            /**
             * create a wifi handler task and control it:
             *      - Failed:      Switch to WIFI_STATE_WAITING_TO_BUTTON
             *      - Successful:  Switch to WIFI_STATE_WAITING_TO_BUTTON
             */
            case WIFI_STATE_CONTROL_WIFI_HANDLER:
                //create task
                if(!wifi_handler_created)
                {
                    ESP_LOGW(TAG, "Create a task to handle WIFI\n");
                    xTaskCreate(taskWifiHandler, "WifiHandler", 4096, NULL, 2, &wifiHandlerHandle);
                    wifi_handler_created = true;
                }else
                {
                    //resume task:
                    ESP_LOGW(TAG, "Resume task WifiHandler\n");
                    get_new_list = false;
                    done_check = true;
                    handler_state = WIFI_HANDLER_STATE_WATING;

                    vTaskResume(wifiHandlerHandle);
                }
                state = WIFI_STATE_WAITING_TO_BUTTON;
                break;
            /**
             * waiting to the button to switch to WEB CONFIG:
             *      - Release:      remain
             *      - Pressed:      Switch to WIFI_STATE_WEB_CONFIG
             */
            case WIFI_STATE_WAITING_TO_BUTTON:
                if(wait_button_to_run_web_server(&state))
                    break;
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
    }
}


void taskWifiHandler(void *pvParameters) 
{
    wifi_cmd_t cmd;
    WifiSetUpState_t connect_state = WIFI_STATE_CONNECT_WIFI;
    while(1)
    {   
        switch(handler_state)
        {
            case WIFI_HANDLER_STATE_WATING:  // connect wifi mới
            {
                if (xQueueReceive(wifiCmdQueue, &cmd, portMAX_DELAY)) 
                {
                    printf("Changing WiFi to SSID=%s\n", cmd.ssid);
                    handler_state = WIFI_HANDLER_STATE_NEW_CONFIG;
                }
            }
            break;
            case WIFI_HANDLER_STATE_NEW_CONFIG:  // connect wifi mới
            {
                // đổi wifi và connect lại
                
                wifi_manager_update_sta_creds(cmd.ssid, cmd.password);
                wifi_disconnect_to_ap();
                wifi_connect_sta();
                //check status
                while(1)
                {
                    vTaskDelay(pdMS_TO_TICKS(500));
                    if(wifi_status == WIFI_STATUS_DISCONNECTED)
                    {
                        handler_state = WIFI_HANDLER_STATE_RECONNECT_OLD;
                        ESP_LOGW(TAG1, "Can't connect to new WIFI");
                        break;
                    }
                    else if(wifi_status == WIFI_STATUS_CONNECTED)
                    {
                        ESP_LOGW(TAG1, "Connect to new WIFI");
                        handler_state = WIFI_HANDLER_STATE_WATING;
                        break;
                    }
                }
            }
            break;
            case WIFI_HANDLER_STATE_RECONNECT_OLD:      // quay lại kiếm các wifi cũ đã có để connect theo chu kì nếu wifi không ổn
            {
                
                if(connect_state != WIFI_STATE_TIME_SYN)    // nếu wifi đã được kết nối, thì không cần kết nối nữa
                {    
                    if(!get_new_list){
                        done_check = false;
                        ap_number = wifi_nvs_get_all_saved_ap(ap_list, MAX_AP_COUNT); // get info
                        get_new_list = true;
                    }
                    connect_to_wifi(&connect_state);
                    ESP_LOGE(TAG1, "Don't connect to old WIFI");
                    done_check = true;  // using to notify that wifi checked
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    done_check = false;
                }
                else
                {
                    get_new_list = false;
                    done_check = true;
                    connect_state = WIFI_STATE_CONNECT_WIFI;
                    handler_state = WIFI_HANDLER_STATE_WATING;
                }
                
            }
            break;
        }
        

    }
}


void taskLedControl(void *pvParameters)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    ESP_LOGI("LED", "Create LED Task");
    while (1)
    {
        switch (led_status)
        {
            case LED_STATUS_DISCONNECTED:
                gpio_set_level(LED_GPIO, 1);
                vTaskDelay(pdMS_TO_TICKS(125));  // ON 125ms
                gpio_set_level(LED_GPIO, 0);
                vTaskDelay(pdMS_TO_TICKS(125));  // OFF 125ms
                break;

            case LED_STATUS_CONNECTING:
                gpio_set_level(LED_GPIO, 1);
                vTaskDelay(pdMS_TO_TICKS(500));  // ON 500ms
                gpio_set_level(LED_GPIO, 0);
                vTaskDelay(pdMS_TO_TICKS(500));  // OFF 500ms
                break;

            case LED_STATUS_CONNECTED:
                gpio_set_level(LED_GPIO, 1);     // LED OFF
                vTaskDelay(pdMS_TO_TICKS(100));  // check every 100ms
                break;

            case LED_STATUS_WEBSERVER:
                gpio_set_level(LED_GPIO, 0);     // LED ON
                vTaskDelay(pdMS_TO_TICKS(100));  // check every 100ms
                break;
        }
    }
}



