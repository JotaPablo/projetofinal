/**********************************
* INCLUDES E DEFINES
**********************************/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "lib/ssd1306.h"
#include "lib/neopixel.h"
#include "lib/buzzer.h"
#include "utils/hardware_config.h"

// Configurações de display
#define TAMANHO_FONTE 8       // Tamanho da fonte em pixels
#define MARGEM 4              // Margem entre elementos
#define TEMPO_TROCA_MENSAGEM 3000 // Tempo de rotação das mensagens

// Configurações do jogo
#define NUM_PLANTAS 5          // Número total de plantas
#define FOLHAS_POR_PLANTA 5    // Folhas por planta
#define CUSTO_POR_FUNGICIDA 10 // Custo em recursos do tratamento

/**********************************
* TIPOS DE DADOS
**********************************/
// Estrutura para armazenar reflectâncias

typedef struct {
    float R;    // Reflectância no vermelho
    float G;    // Reflectância no verde
    float B;    // Reflectância no azul
    float NIR;  // Reflectância no infravermelho
} Reflectancia;

// Estados do sistema de escaneamento
typedef enum {
    MODO_ESCANEAMENTO,
    ANALISE
} EstadoEscaneamento;

// Estados principais da máquina de estados
typedef enum {
    ESTADO_MENU,
    ESTADO_SELECIONAR_FOLHA,
    ESTADO_ANALISAR,
    ESTADO_ESCANEAMENTO,
} Estado;

// Estado de uma folha individual
typedef struct {
    Reflectancia reflectancia; // Valores espectrais
    float ndvi, gndvi;         // Índices de vegetação
    bool visivel;              // Infecção visível a olho nu
    bool infectada;            // Status de infecção
} EstadoFolha;

// Estrutura completa de uma planta
typedef struct {
    int id;                    // Identificador único
    EstadoFolha folhas[FOLHAS_POR_PLANTA]; // Array de folhas
    bool infectada;            // Status geral de infecção
    bool tratada;              // Status de tratamento
} Planta;

/**********************************
* VARIÁVEIS GLOBAIS
**********************************/
// Matriz de representação gráfica da planta (5x5)
int planta_cafe[5][5] = {
    {1, 20, 20, 0, 0},  
    {10, 0, 20, 0, 0},   
    {0, 0, 20, 2, 3},    
    {5, 4, 20, 0, 10},   
    {10, 0, 20, 0, 0}    
};

// Controle do display OLED
ssd1306_t display;

// Estado do sistema de escaneamento
EstadoEscaneamento estado_escaneamento = MODO_ESCANEAMENTO;

// Valores ajustados do joystick para calibração
Reflectancia valores_ajustados;

/**********************************
* PROTÓTIPOS DE FUNÇÕES
**********************************/
// Inicialização
void display_init(ssd1306_t *display);

// Interface gráfica
void escrever_linha(const char* texto, int linha, int coluna, bool centralizado);
void exibir_menu_planta(int atual, int custo);
void exibir_menu_folha(int num);
void exibe_planta(Planta p, int folha);
void exibir_grafico_display(Reflectancia r);
void exibir_grafico_matriz(Reflectancia r);
void animacao_analise(int duracao_ms);
void exibir_resultado_analise(bool resultado, float R, float G, float B, float NIR, float ndvi, float gndvi);
void exibir_resultado_analise_folha(EstadoFolha folha);

// Lógica do programa
Planta gerar_planta(int id, int tipo);
void tratar_planta(Planta *p);
bool detectar_doenca(float R, float G, float B, float NIR);
bool detectar_doenca_folha(EstadoFolha folha);

// Controles
void gerenciar_menu_principal(int *planta_atual, bool *atualiza_display);
void gerenciar_selecao_folha(int *folha_atual, bool *atualiza_display);
void simular_escaneamento();

