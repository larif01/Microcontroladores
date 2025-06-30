#include "main.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define DIFICULDADEMINIMA 14999
#define DIFICULDADEMEDIA 9999
#define DIFICULDADEMAXIMA 4999

TIM_HandleTypeDef htim2;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

void SystemClock_Config(void);
static void MX_GPIO_Display_Init(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);

//Endereços 0-4 botões e LEDs individuais
bool flagsBotoes[5] = {false};
bool flagsLEDs[5] = {false};
uint8_t pontuacao = 0;
volatile bool flagResetPorTimeout = false;
volatile bool flagMudaLED = false;
uint8_t totalErros = 0;
uint32_t tempoInicioJogo = 0;
bool aguardandoSoltarBotoes = false;


bool lerBotaoDebounce(GPIO_TypeDef* porta, uint16_t pino)
{
    if (HAL_GPIO_ReadPin(porta, pino) == GPIO_PIN_RESET)
    {
        HAL_Delay(20);
        if (HAL_GPIO_ReadPin(porta, pino) == GPIO_PIN_RESET)
        {
            return true;
        }
    }
    return false;
}

void novoLED()
{
	static int ultimoLED = -1;
    flagsLEDs[ultimoLED] = false;
    int aux;
    do {
    	aux = rand() % 5;
    } while (aux == ultimoLED);
    ultimoLED = aux;

    flagsLEDs[ultimoLED] = true;

    //Liga LED do botão 1
    if (flagsLEDs[0] == true) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    //Liga LED do botão 2
    if (flagsLEDs[1] == true) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    //Liga LED do botão 3
    if (flagsLEDs[2] == true) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
    //Liga LED do botão 4
    if (flagsLEDs[3] == true) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
    //Liga LED do botão 5
    if (flagsLEDs[4] == true) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
}

void send_byte(uint8_t data)
{
    for (int i = 7; i >= 0; i--) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, (data >> i) & 0x01);  // SDI
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);        // SCLK HIGH
    	HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);      // SCLK LOW
    }
}

void latch()
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);    // LOAD HIGH
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);  // LOAD LOW

}

void display_write(uint8_t num)
{
	//Tabela de segmentos
	const uint8_t digits[10] = {
	    ~0b00111111, // 0
	    ~0b00000110, // 1
	    ~0b01011011, // 2
	    ~0b01001111, // 3
	    ~0b01100110, // 4
	    ~0b01101101, // 5
	    ~0b01111101, // 6
	    ~0b00000111, // 7
	    ~0b01111111, // 8
	    ~0b01101111  // 9
	};

    uint8_t digit1 = num % 10;       // dígito da direita
    uint8_t digit2 = num / 10;       // dígito da esquerda

    send_byte(digits[digit1]);     // primeiro o dígito da direita
    send_byte(digits[digit2]);     // depois o da esquerda
    latch();
}

void display_clear()
{
    send_byte(0xFF); // Apaga o dígito da direita
    send_byte(0xFF); // Apaga o dígito da esquerda
    latch();
}

void resetPontos()
{
    pontuacao = 0;
    totalErros = 0;
    tempoInicioJogo = HAL_GetTick();
    display_write(0);
}

void fimDeJogo()
{
    if (pontuacao >= 30) {
        uint32_t tempoTotal = HAL_GetTick() - tempoInicioJogo;
        char msg[100];
        snprintf(msg, sizeof(msg),
                 "Jogo completo!\r\nTempo total: %lu ms\r\nErros: %d\r\n",
                 tempoTotal, totalErros);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    }

    for(int i = 0; i < 3; i++)
    {
        display_clear();
        HAL_Delay(500);
        display_write(pontuacao);
        HAL_Delay(500);
    }
    resetPontos();  // Reinicia os pontos
}

void ajustarDificuldade()
{
    uint32_t novoPeriodo;
    if (pontuacao < 10)
        novoPeriodo = DIFICULDADEMINIMA;
    else if (pontuacao < 20)
        novoPeriodo = DIFICULDADEMEDIA;
    else if (pontuacao < 30)
        novoPeriodo = DIFICULDADEMAXIMA;
    else {
        // reinicia jogo
        fimDeJogo();
        novoPeriodo = DIFICULDADEMINIMA;
    }

    __HAL_TIM_SET_AUTORELOAD(&htim3, novoPeriodo);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
}

