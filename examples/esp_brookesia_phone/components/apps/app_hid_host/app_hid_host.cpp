#include "app_hid_host.hpp"

#include "bsp_board_extra.h"

// 声明外部图像资源
LV_IMG_DECLARE(img_app_music_player);

static const char *TAG = "app_hid_host";
// 用于静态回调访问类实例的静态指针
static AppHidHost *s_hid_host_instance = nullptr;

/**
 * @brief HID Protocol string names
 */
static const char *hid_proto_name_str[] = {
    "NONE",
    "KEYBOARD",
    "MOUSE"
};

/* Main char symbol for ENTER key */
#define KEYBOARD_ENTER_MAIN_CHAR    '\r'
/* When set to 1 pressing ENTER will be extending with LineFeed during serial debug output */
#define KEYBOARD_ENTER_LF_EXTEND    1

/**
 * @brief Scancode to ascii table
 */
const uint8_t keycode2ascii [57][2] = {
    {0, 0}, /* HID_KEY_NO_PRESS        */
    {0, 0}, /* HID_KEY_ROLLOVER        */
    {0, 0}, /* HID_KEY_POST_FAIL       */
    {0, 0}, /* HID_KEY_ERROR_UNDEFINED */
    {'a', 'A'}, /* HID_KEY_A               */
    {'b', 'B'}, /* HID_KEY_B               */
    {'c', 'C'}, /* HID_KEY_C               */
    {'d', 'D'}, /* HID_KEY_D               */
    {'e', 'E'}, /* HID_KEY_E               */
    {'f', 'F'}, /* HID_KEY_F               */
    {'g', 'G'}, /* HID_KEY_G               */
    {'h', 'H'}, /* HID_KEY_H               */
    {'i', 'I'}, /* HID_KEY_I               */
    {'j', 'J'}, /* HID_KEY_J               */
    {'k', 'K'}, /* HID_KEY_K               */
    {'l', 'L'}, /* HID_KEY_L               */
    {'m', 'M'}, /* HID_KEY_M               */
    {'n', 'N'}, /* HID_KEY_N               */
    {'o', 'O'}, /* HID_KEY_O               */
    {'p', 'P'}, /* HID_KEY_P               */
    {'q', 'Q'}, /* HID_KEY_Q               */
    {'r', 'R'}, /* HID_KEY_R               */
    {'s', 'S'}, /* HID_KEY_S               */
    {'t', 'T'}, /* HID_KEY_T               */
    {'u', 'U'}, /* HID_KEY_U               */
    {'v', 'V'}, /* HID_KEY_V               */
    {'w', 'W'}, /* HID_KEY_W               */
    {'x', 'X'}, /* HID_KEY_X               */
    {'y', 'Y'}, /* HID_KEY_Y               */
    {'z', 'Z'}, /* HID_KEY_Z               */
    {'1', '!'}, /* HID_KEY_1               */
    {'2', '@'}, /* HID_KEY_2               */
    {'3', '#'}, /* HID_KEY_3               */
    {'4', '$'}, /* HID_KEY_4               */
    {'5', '%'}, /* HID_KEY_5               */
    {'6', '^'}, /* HID_KEY_6               */
    {'7', '&'}, /* HID_KEY_7               */
    {'8', '*'}, /* HID_KEY_8               */
    {'9', '('}, /* HID_KEY_9               */
    {'0', ')'}, /* HID_KEY_0               */
    {KEYBOARD_ENTER_MAIN_CHAR, KEYBOARD_ENTER_MAIN_CHAR}, /* HID_KEY_ENTER           */
    {0, 0}, /* HID_KEY_ESC             */
    {'\b', 0}, /* HID_KEY_DEL             */
    {0, 0}, /* HID_KEY_TAB             */
    {' ', ' '}, /* HID_KEY_SPACE           */
    {'-', '_'}, /* HID_KEY_MINUS           */
    {'=', '+'}, /* HID_KEY_EQUAL           */
    {'[', '{'}, /* HID_KEY_OPEN_BRACKET    */
    {']', '}'}, /* HID_KEY_CLOSE_BRACKET   */
    {'\\', '|'}, /* HID_KEY_BACK_SLASH      */
    {'\\', '|'}, /* HID_KEY_SHARP           */  // HOTFIX: for NonUS Keyboards repeat HID_KEY_BACK_SLASH
    {';', ':'}, /* HID_KEY_COLON           */
    {'\'', '"'}, /* HID_KEY_QUOTE           */
    {'`', '~'}, /* HID_KEY_TILDE           */
    {',', '<'}, /* HID_KEY_LESS            */
    {'.', '>'}, /* HID_KEY_GREATER         */
    {'/', '?'} /* HID_KEY_SLASH           */
};

