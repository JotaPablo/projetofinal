# CaféScan - Sistema de Diagnóstico de Ferrugem em Cafeeiros

Sistema portátil para detecção precoce da ferrugem do cafeeiro utilizando análise espectral e índices de vegetação (NDVI/GNDVI). Desenvolvido para a plataforma Raspberry Pi Pico (RP2040) com integração de múltiplos periféricos.

## Vídeo Demonstrativo
[![Assista ao vídeo no YouTube](https://img.youtube.com/vi/NvU5fByL1ds/hqdefault.jpg)](https://youtu.be/NvU5fByL1ds)

## Arquitetura do Sistema

- **Máquina de Estados Finitos (FSM):** Gerencia os modos de operação:
  - Menu Principal
  - Seleção de Folhas
  - Análise Espectral
  - Modo Calibração

- **Sensoriamento:**
  - Joystick (simulação de sensor espectral)
  - Botões para controle de interface

- **Saídas:**
  - Display OLED 128x64 (I2C)
  - Matriz de LEDs 5x5 (Neopixel)
  - Buzzer para feedback sonoro

- **Processamento:**
  - Cálculo de índices NDVI/GNDVI em tempo real
  - Algoritmo de detecção com múltiplos critérios

## Funcionalidades Principais

### Modo Calibração
- Ajuste manual dos valores de reflectância (R, G, B, NIR)
- Visualização em tempo real nos gráficos (OLED e LED Matrix)

### Análise de Plantas
- Sistema com 5 plantas virtuais pré-configuradas
- Detecção em três níveis:
  1. Saudável
  2. Infectada (sintomas visíveis)
  3. Infectada (estágio inicial)

### Interface Visual
- Representação gráfica das plantas na LED Matrix
- Gráficos de barras das reflectâncias no OLED
- Menu interativo com navegação por joystick

### Feedback Multissensorial
- Tons distintos para:
  - Seleção de opções
  - Resultado positivo/negativo

## Componentes Utilizados

| Componente          | GPIO/Pinos      | Função                          |
|---------------------|-----------------|---------------------------------|
| Display OLED SSD1306| GPIO 14 (SDA) e GPIO 15 (SCL)  | Exibição de dados espectrais e interface de usuário    |
| LED Matrix 5x5      | GPIO 28         | Representação visual das plantas e exibição de dados|
| Buzzer              | GPIO 21         | Feedback sonoro                 |
| Joystick            | GPIO 26 (VRY) e GPIO 27 (VRX)  | Controle de navegação e calibração |
| Botão A             | GPIO 5          | Confirmação/Ações               |
| Botão B             | GPIO 6          | Navegação/Seleção               |
| Botão Joystick      | GPIO 22         | Mudança de Estado               |

- **Periféricos Adicionais:**
  - PWM para controle do Buzzer
  - ADC para leitura dos valores do joystick

## ⚙️ Instalação e Uso

1. **Pré-requisitos**
   - Clonar o repositório:
     ```bash
     git clone https://github.com/JotaPablo/projetofinal.git
     cd projetofinal
     ```
   - Instalar o **Visual Studio Code** com as seguintes extensões:
     - **Raspberry Pi Pico SDK**
     - **Compilador ARM GCC**

2. **Compilação**
   - Compile o projeto no terminal:
     ```bash
     mkdir build
     cd build
     cmake ..
     make
     ```
   - Ou utilize a extensão da Raspberry Pi Pico no VS Code.

3. **Execução**
   - **Na placa física:** 
     - Conecte a placa ao computador em modo **BOOTSEL**.
     - Copie o arquivo `.uf2` gerado na pasta `build` para o dispositivo identificado como `RPI-RP2`, ou envie através da extensão da Raspberry Pi Pico no VS Code.
