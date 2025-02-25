#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <stdint.h>
#include "hardware/pio.h"

// Número de LEDs e pino padrão.
#define LED_COUNT 25
#define LED_PIN 7

// Definição de pixel GRB
typedef struct {
    uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
} npLED_t;

// Funções para controle dos LEDs
void npInit(uint pin);
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b);
void npClear();
void npWrite();
int getIndex(int x, int y);
void exibirNumeroComFundo(uint32_t numero, 
                          uint8_t num_r, uint8_t num_g, uint8_t num_b,
                          uint8_t bg_r, uint8_t bg_g, uint8_t bg_b);

#endif // NEOPIXEL_H