AppHidHost::AppHidHost():
    ESP_Brookesia_PhoneApp("HID Host", &img_app_music_player, true),
    _app_event_queue(nullptr)
{
    s_hid_host_instance = this;
    _is_closing = false;
}

AppHidHost::~AppHidHost()
{
    s_hid_host_instance = nullptr;
}

bool AppHidHost::run(void)
{
    ESP_LOGI(TAG, "HID Host App started");
    _is_closing = false;

       // 获取当前屏幕
    _main_screen = lv_scr_act();
    
    // 创建标题
    _title_label = lv_label_create(_main_screen);
    lv_label_set_text(_title_label, "HID Device Monitor");
    lv_obj_align(_title_label, LV_ALIGN_TOP_MID, 0, 10);
    
    // 创建设备信息标签
    _device_info_label = lv_label_create(_main_screen);
    lv_label_set_text(_device_info_label, "No device connected");
    lv_obj_align(_device_info_label, LV_ALIGN_TOP_MID, 0, 40);
    
    // 创建键盘显示区域
    _keyboard_area = lv_obj_create(_main_screen);
    lv_obj_set_size(_keyboard_area, 200, 100);
    lv_obj_align(_keyboard_area, LV_ALIGN_LEFT_MID, 10, 0);
    
    _keyboard_label = lv_label_create(_keyboard_area);
    lv_label_set_text(_keyboard_label, "Keyboard:\nNo input");
    lv_obj_align(_keyboard_label, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_label_set_recolor(_keyboard_label, true);
    
    // 创建鼠标显示区域
    _mouse_area = lv_obj_create(_main_screen);
    lv_obj_set_size(_mouse_area, 200, 150);
    lv_obj_align(_mouse_area, LV_ALIGN_RIGHT_MID, -10, 0);
    
    _mouse_label = lv_label_create(_mouse_area);
    lv_label_set_text(_mouse_label, "Mouse:\nX: 0\nY: 0\nButton 1: []\nButton 2: []");
    lv_obj_align(_mouse_label, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_label_set_recolor(_mouse_label, true);

    BaseType_t task_created;
    task_created = xTaskCreatePinnedToCore(usb_lib_task,
                                           "usb_events",
                                           4096,
                                           xTaskGetCurrentTaskHandle(),
                                           2, NULL, 0);
    assert(task_created == pdTRUE);
    // Wait for notification from usb_lib_task to proceed
    ulTaskNotifyTake(false, 1000);

    /*
    * HID host driver configuration
    * - create background task for handling low level event inside the HID driver
    * - provide the device callback to get new HID Device connection event
    */
    xTaskCreate(event_task, "event_task", 4096, this, 5, NULL);
    return true;
}

bool AppHidHost::close(void)
{
    _is_closing = true;
    app_event_queue_t exit_event = { .event_group = APP_EVENT };
    xQueueSend(_app_event_queue, &exit_event, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "HID Host App closed");
    return true;
}

bool AppHidHost::back(void)
{
    // 退出应用
    ESP_LOGI(TAG, "HID Host App closing via back button");
    notifyCoreClosed();
    return true;
}

bool AppHidHost::init(void)
{
    ESP_LOGI(TAG, "HID Host App initializing");
    return true;
}
/**
 * @brief HID Keyboard modifier verification for capitalization application (right or left shift)
 *
 * @param[in] modifier
 * @return true  Modifier was pressed (left or right shift)
 * @return false Modifier was not pressed (left or right shift)
 *
 */
static inline bool hid_keyboard_is_modifier_shift(uint8_t modifier)
{
    if (((modifier & HID_LEFT_SHIFT) == HID_LEFT_SHIFT) ||
            ((modifier & HID_RIGHT_SHIFT) == HID_RIGHT_SHIFT)) {
        return true;
    }
    return false;
}

/**
 * @brief HID Keyboard get char symbol from key code
 *
 * @param[in] modifier  Keyboard modifier data
 * @param[in] key_code  Keyboard key code
 * @param[in] key_char  Pointer to key char data
 *
 * @return true  Key scancode converted successfully
 * @return false Key scancode unknown
 */
static inline bool hid_keyboard_get_char(uint8_t modifier,
                                         uint8_t key_code,
                                         unsigned char *key_char)
{
    uint8_t mod = (hid_keyboard_is_modifier_shift(modifier)) ? 1 : 0;

    if ((key_code >= HID_KEY_A) && (key_code <= HID_KEY_SLASH)) {
        *key_char = keycode2ascii[key_code][mod];
    } else {
        // All other key pressed
        return false;
    }

    return true;
}

/**
 * @brief HID Keyboard print char symbol
 *
 * @param[in] key_char  Keyboard char to stdout
 */
static inline void hid_keyboard_print_char(unsigned int key_char)
{
    if (!!key_char) {
        putchar(key_char);
#if (KEYBOARD_ENTER_LF_EXTEND)
        if (KEYBOARD_ENTER_MAIN_CHAR == key_char) {
            putchar('\n');
        }
#endif // KEYBOARD_ENTER_LF_EXTEND
        fflush(stdout);
    }
}
/**
 * @brief Key buffer scan code search.
 *
 * @param[in] src       Pointer to source buffer where to search
 * @param[in] key       Key scancode to search
 * @param[in] length    Size of the source buffer
 */
static inline bool key_found(const uint8_t *const src,
                             uint8_t key,
                             unsigned int length)
{
    for (unsigned int i = 0; i < length; i++) {
        if (src[i] == key) {
            return true;
        }
    }
    return false;
}
/**
 * @brief USB HID Host interface callback
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host interface event
 * @param[in] arg                Pointer to arguments, does not used
 */
void AppHidHost::hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                                 const hid_host_interface_event_t event,
                                 void *arg)
{
    uint8_t data[64] = { 0 };
    size_t data_length = 0;
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
        ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(hid_device_handle,
                                                                  data,
                                                                  64,
                                                                  &data_length));

        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                hid_host_keyboard_report_callback(data, data_length);
            } else if (HID_PROTOCOL_MOUSE == dev_params.proto) {
                hid_host_mouse_report_callback(data, data_length);
            }
        } else {
            hid_host_generic_report_callback(data, data_length);
        }

        break;
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HID Device, protocol '%s' DISCONNECTED",
                 hid_proto_name_str[dev_params.proto]);
        ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGI(TAG, "HID Device, protocol '%s' TRANSFER_ERROR",
                 hid_proto_name_str[dev_params.proto]);
        break;
    default:
        ESP_LOGE(TAG, "HID Device, protocol '%s' Unhandled event",
                 hid_proto_name_str[dev_params.proto]);
        break;
    }
}
/**
 * @brief USB HID Host Device event
 *
 * @param[in] hid_device_handle  HID Device handle
 * @param[in] event              HID Host Device event
 * @param[in] arg                Pointer to arguments, does not used
 */
