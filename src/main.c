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

// Credenciais WIFI - Troque pelas suas credenciais
#define WIFI_SSID "SEU_SSID"
#define WIFI_PASSWORD "SUA_SENHA"

// ----------------------------- Escopo de funções ------------------------------

void buttons_irq(uint gpio, uint32_t events);
void update_display();
void setup_display();
void gpio_led_bitdog(void);
void setup_button(uint pin);
// -------------------------------------------------------------------------------

// Função principal
int main() {
    // Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();

    // Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
    gpio_led_bitdog();

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
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default) {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    webserver_init();
    
    // Inicializa o conversor ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);

    setup_button(BUTTON_A);
    setup_button(BUTTON_B);

    setup_display();

    while (true) {
        /* 
        * Efetuar o processamento exigido pelo cyw43_driver ou pela stack TCP/IP.
        * Este método deve ser chamado periodicamente a partir do ciclo principal 
        * quando se utiliza um estilo de sondagem pico_cyw43_arch 
        */
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        
        update_display();

        if ((temperatura > 60) || (umidade > 70) || (oxigenio < 15)) {
            //gpio_put(LED_RED_PIN, 1);
            //gpio_put(LED_GREEN_PIN, 0);
            //gpio_put(LED_BLUE_PIN, 0);

            //set_led_matrix(11, pio, sm);
            //buzzer_tone(50); 
            //sleep_ms(500);
            //buzzer_off();      
        }
        else if ((40 < temperatura && temperatura < 60) && (50 < umidade && umidade < 70) && (oxigenio > 15)) {
            //gpio_put(LED_RED_PIN, 0);
            //gpio_put(LED_GREEN_PIN, 1);
            //gpio_put(LED_BLUE_PIN, 0);
            //set_led_matrix(15, pio, sm);
        }
        else {
            //gpio_put(LED_RED_PIN, 0);
            //gpio_put(LED_GREEN_PIN, 0);
            //gpio_put(LED_BLUE_PIN, 1);
            //set_led_matrix(15, pio, sm);
        }

        sleep_ms(1000);
    }

    // Desligar a arquitetura CYW43
    cyw43_arch_deinit();
    return 0;
}


// ---------------------------------- Funções ------------------------------------

void buttons_irq(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    if (gpio == BUTTON_A) {
        if (current_time - last_time_A > DEBOUNCE_TIME) {
            // ...
          return;
        }
    } 
    else if (gpio == BUTTON_B) {
        if (current_time - last_time_B > DEBOUNCE_TIME) {
            reset_usb_boot(0, 0);
            last_time_B = current_time;
            return;
        }
    }
}

void update_display() {
    ssd1306_fill(&ssd, !cor);                          // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
    char msg_temp[20];
    sprintf(msg_temp, "Temperatura %d", temperatura);
    ssd1306_draw_string(&ssd, msg_temp, 8, 6);
    char msg_umid[20];
    sprintf(msg_umid, "Umidade %d", umidade);
    ssd1306_draw_string(&ssd, msg_umid, 8, 18);
    char msg_oxig[20];
    sprintf(msg_oxig, "Oxigenio %d", oxigenio);
    ssd1306_draw_string(&ssd, msg_oxig, 8, 33);
    
    ssd1306_send_data(&ssd);                           // Atualiza o display
    sleep_ms(735);
}

void setup_display() {
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    //ssd1306_t ssd;                                              // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_led_bitdog(void) {
    // Configuração dos LEDs como saída
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

// Configura o pino GPIO do botão
void setup_button(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
    gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &buttons_irq);
}