int main(void)
{
	HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_GPIO_Display_Init();
    MX_USART1_UART_Init();

    srand(HAL_GetTick());

    HAL_TIM_Base_Start_IT(&htim2);
    HAL_TIM_Base_Start_IT(&htim3);

    resetPontos();

    bool estadoAnterior[5] = {false};  // Para detectar bordas

    while (1)
    {
    	if (flagResetPorTimeout)
    	{
    	    const char* msg = "Inatividade detectada! Jogo reiniciado.\r\n";
    	    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    	    fimDeJogo();
    	    flagResetPorTimeout = false;
    	}
    	if (flagMudaLED)
    	{
    		novoLED();
    	    flagMudaLED = false;
    	}

    	if (lerBotaoDebounce(GPIOB, GPIO_PIN_7))
    	{
    	    const char* msg = "Botão de reset pressionado! Jogo reiniciado.\r\n";
    	    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    	    fimDeJogo();
    	    continue; // Pula o restante do loop
    	}

    	bool estadoAtual[5];
    	bool transicaoPressionado = false;

    	// Lê os botões com debounce
    	estadoAtual[0] = lerBotaoDebounce(GPIOA, GPIO_PIN_0);
    	estadoAtual[1] = lerBotaoDebounce(GPIOA, GPIO_PIN_1);
    	estadoAtual[2] = lerBotaoDebounce(GPIOA, GPIO_PIN_2);
    	estadoAtual[3] = lerBotaoDebounce(GPIOA, GPIO_PIN_3);
    	estadoAtual[4] = lerBotaoDebounce(GPIOA, GPIO_PIN_4);

    	// Detecta transições de borda de descida (pressionamento)
    	for (int i = 0; i < 5; i++) {
    	    if (estadoAtual[i] && !estadoAnterior[i]) {
    	        flagsBotoes[i] = true;
    	        transicaoPressionado = true;
    	    } else {
    	        flagsBotoes[i] = false;
    	    }
    	    estadoAnterior[i] = estadoAtual[i];
    	}

    	if (transicaoPressionado)
    	{
    	    uint32_t tempoResposta = __HAL_TIM_GET_COUNTER(&htim2)/10;  // Tempo desde o novo LED
    	    HAL_TIM_Base_Stop(&htim2); // Para o timer momentaneamente
    	    bool erro = false;
    	    for (int i = 0; i < 5; i++)
    	    {
    	        if (flagsBotoes[i] != flagsLEDs[i]) erro = true;
    	    }

    	    char msg[50];

    	    if (!erro)
    	    {
    	        pontuacao++;

    	        snprintf(msg, sizeof(msg), "Tempo: %lu ms\r\n", tempoResposta);
    	        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    	        snprintf(msg, sizeof(msg), "%d\r\n", pontuacao);
    	        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    	        ajustarDificuldade();
    	    }
    	    else
    	    {
    	        snprintf(msg, sizeof(msg), "Erro! LED errado!\r\n");
    	        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    	        totalErros++;
    	    }

    	    display_write(pontuacao);

    	    __HAL_TIM_SET_COUNTER(&htim2, 0);  // Zera cronômetro
    	    HAL_TIM_Base_Start(&htim2);        // Reinicia para detectar inatividade
    	    novoLED();
    	}
    }
    return 0;
}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}



static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 14399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 49999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  __HAL_RCC_TIM3_CLK_ENABLE();

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7199;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = DIFICULDADEMINIMA;  // Começa com dificuldade mínima (1500ms)
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim3) != HAL_OK) Error_Handler();
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig);
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Display_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // LED no PC13
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // LED inicialmente apagado

  // Entradas PA0–PA4
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Saídas PB5–PB6 PB8-PB9
  GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_8, GPIO_PIN_RESET); // LED inicialmente apagado

  // Saída PA7
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET); // LED inicialmente apagado

  // Botão de reset em PB7
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
    	flagResetPorTimeout = true;
    }
    if (htim->Instance == TIM3)
    {
        flagMudaLED = true;
    }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