void AppHidHost::hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                           const hid_host_driver_event_t event,
                           void *arg)
{
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event) {
    case HID_HOST_DRIVER_EVENT_CONNECTED: {
        ESP_LOGI(TAG, "HID Device, protocol '%s' CONNECTED",
                 hid_proto_name_str[dev_params.proto]);

        // 更新设备信息显示
        if (s_hid_host_instance) {
            char device_info[100];
            snprintf(device_info, sizeof(device_info), "Device Connected:\nProtocol: %s", 
                     hid_proto_name_str[dev_params.proto]);
            s_hid_host_instance->update_device_info(device_info);
        }

        const hid_host_device_config_t dev_config = {
            .callback = hid_host_interface_callback,
            .callback_arg = NULL
        };

        ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
        if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
            ESP_ERROR_CHECK(hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT));
            if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
                ESP_ERROR_CHECK(hid_class_request_set_idle(hid_device_handle, 0, 0));
            }
        }
        ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));
        break;
    }
    default:
        break;
    }
}
/**
 * @brief HID Host Device callback
 *
 * Puts new HID Device event to the queue
 *
 * @param[in] hid_device_handle HID Device handle
 * @param[in] event             HID Device event
 * @param[in] arg               Not used
 */
void AppHidHost::hid_host_device_callback(hid_host_device_handle_t hid_device_handle,
                              const hid_host_driver_event_t event,
                              void *arg)
{
    const app_event_queue_t evt_queue = {
        .event_group = APP_EVENT_HID_HOST,
        // HID Host Device related info
        // .hid_host_device.handle = hid_device_handle,
        // .hid_host_device.event = event,
        // .hid_host_device.arg = arg
        .hid_host_device = {
            .handle = hid_device_handle,
            .event = event,
            .arg = arg
        }
    };

    if (s_hid_host_instance->_app_event_queue) {
        xQueueSend(s_hid_host_instance->_app_event_queue, &evt_queue, 0);
    }
}