/**********************************
* FUNÇÃO PRINCIPAL
**********************************/
int main() {
    //==================================================
    // INICIALIZAÇÃO DO SISTEMA
    //==================================================
    hardware_setup();          // Configura hardware (GPIO, ADC, etc)
    display_init(&display);    // Inicializa display OLED

    Planta plantas[NUM_PLANTAS]; // Array de plantas do sistema
    Estado estado_atual = ESTADO_ESCANEAMENTO; // Estado inicial da máquina de estados
    
    // Variáveis de controle da interface
    int indice_planta = 0;      // Planta selecionada no menu
    int indice_folha = 0;       // Folha selecionada na análise
    int custo_total = 0;        // Custo acumulado em tratamentos
    bool atualizar_display = true; // Flag para atualização do display
    
    // Timers para atualizações periódicas
    uint32_t ultima_atualizacao_menu = 0;
    uint32_t ultima_atualizacao_folha = 0;

    //==================================================
    // CONFIGURAÇÃO INICIAL DAS PLANTAS
    //==================================================
    plantas[0] = gerar_planta(0, 0); // Saudável
    plantas[1] = gerar_planta(1, 0); // Saudável
    plantas[2] = gerar_planta(2, 1); // Infectada visível
    plantas[3] = gerar_planta(3, 2); // Infectada oculta
    plantas[4] = gerar_planta(4, 2); // Infectada oculta

    //==================================================
    // LOOP PRINCIPAL DO SISTEMA
    //==================================================
    while(true) {
        //--------------------------------------------------
        // ATUALIZAÇÃO DE ENTRADAS E TEMPO
        //--------------------------------------------------
        ler_joystick();                   // Lê valores do joystick
        normalizar_joystick();            // Aplica deadzone e normaliza
        uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());
        buzzer_update();                  // Atualiza estado do buzzer

        //--------------------------------------------------
        // MÁQUINA DE ESTADOS PRINCIPAL
        //--------------------------------------------------
        switch(estado_atual) {
            //==============================================
            // ESTADO: MENU PRINCIPAL
            //==============================================
            case ESTADO_MENU:
                //---------- Controle de Navegação ---------
                gerenciar_menu_principal(&indice_planta, &atualizar_display);

                //---------- Tratamento do Botão A ---------
                if(buttonA_flag) {
                    configurar_interrupcoes_botoes(false, false, false);
                    ssd1306_fill(&display, false);
                    
                    // Planta já tratada
                    if(plantas[indice_planta].tratada) {
                        // Feedback sonoro
                        buzzer_som_analise_concluida();
                        sleep_ms(100);
                        buzzer_turn_off();
                        sleep_ms(100);
                        buzzer_som_analise_concluida();    
                        sleep_ms(100);
                        buzzer_turn_off();
                        
                        // Mensagem de status
                        escrever_linha("TRATAMENTO JA", 2, 0, true);
                        escrever_linha("REALIZADO", 3, 0, true);
                        sleep_ms(1000);
                    }
                    // Planta não tratada
                    else {
                        // Animação de tratamento
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
                        
                        // Atualiza custos
                        custo_total += CUSTO_POR_FUNGICIDA;

                        // Trata a planta
                        tratar_planta(&plantas[indice_planta]);

                    }
                    
                    configurar_interrupcoes_botoes(true, true, true);
                    buttonA_flag = false;
                    atualizar_display = true;
                }

                //---------- Tratamento do Botão B ---------
                if(buttonB_flag) {
                    estado_atual = ESTADO_SELECIONAR_FOLHA;
                    indice_folha = 0;
                    configurar_interrupcoes_botoes(true, true, false);
                    atualizar_display = true;
                    buttonB_flag = false;
                    buzzer_som_selecao();
                    sleep_ms(100);
                    continue;
                }

                //---------- Tratamento do Botão Joystick ---
                if(buttonJoyStick_flag) {
                    estado_atual = ESTADO_ESCANEAMENTO;
                    buttonJoyStick_flag = false;
                    buzzer_som_selecao();
                }

                //---------- Atualização de Display ---------
                if(atualizar_display || (tempo_atual - ultima_atualizacao_menu >= TEMPO_TROCA_MENSAGEM)) {
                    exibe_planta(plantas[indice_planta], -1);
                    exibir_menu_planta(indice_planta + 1, custo_total);
                    atualizar_led_status(plantas[indice_planta].infectada, false);
                    atualizar_display = false;
                    ultima_atualizacao_menu = tempo_atual;
                }
                break;

            //==============================================
            // ESTADO: SELEÇÃO DE FOLHA
            //==============================================
            case ESTADO_SELECIONAR_FOLHA:
                //---------- Controle de Navegação ---------
                gerenciar_selecao_folha(&indice_folha, &atualizar_display);
                
                //---------- Tratamento do Botão B (Analisar) ---------
                if(buttonB_flag) {
                    estado_atual = ESTADO_ANALISAR;
                    buttonB_flag = false;
                }
                
                //---------- Tratamento do Botão A (Voltar) ---------
                if(buttonA_flag) {
                    estado_atual = ESTADO_MENU;
                    configurar_interrupcoes_botoes(true, true, true);
                    buttonA_flag = false;
                    atualizar_display = true;
                    buzzer_som_selecao();
                    sleep_ms(100);
                    continue;
                }
                
                //---------- Atualização de Display ---------
                if(atualizar_display || (tempo_atual - ultima_atualizacao_folha >= TEMPO_TROCA_MENSAGEM)) {
                    exibe_planta(plantas[indice_planta], indice_folha + 1);
                    exibir_menu_folha(indice_folha + 1);
                    atualizar_led_status(plantas[indice_planta].infectada, false);
                    atualizar_display = false;
                    ultima_atualizacao_folha = tempo_atual;
                }
                break;

            //==============================================
            // ESTADO: ANÁLISE DE FOLHA
            //==============================================
            case ESTADO_ANALISAR:
                //---------- Preparação para Análise ---------
                configurar_interrupcoes_botoes(false, false, false); // Desativa interrupções
                
                //---------- Processo de Análise ---------
                animacao_analise(500); // Animação de carregamento
                bool resultado = detectar_doenca_folha(plantas[indice_planta].folhas[indice_folha]);
                
                //---------- Atualização de Estado ---------
                if(resultado) {
                    plantas[indice_planta].infectada = true; // Marca planta como infectada
                }
                sleep_ms(200);

                //---------- Feedback Visual/Sonoro ---------
                atualizar_led_status(resultado, false); // Atualiza LEDs
                if(resultado)
                    buzzer_infectada();
                else
                    buzzer_saudavel();
                exibir_resultado_analise_folha(plantas[indice_planta].folhas[indice_folha]); // Mostra resultados
                
                //---------- Retorno ao Estado Anterior ---------
                configurar_interrupcoes_botoes(true, true, false); // Reativa interrupções
                buzzer_som_selecao();
                estado_atual = ESTADO_SELECIONAR_FOLHA;
                atualizar_display = true;
                break;

            //==============================================
            // ESTADO: MODO ESCANEAMENTO
            //==============================================
            case ESTADO_ESCANEAMENTO:
                //---------- Configuração Inicial ---------
                atualizar_led_status(false, true); // Desliga LEDs indicativos
                
                //---------- Execução do Escaneamento ---------
                simular_escaneamento(); // Processo completo de calibração
                
                //---------- Retorno ao Menu ---------
                estado_atual = ESTADO_MENU;
                atualizar_display = true;
                buzzer_som_selecao();
                break;
        }

        sleep_ms(100); 
    }
}


