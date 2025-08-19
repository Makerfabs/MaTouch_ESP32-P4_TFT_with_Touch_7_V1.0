#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_check.h"
#include "lvgl.h"
#include "esp_brookesia.hpp"

#include "usb/usb_host.h"
#include "esp_private/usb_phy.h"
#include "errno.h"

#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/hid_usage_mouse.h"


typedef enum {
    APP_EVENT = 0,
    APP_EVENT_HID_HOST
} app_event_group_t;

typedef struct {
    app_event_group_t event_group;
    /* HID Host - Device related info */
    struct {
        hid_host_device_handle_t handle;
        hid_host_driver_event_t event;
        void *arg;
    } hid_host_device;
} app_event_queue_t;

typedef enum {
    KEY_STATE_PRESSED = 0x00,
    KEY_STATE_RELEASED = 0x01
} key_state_t;


/**
 * @brief Key event
 */
typedef struct {
    key_state_t state;
    uint8_t modifier;
    uint8_t key_code;
} key_event_t;


class AppHidHost: public ESP_Brookesia_PhoneApp {
    public:
        AppHidHost();
        ~AppHidHost();

        bool init(void);
        bool close(void);
        bool back(void);

        bool run(void) override;

        volatile bool _is_closing;

    private:
        QueueHandle_t _app_event_queue;
        // TaskHandle_t _usb_task_handle;
        // TaskHandle_t _event_task_handle;

        // LVGL界面对象
        lv_obj_t* _main_screen;
        lv_obj_t* _title_label;
        lv_obj_t* _keyboard_label;
        lv_obj_t* _mouse_label;
        lv_obj_t* _keyboard_area;
        lv_obj_t* _mouse_area;
        lv_obj_t* _device_info_label;

        void hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                           const hid_host_driver_event_t event,
                           void *arg);

        static void hid_host_device_callback(hid_host_device_handle_t hid_device_handle,
                              const hid_host_driver_event_t event,
                              void *arg);
        static void usb_lib_task(void *arg);
        static void event_task(void *arg);
        static void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                                 const hid_host_interface_event_t event,
                                 void *arg);
        static void hid_host_keyboard_report_callback(const uint8_t *const data, const int length);
        static void key_event_callback(key_event_t *key_event);
        static void hid_print_new_device_report_header(hid_protocol_t proto);
        static void hid_host_mouse_report_callback(const uint8_t *const data, const int length);
        static void hid_host_generic_report_callback(const uint8_t *const data, const int length);

        // 新增方法用于更新UI
        void update_keyboard_ui(const char* key_info);
        void update_mouse_ui(int x, int y, bool button1, bool button2);
        void update_device_info(const char* device_info);
};