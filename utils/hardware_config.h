#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "lib/ssd1306.h"

// Defines de hardware
#define RED_PIN 13
#define GREEN_PIN 11
#define BLUE_PIN 12
#define BUTTON_A 5
#define BUTTON_B 6
#define BUTTON_JOYSTICK 22
#define BUZZER_PIN 21

//Controle do Joystick
#define VRX_PIN 27
#define VRY_PIN 26
#define ADC_MAX 4095
#define CENTRO 2047
#define DEADZONE 250
#define MEIO 0
#define CIMA 1
#define DIREITA 1
#define BAIXO -1
#define ESQUERDA -1



// Variáveis globais do hardware
extern volatile bool buttonA_flag;
extern volatile bool buttonB_flag;
extern volatile bool buttonJoyStick_flag;
extern int16_t vrx_valor;
extern int16_t vry_valor;

// Protótipos de funções
void hardware_setup();
void configurar_interrupcoes_botoes(bool a, bool b, bool joy);
void atualizar_led_status(bool infectado, bool desliga);
uint16_t aplicar_deadzone(uint16_t valor_adc);
void ler_joystick();
void normalizar_joystick();


#endif // HARDWARE_CONFIG_H