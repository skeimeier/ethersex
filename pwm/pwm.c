/* vim:fdm=marker ts=4 et ai
 * {{{
 *
 * Copyright (c) 2009 by Christian Dietrich <stettberger@dokucode.de>
 * Copyright (c) 2009 by Stefan Riepenhausen <rhn@gmx.net>
 * Copyright (c) 2008 by Markus Meissna <markus@meissna.de>
 * Copyright (c) by Ulrich Radig <mail@ulrichradig.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 }}} */

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "../config.h"
#include "../debug.h"
#include "../ecmd_parser/ecmd.h"


#ifdef PWM_SUPPORT
#include "pwm.h"

#ifdef PWM_MELODY_SUPPORT
uint8_t pwm_melody_tone=0;
uint16_t pwm_melody_i=0;
volatile uint16_t pwm_melody_scale=523;

// Noten-Frequenz,
// calculated for 8Mhz & PWM without Prescale
// note, scale, 	real Frequency
#define c 542		// 262
#define cis 574		// 277
#define d 608		// 294
#define dis 644		// 311
#define e 682		// 330
#define f 722		// 349
#define fis 765		// 370
#define g 810		// 392
#define gis 858		// 415
#define a 908		// 440
#define h 1019		// 494

#define p 0   // break


// simple sinus in 255 values
const uint8_t sinewave[1][256] PROGMEM=
{
{
0x80,0x83,0x86,0x89,0x8c,0x8f,0x92,0x95,0x98,0x9c,0x9f,0xa2,0xa5,0xa8,0xab,0xae,
0xb0,0xb3,0xb6,0xb9,0xbc,0xbf,0xc1,0xc4,0xc7,0xc9,0xcc,0xce,0xd1,0xd3,0xd5,0xd8,
0xda,0xdc,0xde,0xe0,0xe2,0xe4,0xe6,0xe8,0xea,0xec,0xed,0xef,0xf0,0xf2,0xf3,0xf5,
0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfc,0xfd,0xfe,0xfe,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0xfe,0xfd,0xfc,0xfc,0xfb,0xfa,0xf9,0xf8,0xf7,
0xf6,0xf5,0xf3,0xf2,0xf0,0xef,0xed,0xec,0xea,0xe8,0xe6,0xe4,0xe2,0xe0,0xde,0xdc,
0xda,0xd8,0xd5,0xd3,0xd1,0xce,0xcc,0xc9,0xc7,0xc4,0xc1,0xbf,0xbc,0xb9,0xb6,0xb3,
0xb0,0xae,0xab,0xa8,0xa5,0xa2,0x9f,0x9c,0x98,0x95,0x92,0x8f,0x8c,0x89,0x86,0x83,
0x80,0x7c,0x79,0x76,0x73,0x70,0x6d,0x6a,0x67,0x63,0x60,0x5d,0x5a,0x57,0x54,0x51,
0x4f,0x4c,0x49,0x46,0x43,0x40,0x3e,0x3b,0x38,0x36,0x33,0x31,0x2e,0x2c,0x2a,0x27,
0x25,0x23,0x21,0x1f,0x1d,0x1b,0x19,0x17,0x15,0x13,0x12,0x10,0x0f,0x0d,0x0c,0x0a,
0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x03,0x02,0x01,0x01,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x02,0x03,0x03,0x04,0x05,0x06,0x07,0x08,
0x09,0x0a,0x0c,0x0d,0x0f,0x10,0x12,0x13,0x15,0x17,0x19,0x1b,0x1d,0x1f,0x21,0x23,
0x25,0x27,0x2a,0x2c,0x2e,0x31,0x33,0x36,0x38,0x3b,0x3e,0x40,0x43,0x46,0x49,0x4c,
0x4f,0x51,0x54,0x57,0x5a,0x5d,0x60,0x63,0x67,0x6a,0x6d,0x70,0x73,0x76,0x79,0x7c
}
};

struct notes_duration_t
{
uint16_t note;
uint16_t duration;
};

struct notes_duration_t entchen[] =
{
{c,1200},{d,1200},{e,1200},{f,1200},{g,1600},
{g,1600},{a,800},{a,800},{a,800},{a,800},
{g,1600},{a,800},{a,800},{a,800},{a,800},
{g,1600},{f,1200},{f,1200},{f,1200},
{f,1200},{e,1200},{e,1600},{d,1200},
{d,1200},{d,1200},{d,1200},{c,1600},
{p,160} // break at end
};

#endif //PWM_MELODY_SUPPORT

#ifdef PWM_WAV_SUPPORT

#include "ethersex_wav.h"

//Sound Daten (got by MegaLOG of Ulrich Radig (see Radig webmodul ))

#define SOUNDFREQ 8000
#define SOUNDDIVISOR (F_CPU/64/SOUNDFREQ)

uint16_t pwmbytecounter = 0;

