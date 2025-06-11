#include "main.h"
#include <stdlib.h>
#include <stdbool.h>

//#define velMIN 150
//#define velMED 100
//#define velMAX 50

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

//Endereços 0-4 botões individuais
//Endereço 5 flag algum botão apertado
bool flagsBotoes[6] = {false};
bool flagsLEDs[5] = {false};
int ultimoLED = -1;

bool lerBotaoDebounce(GPIO_TypeDef* porta, uint16_t pino) {
    if (HAL_GPIO_ReadPin(porta, pino) == GPIO_PIN_RESET) {
        HAL_Delay(20);
        if (HAL_GPIO_ReadPin(porta, pino) == GPIO_PIN_RESET) {
            return true;
        }
    }
    return false;
}

void verificaTodos()
{
    const int tempoDebounce_ms = 20;
    bool estadoAnterior = flagsBotoes[5];
    uint32_t tempoInicial = HAL_GetTick();

    while ((HAL_GetTick() - tempoInicial) < tempoDebounce_ms)
    {
        bool estadoAtual =
            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET ||
            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET ||
            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET ||
            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET ||
            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET;

        if (estadoAtual != estadoAnterior)
        {
            // Houve oscilação, reinicia o tempo de debounce
            estadoAnterior = estadoAtual;
            tempoInicial = HAL_GetTick();
        }
    }

    // Estado estável confirmado
    if (estadoAnterior == true)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // LED ON
        flagsBotoes[5] = true;
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // LED OFF
        flagsBotoes[5] = false;
    }
}

void novoLED()
{
    while (flagsBotoes[5] != false) {
    	verificaTodos();
    }

    for(int i = 0; i < 5; i++) flagsBotoes[i] = false;

    flagsLEDs[ultimoLED] = false;

    int aux;
    do {
        aux = rand() % 5;
    } while (aux == ultimoLED);
    ultimoLED = aux;


    flagsLEDs[ultimoLED] = true;


    if (flagsLEDs[0] == true) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);

    if (flagsLEDs[1] == true) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);

    if (flagsLEDs[2] == true) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);

    if (flagsLEDs[3] == true) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

    if (flagsLEDs[4] == true) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    srand(HAL_GetTick());

    static int pontuacao = 0;

    while (1)
    {

        // PA0 → PB9
    	flagsBotoes[0] = lerBotaoDebounce(GPIOA, GPIO_PIN_0);

        // PA1 → PB8
    	flagsBotoes[1] = lerBotaoDebounce(GPIOA, GPIO_PIN_1);

        // PA2 → PA10
    	flagsBotoes[2] = lerBotaoDebounce(GPIOA, GPIO_PIN_2);

        // PA3 → PB6
    	flagsBotoes[3] = lerBotaoDebounce(GPIOA, GPIO_PIN_3);

        // PA4 → PB5
    	flagsBotoes[4] = lerBotaoDebounce(GPIOA, GPIO_PIN_4);

        verificaTodos();

        if (flagsBotoes[5] == true)
        {

            bool erro = false;

            for (int i = 0; i < 5; i++)
            {
                if (flagsBotoes[i] != flagsLEDs[i]) erro = true;
            }

            if (erro == false)
            {
                pontuacao +=1;
                //{
                //print no 7 segmentos
                //calculo de velocidade
                //}
            }

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
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET); // LED inicialmente apagado
}
