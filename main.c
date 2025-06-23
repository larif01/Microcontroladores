#include "main.h"
#include <stdlib.h>
#include <stdbool.h>

//#define velMIN 150
//#define velMED 100
//#define velMAX 50

void SystemClock_Config(void);
static void MX_GPIO_Display_Init(void);
static void MX_GPIO_Init(void);

//Endereços 0-4 botões e LEDs individuais
bool flagsBotoes[5] = {false};
bool flagsLEDs[5] = {false};

void EXTI9_5_IRQHandler(void);

void resetPontos(void);

void configBotaoInterrup(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	//__HAL_RCC_GPIOB_CLK_ENEABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Speed = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

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
	//Garante que o jogador soltou os botões antes de continuar
	while ((flagsBotoes[0] || flagsBotoes[1] || flagsBotoes[2] || flagsBotoes[3] || flagsBotoes[4]))
	{
		flagsBotoes[0] = lerBotaoDebounce(GPIOA, GPIO_PIN_0);
	    flagsBotoes[1] = lerBotaoDebounce(GPIOA, GPIO_PIN_1);
	    flagsBotoes[2] = lerBotaoDebounce(GPIOA, GPIO_PIN_2);
	    flagsBotoes[3] = lerBotaoDebounce(GPIOA, GPIO_PIN_3);
	    flagsBotoes[4] = lerBotaoDebounce(GPIOA, GPIO_PIN_4);
	}

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

void resetPontos()
{
	display_write(0);
}

void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);  // Chama o callback da HAL
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_7) != RESET)
    {
    	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);
    	resetPontos();
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_GPIO_Display_Init();

    srand(HAL_GetTick());

    configBotaoInterrup();

    static uint8_t pontuacao = 0;

    while (1)
    {
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

    	//Verifica botões e desvia para ver resultado
        if (flagsBotoes[0] || flagsBotoes[1] || flagsBotoes[2] || flagsBotoes[3] || flagsBotoes[4])
        {
            bool erro = false;
            for (int i = 0; i < 5; i++)
            {
                if (flagsBotoes[i] != flagsLEDs[i]) erro = true;
            }
            if (erro == false)
            {
                pontuacao = pontuacao + 1;
            }
            display_write(pontuacao);
            erro = false;

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
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
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

  // Saída PA10
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET); // LED inicialmente apagado
}