void AppHidHost::usb_lib_task(void *arg)
{
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));
    xTaskNotifyGive((TaskHandle_t)arg);

    while (!s_hid_host_instance->_is_closing) {
        uint32_t event_flags;
        if (usb_host_lib_handle_events(100 / portTICK_PERIOD_MS, &event_flags) == ESP_OK) {
            // 处理事件
            if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
                ESP_ERROR_CHECK(usb_host_device_free_all());
                ESP_LOGI(TAG, "USB library clients have all been closed");
                break;
            }
        }
    }
    
    usb_host_device_free_all();
    ESP_LOGI(TAG, "USB shutdown");
    // Clean up USB Host
    vTaskDelay(10); // Short delay to allow clients clean-up
    ESP_ERROR_CHECK(usb_host_uninstall());
    vTaskDelete(NULL);
}

void AppHidHost::event_task(void *arg)
{
    ESP_LOGI(TAG, "Waiting for HID Device to be connected");
    AppHidHost *app = (AppHidHost *)arg;
    if(!app) vTaskDelete(NULL);
    const hid_host_driver_config_t hid_host_driver_config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = hid_host_device_callback,
        .callback_arg = NULL
    };

    ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

    // Create queue
    app->_app_event_queue = xQueueCreate(10, sizeof(app_event_queue_t));

    app_event_queue_t evt_queue;
    while (!app->_is_closing) {
        // Wait queue
        if (xQueueReceive(app->_app_event_queue, &evt_queue, portMAX_DELAY)) {
            if(app->_is_closing) break;
            if (APP_EVENT == evt_queue.event_group) {
                // User pressed button
                usb_host_lib_info_t lib_info;
                ESP_ERROR_CHECK(usb_host_lib_info(&lib_info));
                if (lib_info.num_devices == 0) {
                    // End while cycle
                    ESP_LOGI(TAG, "No devices connected");
                    break;
                } else {
                    ESP_LOGW(TAG, "To shutdown example, remove all USB devices and press button again.");
                    // Keep polling
                    break;
                }
            }

            if (APP_EVENT_HID_HOST ==  evt_queue.event_group) {
                app->hid_host_device_event(evt_queue.hid_host_device.handle,
                                      evt_queue.hid_host_device.event,
                                      evt_queue.hid_host_device.arg);
            }
        }
    }
    ESP_LOGI(TAG, "HID Driver uninstall");
    ESP_ERROR_CHECK(hid_host_uninstall());
    xQueueReset(app->_app_event_queue);
    vQueueDelete(app->_app_event_queue);
    vTaskDelete(NULL);
}
/**
 * @brief USB HID Host Keyboard Interface report callback handler
 *
 * @param[in] data    Pointer to input report data buffer
 * @param[in] length  Length of input report data buffer
 */
