/**********************************
* INCLUDES
**********************************/
// Bibliotecas padrão
#include <stdio.h>
#include <math.h>
#include <string.h>

// Hardware Pico
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "pico/bootrom.h"

// Periféricos
#include "lib/ssd1306.h"
#include "lib/neopixel.h"
#include "lib/buzzer.h"

#include "utils/hardware_config.h"

/**********************************
* DEFINES E CONFIGURAÇÕES
**********************************/
// Display OLED
#define TAMANHO_FONTE 8
#define MARGEM 4


//Programa Principal
#define TEMPO_TROCA_MENSAGEM 3000 // 3 segundos


// Configurações do programa
#define NUM_PLANTAS 5
#define FOLHAS_POR_PLANTA 5
#define CUSTO_POR_FUNGICIDA 10
#define ERRO_DETECCAO 0.2

/**********************************
* TIPOS DE DADOS
**********************************/
typedef struct {
    float R;    // Reflectância no vermelho
    float G;    // Reflectância no verde
    float B;    // Reflectância no azul
    float NIR;  // Reflectância no infravermelho
} Reflectancia;

typedef enum {
    MODO_ESCANEAMENTO,
    ANALISE
} EstadoEscaneamento;

typedef enum {
    ESTADO_MENU,
    ESTADO_SELECIONAR_FOLHA,
    ESTADO_ANALISAR,
    ESTADO_ESCANEAMENTO,
} Estado;

typedef struct {
    Reflectancia reflectancia;
    float ndvi, gndvi;
    bool visivel;
    bool infectada;
} EstadoFolha;

typedef struct {
    int id;
    EstadoFolha folhas[FOLHAS_POR_PLANTA];
    bool infectada;
    bool tratada;
} Planta;

int planta_cafe[5][5] = {
    {1, 20, 20, 0, 0},  
    {10, 0, 20, 0, 0},   
    {0, 0, 20, 2, 3},    
    {5, 4, 20, 0, 10},   
    {10, 0, 20, 0, 0}    
};

/**********************************
* VARIÁVEIS GLOBAIS
**********************************/
// Controle do display
ssd1306_t display;


// Estado do sistema
EstadoEscaneamento estado_escaneamento = MODO_ESCANEAMENTO;
Reflectancia valores_ajustados;

/**********************************
* PROTÓTIPOS DE FUNÇÕES
**********************************/

// Interface gráfica
void display_init(ssd1306_t *display);
void exibir_grafico_display(Reflectancia r);
void exibir_resultado_analise(bool resultado, float R , float G, float B, float NIR, float ndvi, float gndvi);
void exibir_resultado_analise_folha(EstadoFolha folha);
void animacao_analise(int duracao_ms);
void exibir_grafico_matriz(Reflectancia r);
void escrever_linha(const char* texto, int linha, int coluna, bool centralizado);

// Lógica do programa
bool detectar_doenca(float R, float G, float B, float NIR);
bool detectar_doenca_folha(EstadoFolha folha); 
void simular_escaneamento();
Planta gerar_planta(int id, int tipo);
bool analisar_folha(Planta p, int folha);
void tratar_planta(Planta *p);
void exibe_planta(Planta p, int folha);
void teste_deteccao();
void exibir_menu_planta(int atual, int custo);
void exibir_menu_folha(int num);
void gerenciar_menu_principal(int *planta_atual, bool *atualiza_display );
void gerenciar_selecao_folha(int *folha_atual, bool *atualiza_display );

