#include "app_record.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_board_extra.h"

// 声明外部图像资源
LV_IMG_DECLARE(img_app_music_player);

static const char *TAG = "app_record";

AppRecord::AppRecord():
    ESP_Brookesia_PhoneApp("AppRecord", &img_app_music_player, true),
    _screen(nullptr),
    _button(nullptr),
    _label_button(nullptr),
    _file(nullptr)
{

}
AppRecord::~AppRecord()
{

}

bool AppRecord::run(void)
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

bool AppRecord::init(void)
{
    ESP_ERROR_CHECK(bsp_extra_codec_init());
    return true; 
}
bool AppRecord::back(void)
{
    notifyCoreClosed();
    return true;
}
bool AppRecord::close(void)
{
    ESP_ERROR_CHECK(bsp_extra_codec_dev_stop());
    return true;
}

void AppRecord::_button_cb(lv_event_t *e)
{
    AppRecord *instance = static_cast<AppRecord *>(lv_event_get_user_data(e));
    if(instance && !lv_obj_has_state(instance->_button, LV_STATE_DISABLED)) {
        // instance->app_task(instance)
        bsp_extra_codec_mute_set(false);
        ESP_ERROR_CHECK(bsp_extra_codec_set_fs(CODEC_DEFAULT_SAMPLE_RATE, CODEC_DEFAULT_BIT_WIDTH, I2S_SLOT_MODE_MONO));
        xTaskCreate(instance->app_task, "app_task", 4096, instance, 5, NULL);
    }
}

void AppRecord::app_task(void *data)
{
    AppRecord *instance = static_cast<AppRecord *>(data);
    lv_obj_add_state(instance->_button, LV_STATE_DISABLED);
    instance->_file = fopen("/sdcard/music/record.wav", "wb");
    if(instance->_file == NULL) {
        vTaskDelete(NULL);
        return ;
    }
    int bytes_recorded = 0;
    const int byte_rate = 16000 * 1 * 16 / 8;
    const int total_bytes = 10 * byte_rate;
    uint8_t *buffer = (uint8_t *)malloc(1024);
    if(buffer == NULL) {
        fclose(instance->_file);
        vTaskDelete(NULL);
        return ;
    }

    // 写入WAV文件头
    uint8_t wav_header[44] = {0};
    memcpy(wav_header, "RIFF", 4);
    // 文件大小占位符，稍后填写
    memcpy(wav_header + 8, "WAVE", 4);
    memcpy(wav_header + 12, "fmt ", 4);
    *(uint32_t*)(wav_header + 16) = 16; // fmt块大小
    *(uint16_t*)(wav_header + 20) = 1;  // 音频格式 (PCM)
    *(uint16_t*)(wav_header + 22) = 1; // 声道数
    *(uint32_t*)(wav_header + 24) = 16000; // 采样率
    *(uint32_t*)(wav_header + 28) = byte_rate; // 字节率
    *(uint16_t*)(wav_header + 32) = 2; // 块对齐
    *(uint16_t*)(wav_header + 34) = 16; // 位深度
    memcpy(wav_header + 36, "data", 4);

    fwrite(wav_header, 1, 44, instance->_file);

    lv_label_set_text(instance->_label_button, "Recording");

    while(bytes_recorded < total_bytes) {
        size_t bytes_read = 0;
        if(ESP_OK != bsp_extra_i2s_read(buffer, 1024, &bytes_read, 1000)) {
            ESP_LOGE(TAG, "Error reading I2S data");
            break;
        }
        fwrite(buffer, 1, bytes_read, instance->_file);
        bytes_recorded += bytes_read;
        if(bytes_recorded % byte_rate == 0) {
            ESP_LOGI(TAG, "Recorded %d bytes", bytes_recorded);
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    lv_label_set_text(instance->_label_button, "Start");
    ESP_LOGI(TAG, "Finished recording %d bytes", bytes_recorded);
    uint32_t file_size = bytes_recorded + 36;
    uint32_t data_size = bytes_recorded;
    fseek(instance->_file, 4, SEEK_SET);
    fwrite(&file_size, 1, 4, instance->_file);
    fseek(instance->_file, 40, SEEK_SET);
    fwrite(&data_size, 1, 4, instance->_file);

    free(buffer);
    fclose(instance->_file);
    bsp_extra_codec_dev_stop();

    vTaskDelete(NULL);
    
}
