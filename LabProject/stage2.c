#include "msp.h"
#include "Clock.h"
#include <stdint.h>
#include <stdio.h>

#define PWM_PERIOD 7500
#define DEFAULT_SPEED 1

#define N_SLEEP_PINS (0x40 | 0x80)
#define DIR_PINS     (0x10 | 0x20)
#define PWM_PINS     (0x40 | 0x80)

#define ENCODER_PINS (0x10 | 0x20)
#define RIGHT_ENCODER_CHANNEL 0
#define LEFT_ENCODER_CHANNEL 1

#define COUNTS_FOR_TEST 10

volatile uint32_t Right_Encoder_Count = 0;
volatile uint32_t Left_Encoder_Count = 0;

void move(uint16_t leftDuty, uint16_t rightDuty) { TIMER_A0->CCR[4] = leftDuty; TIMER_A0->CCR[3] = rightDuty; }
void left_forward() { P5->OUT &= ~0x10; }
void left_backward() { P5->OUT |= 0x10; }
void right_forward() { P5->OUT &= ~0x20; }
void right_backward() { P5->OUT |= 0x20; }
void stop_motors() { P5->OUT &= 0; }
void enable_motors() { P3->OUT |= N_SLEEP_PINS; }
void disable_motors() { stop_motors(); P3->OUT &= ~N_SLEEP_PINS; }
void motor_init(void) {
    P3->SEL0 &= ~N_SLEEP_PINS; P3->SEL1 &= ~N_SLEEP_PINS; P3->DIR |= N_SLEEP_PINS; P3->OUT &= ~N_SLEEP_PINS;
    P5->SEL0 &= ~DIR_PINS; P5->SEL1 &= ~DIR_PINS; P5->DIR |= DIR_PINS; P5->OUT &= ~DIR_PINS;
    P2->SEL0 |= PWM_PINS; P2->SEL1 &= ~PWM_PINS; P2->DIR |= PWM_PINS;

    TIMER_A0->CTL = 0x0280;
    TIMER_A0->EX0 = 0x0001;
    TIMER_A0->CCR[0] = PWM_PERIOD;
    TIMER_A0->CCTL[3] = 0x00E0; TIMER_A0->CCR[3] = 0;
    TIMER_A0->CCTL[4] = 0x00E0; TIMER_A0->CCR[4] = 0;
    TIMER_A0->CTL |= 0x0030;
}

volatile int save[10000],li=0;

void tachometer_init(void) {
    P10->SEL0 |= 0x30;
    P10->SEL1 &= ~0x30;
    P10->DIR &= ~0x30;

    TIMER_A3->CTL &= ~0x0030;
    TIMER_A3->CTL = 0x0200;

    TIMER_A3->CCTL[0] = 0x4910;
    TIMER_A3->CCTL[1] = 0x4910;
    TIMER_A3->EX0 &= ~0x0007;

    NVIC->IP[3]=(NVIC->IP[3]&0x0000FFFF)|0x404000000;
    NVIC->ISER[0]=0x0000C000;

    TIMER_A3->CTL |= 0x0024;
}
void sensor_init(void){
    P5->SEL0&=~0x08;
    P5->SEL1&=~0x08;
    P5->DIR|=0x08;
    P5->OUT&=~0x08;
    P9->SEL0&=~0x04;
    P9->SEL1&=~0x04;
    P9->DIR|=0x04;
    P9->OUT&=~0x04;
    P7->SEL0&=~0xFF;
    P7->SEL1&=~0xFF;
    P7->DIR&=~0xFF;
}

void TA3_0_IRQHandler(void) {
    TIMER_A3->CCTL[RIGHT_ENCODER_CHANNEL] &= ~0x0001;
    Right_Encoder_Count++;
}

void TA3_N_IRQHandler(void) {
    TIMER_A3->CCTL[LEFT_ENCODER_CHANNEL] &= ~0x0001;
    Left_Encoder_Count++;
}

uint32_t state(void){
    P5->OUT|=0x08;
    P9->OUT|=0x04;
    P7->DIR=0xFF;
    P7->OUT=0xFF;
    Clock_Delay1us(10);
    P7->DIR=0x00;
    Clock_Delay1us(1500);
    int result=P7->IN,temp=result,cnt=0;
    printf("%d\n",result);
    while(temp>0){
        if(temp&1)
            cnt++;
        temp/=2;
    }
    if(cnt>7)
        return 0;
    int cr=(result/4)%16;
    int l=cr%4,r=(cr/4%2)*2+(cr/8);
    if(l==0&&r==0)
        return 0;
    if(l==r)
        return 1;
    if(l<r)
        return 2;
    if(l>r)
        return 3;
    return 1;
}

uint32_t state1(void){
    P5->OUT|=0x08;
    P9->OUT|=0x04;
    P7->DIR=0xFF;
    P7->OUT=0xFF;
    Clock_Delay1us(10);
    P7->DIR=0x00;
    Clock_Delay1us(1500);
    int result=P7->IN,temp=result,cnt=0;
    printf("%d\n",result);
    while(temp>0){
        if(temp&1) cnt++;
        temp/=2;
    }
    if(cnt>7)
        return 0;
    int cr=(result/4)%16;
    int l=cr%4,r=(cr/4%2)*2+(cr/8);
    if(l==0&&r==0) return 1;
    if(l==r) return 1;
    if(l<r) return 2;
    if(l>r) return 3;
    return 0;
}

void move1(void){
    move(DEFAULT_SPEED, DEFAULT_SPEED);
    volatile uint32_t action=state();
    while(action>0){
        printf("%d\n",action);
        if(action==0){
            stop_motors();
        }
        if(action==1){
            //stop_motors();
            left_forward();
            right_forward();
        }
        else if(action==2){
            //stop_motors();
            left_backward();
            right_forward();
        }
        else if(action==3){
            //stop_motors();
            right_backward();
            right_backward();
        }
        enable_motors();
        Clock_Delay1us(750);
        disable_motors();
        Clock_Delay1ms(1);
        action=state();
    }
    stop_motors();
    disable_motors();
    Clock_Delay1ms(500);
}

void main(void) {
    Clock_Init48MHz();
    sensor_init();
    motor_init();
    tachometer_init();
    __enable_irq();
    disable_motors();
    Clock_Delay1ms(1000);
    move1();
    return ;
}