int main() {
    hardware_setup();
    display_init(&display);
    Planta plantas[NUM_PLANTAS];
    Estado estado = ESTADO_MENU;
    int planta_atual = 0, folha_atual = 0;
    int custo = 0;
    bool atualiza_display  = true;
    uint32_t tempo_ultima_atualizacao_menu = 0; // Timer para atualizações periódicas
    uint32_t tempo_ultima_atualizacao_folha = 0; // Timer para atualizações periódicas
    uint32_t tempo_atual = 0;
    // Inicialização das plantas
    plantas[0] = gerar_planta(0, 0); // Saudável
    plantas[1] = gerar_planta(1, 0); // Saudável
    plantas[2] = gerar_planta(2, 1); // Visivelmente infectada
    plantas[3] = gerar_planta(3, 2); // Infectada sem sintomas
    plantas[4] = gerar_planta(4, 2); // Infectada sem sintomas
    
    while(true) {
        ler_joystick();
        normalizar_joystick();

        tempo_atual = to_ms_since_boot(get_absolute_time());

        buzzer_update();

        switch(estado) {
            case ESTADO_MENU:
                gerenciar_menu_principal(&planta_atual, &atualiza_display);
                

                if(buttonA_flag){
                    configurar_interrupcoes_botoes(false, false, false);
                    ssd1306_fill(&display, false);
                    if(plantas[planta_atual].tratada){
                        buzzer_som_analise_concluida();
                        sleep_ms(100);
                        buzzer_turn_off();
                        sleep_ms(100);
                        buzzer_som_analise_concluida();    
                        sleep_ms(100);
                        buzzer_turn_off();
                        escrever_linha("TRATAMENTO JA", 2, 0, true);
                        escrever_linha("REALIZADO", 3, 0, true);

                        sleep_ms(1000);
                    }
                    else{
                        buzzer_som_analise_iniciada();
                        sleep_ms(100);
                        buzzer_turn_off();
                        sleep_ms(100);
                        buzzer_som_analise_iniciada();    
                        sleep_ms(100);
                        buzzer_turn_off();
                        escrever_linha("TRATANDO PLANTA", 2, 0, true);
                        escrever_linha("", 3, 0, true);
                        sleep_ms(250);
                        escrever_linha("TRATANDO PLANTA", 2, 0, true);
                        escrever_linha(".", 3, 0, true);
                        sleep_ms(250);
                        escrever_linha("TRATANDO PLANTA", 2, 0, true);
                        escrever_linha("..", 3, 0, true);
                        sleep_ms(250);
                        escrever_linha("TRATANDO PLANTA", 2, 0, true);
                        escrever_linha("...", 3, 0, true);
                        sleep_ms(250);
                        buzzer_som_analise_concluida();
                        sleep_ms(100);
                        buzzer_turn_off();
                        sleep_ms(100);
                        buzzer_som_analise_concluida();    
                        sleep_ms(100);
                        buzzer_turn_off();
                        custo += CUSTO_POR_FUNGICIDA;
                    }
                    tratar_planta(&plantas[planta_atual]);
                    configurar_interrupcoes_botoes(true, true, true);
                    buttonA_flag = false;
                    atualiza_display = true;
                }

                if(buttonB_flag) {
                    estado = ESTADO_SELECIONAR_FOLHA;
                    folha_atual = 0;
                    configurar_interrupcoes_botoes(true, true, false);
                    atualiza_display = true;
                    buttonB_flag = false;
                    buzzer_som_selecao();
                }
                
                if(buttonJoyStick_flag) {
                    estado = ESTADO_ESCANEAMENTO;
                    buttonJoyStick_flag = false;
                    buzzer_som_selecao();
                }
                
                if(atualiza_display) {
                    exibe_planta(plantas[planta_atual], -1);
                    exibir_menu_planta(planta_atual + 1, custo);
                    atualizar_led_status(plantas[planta_atual].infectada, false);
                    atualiza_display = false;
                }

                // Verifica se passaram 3 segundos
                if (tempo_atual - tempo_ultima_atualizacao_menu >= 3000) {
                    atualiza_display = true;
                    tempo_ultima_atualizacao_menu = tempo_atual;
                }
                break;

            case ESTADO_SELECIONAR_FOLHA:
                gerenciar_selecao_folha(&folha_atual, &atualiza_display);
                
                if(buttonB_flag) {
                    estado = ESTADO_ANALISAR;
                    buttonB_flag = false;
                }
                
                if(buttonA_flag) {
                    estado = ESTADO_MENU;
                    configurar_interrupcoes_botoes(true, true, true);
                    buttonA_flag = false;
                    atualiza_display = true;
                    buzzer_som_selecao();
                }
                
                if(atualiza_display) {
                    exibe_planta(plantas[planta_atual], folha_atual + 1);
                    exibir_menu_folha(folha_atual + 1);
                    atualizar_led_status(plantas[planta_atual].infectada, false);
                    atualiza_display = false;
                }

                 // Verifica se passaram 3 segundos
                 if (tempo_atual - tempo_ultima_atualizacao_folha >= 3000) {
                    atualiza_display = true;
                    tempo_ultima_atualizacao_folha = tempo_atual;
                }
                break;

            case ESTADO_ANALISAR:
                configurar_interrupcoes_botoes(false, false, false); // Desativar todas
                animacao_analise(500);
                bool resultado = detectar_doenca_folha(plantas[planta_atual].folhas[folha_atual]);
                if(resultado) plantas[planta_atual].infectada = true;
                atualizar_led_status(resultado, false);
                exibir_resultado_analise_folha(plantas[planta_atual].folhas[folha_atual]);
                configurar_interrupcoes_botoes(true, true, false); // Restaurar
                buzzer_som_selecao();
                estado = ESTADO_SELECIONAR_FOLHA;
                atualiza_display = true;
                break;

            case ESTADO_ESCANEAMENTO:
                atualizar_led_status(false, true);
                simular_escaneamento();
                estado = ESTADO_MENU;
                atualiza_display = true;
                buzzer_som_selecao();
                break;
        }

        sleep_ms(100);
    }
}

