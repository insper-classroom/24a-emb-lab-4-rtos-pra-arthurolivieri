/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "ssd1306.h"
#include "gfx.h"

#include "pico/stdlib.h"
#include <stdio.h>

const uint BTN_1_OLED = 28;
const uint BTN_2_OLED = 26;
const uint BTN_3_OLED = 27;

const uint LED_1_OLED = 20;
const uint LED_2_OLED = 21;
const uint LED_3_OLED = 22;

const int TRIG_PIN = 19;
const int ECHO_PIN = 18;
const int VEL_SOM = 343;

QueueHandle_t xQueueTime;
QueueHandle_t xQueueDistance;
SemaphoreHandle_t xSemaphoreTrigger;

void oled1_btn_led_init(void) {
    gpio_init(LED_1_OLED);
    gpio_set_dir(LED_1_OLED, GPIO_OUT);

    gpio_init(LED_2_OLED);
    gpio_set_dir(LED_2_OLED, GPIO_OUT);

    gpio_init(LED_3_OLED);
    gpio_set_dir(LED_3_OLED, GPIO_OUT);

    gpio_init(BTN_1_OLED);
    gpio_set_dir(BTN_1_OLED, GPIO_IN);
    gpio_pull_up(BTN_1_OLED);

    gpio_init(BTN_2_OLED);
    gpio_set_dir(BTN_2_OLED, GPIO_IN);
    gpio_pull_up(BTN_2_OLED);

    gpio_init(BTN_3_OLED);
    gpio_set_dir(BTN_3_OLED, GPIO_IN);
    gpio_pull_up(BTN_3_OLED);
}

void oled1_demo_1(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    char cnt = 15;
    while (1) {

        if (gpio_get(BTN_1_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_1_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 1 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_2_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_2_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 2 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_3_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_3_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 3 - ON");
            gfx_show(&disp);
        } else {

            gpio_put(LED_1_OLED, 1);
            gpio_put(LED_2_OLED, 1);
            gpio_put(LED_3_OLED, 1);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "PRESSIONE ALGUM");
            gfx_draw_string(&disp, 0, 10, 1, "BOTAO");
            gfx_draw_line(&disp, 15, 27, cnt,
                          27);
            vTaskDelay(pdMS_TO_TICKS(50));
            if (++cnt == 112)
                cnt = 15;

            gfx_show(&disp);
        }
    }
}

void oled1_demo_2(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    while (1) {

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 1, "Mandioca");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 2, "Batata");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 4, "Inhame");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

void pin_callback(uint gpio, uint32_t events) {
    uint32_t time;
    if (events == 0x4) { // fall edge
        if (gpio == ECHO_PIN){
                time = to_us_since_boot(get_absolute_time());
                xQueueSendFromISR(xQueueTime, &time, 0);
            }
            

    } else if (events == 0x8) { // rise edge
        if (gpio == ECHO_PIN)
            time = to_us_since_boot(get_absolute_time());
            xQueueSendFromISR(xQueueTime, &time, 0);
    }
}

void trigger_task(void *p){
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    
    gpio_set_irq_enabled_with_callback(
        ECHO_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &pin_callback);

    while(true){
        xSemaphoreGive(xSemaphoreTrigger);
        gpio_put(TRIG_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(0.005));
        gpio_put(TRIG_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000.995));

    }
}

void echo_task(void *p){
    uint32_t time;
    int receive_count = 0;
    uint32_t time_rise;
    uint32_t time_fall;
    double time_high;
    double dist;

    while(true){
        while (receive_count < 2){
            if (xQueueReceive(xQueueTime, &time, 0)){
                if (receive_count == 0) time_rise = time;
                if (receive_count == 1) time_fall = time;
                receive_count++;
            }
        }
        if (receive_count == 2){
            time_high = time_fall - time_rise;
            dist = (double) time_high*VEL_SOM/(2*10000);

            //printf("%f\n", dist);
            xQueueSend(xQueueDistance, &dist, 0);
            receive_count = 0;
        }
    }
}

void oled_task(void *p){
    double distancia_min = 2.0;
    double distancia_max = 400.0;
    int linha_min = 10;
    int linha_max = 120;

    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    double dist;
    while(true){
        if (xSemaphoreTake(xSemaphoreTrigger, pdMS_TO_TICKS(1000)) == pdTRUE){
            if (xQueueReceive(xQueueDistance, &dist, pdMS_TO_TICKS(250))){
                char distStr[20]; // Array de char para armazenar a string formatada da distância
                snprintf(distStr, sizeof(distStr), "Dist: %.2f cm", dist); // Formata a distância com duas casas decimais
                float proporcao = (dist - distancia_min) / (distancia_max - distancia_min);
                int comprimento_linha = linha_min + (int)(proporcao * (linha_max - linha_min));

                // Limitar o comprimento da linha aos limites do display
                if (comprimento_linha > linha_max) comprimento_linha = linha_max;
                if (comprimento_linha < linha_min) comprimento_linha = linha_min;


                //printf("%f\n", dist);
                gpio_put(LED_1_OLED, 1);
                gpio_put(LED_2_OLED, 1);
                gpio_put(LED_3_OLED, 1);
                gfx_clear_buffer(&disp);
                gfx_draw_string(&disp, 0, 0, 1, distStr);
                gfx_draw_line(&disp, 15, 27, 15 + comprimento_linha,
                          27);
                gfx_show(&disp);
            }
            else{
                gfx_clear_buffer(&disp);
                gfx_draw_string(&disp, 0, 0, 1, "FAIL");
                gfx_show(&disp);
            }
        }
    }
}

int main() {
    stdio_init_all();

    xQueueTime = xQueueCreate(32, sizeof(uint32_t));
    xQueueDistance = xQueueCreate(64, sizeof(double));
    xSemaphoreTrigger = xSemaphoreCreateBinary();

    xTaskCreate(trigger_task, "trigger_task", 4095, NULL, 1, NULL);
    xTaskCreate(echo_task, "echo_task", 4095, NULL, 1, NULL);
    xTaskCreate(oled_task, "oled_task", 4095, NULL, 1, NULL);

    //xTaskCreate(oled1_demo_1, "Demo 1", 4095, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