void AppHidHost::hid_host_keyboard_report_callback(const uint8_t *const data, const int length)
{
    hid_keyboard_input_report_boot_t *kb_report = (hid_keyboard_input_report_boot_t *)data;

    if (length < sizeof(hid_keyboard_input_report_boot_t)) {
        return;
    }

    static uint8_t prev_keys[HID_KEYBOARD_KEY_MAX] = { 0 };
    key_event_t key_event;

    for (int i = 0; i < HID_KEYBOARD_KEY_MAX; i++) {

        // key has been released verification
        if (prev_keys[i] > HID_KEY_ERROR_UNDEFINED &&
                !key_found(kb_report->key, prev_keys[i], HID_KEYBOARD_KEY_MAX)) {
            key_event.key_code = prev_keys[i];
            key_event.modifier = 0;
            key_event.state = KEY_STATE_RELEASED;
            key_event_callback(&key_event);
        }

        // key has been pressed verification
        if (kb_report->key[i] > HID_KEY_ERROR_UNDEFINED &&
                !key_found(prev_keys, kb_report->key[i], HID_KEYBOARD_KEY_MAX)) {
            key_event.key_code = kb_report->key[i];
            key_event.modifier = kb_report->modifier.val;
            key_event.state = KEY_STATE_PRESSED;
            key_event_callback(&key_event);
        }
    }

    memcpy(prev_keys, &kb_report->key, HID_KEYBOARD_KEY_MAX);
}

/**
 * @brief Key Event. Key event with the key code, state and modifier.
 *
 * @param[in] key_event Pointer to Key Event structure
 *
 */
void AppHidHost::key_event_callback(key_event_t *key_event)
{
    unsigned char key_char;

    hid_print_new_device_report_header(HID_PROTOCOL_KEYBOARD);

    if (KEY_STATE_PRESSED == key_event->state) {
        if (hid_keyboard_get_char(key_event->modifier,
                                  key_event->key_code, &key_char)) {

            // hid_keyboard_print_char(key_char);
            // 更新UI显示
            if (s_hid_host_instance) {
                char key_info[50];
                if (key_char == '\r') {
                    snprintf(key_info, sizeof(key_info), "ENTER pressed");
                } else if (key_char == '\b') {
                    snprintf(key_info, sizeof(key_info), "BACKSPACE pressed");
                } else {
                    snprintf(key_info, sizeof(key_info), "Key '%c' pressed", key_char);
                }
                s_hid_host_instance->update_keyboard_ui(key_info);
            }

        } else {
            // 处理特殊按键
            if (s_hid_host_instance) {
                char key_info[50];
                switch (key_event->key_code) {
                    case HID_KEY_ESC:
                        snprintf(key_info, sizeof(key_info), "ESC pressed");
                        break;
                    case HID_KEY_TAB:
                        snprintf(key_info, sizeof(key_info), "TAB pressed");
                        break;
                    case HID_KEY_SPACE:
                        snprintf(key_info, sizeof(key_info), "SPACE pressed");
                        break;
                    default:
                        snprintf(key_info, sizeof(key_info), "Key code %d pressed", key_event->key_code);
                        break;
                }
                s_hid_host_instance->update_keyboard_ui(key_info);
            }
        }
    }
}

void AppHidHost::hid_print_new_device_report_header(hid_protocol_t proto)
{
    static hid_protocol_t prev_proto_output = static_cast<hid_protocol_t>(-1);;

    if (prev_proto_output != proto) {
        prev_proto_output = proto;
        // printf("\r\n");
        if (proto == HID_PROTOCOL_MOUSE) {
            // printf("Mouse\r\n");
            ESP_LOGI(TAG, "Mouse");
        } else if (proto == HID_PROTOCOL_KEYBOARD) {
            // printf("Keyboard\r\n");
            ESP_LOGI(TAG, "Keyboard");
        } else {
            // printf("Generic\r\n");
            ESP_LOGI(TAG, "Generic");
        }
        // fflush(stdout);
    }
}

