#pragma once

#include "esp_log.h"
#include "esp_check.h"
#include "lvgl.h"
#include "file_iterator.h"
#include "esp_brookesia.hpp"

class AppRecord: public ESP_Brookesia_PhoneApp {
    public:
        AppRecord();
        ~AppRecord();

        bool init(void);
        bool close(void);
        bool back(void);

        bool run(void) override;
        
    private:
        
        lv_obj_t *_screen;
        lv_obj_t *_button;
        lv_obj_t *_label_button;

        FILE *_file;

        static void _button_cb(lv_event_t *e);
        static void app_task(void *arg);

};