//Timer2 Interrupt
ISR (TIMER0_OVF_vect)
{
	uint8_t s = pgm_read_byte(&pwmsound[pwmbytecounter]);
#ifdef DEBUG_PWM
    	if (pwmbytecounter < 10 || ((pwmbytecounter % 1000) == 0) ) debug_printf("PWM sound %x at pos %i\n",s, pwmbytecounter);
#endif
	TCNT0 = 255 - SOUNDDIVISOR;
	OCR2A = s;
	pwmbytecounter++;
	if(pwmbytecounter > sizeof(pwmsound))
	{
		pwmbytecounter = 0;
		pwm_stop();
	}
}
#endif // PWM_WAV_SUPPORT

#ifdef PWM_MELODY_SUPPORT
// Interrupt-Funktion, die den "Zeiger" hochzählt
// je nach gewünschter Frequenz wird "scale" verändert, 
// und somit die Sinuswelle schneller (hoher ton) 
// oder langsamer (tiefer Ton) abgelaufen
ISR(TIMER2_COMPA_vect){
	OCR2A=pgm_read_byte(&sinewave[pwm_melody_tone][(pwm_melody_i>>8)]);
   	pwm_melody_i += pwm_melody_scale;
}
#endif // PWM_MELODY_SUPPORT


#ifdef PWM_WAV_SUPPORT
void
pwm_wav_init(void)
{
	pwmbytecounter = 0;
#ifdef DEBUG_PWM
    	debug_printf("PWM wav init, size: %i, %i Hz \n", sizeof(pwmsound), SOUNDFREQ );
#endif
	//Set TIMER2 (PWM OC2 Pin = PD7)
	DDRD |= (1<<7);
	TCCR2A |= (1<<WGM21|1<<WGM20|1<<COM2A1);
	TCCR2B |= (1<<CS20);
	OCR2A = 128;
	
	//Set TIMER0 
	TIMSK0 |= (1 << TOIE0);
	TCCR0B = (1<<CS00|1<<CS01) ;
	TCNT0 = 255 - SOUNDDIVISOR;
}

void 
pwm_stop()
{
#ifdef DEBUG_PWM
    	debug_printf("PWM stop: \n");
#endif
	// timer 2 stop
	TCCR2B = 0;

	// timer 0 stop
	TCCR0B = 0 ;
   
}

#endif // PWM_WAV_SUPPORT

#ifdef PWM_MELODY_SUPPORT
void
pwm_melody_init()
{
	uint8_t songsize = sizeof(entchen) / (sizeof(struct notes_duration_t));
#ifdef DEBUG_PWM
    	debug_printf("PWM melody init, songsize %i: \n", songsize);
#endif
// see example at http://www.infolexikon.de/blog/atmega-music/

	// D7-Pin als Ausgang
	DDRD |= (1<<7);
	
	// Anfangswert der PWM
	OCR2A=0x80;
	
	//Output compare OCxA 8 bit non inverted PWM
	// Timer Counter Control Register!
	// Bit:   7	  6	 5	4     3     2     1     0
	// Bed: COMxA1 COMxA0 COMxB1 COMxB0 FOCxA FOCxB WGMx1 WGMx0
	// Hier:  1       0      0      1     0     0     0     1
	TCCR2A |= (1<<COM2A1|1<<COM2B0|1<<WGM20); // 0x91
	
	// Timer ohne Prescaler starten
	TCCR2B|=(1<<CS20); // 0x01;
	
	// Einschalten des Ausgangs-Vergleichs-Interrupts auf OCRxA 
	// Timer/Counter Interrupt Mask!
	// Bit:   7	6      5      4     3      2     1      0
	// Bed: OCIE2 TOIE2 TICIE1 OCIE1A OCIE1B TOIE1 ------ TOIE0	
	// Hier:  0	0      0      1     0      0     0      0
	TIMSK2 |= (1 << OCIE2A); // 0x10
	//enable global interrupts
	//sei();

    //  ------ Play it once, Sam ---------

	// durch das Noten-Array laufen und nacheinander
	// die Töne in jeweiliger Länge abspielen
	// da "scale" global definiert ist, kann es einfach
	// hier geändert werden!
	for(int y=0; y < songsize; y++){
		pwm_melody_scale = entchen[y].note;
#ifdef DEBUG_PWM
    	debug_printf("%i. note %i, dauer %i, i=%i, y=%i\n", y, entchen[y].note, entchen[y].duration, pwm_melody_i, y);
#endif
		_delay_ms(entchen[y].duration / 8 );
		// Interrupt kurz ausschalten, gibt kurze Pause
		// so werden die Töne getrennt
		cli();
		_delay_ms(200);
		pwm_melody_i=0;
		sei();
	}
}
#endif // PWM_MELODY_SUPPORT

#endif /*PWM_SUPPORT*/
