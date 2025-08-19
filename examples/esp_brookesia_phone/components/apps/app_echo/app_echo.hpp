#pragma once

#include "esp_log.h"
#include "esp_check.h"
#include "lvgl.h"
#include "file_iterator.h"
#include "esp_brookesia.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class AppEcho: public ESP_Brookesia_PhoneApp {
    public:
        AppEcho();
        ~AppEcho();

        bool init(void);
        bool close(void);
        bool back(void);

        bool run(void) override;
        
    private:
        
        lv_obj_t *_screen;
        lv_obj_t *_button;
        lv_obj_t *_label_button;

        FILE *_file;
        bool _is_running;
        TaskHandle_t _task_handle;

        static void _button_cb(lv_event_t *e);
        static void app_task(void *arg);

};