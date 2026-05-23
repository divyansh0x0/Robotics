#include "stm32f1xx_hal.h"
#include "TB6612FNGController.h"
#include <string.h>

UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim2;

Robo::TB6612FNGController MotorController;

// UART RX Variables
uint8_t rx_byte;
uint8_t rx_buffer[8];
uint8_t rx_state = 0;
uint8_t rx_index = 0;
uint32_t lastByteTime = 0;
float joystickBuffer[2] = {0.0f, 0.0f};
uint32_t lastDataTime = 0;

// LED Variables
static bool ledOn = false;
static uint32_t lastToggleTime = 0;

extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
}

extern "C" void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        uint32_t now = HAL_GetTick();
        if (rx_state != 0 && (now - lastByteTime > 50)) {
            rx_state = 0;
            rx_index = 0;
        }
        lastByteTime = now;

        switch (rx_state) {
            case 0:
                if (rx_byte == 0xAA) rx_state = 1;
                break;
            case 1:
                rx_state = (rx_byte == 0x55) ? 2 : 0;
                rx_index = 0;
                break;
            case 2:
                rx_buffer[rx_index++] = rx_byte;
                if (rx_index >= 8) {
                    memcpy(&joystickBuffer[0], &rx_buffer[0], 4);
                    memcpy(&joystickBuffer[1], &rx_buffer[4], 4);
                    rx_state = 0;
                    lastDataTime = now;
                }
                break;
        }
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL3; // 8MHz * 3 = 24MHz
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        while(1);
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1; // 24MHz doesn't need APB1 div
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        while(1);
    }
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE(); // Enable PD for HSE if necessary

    // STDBY (PA4) and Motor IN Pins (PA2, PA3, PA5, PA6)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // LED OFF

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // LED BUILTIN (PC13)
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

static void MX_TIM2_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 23; // 24MHz / 24 = 1MHz
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 255;   // 1MHz / 256 = ~3.9kHz PWM
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        while(1);
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
        while(1);
    }
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        while(1);
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) {
        while(1);
    }
    
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
        while(1);
    }

    // PWM Output Pins (PA1 -> CH2)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_TIM3_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM3_CLK_ENABLE();

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 23; // 24MHz / 24 = 1MHz
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 255;   // 1MHz / 256 = ~3.9kHz PWM
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        while(1);
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
        while(1);
    }
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        while(1);
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
        while(1);
    }
    
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
        while(1);
    }

    // PWM Output Pin (PA7 -> CH2)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_USART1_UART_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        while(1);
    }

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // USART1 TX (PA9)
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // USART1 RX (PA10)
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void updateLED() {
    uint32_t now = HAL_GetTick();
    bool dataActive = (now - lastDataTime < 500);

    if (dataActive) {
        // Switch between displaying left and right motor speed every 1000ms
        int speed = 0;
        if ((now / 1000) % 2 == 0) {
            speed = MotorController.getLeftSpeed();
        } else {
            speed = MotorController.getRightSpeed();
        }

        // Map speed (0-255) to a blink interval (500ms to 50ms)
        uint32_t blinkInterval = 500 - (speed * 450 / 255);
        if (speed == 0) {
            blinkInterval = 1000; // very slow toggle if stopped
        }

        if (now - lastToggleTime >= blinkInterval) {
            ledOn = !ledOn;
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, ledOn ? GPIO_PIN_RESET : GPIO_PIN_SET); 
            lastToggleTime = now;
        }
    } else {
        if (ledOn) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            ledOn = false;
        }
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_USART1_UART_Init();

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    Robo::MotorPins leftPins, rightPins;
    leftPins.htim = &htim2;
    leftPins.channel = TIM_CHANNEL_2; // PWMA PA1
    leftPins.in1_port = GPIOA;
    leftPins.in1_pin = GPIO_PIN_3;    // AIN1 PA3
    leftPins.in2_port = GPIOA;
    leftPins.in2_pin = GPIO_PIN_2;    // AIN2 PA2

    rightPins.htim = &htim3;
    rightPins.channel = TIM_CHANNEL_2; // PWMB PA7
    rightPins.in1_port = GPIOA;
    rightPins.in1_pin = GPIO_PIN_5;    // BIN1 PA5
    rightPins.in2_port = GPIOA;
    rightPins.in2_pin = GPIO_PIN_6;    // BIN2 PA6

    MotorController.init(leftPins, rightPins, GPIOA, GPIO_PIN_4);

    while (1) {
        uint32_t now = HAL_GetTick();
        if (now - lastDataTime < 500) {
            MotorController.update(joystickBuffer[0], joystickBuffer[1]);
        } else {
            MotorController.update(0.0f, 0.0f);
        }
        updateLED();
    }
}