/**
 * @brief USB HID Host Mouse Interface report callback handler
 *
 * @param[in] data    Pointer to input report data buffer
 * @param[in] length  Length of input report data buffer
 */
void AppHidHost::hid_host_mouse_report_callback(const uint8_t *const data, const int length)
{
    hid_mouse_input_report_boot_t *mouse_report = (hid_mouse_input_report_boot_t *)data;

    if (length < sizeof(hid_mouse_input_report_boot_t)) {
        return;
    }

    static int x_pos = 0;
    static int y_pos = 0;

    // Calculate absolute position from displacement
    x_pos += mouse_report->x_displacement;
    y_pos += mouse_report->y_displacement;

    hid_print_new_device_report_header(HID_PROTOCOL_MOUSE);

    // printf("X: %06d\tY: %06d\t|%c|%c|\r",
    //        x_pos, y_pos,
    //        (mouse_report->buttons.button1 ? 'o' : ' '),
    //        (mouse_report->buttons.button2 ? 'o' : ' '));
    // fflush(stdout);

        // 更新UI显示
    if (s_hid_host_instance) {
        s_hid_host_instance->update_mouse_ui(x_pos, y_pos, 
                                           mouse_report->buttons.button1, 
                                           mouse_report->buttons.button2);
    }
}
/**
 * @brief USB HID Host Generic Interface report callback handler
 *
 * 'generic' means anything else than mouse or keyboard
 *
 * @param[in] data    Pointer to input report data buffer
 * @param[in] length  Length of input report data buffer
 */
void AppHidHost::hid_host_generic_report_callback(const uint8_t *const data, const int length)
{
    hid_print_new_device_report_header(HID_PROTOCOL_NONE);
    char hex_data[100] = {0};
    int offset = 0;
    for (int i = 0; i < length && offset < sizeof(hex_data) - 4; i++) {
        offset += snprintf(hex_data + offset, sizeof(hex_data) - offset, "%02X ", data[i]);
    }
    // for (int i = 0; i < length; i++) {
    //     printf("%02X", data[i]);
    // }
    // putchar('\r');

        // 更新UI显示
    if (s_hid_host_instance) {
        char generic_info[150];
        snprintf(generic_info, sizeof(generic_info), "Generic Device:\nData: %s", hex_data);
        s_hid_host_instance->update_keyboard_ui(generic_info); // 在键盘区域显示通用设备信息
    }
}

void AppHidHost::update_keyboard_ui(const char* key_info)
{
    if (_keyboard_label) {
        char text[200];
        snprintf(text, sizeof(text), "Keyboard:#00ff00 %s#", key_info);
        lv_label_set_text(_keyboard_label, text);
        lv_obj_align(_keyboard_label, LV_ALIGN_TOP_LEFT, 5, 5);
    }
}

void AppHidHost::update_mouse_ui(int x, int y, bool button1, bool button2)
{
    if (_mouse_label) {
        char text[200];
        snprintf(text, sizeof(text), 
                 "Mouse:\nX: #ff0000 %d#\nY: #ff0000 %d#\nButton 1: %s\nButton 2: %s", 
                 x, y, 
                 button1 ? "#00ff00 [X]#" : "[ ]",
                 button2 ? "#00ff00 [X]#" : "[ ]");
        lv_label_set_text(_mouse_label, text);
        lv_obj_align(_mouse_label, LV_ALIGN_TOP_LEFT, 5, 5);
    }
}

void AppHidHost::update_device_info(const char* device_info)
{
    if (_device_info_label) {
        lv_label_set_text(_device_info_label, device_info);
        lv_obj_align(_device_info_label, LV_ALIGN_TOP_MID, 0, 40);
    }
}