/**********************************
* INICIALIZAÇÃO DO DISPLAY
**********************************/

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


/**********************************
* IMPLEMENTAÇÃO DAS FUNÇÕES DE CONTROLE
**********************************/

/*
* Gerencia a navegação no menu principal de plantas
* @param planta_atual Ponteiro para o índice da planta selecionada
* @param atualiza_display Ponteiro para flag de atualização do display
*/

void gerenciar_menu_principal(int *planta_atual, bool *atualiza_display) {
    if(vrx_valor == DIREITA) {
        *planta_atual = (*planta_atual + 1) % NUM_PLANTAS;  // Avança para próxima planta
        *atualiza_display = true;                           // Força atualização
    }
    else if(vrx_valor == ESQUERDA) {
        *planta_atual = (*planta_atual - 1 + NUM_PLANTAS) % NUM_PLANTAS; // Volta para planta anterior
        *atualiza_display = true;                                        // Força atualização
    }
}

 /*
* Gerencia a navegação na seleção de folhas
* @param folha_atual Ponteiro para o índice da folha selecionada
* @param atualiza_display Ponteiro para flag de atualização do display
*/
void gerenciar_selecao_folha(int *folha_atual, bool *atualiza_display) {
    
    if(vrx_valor == DIREITA) {
        *folha_atual = (*folha_atual + 1) % FOLHAS_POR_PLANTA;  // Avança para próxima folha
        *atualiza_display = true;                               // Força atualização
    }
    else if(vrx_valor == ESQUERDA) {
        *folha_atual = (*folha_atual - 1 + FOLHAS_POR_PLANTA) % FOLHAS_POR_PLANTA; // Volta para folha anterior
        *atualiza_display = true;                                                  // Força atualização
    }
}

