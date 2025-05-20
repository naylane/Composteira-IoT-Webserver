#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"     // Biblioteca para arquitetura Wi-Fi da Pico com CYW43
#include "pico/bootrom.h"

#include "hardware/adc.h"
#include "hardware/i2c.h"

#include "lwip/pbuf.h"           // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"            // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h"          // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)

#include "setup.h"
#include "src/webserver.h"
#include "lib/ws2812.h"
#include "lib/joystick.h"
#include "ws2812.pio.h"

// Credenciais WIFI - Troque pelas suas credenciais
#define WIFI_SSID "Familia-2.4G"
#define WIFI_PASSWORD "31261112"

uint MAX_TEMP = 60; // Limite máximo de temperatura
uint MIN_TEMP = 40; // Limite mínimo de temperatura
uint MAX_UMID = 70; // Limite de umidade
uint MIN_UMID = 70; // Limite de umidade
uint LIM_OXIG = 15; // Limite de oxigênio

uint16_t x_pos;
uint16_t y_pos;

int oxygenLevels[] = {10, 15, 20, 25};
int oxygenIndex = 0;




// ----------------------------- Escopo de funções ------------------------------

int clamp(int val, int min_val, int max_val);
int map_value_clamped(int val, int in_min, int in_max, int out_min, int out_max);
void le_valores();
void buttons_irq(uint gpio, uint32_t events);
void update_display();
void setup_display();
void setup_matrix();
void gpio_led_bitdog(void);
void setup_button(uint pin);
// -------------------------------------------------------------------------------


int main() {
    // Configurações
    stdio_init_all();
    gpio_led_bitdog();
    setup_button(BUTTON_B);
    //setup_button(JOYSTICK_BTN);
    joystick_init();
    setup_display();
    setup_matrix();
    adc_gpio_init(JOYSTICK_Y);
    adc_gpio_init(JOYSTICK_X);
    adc_init();

    // Inicializa a arquitetura do cyw43
    while (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    // GPIO do CI CYW43 em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI - fazer um loop até que esteja conectado
    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("%d\n", cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000));
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi!\n");

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default) {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    webserver_init();

    while (true) {
        /* 
        * Efetuar o processamento exigido pelo cyw43_driver ou pela stack TCP/IP.
        * Este método deve ser chamado periodicamente a partir do ciclo principal 
        * quando se utiliza um estilo de sondagem pico_cyw43_arch 
        */
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        
        le_valores();
        update_display();

        if ((temperatura > MAX_TEMP) || (umidade > MAX_UMID) || (oxigenio < LIM_OXIG)) {
            gpio_put(LED_RED_PIN, 1);
            gpio_put(LED_GREEN_PIN, 0);
            gpio_put(LED_BLUE_PIN, 0);
        }
        else if ((MIN_TEMP < temperatura && temperatura < MAX_TEMP) && (MIN_UMID < umidade && umidade < MAX_UMID) && (oxigenio > LIM_OXIG)) {
            gpio_put(LED_RED_PIN, 0);
            gpio_put(LED_GREEN_PIN, 1);
            gpio_put(LED_BLUE_PIN, 0);
        }
        else {
            gpio_put(LED_RED_PIN, 0);
            gpio_put(LED_GREEN_PIN, 0);
            gpio_put(LED_BLUE_PIN, 1);
        }

        sleep_ms(1000);
    }

    // Desligar a arquitetura CYW43
    cyw43_arch_deinit();
    return 0;
}


// ---------------------------------- Funções ------------------------------------

int clamp(int val, int min_val, int max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

int map_value_clamped(int val, int in_min, int in_max, int out_min, int out_max) {
    val = clamp(val, in_min, in_max);
    return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void le_valores() {
    joystick_read_x(&x_pos); // Temperatura
    joystick_read_y(&y_pos); // Umidade

    temperatura = map_value_clamped(x_pos, XY_MIN_ADC, XY_MAX_ADC, 20, 70);  // 20 °C – 70 °C
    umidade = map_value_clamped(y_pos, XY_MIN_ADC, XY_MAX_ADC, 90, 30);      // 90% – 30%

    // Simular oxigênio com clique
    if (gpio_get(JOYSTICK_BTN)) { // Pressionado
        oxygenIndex = (oxygenIndex + 1) % 4;
        sleep_ms(300); // debounce
    }
    oxigenio = oxygenLevels[oxygenIndex];
}

void buttons_irq(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    if (gpio == BUTTON_B) {
        if (current_time - last_time_B > DEBOUNCE_TIME) {
            reset_usb_boot(0, 0);
            last_time_B = current_time;
            return;
        }
    }
}

void update_display() {
    ssd1306_fill(&ssd, !cor);
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);
    char msg_temp[20];
    sprintf(msg_temp, "Temperatura %d", temperatura);
    ssd1306_draw_string(&ssd, msg_temp, 8, 6);
    char msg_umid[20];
    sprintf(msg_umid, "Umidade %d", umidade);
    ssd1306_draw_string(&ssd, msg_umid, 8, 20);
    char msg_oxig[20];
    sprintf(msg_oxig, "Oxigenio %d", oxigenio);
    ssd1306_draw_string(&ssd, msg_oxig, 8, 34);
    
    ssd1306_send_data(&ssd);
    sleep_ms(735);
}

void setup_display() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

void setup_matrix() {
    uint offset = pio_add_program(pio, &pio_matrix_program);
    pio_matrix_program_init(pio, sm, offset, WS2812_PIN);
    clear_matrix(pio, sm);
    sleep_ms(100);
}

// Configura os pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_led_bitdog(void) {
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, 0);
    
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, 0);
    
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, 0);
}

// Configura o pino GPIO do botão push-up da BitDogLab
void setup_button(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
    gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &buttons_irq);
}