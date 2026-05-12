#include "buzzer.h"

void Buzzer_Init(void)
{
  HAL_GPIO_WritePin(Bee_GPIO_Port, Bee_Pin, GPIO_PIN_SET);
}

void Buzzer_On(void)
{
  HAL_GPIO_WritePin(Bee_GPIO_Port, Bee_Pin, GPIO_PIN_RESET);
}

void Buzzer_Off(void)
{
  HAL_GPIO_WritePin(Bee_GPIO_Port, Bee_Pin, GPIO_PIN_SET);
}

static void Buzzer_BeepPattern(uint32_t on_ms, uint32_t off_ms)
{
  for (uint8_t i = 0; i < 2; i++)
  {
    Buzzer_On();
    HAL_Delay(on_ms);
    Buzzer_Off();
    if (i == 0)
    {
      HAL_Delay(off_ms);
    }
  }
}

void Buzzer_BeepSlowTwice(void)
{
  Buzzer_BeepPattern(100, 80);
}

void Buzzer_BeepFastTwice(void)
{
  Buzzer_BeepPattern(40, 30);
}