/**********************************
* IMPLEMENTAÇÃO DA LÓGICA DAS PLANTAS
**********************************/
/*
* Gera uma nova planta com características específicas
* @param id Identificador único da planta
* @param tipo Tipo de planta (0-Saudável, 1-Infectada visível, 2-Infectada oculta)
* @return Estrutura Planta configurada
*/
Planta gerar_planta(int id, int tipo) {
    Planta p;
    p.id = id;
    p.infectada = false;
    p.tratada = false;

    for (int i = 0; i < FOLHAS_POR_PLANTA; i++) {
        float R, G, B, NIR;
        bool doente = false;
        bool visivel_doente = false;

        // Configuração baseada no tipo de planta
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
                visivel_doente = true; // Marca infecção visível
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

        // Configuração da folha
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



 /*
* Aplica tratamento fungicida na planta
* @param p Ponteiro para a planta a ser tratada
*/
void tratar_planta(Planta *p) {
    for (int i = 0; i < FOLHAS_POR_PLANTA; i++) {
        // Define valores saudáveis
        p->folhas[i] = (EstadoFolha) {
            .reflectancia = {0.40f, 0.70f, 0.50f, 0.95f}, // Valores de referência saudáveis
            .ndvi = (0.95f - 0.40f) / (0.95f + 0.40f + 0.001f),
            .gndvi = (0.95f - 0.70f) / (0.95f + 0.70f + 0.001f),
            .visivel = false,
            .infectada = false
        };
    }
    p->infectada = false;
    p->tratada = true;
}

/**********************************
* IMPLEMENTAÇÃO DA INTERFACE GRÁFICA
**********************************/

/*
* Renderiza a representação visual da planta na matriz de LEDs
* @param p Planta a ser exibida
* @param folha Folha selecionada (-1 para nenhuma)
*/


void exibe_planta(Planta p, int folha) {
    // Cores pré-definidas (const para otimização)
    static const uint8_t MARROM_VINHO[3] = {5, 0, 0};
    static const uint8_t VERDE_FRACO[3]  = {0, 100, 0};
    static const uint8_t VERDE_FORTE[3]  = {0, 5, 0};
    static const uint8_t LARANJA_FRACO[3] = {100, 100, 0};
    static const uint8_t LARANJA_FORTE[3] = {5, 5, 0};

    for(int y = 0; y < 5; y++) {      // Linhas lógicas (0-4)
        for(int x = 0; x < 5; x++) {  // Colunas lógicas (0-4)
            int valor = planta_cafe[y][x]; // Observacao: invertemos x e y aqui
            int index = getIndex(x, 4 - y); // Inverte a ordem das linhas

            if(valor >= 1 && valor <= FOLHAS_POR_PLANTA) { 
                int folha_atual = valor;
                
                // Seleção de cores baseada no estado

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

 /*
* Escreve texto no display OLED com posicionamento controlado
* @param texto String a ser exibida
* @param linha Linha vertical (0-7)
* @param coluna Coluna horizontal (0-15)
* @param centralizado Centraliza o texto horizontalmente
*/
void escrever_linha(const char* texto, int linha, int coluna, bool centralizado) {
    int pos_x = coluna * TAMANHO_FONTE;
    int pos_y = linha * 10; // 10px por linha
    
    if(centralizado) {
        int len = strlen(texto);
        pos_x = (128 - (len * TAMANHO_FONTE)) / 2; // Cálculo do centro
    }
    
    ssd1306_draw_string(&display, texto, pos_x, pos_y);
    ssd1306_send_data(&display);
}

/**********************************
* IMPLEMENTAÇÃO DOS MENUS
**********************************/
 /*
* Exibe o menu principal de seleção de plantas
* @param atual Índice da planta atual
* @param custo Custo acumulado de tratamentos
*/
void exibir_menu_planta(int atual, int custo) {
    static uint8_t mensagem_atual = 0;
    static uint32_t ultima_troca = 0;
    char buffer[30];
    
    // Limpa display
    ssd1306_fill(&display, false);

    // Cabeçalho fixo
    escrever_linha("<", 0, 0, false);
    snprintf(buffer, sizeof(buffer), "%s %d/%d", 
           "PLANTA:", atual, 5);
    escrever_linha(buffer, 0, 0, true); // Centralizado
    escrever_linha(">", 0, 15, false); 
    
    // Custo
    snprintf(buffer, sizeof(buffer), "%s %d.00", 
           "CUSTO:", custo);
    escrever_linha(buffer, 1, 0, true); // Centralizado

    // Mensagens rotativas
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

 /*
* Exibe o menu principal de seleção de folhas
* @param atual Índice da folha atual
*/
void exibir_menu_folha(int atual) {
    static uint8_t mensagem_atual = 0;
    static uint32_t ultima_troca = 0;
    char buffer[30];
    
    // Limpa display
    ssd1306_fill(&display, false);

    // Cabeçalho fixo
    escrever_linha("<", 0, 0, false);
    snprintf(buffer, sizeof(buffer), "%s %d/%d", 
           "FOLHA:", atual, 5);
    escrever_linha(buffer, 0, 0, true);
    escrever_linha(">", 0, 15, false); 
    

    // Mensagens rotativas
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

/**********************************
* IMPLEMENTAÇÃO DO SISTEMA DE ESCANEAMENTO
**********************************/

  /*
* Controla o processo completo de calibração e escaneamento
* Estados:
* - Etapa 0: Ajuste de Vermelho (R) e Infravermelho (NIR)
* - Etapa 1: Ajuste de Verde (G) e Azul (B)
* - Etapa 2: Modo inativo
*/
void simular_escaneamento() {
    uint8_t etapa_calibracao = 2; // Começa desativado
    estado_escaneamento = MODO_ESCANEAMENTO;

    bool atualizar_interface = true;

    while (true){
        // Modo de ajuste de calibração
        if(estado_escaneamento == MODO_ESCANEAMENTO){

            ler_joystick();
            buzzer_update();                  // Atualiza estado do buzzer

            // Calibração de R e NIR            
            if(etapa_calibracao == 0){
                valores_ajustados.R = vry_valor / (float)ADC_MAX;
                valores_ajustados.NIR = vrx_valor / (float)ADC_MAX;
                atualizar_interface = true;
            }
            // Calibração de G e B
            else if(etapa_calibracao == 1) {
                valores_ajustados.G = vry_valor / (float)ADC_MAX;
                valores_ajustados.B = vrx_valor / (float)ADC_MAX;
                atualizar_interface = true;
            }
            
            // Controle de etapas
            if(buttonB_flag) {
                etapa_calibracao = (etapa_calibracao + 1) % 3;
                buttonB_flag = false;
                buzzer_som_selecao();
            }
            
            // Iniciar análise
            if(buttonA_flag) {
                estado_escaneamento = ANALISE;
                configurar_interrupcoes_botoes(false, false, false);
                buttonA_flag = false;
            }

            // Atualização em tempo real
            if(atualizar_interface){
                exibir_grafico_display(valores_ajustados);
                exibir_grafico_matriz(valores_ajustados);
                atualizar_interface = false;
            }

        }
        // Processamento da análise
        else if(estado_escaneamento == ANALISE){
                            
            bool resultado = detectar_doenca(valores_ajustados.R,
                                            valores_ajustados.G,
                                            valores_ajustados.B,
                                            valores_ajustados.NIR);
            
            // Cálculo de índices
            float ndvi = (valores_ajustados.NIR - valores_ajustados.R) / (valores_ajustados.NIR + valores_ajustados.R + 0.001f);
            float gndvi = (valores_ajustados.NIR - valores_ajustados.G) / (valores_ajustados.NIR + valores_ajustados.G + 0.001f);
            
            // Feedback ao usuário
            animacao_analise(500);
            sleep_ms(200);
            atualizar_led_status(resultado, false);
            if(resultado)
                buzzer_infectada();
            else
                buzzer_saudavel();
            exibir_resultado_analise(resultado,valores_ajustados.R,
                                    valores_ajustados.G,
                                    valores_ajustados.B,
                                    valores_ajustados.NIR,
                                    ndvi,
                                    gndvi
                                    );
            buzzer_som_selecao();
            atualizar_led_status(false, true);

            // Reset do sistema 
            atualizar_interface = true;
            estado_escaneamento = MODO_ESCANEAMENTO;
            etapa_calibracao = 2;
            configurar_interrupcoes_botoes(true, true, true);
            
        }

        // Saída do modo escaneamento
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

/**********************************
* IMPLEMENTAÇÃO DA VISUALIZAÇÃO DE DADOS
**********************************/

 /*
* Exibe os valores de reflectância na matriz de LEDs
* @param r Valores de reflectância a serem visualizados
* Layout:
* - Coluna 0: Vermelho (R)
* - Coluna 1: Verde (G)
* - Coluna 2: Azul (B)
* - Colunas 3-4: Infravermelho (NIR)
*/

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

/*
* Exibe gráfico de barras com valores de reflectância no display OLED
* @param r Valores de reflectância a serem plotados
* Cada banda ocupa 34 pixels de largura com 20px de área útil
*/

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

/**********************************
* IMPLEMENTAÇÃO DE ANIMAÇÕES E RESULTADOS
**********************************/

 /*
* Exibe animação de progresso durante a análise
* @param duracao_ms Tempo total da animação
*/
void animacao_analise(int duracao_ms) {
    const int total_passos = 50; // Número de quadros da animação
    const int delay_por_passo = (duracao_ms) / total_passos;
    
    // Feedback sonoro inicial
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

    // Feedback sonoro final
    buzzer_som_analise_concluida();
    sleep_ms(100);
    buzzer_turn_off();
    sleep_ms(100);
    buzzer_som_analise_concluida();    
    sleep_ms(100);
    buzzer_turn_off();
}


/*
* Exibe tela detalhada com resultados da análise
* @param resultado Diagnóstico final
* @param R,G,B,NIR Valores de reflectância
* @param ndvi,gndvi Índices calculados
*/
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

    // Espera confirmação do usuário
    while(!buttonA_flag) 
        sleep_ms(10);
    
    buttonA_flag = false;
}

/*
* Exibe tela detalhada com resultados da análise de uma folha
*/

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

/**********************************
* IMPLEMENTAÇÃO DA LÓGICA DE DETECÇÃO
**********************************/

/*
* Implementa o algoritmo principal de detecção de doenças
* @param R,G,B,NIR Valores de reflectância
* @return true se detectada anomalia
* 
* Critérios:
* 1. NDVI < 0.4
* 2. GNDVI < 0.35
* 3. Vermelho alto (R > 65%) com Verde baixo (G < 55%)
*/
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

bool detectar_doenca_folha(EstadoFolha folha){
    float R = folha.reflectancia.R;
    float G = folha.reflectancia.G;
    float B = folha.reflectancia.B;
    float NIR = folha.reflectancia.NIR;
    return detectar_doenca(R, G, B, NIR);
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
