
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "ggwave.h"
#include "DFRobot_VEML7700.h"

// Pin configuration
#define PIN_LED0    GPIO_NUM_32
#define PIN_SPEAKER GPIO_NUM_33

#define SAMPLES_PER_FRAME 128
#define SAMPLE_RATE      6000

// Global GGWave instance
GGWave ggwave;

// Helper function to send text using GGWave
void send_text(const char *text, GGWave::TxProtocolId protocolId) {
    ESP_LOGI("send_text", "Sending text: %s", text);

    ggwave.init(text, protocolId);
    ggwave.encode();

    const auto &protocol = GGWave::Protocols::tx()[protocolId];
    const auto tones = ggwave.txTones();
    const auto duration_ms = protocol.txDuration_ms(ggwave.samplesPerFrame(), ggwave.sampleRateOut());
    
    for (auto &curTone : tones) {
        const auto freq_hz = (protocol.freqStart + curTone) * ggwave.hzPerSample();
        dac_output_enable(DAC_CHANNEL_1);
        dac_output_voltage(DAC_CHANNEL_1, 0);
        vTaskDelay(duration_ms / portTICK_PERIOD_MS);
    }

    dac_output_voltage(DAC_CHANNEL_1, 0);
    gpio_set_level(PIN_SPEAKER, LOW);
}

void app_main() {
    float lux;
    DFRobot_VEML7700 als;

    // Initialize peripherals
    gpio_set_direction(PIN_LED0, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_SPEAKER, GPIO_MODE_OUTPUT);
    als.begin();

    // Initialize GGWave
    GGWave::Parameters p = GGWave::getDefaultParameters();
    p.payloadLength = 16;
    p.sampleRateInp = SAMPLE_RATE;
    p.sampleRateOut = SAMPLE_RATE;
    p.sampleRate = SAMPLE_RATE;
    p.samplesPerFrame = SAMPLES_PER_FRAME;
    p.sampleFormatInp = GGWAVE_SAMPLE_FORMAT_I16;
    p.sampleFormatOut = GGWAVE_SAMPLE_FORMAT_U8;
    p.operatingMode = GGWAVE_OPERATING_MODE_TX | GGWAVE_OPERATING_MODE_TX_ONLY_TONES | GGWAVE_OPERATING_MODE_USE_DSS;
    
    ggwave.prepare(p);
    ESP_LOGI("setup", "GGWave instance initialized. Heap size: %d", ggwave.heapSize());

    // Main loop
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        gpio_set_level(PIN_LED0, HIGH);
        send_text("Hello!", GGWAVE_PROTOCOL_MT_FASTEST);
        gpio_set_level(PIN_LED0, LOW);

        vTaskDelay(200 / portTICK_PERIOD_MS);

        gpio_set_level(PIN_LED0, HIGH);
        send_text("This is a", GGWAVE_PROTOCOL_MT_FASTEST);
        send_text("ggwave demo", GGWAVE_PROTOCOL_MT_FASTEST);
        gpio_set_level(PIN_LED0, LOW);

        vTaskDelay(200 / portTICK_PERIOD_MS);

        gpio_set_level(PIN_LED0, HIGH);
        send_text("The ESP32", GGWAVE_PROTOCOL_MT_FASTEST);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        send_text("transmits data", GGWAVE_PROTOCOL_MT_FASTEST);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        send_text("using sound", GGWAVE_PROTOCOL_MT_FASTEST);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        send_text("through a buzzer", GGWAVE_PROTOCOL_MT_FASTEST);
        gpio_set_level(PIN_LED0, LOW);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Read ALS sensor
        als.getALSLux(lux);
        ESP_LOGI("loop", "Lux: %f lx", lux);
        char txt[64];
        snprintf(txt, sizeof(txt), "Lux: %f lx", lux);

        gpio_set_level(PIN_LED0, HIGH);
        send_text(txt, GGWAVE_PROTOCOL_MT_FASTEST);
        gpio_set_level(PIN_LED0, LOW);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

extern "C" void app_main()
{
    initArduino();
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    // Do your own thing
}