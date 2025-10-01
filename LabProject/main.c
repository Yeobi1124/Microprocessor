#include "msp.h"
#include "Clock.h"
#include <stdio.h>

#define LED_RED     0b001
#define LED_GREEN   0b010
#define LED_BLUE    0b100

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
}

int main(void)
{
    // Initialize
    Clock_Init48MHz();
    led_init();
    IR_Init();
    motor_init();

    int sensor;
    // Running
    while(1)
    {
        P5->OUT |= 0x08;
        P9->OUT |= 0x04;

        P7->DIR = 0xFF;
        P7->OUT = 0xFF;

        Clock_Delay1us(10);

        P7->DIR = 0x00;

        Clock_Delay1us(3000);

        if((P7->IN & 0b01111110) == 0b01111110){
            P2->OUT &= ~0x07;
            P2->OUT |=  0x02;
            P2->OUT &= ~0xC0;
        }
        else if((P7->IN & 0b00011000) == 0b00011000){
            P2->OUT |= 0x01;
            P5->OUT &= ~0x30;
            P2->OUT |=  0xC0;
            P3->OUT |=  0xC0;
        }
        else{
            P2->OUT &= ~0x07;
            P2->OUT &= ~0xC0;
        }

        P5->OUT &= ~0x08;
        P9->OUT &= ~0x04;

        Clock_Delay1ms(10);

//        P5->OUT &= ~0x30;
//        P2->OUT |=  0xC0;
//        P3->OUT |=  0xC0;
//        Clock_Delay1ms(1000);
//
//        P2->OUT &= ~0xC0;
//        Clock_Delay1ms(1000);
    }
}