void gerenciar_menu_principal(int *planta_atual, bool *atualiza_display) {
    if(vrx_valor == DIREITA) {
        *planta_atual = (*planta_atual + 1) % NUM_PLANTAS;
        *atualiza_display = true;
    }
    else if(vrx_valor == ESQUERDA) {
        *planta_atual = (*planta_atual - 1 + NUM_PLANTAS) % NUM_PLANTAS;
        *atualiza_display = true;
        
    }
}

void gerenciar_selecao_folha(int *folha_atual, bool *atualiza_display) {
    if(vrx_valor == DIREITA) {
        *folha_atual = (*folha_atual + 1) % FOLHAS_POR_PLANTA;
        *atualiza_display = true;
        
    }
    else if(vrx_valor == ESQUERDA) {
        *folha_atual = (*folha_atual - 1 + FOLHAS_POR_PLANTA) % FOLHAS_POR_PLANTA;
        *atualiza_display = true;
        
    }
}

// Função para criar uma planta com infecção aleatória
// Função para criar uma planta com id e configurar suas folhas
// Função para gerar uma nova planta
Planta gerar_planta(int id, int tipo) {
    Planta p;
    p.id = id;
    p.infectada = false;
    p.tratada = false;

    for (int i = 0; i < FOLHAS_POR_PLANTA; i++) {
        float R, G, B, NIR;
        bool doente = false;
        bool visivel_doente = false;

        // Configuração baseada no tipo de planta desejado
        switch (tipo) {
            case 0: // Saudável
                // Garante que os valores vão gerar um NDVI e GNDVI sempre acima do limiar saudável
                R = (rand() % 10 + 30) / 100.0f;  // Mantém baixo para um NDVI alto
                G = (rand() % 10 + 60) / 100.0f;  // Mantém alto para um GNDVI alto
                B = (rand() % 20 + 40) / 100.0f;  
                NIR = (rand() % 20 + 95) / 100.0f;  // Mantém bem alto para NDVI/GNDVI saudáveis
                break;

            case 1: // Visivelmente infectada
                R = 0.70f + (rand() % 15) / 100.0f;
                G = 0.40f + (rand() % 10) / 100.0f;
                B = 0.30f + (rand() % 10) / 100.0f;
                NIR = 0.60f + (rand() % 10) / 100.0f;
                visivel_doente = true;
                doente = true;
                break;

            case 2: // Infectada sem sintomas visíveis
                R = 0.50f + (rand() % 10) / 100.0f;
                G = 0.55f + (rand() % 10) / 100.0f;
                B = 0.45f + (rand() % 10) / 100.0f;
                NIR = 0.70f + (rand() % 20) / 100.0f;
                doente = true;
                break;
        }

        // Verifica infecção com a função existente
        doente = detectar_doenca(R, G, B, NIR);

        // Calcula índices NDVI e GNDVI
        float ndvi = (NIR - R) / (NIR + R + 0.001f);
        float gndvi = (NIR - G) / (NIR + G + 0.001f);

        p.folhas[i] = (EstadoFolha) {
            .reflectancia = {R, G, B, NIR},
            .ndvi = ndvi,
            .gndvi = gndvi,
            .visivel = visivel_doente,
            .infectada = doente
        };

        // Se houver uma folha visivelmente doente, a planta toda é marcada como infectada
        if (visivel_doente) {
            p.infectada = true;
        }
    }

    return p;
}


