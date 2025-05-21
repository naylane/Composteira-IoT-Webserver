#ifndef SETUP_H
#define SETUP_H

#include "hardware/pio.h"
#include "lib/ssd1306.h"
#include "lib/ws2812.h"

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN   // GPIO do CI CYW43
#define LED_RED_PIN 13
#define LED_BLUE_PIN 12      
#define LED_GREEN_PIN 11

#define BUTTON_B 6
#define DEBOUNCE_TIME 200000        // Tempo para debounce em ms
static uint32_t last_time_B = 0;    // Tempo da última interrupção do botão B

#define WS2812_PIN 7
extern PIO pio;
extern uint sm;

#define MAX_TEMP 60 // Limite máximo de temperatura
#define MIN_TEMP 40 // Limite mínimo de temperatura
#define MAX_UMID 70 // Limite máximo de umidade
#define MIN_UMID 50 // Limite mínimo de umidade
#define LIM_OXIG 15 // Limite de oxigênio

extern int temperatura;
extern int umidade;
extern int oxigenio;

#endif