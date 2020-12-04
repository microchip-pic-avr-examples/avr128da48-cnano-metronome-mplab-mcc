/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "mcc_generated_files/mcc.h"
#include <util/delay.h>

#define NUMBER_OF_PULSES                100
#define DURATION_OF_A_PULSE             30      //microseconds
#define STEPS_COMPLETE_ROTATION         0x780
#define STEPS_TO_INITIAL_POSITION       0x32A
#define STEPS_IN_DIAL                   0x12C
#define CMPBCLR_MAX_VALUE               2500
#define DIVISION_RATIO                  0.971
#define SHIFT_DIV_5                     0x05

#define TCA1_ENABLE()               do { TCA1.SINGLE.CTRLA |= (1 << TCA_SINGLE_ENABLE_bp); } while (0)
#define TCA1_DISABLE()              do { TCA1.SINGLE.CTRLA &= ~(1 << TCA_SINGLE_ENABLE_bp); } while (0)
#define TCD0_ENABLE()               do { while(!(TCD0.STATUS & TCD_ENRDY_bm)) { ; } TCD0.CTRLA |= TCD_ENABLE_bm;} while (0)
#define TCD0_DISABLE()              do { while(!(TCD0.STATUS & TCD_ENRDY_bm)) { ; } TCD0.CTRLA &= ~TCD_ENABLE_bm;} while (0)
#define MODE_METRONOME()            do { PORTB.OUTCLR = PIN2_bm;} while (0)        
#define MODE_FREE_RUN()             do { PORTB.OUTSET = PIN2_bm;} while (0)
#define RUN_CLOCKWISE()             do { PORTB.OUTCLR = PIN0_bm;} while (0)
#define RUN_COUNTERCLOCKWISE()      do { PORTB.OUTSET = PIN0_bm;} while (0)
#define TCA1WO1_AS_OUTPUT()         do { PORTB.DIRSET = PIN1_bm;} while (0)

static void METRONOME_updateOscillatingFrequency(uint16_t potentiometerValue);
static void METRONOME_setInInitialPosition(void);
static inline void METRONOME_runCompletelyCounterclockwise(void);
static inline void METRONOME_runInStartPosition(void);
static inline void METRONOME_prepareForOscillating(void);
static void METRONOME_turnLedAndBuzzerOn(void);
TCA1_cb_t TCA1_ovf(void);
TCA1_cb_t TCA1_cmp1(void);

static uint16_t potValue;

static void METRONOME_updateOscillatingFrequency(uint16_t potentiometerValue)
{
    TCD0.CMPBCLR = CMPBCLR_MAX_VALUE - (uint16_t)((float)potentiometerValue/DIVISION_RATIO);
    TCD0.CMPBSET = TCD0.CMPBCLR >> 1;
    TCD0.CTRLE |= TCD_SYNCEOC_bm;
}

static void METRONOME_setInInitialPosition(void)
{
    MODE_FREE_RUN();
    
    METRONOME_runCompletelyCounterclockwise();
    
    METRONOME_runInStartPosition();
    
    METRONOME_prepareForOscillating();  
}

static inline void METRONOME_runCompletelyCounterclockwise(void)
{
    RUN_COUNTERCLOCKWISE();
    
    TCA1WO1_AS_OUTPUT();
    
    TCA1.SINGLE.PER = STEPS_COMPLETE_ROTATION;
    TCA1.SINGLE.CNT = 0x00;
 
    TCD0_ENABLE();
    
    TCA1_ENABLE();
    while(TCA1.SINGLE.CNT < TCA1.SINGLE.PER)
    {
        ;
    }
    TCA1_DISABLE(); 

    TCD0_DISABLE();
    
    _delay_ms(100);
}

static inline void METRONOME_runInStartPosition(void)
{ 
    RUN_CLOCKWISE();
    
    TCA1.SINGLE.PER = STEPS_TO_INITIAL_POSITION;
    TCA1.SINGLE.CNT = 0x00;
    
    TCD0_ENABLE();
        
    TCA1_ENABLE();
    while(TCA1.SINGLE.CNT < TCA1.SINGLE.PER)
    {
        ;
    }
    TCA1_DISABLE(); 
      
    TCD0_DISABLE();
    
    _delay_ms(100);
}

static inline void METRONOME_prepareForOscillating(void)
{
    MODE_METRONOME();
    
    TCA1.SINGLE.CTRLB |= TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
    //PER = 2 * CMP1 => 50% duty cycle
    TCA1.SINGLE.CMP1 = STEPS_IN_DIAL;
    TCA1.SINGLE.PER = STEPS_IN_DIAL * 2;
    TCA1.SINGLE.CNT = 0x00;
    
    TCD0_ENABLE();
    
    TCA1_ENABLE(); 
}

/*The ON duration was divided into 100 pulses in order to obtain a louder sound of the buzzer*/
static void METRONOME_turnLedAndBuzzerOn(void)
{
    uint8_t index;
    
    for(index = 0; index < NUMBER_OF_PULSES; index++)
    {
        LED_BUZZER_Toggle();
        _delay_us(DURATION_OF_A_PULSE);
    }
}

TCA1_cb_t TCA1_ovf(void)
{
    METRONOME_turnLedAndBuzzerOn();

    TCA1.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}

TCA1_cb_t TCA1_cmp1(void)
{
    METRONOME_turnLedAndBuzzerOn();

    TCA1.SINGLE.INTFLAGS = TCA_SINGLE_CMP1_bm;
}

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	SYSTEM_Initialize();
    
    TCA1_SetOVFIsrCallback(TCA1_ovf);
    
    TCA1_SetCMP1IsrCallback(TCA1_cmp1);

    METRONOME_setInInitialPosition();
    
    sei();

	/* Replace with your application code */
	while (1) 
    {
       potValue = (uint16_t)(ADC0_GetConversion(ADC_MUXPOS_AIN0_gc)) >> SHIFT_DIV_5;
        
        if(potValue == 0)
        {
            TCD0_DISABLE();
        }
        else
        {
            TCD0_ENABLE();
        }
        
        METRONOME_updateOscillatingFrequency(potValue);
	}
}
