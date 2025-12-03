#include "msp.h"
#include "Clock.h"
#include <stdio.h>

#define LED_OFF     0b000
#define LED_RED     0b001
#define LED_GREEN   0b010
#define LED_BLUE    0b100
#define LED_YELLOW  0b011
#define LED_PURPLE  0b101
#define LED_CYAN    0b110

void systick_init()
{
    SysTick->LOAD = 0x00FFFFFF;
    SysTick->CTRL = 0x00000005;
}

void systick_wait1ms()
{
    SysTick->LOAD = 48000; // 48000 Hz Clock Hardware?
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

void turn_on_led(int color)
{
    P2->OUT = (P2->OUT & ~0x07) | (color & 0x07);
}

void IR_Init()
{
    // 0, 2, 4, 6 IR
    P5->SEL0 &= ~0x08;
    P5->SEL1 &= ~0x08;      // GPIO
    P5->DIR  |=  0x08;      // OUTPUT
    P5->OUT  &= ~0x08;      // turn off 4 even IR LEDs

    // 1, 3, 5, 7 IR
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

    NVIC->IP[3] = (NVIC->IP[3]&0x0000FFFF) | 0x40400000;
    NVIC->ISER[0] = 0x0000C000;
    TIMER_A3->CTL |= 0x0024;
}

uint16_t first_left;
uint16_t first_right;

uint16_t period_left;
uint16_t period_right;

uint32_t right_count;
uint32_t left_count;

void TA3_0_IRQHandler()
{
    TIMER_A3->CCTL[0] &= ~0x0001;
    period_right = TIMER_A3->CCR[0] - first_right;
    first_right = TIMER_A3->CCR[0];

    right_count++;
    P2->OUT &= ~0x07;
    P2->OUT |= 0x02;
}

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

uint32_t get_right_rpm()
{
    return 2000000 / period_right;
}

void rotate_right(uint16_t duty)
{
    right_backward();
    left_forward();
    move(duty, duty);
}

void rotate_left(uint16_t duty)
{
    right_forward();
    left_backward();
    move(duty, duty);
}

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
    int step = 2;
    int cnt = 0;
    int left_speed = 600, right_speed = 600;
    int stage_progress = 0;
    // step 1: 직진하는 상태
    // step 2: 왼쪽으로 회전해야 하는 상태
    // step 3: 오른쪽으로 회전해야 하는 상태
    // step 4: 곡선 추적 상태
    // step 5: 종료 상태
    while(1)
    {   
        P5->OUT |= 0x08;
        P9->OUT |= 0x04;

        P7->DIR = 0xFF;
        P7->OUT = 0xFF;

        Clock_Delay1us(10);

        P7->DIR = 0x00;

        Clock_Delay1us(10);

        if(step == 0){
            move(0,0);
            if((P7->IN & 0b00011000) == 0b00011000){
                step = 1;
            }
        }
        else if(step == 1)
        {   
            turn_on_led(LED_GREEN);
            if((P7->IN & 0b00011000) == 0b00011000){
                // align 잘 된 경우
                right_forward();
                left_forward();
                move(left_speed, right_speed);
                P2->OUT |=  0xC0;
                systick_wait1ms();
            }
            else{
                if(cnt == 0 || cnt == 3 || cnt == 4) step = 2;
                else step = 3;
                cnt++;
                left_count = 0; right_count = 0;
            }
        }
        else if(step == 2)
        {
            turn_on_led(LED_BLUE);
            rotate_left(1000);

            if(right_count > 1500){
                step = 1;
                if (cnt == 4) step = 4;
                left_count = 0;
                right_count = 0;
            }
        }
        else if(step == 3)
        {
            turn_on_led(LED_BLUE);
            rotate_right(1000);

            if(left_count > 1500){
                step = 1;
                if (cnt == 5) step = 5;
                left_count = 0;
                right_count = 0;
            }
        }
        else if(step == 4)
        {
            turn_on_led(LED_YELLOW);
            if((P7->IN & 0b00011000) == 0b00011000){
                // align 잘 된 경우
                right_forward();
                left_forward();
                move(left_speed, right_speed);
                P2->OUT |=  0xC0;
                systick_wait1ms();
            }
            else if((P7->IN & 0b00011000) == 0b0000000){
                step = 3;
                cnt++;
            }
            else{
                if((P7->IN & 0b00010000) == 0){ 
                    // 선에서 왼쪽으로 벗어난 경우
                    right_backward(); left_forward();
                    move(200, 200);
                    P2->OUT &= ~0x07;
                    P2->OUT |= 0x04;
                    systick_wait1ms();
                }
                else if((P7->IN & 0b00001000) == 0){ 
                    // 선에서 오른쪽으로 벗어난 경우
                    right_forward(); left_backward();
                    move(200, 200);
                    P2->OUT &= ~0x07;
                    P2->OUT |= 0x04;
                    systick_wait1ms();
                }
            }
        }
        else if(step == 5)
        {
            turn_on_led(LED_PURPLE);
            if((P7->IN & 0b00011000) == 0b00011000){
                // align 잘 된 경우
                right_forward();
                left_forward();
                move(left_speed, right_speed);
                P2->OUT |=  0xC0;
                systick_wait1ms();
            }
            else if((P7->IN & 0b01111110) == 0b01111110){
                // 종료 선 만난 경우
                P2->OUT &= ~0x07;
                P2->OUT |=  0x02;

                move(0,0);
            }
        }
        else{
            turn_on_led(LED_CYAN);
            move(0,0);
        }


        
        // else if((P7->IN & 0b01111110) == 0b01111110){
        //     // 종료 선 만난 경우
        //     P2->OUT &= ~0x07;
        //     P2->OUT |=  0x02;

        //     move(0,0);
        // }

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
