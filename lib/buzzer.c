
#include "buzzer.h"

void buzzer_setup_pwm(uint pin, uint freq_hz){
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);

    // Configura o clock do PWM
    uint32_t clock_div = clock_get_hz(clk_sys) / (freq_hz * 4096);
    pwm_set_clkdiv(slice_num, clock_div);

    // Ativa o PWM e define duty cycle como 0% inicialmente
    pwm_set_chan_level(slice_num, channel, 0);
    pwm_set_enabled(slice_num, true);
}


// Função para tocar o buzzer com PWM. (500, 800, 1000)
void buzzer_play(uint pin, uint times, uint freq_hz, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin); // Obtém o slice
    uint channel = pwm_gpio_to_channel(pin);     // Obtém o canal PWM

    for (int i = 0; i < times; i++) {
        pwm_set_chan_level(slice_num, channel, freq_hz/2); // Duty cycle 50% (som ligado)
        sleep_ms(duration_ms);
        pwm_set_chan_level(slice_num, channel, 0); // Duty cycle 0% (som desligado)
        sleep_ms(200); // Pausa entre os toques
    }
}
