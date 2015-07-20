#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

#define BAUD 9600
#define BAUDRATE ((F_CPU)/(BAUD*16UL)-1)

char pattern[] = {
	0b00010001,
	0b00110011,
	0b00100010,
	0b01100110,
	0b01000100,
	0b11001100,
	0b10001000,
	0b10011001,
};

char buffer[32];				// We will store data received via serial port here
uint8_t buffer_position = 0;

volatile int16_t motor1 = 0;
volatile uint8_t motor1_offset = 0;

volatile int16_t motor2 = 0;
volatile uint8_t motor2_offset = 0;

void uart_init (void) {
	UBRRH = (BAUDRATE>>8);			// shift the register right by 8 bits
	UBRRL = BAUDRATE;			// set baud rate
	UCSRB |= (1<<TXEN)|(1<<RXEN);		// enable receiver and transmitter
	UCSRC |= (1<<UCSZ0)|(1<<UCSZ1);		// 8bit data format
	UCSRB |= (1 << RXCIE);			// Enable the USART Recieve Complete interrupt (USART_RXC)
}

void io_init (void) {
	DDRB = 0xFF;				// make all pins on PORTB output
}

void timer_init(void) {
	TCCR0A |= (1<<WGM01);			// CTC (Reset after interrupt)
	TCCR0B |= (1<<CS02);			// Prescaler=clock/256
	OCR0A = 0x0A;				// Interrupt after 10 ticks ~ 1 ms @ 2.4576 MHz
	TCNT0 = 0;				// Initialize timer0
	TIMSK |= (1<<OCIE0A);			// Enable interrupt on compare
}

void uart_transmit (char data) {
	while (!( UCSRA & (1<<UDRE)));		// 'RTS'
	UDR = data;
}

// Send string via serial line
void msg(char* str) {
	int i;
	for (i=0; str[i] != '\0' && i<=32; i++) {
		uart_transmit( str[i] );
	}
}

void msg_ln(char* str) {
	msg(str);
	msg("\r\n");
}

void process_cmd(void) {
	if ( buffer[0] == 's' ) {	// Stepper
		int16_t val = 0;
		char *p = buffer+2;

		while (! (*p == '\0' || *p == '-' || (*p >= '0' && *p <= '9')) ) {
			p ++;
		}

		val = atoi(p);

		if (buffer[1] != '2') {
			motor1 += val;
			msg_ln("Stepper1");
		}

		if (buffer[1] != '1') {
			motor2 += val;
			msg_ln("Stepper2");
		}

		return;
	}

	if ( buffer[0] =='r' ) {	// Reset
		if (buffer[1] != '2') {
			motor1 = 0;
		}

		if (buffer[1] != '1') {
			motor2 = 0;
		}
	}

	msg_ln("Unknown command");
}

int main() {
	uart_init();
	io_init();
	timer_init();

	buffer[31] = '\0';

	sei();

	msg_ln("\r\nReset");

	for (;;) {
	}
}

// Received char
ISR(USART_RX_vect) {
	char in;
	in = UDR;

	if (in == '\r' || in == '\n') {
		msg_ln("");

		buffer[ buffer_position ] = '\0';
		process_cmd();

		buffer[0] = '\0';
		buffer_position = 0;
	} else if (in == 0x1B) {	// ESC
		buffer[0] = '\0';
		buffer_position = 0;
		msg_ln("\r\nEscape");
	} else if (buffer_position >= 31) {
		buffer[0] = '\0';
		buffer_position = 0;
		msg_ln("\r\nBuffer overflow!");
	} else if ( in >= 32) {
		buffer[ buffer_position ] = in;
		uart_transmit(in);
		++ buffer_position;
	}

}

// We cannot make steps too often. My motor works best
// at ~1 kHz so We set timer to ~1 ms and only make
// steps in the interrupt.
ISR(TIMER0_COMPA_vect) {
	if (motor1 || motor2) {
		if (motor1 > 1) {
			motor1 --;
			motor1_offset --;
		} else if (motor1 < -1) {
			motor1 ++;
			motor1_offset ++;
		}

		if (motor2 > 1) {
			motor2 --;
			motor2_offset --;
		} else if (motor2 < -1) {
			motor2 ++;
			motor2_offset ++;
		}

		motor1_offset &= 0x07;
		motor2_offset &= 0x07;

		PORTB = (pattern[motor1_offset] & 0xF0) | (pattern[motor2_offset] & 0x0F);
	} else {
		PORTB = 0;
	}
}

