#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

#define BAUD 9600
#define BAUDRATE ((F_CPU)/(BAUD*16UL)-1)

char pattern[] = {
	0b00000001,
	0b00000011,
	0b00000010,
	0b00000110,
	0b00000100,
	0b00001100,
	0b00001000,
	0b00001001,
};

char buffer[32];				// We will store data received via serial port here
uint8_t buffer_position = 0;

int16_t motor1 = 0;
uint8_t motor1_offset = 0;

void uart_init (void) {
	UBRRH = (BAUDRATE>>8);			// shift the register right by 8 bits
	UBRRL = BAUDRATE;			// set baud rate
	UCSRB |= (1<<TXEN)|(1<<RXEN);		// enable receiver and transmitter
	UCSRC |= (1<<UCSZ0)|(1<<UCSZ1);		// 8bit data format
	UCSRB |= (1 << RXCIE);			// Enable the USART Recieve Complete interrupt (USART_RXC)
}

void io_init (void) {
	DDRB = 0x0F;				// make all pins on PORTB output
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

void process_cmd() {
	if (buffer[0] == 'm' && buffer[1] == ' ') {
		int16_t val = 0;
		char *p = buffer+1;

		while (! (*p == '\0' || *p == '-' || (*p >= '0' && *p <= '9')) ) {
			p ++;
		}

		val = atoi(p);
		motor1 += val;

		msg_ln("Motor1");
	} else {
		msg_ln("Unknown command");
	}
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

	if (in == '\r') {
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
	if (motor1) {
		if (motor1 > 1) {
			motor1 --;
			motor1_offset --;
		} else if (motor1 < -1) {
			motor1 ++;
			motor1_offset ++;
		}

		motor1_offset &= 0x07;
		PORTB = pattern[ motor1_offset ];
	} else {
		PORTB = 0;
	}
}

