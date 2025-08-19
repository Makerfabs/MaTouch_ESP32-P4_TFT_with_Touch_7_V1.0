#include "app_echo.hpp"

#include "bsp_board_extra.h"

// 声明外部图像资源
LV_IMG_DECLARE(img_app_music_player);

static const char *TAG = "app_echo";

AppEcho::AppEcho():
    ESP_Brookesia_PhoneApp("AppEcho", &img_app_music_player, true),
    _screen(nullptr),
    _button(nullptr),
    _label_button(nullptr),
    _file(nullptr),
    _is_running(false),
    _task_handle(NULL)
{

}
AppEcho::~AppEcho()
{

}

bool AppEcho::run(void)
{
    _screen = lv_scr_act();
    lv_obj_clean(_screen);

    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x2c3e50), LV_PART_MAIN);

    _button = lv_btn_create(_screen);
    lv_obj_set_size(_button, 150, 150);
    lv_obj_set_style_radius(_button, 75, LV_PART_MAIN);
    lv_obj_align(_button, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(_button, lv_color_hex(0x3498db), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(_button, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(_button, lv_color_hex(0x2980b9), LV_PART_MAIN);
    lv_obj_add_event_cb(_button, _button_cb, LV_EVENT_CLICKED, this);

    _label_button = lv_label_create(_button);
    lv_label_set_text(_label_button, "Start");
    lv_obj_set_style_text_color(_label_button, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_center(_label_button);

    return true;
}

bool AppEcho::init(void)
{
    ESP_ERROR_CHECK(bsp_extra_codec_init());
    return true; 
}
bool AppEcho::back(void)
{
    notifyCoreClosed();
    return true;
}
bool AppEcho::close(void)
{
    bsp_extra_codec_mute_set(true);
    ESP_ERROR_CHECK(bsp_extra_codec_dev_stop());
    return true;
}

void AppEcho::_button_cb(lv_event_t *e)
{
    AppEcho *instance = static_cast<AppEcho *>(lv_event_get_user_data(e));
    if(instance) {
        // instance->app_task(instance)
        if(instance->_is_running) {
            lv_label_set_text(instance->_label_button, "Start");
            instance->_is_running = false;
            bsp_extra_codec_mute_set(true);
            vTaskDelete(instance->_task_handle);
            instance->_task_handle = NULL;
        } else {
            lv_label_set_text(instance->_label_button, "Stop");
            bsp_extra_codec_mute_set(false);
            bsp_extra_codec_set_fs(CODEC_DEFAULT_SAMPLE_RATE, CODEC_DEFAULT_BIT_WIDTH, I2S_SLOT_MODE_MONO);
            xTaskCreate(instance->app_task, "app_task", 4096, instance, 5, &instance->_task_handle);
            instance->_is_running = true;
        }
    }
}

void AppEcho::app_task(void *data)
{
    AppEcho *instance = static_cast<AppEcho *>(data);
    bsp_extra_codec_volume_set(60, NULL);

    uint8_t *buff = (uint8_t *)malloc(256);
    size_t bytes_read = 0;
    while (1) {
        esp_err_t err = bsp_extra_i2s_read(buff, 256, &bytes_read, 1000);
        if(err != ESP_OK) {
            ESP_LOGI("TAG", "I2S read error");
            vTaskDelete(instance->_task_handle);
        }
        bsp_extra_i2s_write(buff, 256, &bytes_read, 1000);

        vTaskDelay(pdTICKS_TO_MS(8));
    }
    
}
