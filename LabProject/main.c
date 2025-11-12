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

void IR_Init()
{
    // 0, 2, 4, 6 IR Emitter
    P5->SEL0 &= ~0x08;
    P5->SEL1 &= ~0x08;      // GPIO
    P5->DIR  |=  0x08;      // OUTPUT
    P5->OUT  &= ~0x08;      // turn off 4 even IR LEDs

    // 1, 3, 5, 7 IR Emitter
    P9->SEL0 &= ~0x04;
    P9->SEL1 &= ~0x04;      // GPIO
    P9->DIR  |=  0x04;      // OUTPUT
    P9->OUT  &= ~0x04;      // turn off 4 odd IR LEDs

    // 0~7 IR SEnsor
    P7->SEL0 &= ~0xFF;
    P7->SEL1 &= ~0xFF;      // GPIO
    P7->DIR  &= ~0xFF;      // INPUT
}

void pwm_init34(uint16_t period, uint16_t duty3, uint16_t duty4)
{
    TIMER_A0->CCR[0] = period;

    TIMER_A0->EX0 =0x0000;

    TIMER_A0->CCTL[3] = 0x0040;
    TIMER_A0->CCR[3] = duty3;
    TIMER_A0->CCTL[4] = 0x0040;
    TIMER_A0->CCR[4] = duty4;

    TIMER_A0->CTL = 0x02F0;

    P2->DIR |= 0xC0;
    P2->SEL0 |= 0xC0;
    P2->SEL1 &=~0xC0;
}

void (*TimerA2Task) (void);
void TimerA2_Init(void(*task)(void), uint16_t period)
{
    TimerA2Task = task;
    TIMER_A2->CTL = 0x0280;
    TIMER_A2->CCTL[0] = 0x0010;
    TIMER_A2->CCR[0] = (period - 1);
    TIMER_A2->EX0 = 0x0005;

    NVIC->IP[3] = (NVIC->IP[3] & 0xFFFFFF00) | 0x00000040;
    NVIC->ISER[0] = 0x00001000;
    TIMER_A2->CTL |= 0x0014;
}

void motor_init()
{
    P3->SEL0 &= ~0xC0;
    P3->SEL1 &= ~0xC0;
    P3->DIR  |=  0xC0;
    P3->OUT  &= ~0xC0;

    P5->SEL0 &= ~0x30;
    P5->SEL1 &= ~0x30;
    P5->DIR  |=  0x30;
    P5->OUT  &= ~0x30;

    P2->SEL0 &= ~0xC0;
    P2->SEL1 &= ~0xC0;
    P2->DIR  |=  0xC0;
    P2->OUT  &= ~0xC0;

    pwm_init34(7500, 0, 0);
}

void move(uint16_t leftDuty, uint16_t rightDuty)
{
    P3->OUT |= 0xC0;
    TIMER_A0->CCR[3] = leftDuty;
    TIMER_A0->CCR[4] = rightDuty;
}

void left_forward()
{
    P5->OUT &= ~0x10;
}

void left_backward()
{
    P5->OUT |= 0x10;
}

void right_forward()
{
    P5->OUT &= ~0x20;
}

void right_backward()
{
    P5->OUT |= 0x20;
}

void timer_A3_capture_init(void)
{
    P10->SEL0 |= 0x30;
    P10->SEL1 &= ~0x30;
    P10->DIR  &= ~0x30;

    TIMER_A3->CTL &= ~0x0030;
    TIMER_A3->CTL = 0x0200;

    TIMER_A3->CCTL[0] = 0x4910;
    TIMER_A3->CCTL[1] = 0x4910;
    TIMER_A3->EX0 &= ~0x0007;

    NVIC->IP[3] = (NVIC->IP[3]&0x0000FFFF) | 0x404000000;
    NVIC->ISER[0] = 0x0000C000;
    TIMER_A3->CTL |= 0x0024;
}

uint16_t first_left;
uint16_t first_right;

uint16_t period_left;
uint16_t period_right;

//void TA3_0_IRQHandler()
//{
//    TIMER_A3->CCTL[0] &= ~0x0001;
//    period_right = TIMER_A3->CCR[0] - first_right;
//    first_right = TIMER_A3->CCR[0];
//}

uint32_t left_count;
void TA3_N_IRQHandler(void)
{
    TIMER_A3->CCTL[1] &= ~0x0001;
    period_left = TIMER_A3->CCR[1] - first_left;
    first_left = TIMER_A3->CCR[1];


    left_count++;

    P2->OUT &= ~0x07;
    P2->OUT |= 0x01;
}

uint32_t get_left_rpm()
{
    return 2000000 / period_left;
}

//void TA3_N_IRQHandler()
//{
//    TIMER_A3->CCTL[1] &= ~0x0001;
//    left_count++;
//}



int main(void)
{
    // Initialize
    Clock_Init48MHz();
    led_init();
    IR_Init();
    motor_init();
    systick_init();
    timer_A3_capture_init();

    int sensor;
    // Running
//    while(1)
//    {
//        P5->OUT |= 0x08;
//        P9->OUT |= 0x04;
//
//        P7->DIR = 0xFF;
//        P7->OUT = 0xFF;
//
//        Clock_Delay1us(10);
//
//        P7->DIR = 0x00;
//
//        Clock_Delay1us(1000);
//
//        if((P7->IN & 0b01111110) == 0b01111110){ // stop
//            P2->OUT &= ~0x07;
//            P2->OUT |=  0x02;
//
//            P2->OUT &= ~0xC0;
//
//            move(0,0);
//        }
//        else if((P7->IN & 0b00000000) == 0b00000000){ // go straight
//            P2->OUT &= ~0x07;
//            P2->OUT |= 0x01;
//
////            P5->OUT &= ~0x30;
//            right_forward();
//            left_forward();
//            move(1500, 1500);
//            P2->OUT |=  0xC0;
////            P3->OUT |=  0xC0;
//            systick_wait1ms();
//        }
//        else{ // stop
//            P2->OUT &= ~0x07;
//            P2->OUT &= ~0xC0;
//
//            move(0,0);
//        }
//
//        P5->OUT &= ~0x08;
//        P9->OUT &= ~0x04;
//
//        Clock_Delay1ms(10);
//
////        P5->OUT &= ~0x30;
////        P2->OUT |=  0xC0;
////        P3->OUT |=  0xC0;
////        Clock_Delay1ms(1000);
////
////        P2->OUT &= ~0xC0;
////        Clock_Delay1ms(1000);
//    }

//    TimerA2_Init(&TA3_N_IRQHandler, 50000);
    int stage = 0;

    while(1)
    {
        P5->OUT |= 0x08;
        P9->OUT |= 0x04;

        P7->DIR = 0xFF;
        P7->OUT = 0xFF;

        Clock_Delay1us(10);

        P7->DIR = 0x00;

        Clock_Delay1us(1000);

        if(stage == 0 && left_count > 491){
            stage = 1;
            left_count = 0;
        }
        else if(stage == 1 && left_count > 60) stage = 2;


        stage = 1;

        if(stage == 1)
        {
            // Rotate
            right_forward();
            left_backward();
            move(1500, 1500);

            // 60 나중에 체크
        }
        else if (stage == 2)
        {

            P2->OUT &= ~0x07;
            P2->OUT &= ~0xC0;
            move(0, 0);
        }
        else
        {
            right_forward();
            left_forward();
            move(1500, 1500);
            P2->OUT |=  0xC0;

            systick_wait1ms();
        }

        P5->OUT &= ~0x08;
        P9->OUT &= ~0x04;

        Clock_Delay1ms(10);
    }
}
