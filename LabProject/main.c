#include "msp.h"
#include "Clock.h"
#include <stdio.h>

#define LED_RED     0b001
#define LED_GREEN   0b010
#define LED_BLUE    0b100

void systick_init()
{
    SysTick->LOAD = 0x00FFFFFF;
    SysTick->CTRL = 0x00000005;
}

void systick_wait1ms()
{
    SysTick->LOAD = 48000; // 48000 MHz Clock Hardware?
    SysTick->VAL = 0;
    while((SysTick->CTRL & 0x00010000) == 0){};
}

void systick_wait1s()
{
    int i = 0;
    int count = 1000;
    for(i=0;i<count;i++)
    {
        systick_wait1ms();
    }
}

void led_init()
{
    P2->SEL0 &= ~0x07;
    P2->SEL1 &= ~0x07;
    P2->DIR |= 0x07;
    P2->OUT &= ~0x07;
}

void turn_on_led(int n)
{
    P2->OUT  &= ~0x07;
    P2->OUT |= n;
}

void switch_init() // Todo
{
//    P1->IN = 0;
}

int main(void)
{
    Clock_Init48MHz();
    led_init();
    switch_init();
    systick_init();

    while(1)
    {
        turn_on_led(LED_RED);
        systick_wait1s();
        turn_on_led(0b010);
        systick_wait1s();

        printf("P1 : %d",P1->OUT);
    }
}
