#include "buzzer.h"

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"  // para clock_get_hz()

// Variáveis estáticas internas para controle do buzzer
static uint buzzer_pin;            // Pino configurado para o buzzer
static bool buzzer_active = false; // Indica se um beep está ativo
static uint32_t beep_end_time = 0; // Tempo (em ms) para desligar o beep

// Inicializa o buzzer: configura o pino como PWM e desliga o som inicialmente
void buzzer_init(uint pin) {
    buzzer_pin = pin;
    gpio_set_function(buzzer_pin, GPIO_FUNC_PWM);
    
    uint slice_num = pwm_gpio_to_slice_num(buzzer_pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta o divisor do clock (pode ser modificado conforme necessário)
    pwm_init(slice_num, &config, true);
    
    // Garante que o buzzer inicie desligado
    pwm_set_gpio_level(buzzer_pin, 0);
}

// Liga o buzzer com a frequência especificada
void buzzer_turn_on(uint frequency) {
    uint slice_num = pwm_gpio_to_slice_num(buzzer_pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;
    
    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(buzzer_pin, top * 0.7);  // Duty cycle de 50%
}

// Desliga o buzzer
void buzzer_turn_off(void) {
    pwm_set_gpio_level(buzzer_pin, 0);
}

// Inicia um beep não bloqueante: liga o buzzer com a frequência e agenda seu desligamento
void buzzer_start(uint frequency, uint duration_ms) {
    // Se já estiver tocando, interrompe para reiniciar
    buzzer_turn_off();
    buzzer_turn_on(frequency);
    
    buzzer_active = true;
    beep_end_time = to_ms_since_boot(get_absolute_time()) + duration_ms;
}

// Para o beep (desliga o buzzer e zera o estado)
void buzzer_stop(void) {
    buzzer_turn_off();
    buzzer_active = false;
}

// Deve ser chamada periodicamente (ex.: no loop principal) para atualizar o estado do beep
void buzzer_update(void) {
    if (buzzer_active && (to_ms_since_boot(get_absolute_time()) >= beep_end_time)) {
        buzzer_stop();
    }
}

// ------------------------
// Funções de efeitos sonoros
// ------------------------




// Som de erro: 392 Hz (G4) por 300 ms
void buzzer_som_selecao(void) {
    buzzer_start(392, 100);
}

// Som de análise iniciada: 440 Hz por 50 ms
void buzzer_som_analise_iniciada(void) {
    buzzer_start(440, 50);
}

// Som de análise concluída: se sucesso, 523 Hz por 200 ms; senão, chama o som de erro
void buzzer_som_analise_concluida() {
   
    buzzer_start(523, 200);
} 