bool analisar_folha(Planta p, int folha) {
    EstadoFolha estado_folha = p.folhas[folha];

    if (estado_folha.visivel) {
        // Se a folha tem infecção visível, sempre será detectada como infectada
        return true;
    } 
    else {
        Reflectancia relectancia = estado_folha.reflectancia;
        return detectar_doenca(relectancia.R, relectancia.G, relectancia.B, relectancia.NIR);
    }
}

// Função para tratar a planta
void tratar_planta(Planta *p) {
    for (int i = 0; i < FOLHAS_POR_PLANTA; i++) {
        // Define valores saudáveis
        p->folhas[i] = (EstadoFolha) {
            .reflectancia = {0.40f, 0.70f, 0.50f, 0.95f},
            .ndvi = (0.95f - 0.40f) / (0.95f + 0.40f + 0.001f),
            .gndvi = (0.95f - 0.70f) / (0.95f + 0.70f + 0.001f),
            .visivel = false,
            .infectada = false
        };
    }
    p->infectada = false;
    p->tratada = true;
}

void exibe_planta(Planta p, int folha) {
    // Cores pré-definidas (const para otimização)
    static const uint8_t MARROM_VINHO[3] = {5, 0, 0};
    static const uint8_t VERDE_FRACO[3]  = {0, 100, 0};
    static const uint8_t VERDE_FORTE[3]  = {0, 5, 0};
    static const uint8_t LARANJA_FRACO[3] = {100, 100, 0};
    static const uint8_t LARANJA_FORTE[3] = {5, 5, 0};

    for(int y = 0; y < 5; y++) {      // Linhas lógicas (0-4)
        for(int x = 0; x < 5; x++) {  // Colunas lógicas (0-4)
            int valor = planta_cafe[y][x]; // Note: invertemos x e y aqui
            int index = getIndex(x, 4 - y); // Inverte a ordem das linhas

            if(valor >= 1 && valor <= FOLHAS_POR_PLANTA) { 
                int folha_atual = valor;

                if(folha_atual == folha) { // Selecionada
                    if(p.folhas[folha_atual-1].visivel) {
                        npSetLED(index, LARANJA_FORTE[0], LARANJA_FORTE[1], LARANJA_FORTE[2]);
                    } else {
                        npSetLED(index, VERDE_FORTE[0], VERDE_FORTE[1], VERDE_FORTE[2]);
                    }
                } else { // Não selecionada
                    if(p.folhas[folha_atual-1].visivel) {
                        npSetLED(index, LARANJA_FRACO[0], LARANJA_FRACO[1], LARANJA_FRACO[2]);
                    } else {
                        npSetLED(index, VERDE_FRACO[0], VERDE_FRACO[1], VERDE_FRACO[2]);
                    }
                }
            } 
            else if(valor == 10) {
                npSetLED(index, MARROM_VINHO[0], MARROM_VINHO[1], MARROM_VINHO[2]);
            }
            else if(valor == 20){
                npSetLED(index, VERDE_FRACO[0], VERDE_FRACO[1], VERDE_FRACO[2]);
            }
            else {
                npSetLED(index, 0, 0, 0);
            }
        }
    }
    npWrite();
}

// Escreve diretamente no display com controle preciso
void escrever_linha(const char* texto, int linha, int coluna, bool centralizado) {
    int pos_x = coluna * TAMANHO_FONTE;
    int pos_y = linha * 10; // 10px por linha
    
    if(centralizado) {
        int len = strlen(texto);
        pos_x = (128 - (len * TAMANHO_FONTE)) / 2;
    }
    
    ssd1306_draw_string(&display, texto, pos_x, pos_y);
    ssd1306_send_data(&display);
}

