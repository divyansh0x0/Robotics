import re

with open('main.cpp', 'r') as f:
    content = f.read()

content = content.replace('UART_HandleTypeDef huart3;', 'UART_HandleTypeDef huart1;\nTIM_HandleTypeDef htim3;')

content = content.replace('extern "C" void USART3_IRQHandler(void) {\n    HAL_UART_IRQHandler(&huart3);\n}', 'extern "C" void USART1_IRQHandler(void) {\n    HAL_UART_IRQHandler(&huart1);\n}')

content = content.replace('if (huart->Instance == USART3) {', 'if (huart->Instance == USART1) {')

content = content.replace('HAL_UART_Receive_IT(&huart3, &rx_byte, 1);', 'HAL_UART_Receive_IT(&huart1, &rx_byte, 1);')

mx_gpio_init_old = """    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE(); // Enable PD for HSE if necessary

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_12 | GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // LED OFF

    // STDBY
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Motor IN Pins (PB8, PB9, PB12, PB13)
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // LED BUILTIN (PC13)
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);"""

mx_gpio_init_new = """    __HAL_RCC_GPIOA_CLK_ENABLE();
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
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);"""

content = content.replace(mx_gpio_init_old, mx_gpio_init_new)


tim2_usart3_old = """    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
        while(1);
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) {
        while(1);
    }

    // PWM Output Pins (PA1 -> CH2, PA2 -> CH3)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_USART3_UART_Init(void) {
    __HAL_RCC_USART3_CLK_ENABLE();

    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK) {
        while(1);
    }

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // USART3 TX (PB10)
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // USART3 RX (PB11)
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}"""


tim2_usart1_new = """    sConfigOC.OCMode = TIM_OCMODE_PWM1;
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
}"""

content = content.replace(tim2_usart3_old, tim2_usart1_new)

main_old = """int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_USART3_UART_Init();

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

    HAL_UART_Receive_IT(&huart3, &rx_byte, 1);

    Robo::MotorPins leftPins, rightPins;
    leftPins.htim = &htim2;
    leftPins.channel = TIM_CHANNEL_2; // PWMA PA1
    leftPins.in1_port = GPIOB;
    leftPins.in1_pin = GPIO_PIN_13;   // AIN1 PB13
    leftPins.in2_port = GPIOB;
    leftPins.in2_pin = GPIO_PIN_12;   // AIN2 PB12

    rightPins.htim = &htim2;
    rightPins.channel = TIM_CHANNEL_3; // PWMB PA2
    rightPins.in1_port = GPIOB;
    rightPins.in1_pin = GPIO_PIN_9;    // BIN1 PB9
    rightPins.in2_port = GPIOB;
    rightPins.in2_pin = GPIO_PIN_8;    // BIN2 PB8

    MotorController.init(leftPins, rightPins, GPIOA, GPIO_PIN_3);"""


main_new = """int main(void) {
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

    MotorController.init(leftPins, rightPins, GPIOA, GPIO_PIN_4);"""

content = content.replace(main_old, main_new)

with open('main.cpp', 'w') as f:
    f.write(content)
