#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

#include "alarm_logic.h"

static const char *TAG = "alarm";

#define STATUS_LED_GPIO CONFIG_ALARM_STATUS_LED_GPIO

/* Fixed by the schematic: 3V3 -> LDR -> GPIO4 -> 10k -> GND. On this board
   GPIO4 is ADC1 channel 3 -- chip-specific, so not made Kconfig-configurable. */
#define LDR_ADC_UNIT    ADC_UNIT_1
#define LDR_ADC_CHANNEL ADC_CHANNEL_3

#define LEDC_TIMER    LEDC_TIMER_0
#define LEDC_MODE     LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL  LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_8_BIT
#define LEDC_DUTY_ON  (1 << 7) /* ~50% of an 8-bit duty range */

#define POLL_PERIOD_MS 200

/* Onboard status LED is an addressable (WS2812-style) LED strip, driven over RMT. */
static led_strip_handle_t led_strip;

static void configure_led(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = STATUS_LED_GPIO,
        .max_leds       = 1,  // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz  = 10 * 1000 * 1000,  // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
}

static void set_status_led(bool on)
{
    if (on) {
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        led_strip_refresh(led_strip);
    } else {
        led_strip_clear(led_strip);
    }
}

static adc_oneshot_unit_handle_t s_adc_handle;

static void configure_ldr(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = LDR_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &s_adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, LDR_ADC_CHANNEL, &chan_config));
}

static int read_light_raw(void)
{
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc_handle, LDR_ADC_CHANNEL, &raw));
    return raw;
}

static void configure_buzzer(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode      = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = CONFIG_ALARM_BUZZER_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = CONFIG_ALARM_BUZZER_GPIO,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

static void set_buzzer(bool on)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, on ? LEDC_DUTY_ON : 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

static void configure_button(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << CONFIG_ALARM_BUTTON_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

static bool button_pressed(void)
{
    /* Active-low: internal pull-up holds the line high until the button shorts it low. */
    return gpio_get_level(CONFIG_ALARM_BUTTON_GPIO) == 0;
}

void app_main(void)
{
    configure_led();
    configure_ldr();
    configure_buzzer();
    configure_button();

    alarm_state_t state = ALARM_STATE_IDLE;
    bool buzzer_on      = false;

    while (1) {
        int light_raw = read_light_raw();
        bool is_light = light_raw > CONFIG_ALARM_LIGHT_THRESHOLD;
        bool pressed  = button_pressed();

        alarm_state_t next_state = alarm_next_state(state, is_light, pressed);

        if (next_state != state) {
            ESP_LOGW(TAG, "light raw=%d -- state %s -> %s", light_raw, alarm_state_name(state), alarm_state_name(next_state));
            state = next_state;
        }

        buzzer_on = alarm_next_buzzer(state, buzzer_on);
        set_buzzer(buzzer_on);
        set_status_led(buzzer_on);

        /* Always-on so the threshold can be calibrated live from the monitor. */
        ESP_LOGI(TAG, "light raw=%d threshold=%d state=%s", light_raw, CONFIG_ALARM_LIGHT_THRESHOLD, alarm_state_name(state));

        vTaskDelay(pdMS_TO_TICKS(POLL_PERIOD_MS));
    }
}
