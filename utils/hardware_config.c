#include "hardware_config.h"
#include "lib/neopixel.h"
#include "lib/buzzer.h"
#include <stdio.h>

// Variáveis estáticas para debounce
static volatile uint32_t last_time_button_a = 0;
static volatile uint32_t last_time_button_b = 0;
static volatile uint32_t last_time_button_joystick = 0;

// Variáveis globais exportadas
volatile bool buttonA_flag = false;
volatile bool buttonB_flag = false;
volatile bool buttonJoyStick_flag = false;
int16_t vrx_valor = 0;
int16_t vry_valor = 0;

// Handler único de interrupção
static void gpio_button_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    
    switch(gpio) {
        case BUTTON_A:
            if (current_time - last_time_button_a > 200000) {
                last_time_button_a = current_time;
                buttonA_flag = true;
            }
            break;
            
        case BUTTON_B:
            if (current_time - last_time_button_b > 200000) {
                last_time_button_b = current_time;
                buttonB_flag = true;
            }
            break;
            
        case BUTTON_JOYSTICK:
            if (current_time - last_time_button_joystick > 200000) {
                last_time_button_joystick = current_time;
                buttonJoyStick_flag = true;
            }
            break;
    }
}

void hardware_setup() {
    stdio_init_all();
    
    // Inicialização de periféricos
    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);

    // Configuração dos LEDs
    gpio_init(RED_PIN);
    gpio_set_dir(RED_PIN, GPIO_OUT);
    gpio_init(GREEN_PIN);
    gpio_set_dir(GREEN_PIN, GPIO_OUT);
    gpio_init(BLUE_PIN);
    gpio_set_dir(BLUE_PIN, GPIO_OUT);

    // Configuração dos botões
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_init(BUTTON_JOYSTICK);
    gpio_set_dir(BUTTON_JOYSTICK, GPIO_IN);
    gpio_pull_up(BUTTON_JOYSTICK);

    // Configurar interrupções
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_JOYSTICK, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
    
    //while(1) printf("OK\n");

    npInit(LED_PIN);
    buzzer_init(BUZZER_PIN);

}

void atualizar_led_status(bool infectado, bool desliga) {
    if(!desliga) {
        gpio_put(RED_PIN, infectado);
        gpio_put(GREEN_PIN, !infectado);
        gpio_put(BLUE_PIN, false);
    } else {
        gpio_put(RED_PIN, false);
        gpio_put(GREEN_PIN, false);
        gpio_put(BLUE_PIN, false);
    }
}

void configurar_interrupcoes_botoes(bool a, bool b, bool joy) {
    gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, a);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, b);
    gpio_set_irq_enabled(BUTTON_JOYSTICK, GPIO_IRQ_EDGE_FALL, joy);
}

uint16_t aplicar_deadzone(uint16_t valor_adc) {
    if (valor_adc >= (CENTRO - DEADZONE) && valor_adc <= (CENTRO + DEADZONE)) {
        return CENTRO;  // Define o valor como centro se estiver dentro da zona morta
    }
    return valor_adc;  // Retorna o valor original se estiver fora da zona morta
}

void ler_joystick(){
    // Leitura dos valores dos eixos do joystick
    adc_select_input(1); // Seleciona o canal para o eixo X
    vrx_valor = adc_read();
    vrx_valor = aplicar_deadzone(vrx_valor);  // Aplica deadzone no eixo X

    adc_select_input(0); // Seleciona o canal para o eixo Y
    vry_valor = adc_read();
    vry_valor = aplicar_deadzone(vry_valor);  // Aplica deadzone no eixo Y
}

int8_t normalizar_direcao(uint16_t valor_adc) {
    if (valor_adc > CENTRO + DEADZONE) {
        return DIREITA;  // Ou CIMA, dependendo do eixo
    } else if (valor_adc < CENTRO - DEADZONE) {
        return ESQUERDA; // Ou BAIXO, dependendo do eixo
    }
    return MEIO; // Joystick centralizado
}

void normalizar_joystick() {
    vrx_valor = normalizar_direcao(vrx_valor);  // Esquerda (-1), Centro (0), Direita (1)
    vry_valor = normalizar_direcao(vry_valor);  // Baixo (-1), Centro (0), Cima (1)
}