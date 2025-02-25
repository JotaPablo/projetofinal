#include "neopixel.h"
#include "ws2818b.pio.h" // Biblioteca gerada pelo arquivo .pio durante compilação.
#include <stdio.h>
#include "hardware/clocks.h"
#include "pico/stdlib.h"


// Buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin) {
    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }

    npWrite();
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

/**
 * Limpa o buffer de pixels.
 */
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        npSetLED(i, 0, 0, 0);
    }
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite() {
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}
// Calcula o índice na matriz de LEDs
// Linhas pares(0, 2, 4): esquerda para direita; ímpares(1, 3): direita para esquerda.
int getIndex(int x, int y) {
    if (y % 2 == 0) {
        return y * 5 + (4 - x); // Linha par (esquerda para direita).
    } else {
        return y * 5 + x; // Linha ímpar (direita para esquerda).
    }
}

void exibirNumeroComFundo(uint32_t numero, 
                             uint8_t num_r, uint8_t num_g, uint8_t num_b,
                             uint8_t bg_r, uint8_t bg_g, uint8_t bg_b) {
    // Preenche todo o fundo primeiro
    for(int i = 0; i < 25; i++) {
        npSetLED(i, bg_r, bg_g, bg_b);
    }

    switch(numero){
        case 0:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(1,1), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 1:
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(2,3), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 2:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(1,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 3:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 4:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 5:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 6:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(1,1), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 7:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 8:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(1,1), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 9:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        default: break;
    }
    npWrite();
}