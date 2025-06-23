#include "main.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

//Constantes de tempo *100us
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
//Variável de pontuação Geral
uint8_t pontuacao = 0;
//Flags para as interrupções
volatile bool flagResetPorTimeout = false;
volatile bool flagMudaLED = false;

bool lerBotaoDebounce(GPIO_TypeDef* porta, uint16_t pino)	//Um debouce para os botões é necessário
{
    if (HAL_GPIO_ReadPin(porta, pino) == GPIO_PIN_RESET)
    {
        HAL_Delay(20);
        if (HAL_GPIO_ReadPin(porta, pino) == GPIO_PIN_RESET)	//Verifica estado em uma janela de tempo de 20ms
        {
            return true;
        }
    }
    return false;
}

void novoLED()	//Para trocar o LED para um novo, verificamos o anterior, apagamos e criamos um novo diferente
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

void send_byte(uint8_t data)	//Viaja o vetor de segmentos, atualizando o número
{
    for (int i = 7; i >= 0; i--) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, (data >> i) & 0x01);  // SDI
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);        // SCLK HIGH
    	HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);      // SCLK LOW
    }
}

void latch()	//É feito o LOAD nos displays
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);    // LOAD HIGH
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);  // LOAD LOW

}

void display_write(uint8_t num)	//Para escrever nos displays, o numero atual da pontuação é passado
{
	//Tabela de segmentos para cada número
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

void display_clear()	//Função rápida para apagar os 7 segmentos, não necessitando de parametros
{
    send_byte(0xFF); // Apaga o dígito da direita
    send_byte(0xFF); // Apaga o dígito da esquerda
    latch();
}

void ajustarDificuldade()	//Aqui se verifica se é necessário mudar a constante do counter do TIM2 
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
        fimDeJogo();	//Em caso de mais de 30 pontos, o jogo termina
        novoPeriodo = DIFICULDADEMINIMA;
    }

    __HAL_TIM_SET_AUTORELOAD(&htim3, novoPeriodo);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
}

void resetPontos()	//Reset da pontuação
{
	pontuacao = 0;
	display_write(0);
}

void fimDeJogo()
{
	//Pisca os 7 segmentos para avisar o jogador
	for(int i = 0; i < 3; i++)
	{
		display_clear();
		HAL_Delay(500);
		display_write(pontuacao);
		HAL_Delay(500);
	}
	resetPontos();	//Reinicia os pontos
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

    //Inicia o sistema em 0, para se livrar de lixo de memória
    resetPontos();

    while (1)
    {
	//verifica as flags
    	if (flagResetPorTimeout)
    	{
    	    const char* msg = "Inatividade detectada! Jogo reiniciado.\r\n";
    	    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    	    fimDeJogo();	//Em caso de ociosidade por 10 segundos, o jogo é reiniciado
    	    flagResetPorTimeout = false;
    	}
    	if (flagMudaLED)
    	{
    		novoLED();
    	    flagMudaLED = false;	//Muda os LEDs depois da contagem de tempo atual
    	}
	//Verifica-se as entradas (botões)
          // PA0
      	flagsBotoes[0] = lerBotaoDebounce(GPIOA, GPIO_PIN_0);
          // PA1
      	flagsBotoes[1] = lerBotaoDebounce(GPIOA, GPIO_PIN_1);
          // PA2
      	flagsBotoes[2] = lerBotaoDebounce(GPIOA, GPIO_PIN_2);
          // PA3
      	flagsBotoes[3] = lerBotaoDebounce(GPIOA, GPIO_PIN_3);
          // PA4
      	flagsBotoes[4] = lerBotaoDebounce(GPIOA, GPIO_PIN_4);

      	//Desvio para ver resultado
        if (flagsBotoes[0] || flagsBotoes[1] || flagsBotoes[2] || flagsBotoes[3] || flagsBotoes[4])
        {
        	uint32_t tempoResposta = __HAL_TIM_GET_COUNTER(&htim2);  // Tempo desde o novo LED
        	HAL_TIM_Base_Stop(&htim2); // Para o timer momentaneamente
            bool erro = false;	//Flag de acerto
		//Verifica se o botão acionado é o mesmo que o LED 
            for (int i = 0; i < 5; i++)
            {
                if (flagsBotoes[i] != flagsLEDs[i]) erro = true; //Se LED == Botão, marca a flag de acerto
            }

            char msg[50];
		//Desvio condicional de pontuação
            if (erro == false)
            {
                pontuacao = pontuacao + 1;	//Adiciona 1 ponto
                ajustarDificuldade();		//Verifica a dificuldade atual
                snprintf(msg, sizeof(msg), "Tempo: %lu ms\r\n", tempoResposta);
                // Envia o número pela UART
                char msg[10];
                snprintf(msg, sizeof(msg), "%d\r\n", pontuacao);  // formata "pontuacao\n"
                HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
            }
            else
            {
            	snprintf(msg, sizeof(msg), "Erro! LED errado!\r\n");
            }

            HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);	//Escreve para o usuário

            display_write(pontuacao);		//Atualiza displays
            erro = false;
          	//Garante que o jogador soltou os botões antes de continuar
          	while ((flagsBotoes[0] || flagsBotoes[1] || flagsBotoes[2] || flagsBotoes[3] || flagsBotoes[4]))
          	{
          		flagsBotoes[0] = lerBotaoDebounce(GPIOA, GPIO_PIN_0);
          	    flagsBotoes[1] = lerBotaoDebounce(GPIOA, GPIO_PIN_1);
          	    flagsBotoes[2] = lerBotaoDebounce(GPIOA, GPIO_PIN_2);
          	    flagsBotoes[3] = lerBotaoDebounce(GPIOA, GPIO_PIN_3);
          	    flagsBotoes[4] = lerBotaoDebounce(GPIOA, GPIO_PIN_4);
          	}
          	__HAL_TIM_SET_COUNTER(&htim2, 0);  // Zera cronômetro
          	HAL_TIM_Base_Start(&htim2);        // Reinicia para detectar inatividade
            novoLED();	//Termina atualizando os LEDs
        }
    }
    return 0;
}

//Geração de clock
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

//Timer 2 conta a cada 200us
static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 14399; 	// 72M / 14400 = 5000
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 49999;		//50000 / 5000 = 10 segundos
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

//Timer 3 conta a cada 100us
static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  __HAL_RCC_TIM3_CLK_ENABLE();

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7199;	//72M / 7200 = 10000
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

//Declaração de UART
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

//Pinos dos displays de 7 segmentos
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

//Declarações de GPIO
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
}

//Interrupções dos Timers 2 e 3, modificando as flags
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