// Função para exibir menu de plantas
void exibir_menu_planta(int atual, int custo) {
    static uint8_t mensagem_atual = 0;
    static uint32_t ultima_troca = 0;
    char buffer[30];
    
    // Limpa display
    ssd1306_fill(&display, false);

    // ----- CABEÇALHO -----
    // Seta esquerda
   
    escrever_linha("<", 0, 0, false);
    
    
    // Título + contador
    snprintf(buffer, sizeof(buffer), "%s %d/%d", 
           "PLANTA:", atual, 5);
    escrever_linha(buffer, 0, 0, true); // Centralizado

    // Seta direita
    escrever_linha(">", 0, 15, false); // Coluna 15 (128 - 8px)
    
    // CUSTO
    snprintf(buffer, sizeof(buffer), "%s %d.00", 
           "CUSTO:", custo);
    escrever_linha(buffer, 1, 0, true); // Centralizado

    // ----- MENSAGENS -----
    uint32_t agora = to_ms_since_boot(get_absolute_time());
    if((agora - ultima_troca > 3000)) {
        mensagem_atual = (mensagem_atual + 1) % 4;
        ultima_troca = agora;
    }
    
    switch (mensagem_atual)
    {
    case 0:
        escrever_linha("MOVA O", 3, 0, true);    
        escrever_linha("JOYSTICK < >", 4, 0, true);       
        break;
    case 1:
        escrever_linha("APERTE B", 3, 0, true);    
        escrever_linha("p/ SELECIONAR", 4, 0, true);  
        break;
    case 2:
        escrever_linha("APERTE A", 3, 0, true);    
        escrever_linha("p/ TRATAR", 4, 0, true);
        break;
    case 3:
        escrever_linha("APERTE JOY", 3, 0, true);    
        escrever_linha("p/ ESCANEAR", 4, 0, true);
        break;
    }

    ssd1306_send_data(&display);
}

// Função para exibir menu de folhas
void exibir_menu_folha(int num) {
    static uint8_t mensagem_atual = 0;
    static uint32_t ultima_troca = 0;
    char buffer[30];
    
    // Limpa display
    ssd1306_fill(&display, false);

    // ----- CABEÇALHO -----
    // Seta esquerda
   
    escrever_linha("<", 0, 0, false);
    
    
    // Título + contador
    snprintf(buffer, sizeof(buffer), "%s %d/%d", 
           "FOLHA:", num, 5);
    escrever_linha(buffer, 0, 0, true); // Centralizado

    // Seta direita
    escrever_linha(">", 0, 15, false); // Coluna 15 (128 - 8px)
    

    // ----- MENSAGENS -----
    uint32_t agora = to_ms_since_boot(get_absolute_time());
    if((agora - ultima_troca > 3000)) {
        mensagem_atual = (mensagem_atual + 1) % 3;
        ultima_troca = agora;
    }
    
    switch (mensagem_atual)
    {
    case 0:
        escrever_linha("MOVA O", 3, 0, true);    
        escrever_linha("JOYSTICK < >", 4, 0, true);       
        break;
    case 1:
        escrever_linha("APERTE B", 3, 0, true);    
        escrever_linha("p/ ANALISAR", 4, 0, true);  
        break;
    case 2:
        escrever_linha("APERTE A", 3, 0, true);    
        escrever_linha("p/ VOLTAR", 4, 0, true);
        break;
    }
    ssd1306_send_data(&display);
}


// Função modificada para controle por etapas
void simular_escaneamento() {
    uint8_t etapa = 2; // 0 = R/NIR, 1 = G/B, 2 = não faz nenhuma leitura
    estado_escaneamento = MODO_ESCANEAMENTO;

    bool mudou_o_valor = true;

    printf("ESTADO B: %d\n", buttonB_flag);

    while (true){
        if(estado_escaneamento == MODO_ESCANEAMENTO){

            // Ajusta V e NIR
            if(etapa == 0){
                ler_joystick();

                valores_ajustados.R = vry_valor / (float)ADC_MAX;
                valores_ajustados.NIR = vrx_valor / (float)ADC_MAX;
                
                mudou_o_valor = true;
            }
            // Ajusta G e B
            else if(etapa == 1) {
                ler_joystick();

                valores_ajustados.G = vry_valor / (float)ADC_MAX;
                valores_ajustados.B = vrx_valor / (float)ADC_MAX;
                
                mudou_o_valor = true;
            }
            
            if(buttonB_flag) {
                etapa = (etapa + 1) % 3;
                buttonB_flag = false;
            }
            

            if(buttonA_flag) {
                estado_escaneamento = ANALISE;
                configurar_interrupcoes_botoes(false, false, false);
                buttonA_flag = false;
            }


            printf("R: %.2f\n",  valores_ajustados.R);
            printf("NIR: %.2f\n",  valores_ajustados.NIR);
            printf("G: %.2f\n",  valores_ajustados.G);
            printf("B: %.2f\n",  valores_ajustados.B);
            printf("ETAPA: %d\n", etapa);

            //atualiza matriz e display
            if(mudou_o_valor){
                exibir_grafico_display(valores_ajustados);
                exibir_grafico_matriz(valores_ajustados);
                mudou_o_valor = false;
            }

        }

        else if(estado_escaneamento == ANALISE){
                            
            bool resultado = detectar_doenca(valores_ajustados.R,
                                            valores_ajustados.G,
                                            valores_ajustados.B,
                                            valores_ajustados.NIR);
            animacao_analise(500);
            float ndvi = (valores_ajustados.NIR - valores_ajustados.R) / (valores_ajustados.NIR + valores_ajustados.R + 0.001f);
            float gndvi = (valores_ajustados.NIR - valores_ajustados.G) / (valores_ajustados.NIR + valores_ajustados.G + 0.001f);
            
            atualizar_led_status(resultado, false);
            exibir_resultado_analise(resultado,valores_ajustados.R,
                                    valores_ajustados.G,
                                    valores_ajustados.B,
                                    valores_ajustados.NIR,
                                    ndvi,
                                    gndvi
                                    );

            //teste_deteccao();
        
            mudou_o_valor = true;
            estado_escaneamento = MODO_ESCANEAMENTO;
            
            etapa = 2;
            configurar_interrupcoes_botoes(true, true, true);
            
        }

        if(buttonJoyStick_flag){
            npClear();
            npWrite();
            ssd1306_fill(&display, false);
            ssd1306_send_data(&display);
            buttonJoyStick_flag = false;
            configurar_interrupcoes_botoes(true, true, true);
            break;
        }
    }

}

void exibir_grafico_matriz(Reflectancia r){
   
   // Limpa todos os LEDs primeiro
   npClear();

   // Mapeamento das bandas espectrais para colunas
   const uint8_t colunas[5] = {0, 1, 2, 3, 4}; // R, G, B, NIR
   float valores[5] = {r.R, r.G, r.B, r.NIR, r.NIR};
   
   // Cores correspondentes para cada banda (R, G, B)
   const uint8_t cores[5][3] = {
       {255, 0, 0},    // Vermelho para R
       {0, 255, 0},    // Verde para G
       {0, 0, 255},    // Azul para B
       {255, 255, 255}, // Branco para NIR
       {255, 255, 255} // Branco para NIR
   };

   // Para cada banda espectral
   for(int banda = 0; banda < 5; banda++) {
       // Calcula a altura da coluna (0-5 LEDs)
       uint8_t altura = (uint8_t)(valores[banda] * 6);
       
       // Determina a coluna atual
       uint8_t x = colunas[banda];
       
       // Acende os LEDs de baixo para cima
       for(int i = 0; i < altura; i++) {
           // Calcula a posição Y (0 = base, 4 = topo)
           uint8_t y = i;
           
           // Obtém o índice correto na matriz
           int index = getIndex(x, y);
           
           // Define a cor do LED
           npSetLED(index, 
                  cores[banda][0],  // R
                  cores[banda][1],  // G 
                  cores[banda][2]); // B
       }
   }

   // Atualiza os LEDs físicos
   npWrite();
    
}

void exibir_grafico_display(Reflectancia r) {
    // Limpa a tela
    ssd1306_fill(&display, false);
    

    
    // Define a posição base para as barras (deixando espaço para o título)
    // Supondo que 'ssd.height' é a altura do display e 'TAMANHO_FONTE' é o tamanho da fonte (ex: 8)
    uint8_t y_base = display.height - MARGEM - TAMANHO_FONTE - 10;  // "10" é um deslocamento extra para separar a barra dos valores
    
    // Posições horizontais para as 4 bandas
    const uint8_t colunas[4] = {MARGEM, 34, 64, 94};
    const char* rotulos[4] = {"R", "G", "B", "NIR"};
    
    // Para cada banda, calcular e desenhar a barra, o valor (em porcentagem) e o rótulo
    for(uint8_t i = 0; i < 4; i++) {
        float valor;
        switch(i) {
            case 0: valor = r.R; break;
            case 1: valor = r.G; break;
            case 2: valor = r.B; break;
            case 3: valor = r.NIR; break;
        }
        
        // Converte o valor (0.0 a 1.0) para porcentagem (0 a 100)
        float porcentagem = (valor * 100);
        
        // Define a escala: 40 pixels corresponde a valor 1.0 (ou 100%)
        uint8_t escala = 40;
        uint8_t altura = (uint8_t)(valor * escala);
        if (altura > y_base) {
            altura = y_base;
        }
        // Calcula a posição vertical (topo) da barra
        uint8_t y_top = y_base - altura;
        
        // Desenha a barra de reflectância
        // Parâmetros: top = y_top, left = colunas[i], width = 20, height = altura
        ssd1306_rect(&display, y_top, colunas[i], 20, altura, true, true);
        
        // Prepara o valor numérico (em porcentagem)
        char buffer[8];
        snprintf(buffer, sizeof(buffer), "%.0f%%", porcentagem);
        
        // Exibe o valor numérico abaixo da barra
        ssd1306_draw_string(&display, buffer, colunas[i], y_base + 2);
        
        // Exibe o rótulo da banda ainda mais abaixo
        ssd1306_draw_string(&display, rotulos[i], colunas[i] + 6, y_base + TAMANHO_FONTE + 4);
    }
    
    // Envia os dados para o display
    ssd1306_send_data(&display);
}

void animacao_analise(int duracao_ms) {
    const uint8_t centro_x = 64;
    const uint8_t centro_y = 32;
    const int total_passos = 50; // Número de quadros da animação
    const int delay_por_passo = (duracao_ms) / total_passos;
    
    buzzer_som_analise_iniciada();
    sleep_ms(100);
    buzzer_turn_off();
    sleep_ms(100);
    buzzer_som_analise_iniciada();    
    sleep_ms(100);
    buzzer_turn_off();

    // Animação de carregamento
    for (int i = 0; i <= total_passos; i++) {
        ssd1306_fill(&display, false);
        
        // Exibe o texto "Analisando" acima da barra
        ssd1306_draw_string(&display, "ANALISANDO", 24, 20);
        
        // Barra de progresso horizontal: largura máxima agora é 100 pixels
        uint8_t largura = (i * 100) / total_passos;
        // Desenha a barra na posição: top = 20, left = 55, com altura de 6 pixels
        ssd1306_rect(&display, 35, 14, largura, 6, true, true);

        
        ssd1306_send_data(&display);
        sleep_ms(delay_por_passo);
    }

    buzzer_som_analise_concluida();
    sleep_ms(100);
    buzzer_turn_off();
    sleep_ms(100);
    buzzer_som_analise_concluida();    
    sleep_ms(100);
    buzzer_turn_off();
}

void exibir_resultado_analise_folha(EstadoFolha folha){
    float R = folha.reflectancia.R;
    float G = folha.reflectancia.G;
    float B = folha.reflectancia.B;
    float NIR = folha.reflectancia.NIR;
    float ndvi =  folha.ndvi;
    float gndvi = folha.gndvi;
    bool resultado = detectar_doenca(R, G, B, NIR);
    exibir_resultado_analise(resultado, R, G, B, NIR, ndvi, gndvi);
}
void exibir_resultado_analise(bool resultado, float R , float G, float B, float NIR, float ndvi, float gndvi ){
    char buffer[24];
    ssd1306_fill(&display, false);

    // Linha 1 - Status principal centralizado
    snprintf(buffer, sizeof(buffer), "%s", resultado ? "INFECTADA" : "SAUDAVEL");
    escrever_linha(buffer, 0, 0, true); // Centralizado

    // Linha 2 - Reflectância RGB
    snprintf(buffer, sizeof(buffer), "R:%.0f%% G:%.0f%%", 
             R * 100, 
             G * 100);
    escrever_linha(buffer, 2, 0, false);

    // Linha 3 - Reflectância B e NIR
    snprintf(buffer, sizeof(buffer), "B:%.0f%% NIR:%.0f%%", 
             B * 100, 
             NIR * 100);
    escrever_linha(buffer, 3, 0, false);

    // Linha 4 - Índices NDVI e GNDVI
    snprintf(buffer, sizeof(buffer), "NDVI:%.2f", 
             ndvi);
    escrever_linha(buffer, 4, 0, false);

    // Linha 4 - Índices GNDVI
    snprintf(buffer, sizeof(buffer), "GNDVI:%.2f",  
             gndvi);
    escrever_linha(buffer, 5, 0, false);

    
    ssd1306_send_data(&display);

    configurar_interrupcoes_botoes(true, false, false);

    // Espera pelo botão A
    while(!buttonA_flag) {
        sleep_ms(10);
    }
    buttonA_flag = false;
}


bool detectar_doenca_folha(EstadoFolha folha){
    float R = folha.reflectancia.R;
    float G = folha.reflectancia.G;
    float B = folha.reflectancia.B;
    float NIR = folha.reflectancia.NIR;
    return detectar_doenca(R, G, B, NIR);
}
bool detectar_doenca(float R, float G, float B, float NIR) {
    // 1. Cálculo dos índices
    float ndvi = (NIR - R) / (NIR + R + 0.001f);
    float gndvi = (NIR - G) / (NIR + G + 0.001f);
    
    // 2. Limiares científicos
    const float NDVI_SAUDAVEL = 0.4f;
    const float GNDVI_SAUDAVEL = 0.35f;
    const float ERRO = 0.2f;

    // 3. Critérios de detecção
    bool criterio_ndvi = ndvi < NDVI_SAUDAVEL;
    bool criterio_gndvi = gndvi < GNDVI_SAUDAVEL;
    bool criterio_visivel = (R > 0.65f) && (G < 0.55f);

    // 4. Lógica de decisão
    bool resultado = (criterio_ndvi && criterio_gndvi) || criterio_visivel;

    /* 5. Simulação de erro
    if((float)rand()/RAND_MAX < ERRO) {
        return !resultado;
    }
    */
    return resultado;
}

/*
void teste_deteccao() {

    bool resultado;

    // Teste 1 (Verdadeiro)
    resultado = detectar_doenca(0.70f, 0.50f, 0.30f, 0.60f);
    exibir_resultado_analise(resultado);
    sleep_ms(500);

    // Teste 2 (Falso)
    resultado = detectar_doenca(0.40f, 0.70f, 0.50f, 0.95f);
    exibir_resultado_analise(resultado);
    sleep_ms(500);

    // Teste 3 (Verdadeiro)
    resultado = detectar_doenca(0.68f, 0.40f, 0.25f, 0.55f);
    exibir_resultado_analise(resultado);
    sleep_ms(500);

    // Teste 4 (Falso)
    resultado = detectar_doenca(0.50f, 0.60f, 0.40f, 1.20f);
    exibir_resultado_analise(resultado);
    sleep_ms(500);

    // Teste 5 (Verdadeiro)
    resultado = detectar_doenca(0.80f, 0.45f, 0.20f, 0.65f);
    exibir_resultado_analise(resultado);
    sleep_ms(500);

    // Teste 6 (Falso)
    resultado = detectar_doenca(0.30f, 0.70f, 0.50f, 0.85f);
    exibir_resultado_analise(resultado);
    sleep_ms(500);

    // Teste 7 (Verdadeiro)
    resultado = detectar_doenca(0.60f, 0.30f, 0.35f, 0.55f);
    exibir_resultado_analise(resultado);
    sleep_ms(500);

    // Teste 8 (Falso)
    resultado = detectar_doenca(0.40f, 0.58f, 0.35f, 1.0f);
    exibir_resultado_analise(resultado);
    sleep_ms(500);

    // Teste 9 (Verdadeiro)
    resultado = detectar_doenca(0.72f, 0.50f, 0.40f, 0.58f);
    exibir_resultado_analise(resultado);
    sleep_ms(500);

    // Teste 10 (Falso)
    resultado = detectar_doenca(0.35f, 0.50f, 0.40f, 1.0f);
    exibir_resultado_analise(resultado);
    sleep_ms(500);


}
*/

void display_init(ssd1306_t *display) {
    // Configuração I2C a 400kHz
    i2c_init(I2C_PORT, 400 * 1000);
    
    // Configura pinos I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA); // Ativa pull-ups internos
    gpio_pull_up(I2C_SCL);

    // Inicialização do controlador SSD1306
    ssd1306_init(display, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(display);
    ssd1306_send_data(display);

    // Limpa a tela inicialmente
    ssd1306_fill(display, false);
    ssd1306_send_data(display);
}