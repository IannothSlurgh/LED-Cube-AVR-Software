	//Layer transistors off
	//OE high to disable output from latches
	//do addr bus and data bus
	//OE low
	//transistors on
	//current layer incremented.
  
	//Port A data bus
	//B (7..5) isp (4) debug button on address selector PCB
	//(3) output enable (2..0) row selction address bus
	//Port C are all outputs to layer transistors (power to layer cathodes)
	//D (7..6) OPEN probably user buttons (5..4) rs232 mode switch and debug led
	//(3..2) debug LEDs on address selector (1..0) rs232

	//Credits to Mr. CHR at http://www.instructables.com/id/Led-Cube-8x8x8/ for providing code which has been heavily modified
	//ISR is the piece of code that most resembles his work.
	
	#define F_CPU 1000000UL // 1MHZ clock 
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <util/delay.h>
	#include <stdlib.h>
  
	#define OCIE2 7
	#define CS20 0	
	#define CS22 2
	#define WGM21 3
  
	#define B_OR_DISABLE_OE 0x08 //1<<3
	#define B_AND_ENABLE_OE 0xF7 //~(1<<3)
	#define D_PATTERN 0x80
	#define D_SPEED 0x40
  
	//8 bits to a char one for each pin of port A (represents the LEDs on in a row)
	volatile unsigned char cube[8][8];
	volatile unsigned char current_layer;
	volatile uint8_t current_pattern;
	volatile uint8_t current_speed;
	volatile uint8_t button_pattern;
	volatile uint8_t button_speed;
	volatile uint16_t rotate;
  
	char valid_element(uint8_t x, uint8_t y, uint8_t z)
	{
		if(x>7 || y>7 || z>7)
		{
			return 0;
		}	
		return 1;
	}
  
	void element_on(uint8_t x, uint8_t y, uint8_t z)
	{
		if(valid_element(x,y,z))
		{
			cube[z][y] |= (1 << x);
		}
	}
  
	void element_off(uint8_t x, uint8_t y, uint8_t z)
	{
		if(valid_element(x,y,z))
		{
			cube[z][y] &= ~(1<<x);
		}
	}
  
	void set_row(uint8_t x_value, uint8_t y, uint8_t z)
	{
		//Check y-z
		if(valid_element(0,y,z))
		{
			uint8_t i = 0;
			cube[z][y] = x_value;
		}
	}

	void column_off(uint8_t x, uint8_t y)
	{
		if(!valid_element(x,y,0))
		{
			return;
		}
		uint8_t z = 0;
		for(;z<8;++z)
		{
			element_off(x, y, z);
		}
	}
	
	void clear_cube()
	{
		uint8_t z;
		uint8_t y;
		for(y=0;y<8;++y)
		{
			for(z=0;z<8;++z)
			{
				set_row(0x00, y, z);
			}	
		}
	}
 
	void xy_plane_on(uint8_t z)
	{
		if(!valid_element(0,0,z))
		{
			return;
		}
		uint8_t y = 0;
		for(;y<8;++y)
		{
			set_row(0xFF, y, z);
		}
	}
 
	void yz_plane_on(uint8_t x)
	{
		if(!valid_element(x,0,0))
		{
			return;
		}
		uint8_t z;
		uint8_t y;
		for(z=0;z<8;++z)
		{
			for(y=0;y<8;++y)
			{
				element_on(x,y,z);
			}
		}
	}
	
	void xz_plane_on(uint8_t y)
	{
		if(!valid_element(0,y,0))
		{
			return;
		}
		uint8_t z;
		for(z=0;z<8;++z)
		{
			set_row(0xFF, y, z);
		}
	}
 
	void xy_plane_off(uint8_t z)
	{
		if(!valid_element(0,0,z))
		{
			return;
		}
		uint8_t y = 0;
		for(;y<8;++y)
		{
			set_row(0x00, y, z);
		}	
	}
 	void yz_plane_off(uint8_t x)
	{
		if(!valid_element(x,0,0))
		{
			return;
		}
		uint8_t z;
		uint8_t y;
		for(z=0;z<8;++z)
		{
			for(y=0;y<8;++y)
			{
				element_off(x,y,z);
			}
		}
	}
	
	void xz_plane_off(uint8_t y)
	{
		if(!valid_element(0,y,0))
		{
			return;
		}
		uint8_t z;
		for(z=0;z<8;++z)
		{
			set_row(0x00, y, z);
		}
	}
 
	void scaled_delay(float max)
	{
		_delay_ms(max/(current_speed+1));
	}
  
	ISR(TIMER2_COMP_vect)
	{
		PORTC=0x00; //Layer transistors off (turn off all LEDs currently on)
		PORTB |=B_OR_DISABLE_OE; // Set Output Enable bit to 1 (which actually disables latch output)
		//Go through each row in the upcoming layer to light up
		uint8_t i = 0;
		for(; i<8; ++i)
		{
			//Set the data for each row and store it in the latches.
			PORTA = cube[current_layer][i];
			//Clear row addr selection bits and then set the addr to i.
			PORTB = (PORTB & ~(0x07)) | (i+1);
		}
		PORTB &= B_AND_ENABLE_OE;//Re-enable latch output
		//Turn on GND to layer
		PORTC = (1<<current_layer);
		//Prepare for next interrupt
		current_layer++;
		current_layer %= 8; //layer is 0..7
	}	
  
	void effect_xy_bounce()
	{
		uint8_t z = 0;
		for(;z<8;++z)
		{
			xy_plane_on(z);
			scaled_delay(7000);
			xy_plane_off(z);
		}
		for(z=0;z<8;++z)
		{
			xy_plane_on(7-z);
			scaled_delay(7000);
			xy_plane_off(7-z);
		}
	}  
 
	void effect_yz_bounce()
	{
		uint8_t x = 0;
		for(;x<8;++x)
		{
			yz_plane_on(x);
			scaled_delay(7000);
			yz_plane_off(x);
		}
		for(x=0;x<8;++x)
		{
			yz_plane_on(7-x);
			scaled_delay(7000);
			yz_plane_off(7-x);
		}
	}
	
	void effect_xz_bounce()
	{
		uint8_t y = 0;
		for(;y<8;++y)
		{
			xz_plane_on(y);
			scaled_delay(7000);
			xz_plane_off(y);
		}
		for(y=0;y<8;++y)
		{
			xz_plane_on(7-y);
			scaled_delay(7000);
			xz_plane_off(7-y);
		}
	}	
 
	void effect_full_cube()
	{
		uint8_t z = 0;
		for(;z<8;++z)
		{
			xy_plane_on(z);
		}
		scaled_delay(3000);
		clear_cube;
		scaled_delay(3000);
	}
  
	void effect_water_torture()
	{
		//unsigned long start = rand() / (RAND_MAX / 4);
		unsigned long start = 0;
		int x=3;
		int y=3;
		int z = 0;
		//Choose point of origin via randum
		//Drop falling
		for(;z<8;z++)
		{
			element_on(x, y, z);
			scaled_delay(1000);
			element_off(x,y,z);
		}
		z=7;
		//A single ripple
		element_on(x-1, y-1, z);
		element_on(x-1, y, z);
		element_on(x-1, y+1, z);
		element_on(x, y-1, z);
		element_on(x, y+1, z);
		element_on(x+1, y-1, z);
		element_on(x+1, y, z);
		element_on(x+1, y+1, z);
		element_off(x,y,z);
		scaled_delay(800);
		//Then it vanishes.
		element_off(x-1, y-1, z);
		element_off(x-1, y, z);
		element_off(x-1, y+1, z);
		element_off(x, y-1, z);
		element_off(x, y+1, z);
		element_off(x+1, y-1, z);
		element_off(x+1, y, z);
		element_off(x+1, y+1, z);
		scaled_delay(500);
	}
 
	void spiral_1(uint8_t y)
	{
		uint8_t x=0;
		uint8_t z=0;
		for(z=0;z<8;++z)
		{
			element_on(x, y, z);
			scaled_delay(200);			
		}
		z=7;
		for(x=1;x<8;++x)
		{
			element_on(x, y, z);
			scaled_delay(200);
		}
		x=7;
		for(z=1;z<8;++z)
		{
			element_on(x, y, 7-z);
			scaled_delay(200);
		}
		z=0;
		for(x=1;x<7;++x)
		{
			element_on(7-x, y, z);
			scaled_delay(200);
		}
		x=1;
		
		for(z=1;z<7;++z)
		{
			element_on(x, y, z);
			scaled_delay(200);			
		}
		z=6;
		for(x=2;x<7;++x)
		{
			element_on(x, y, z);
			scaled_delay(200);
		}
		x=6;
		for(z=2;z<7;++z)
		{
			element_on(x, y, 7-z);
			scaled_delay(200);
		}
		z=1;
		for(x=2;x<6;++x)
		{
			element_on(7-x, y, z);
			scaled_delay(200);
		}
		x=2;
		
		for(z=2;z<6;++z)
		{
			element_on(x, y, z);
			scaled_delay(200);			
		}
		z=5;
		for(x=2;x<6;++x)
		{
			element_on(x, y, z);
			scaled_delay(200);
		}
		x=5;
		for(z=2;z<6;++z)
		{
			element_on(x, y, 7-z);
			scaled_delay(200);
		}
		z=2;
		for(x=3;x<5;++x)
		{
			element_on(7-x, y, z);
			scaled_delay(200);
		}
		x=3;
		
		for(z=3;z<5;++z)
		{
			element_on(x, y, z);
			scaled_delay(200);			
		}
		z=4;
		for(x=4;x<5;++x)
		{
			element_on(x, y, z);
			scaled_delay(200);
		}
		element_on(4, y, 3);
		scaled_delay(200);	 
	}
 
	void spiral_2(uint8_t y)
	{
		uint8_t x=0;
		uint8_t z=0;
		
		element_on(4, y, 3);
		scaled_delay(200);	 	

		z=4;
		for(x=4;x>2;--x)
		{
			element_on(x, y, z);
			scaled_delay(200);
		}
		x=3;
		for(z=4;z>1;--z)
		{
			element_on(x, y, z);
			scaled_delay(200);			
		}
		z=2;

		for(x=4;x<6;++x)
		{
			element_on(x, y, z);
			scaled_delay(200);
		}
		x=5;
		
		for(z=2;z<6;++z)
		{
			element_on(x, y, z);
			scaled_delay(200);
		}
		z=5;
		for(x=5;x>1;--x)
		{
			element_on(x, y, z);
			scaled_delay(200);
		}
		x=2;
		for(z=4;z>0;--z)
		{
			element_on(x, y, z);
			scaled_delay(200);			
		}
		z=1;
		for(x=3;x<7;x++)
		{
			element_on(x, y, z);
			scaled_delay(200);		
		}
		x=6;
		for(z=1;z<7;++z)
		{
			element_on(x, y, z);
			scaled_delay(200);			
		}
		z=6;
		for(x=6;x>0;--x)
		{
			element_on(x, y, z);
			scaled_delay(200);					
		}
		x=1;
		for(z=1;z<8;++z)
		{
			element_on(x, y, 7-z);
			scaled_delay(200);		
		}
		z=0;
		for(x=1;x<8;++x)
		{
			element_on(x, y, z);
			scaled_delay(200);				
		}
		x=7;
		for(z=0;z<8;++z)
		{
			element_on(x, y, z);
			scaled_delay(200);				
		}
		z=7;
		for(x=0;x<8;++x)
		{
			element_on(7-x, y, z);
			scaled_delay(200);			
		}
		x=0;
		for(z=0;z<8;++z)
		{
			element_on(x, y, 7-z);
			scaled_delay(200);			
		}
	}
 
	void effect_spiral()
	{
		uint8_t i = 0;
		uint8_t y = 0;
		for(;i<4;++i)
		{
			spiral_1(2*i);
			xz_plane_off(2*i);
			scaled_delay(200);
			spiral_2(2*i+1);
			xz_plane_off(2*i+1);
			scaled_delay(200);
		}
	
	}

	void effect_stairs()
	{
		uint8_t x = 0;
		uint8_t y = 0;
		uint8_t z = 0;
		scaled_delay(1000);
		for(z=0;z<8;++z)
		{
			y=0;
			for(x=0;x<8;++x)
			{
				element_on(x, y, z);
				scaled_delay(200);
				//element_off(x, y, z);
			}
			x=7;
			for(y=1;y<8;++y)
			{
				element_on(x, y, z);
				scaled_delay(200);
				//element_on(x, y, z);			
			}
			y=7;
			for(x=1;x<8;++x)
			{
				element_on(7-x, y, z);
				scaled_delay(200);
				//element_off(7-x, y, z);
			}		
			x=0;
			for(y=1;y<8;++y)
			{
				element_on(x, 7-y, z);
				scaled_delay(200);
				//element_off(x, 7-y, z);			
			}
		}		
	}

	void effect_compress()
	{
		uint8_t x = 0;
		uint8_t y = 0;
		uint8_t z = 0;
		uint8_t top_edge = 0xff;
		uint8_t side_edge = 0x81;
		uint8_t min = 0;
		uint8_t max = 7;
		

		for(min=0;min<max;++min)
		{
			side_edge = (0 | (1<<max))|(1<<min);
			set_row(top_edge,min,min);
			set_row(top_edge,max,min);
			set_row(top_edge,min,max);
			set_row(top_edge,max,max);
			if(min<max)
			{
				for(y=min+1;y<max;++y)
				{
					set_row(side_edge, y, min);
				}
				for(y=min+1;y<max;++y)
				{
					set_row(side_edge, y, max);
				}			
				for(z=min+1;z<max;++z)
				{
					set_row(side_edge,min,z);
					set_row(side_edge,max,z);			
				}
			}
		scaled_delay(3000);
		clear_cube();
			top_edge = top_edge & ~(1<<max);
			top_edge = top_edge & ~(1<<min);
		max--;	
		}

		for(;max<8;++max)
		{
			top_edge = top_edge | (1<<max);
			top_edge = top_edge | (1<<min);
			side_edge = (0 | (1<<max))|(1<<min);
			set_row(top_edge,min,min);
			set_row(top_edge,max,min);
			set_row(top_edge,min,max);
			set_row(top_edge,max,max);
			if(min<max)
			{
				for(y=min+1;y<max;++y)
				{
					set_row(side_edge, y, min);
				}
				for(y=min+1;y<max;++y)
				{
					set_row(side_edge, y, max);
				}			
				for(z=min+1;z<max;++z)
				{
					set_row(side_edge,min,z);
					set_row(side_edge,max,z);			
				}
			}
		scaled_delay(3000);
		clear_cube();
		min--;	
		}
		
	}
	
	void launch_effect()
	{
		if(rotate>0)
		{
			current_pattern = rotate-1;
		}
		switch(current_pattern)
		{
			case 0x00:	
			effect_xy_bounce();
			break;
			case 0x01:
			effect_yz_bounce();
			break;
			case 0x02:
			effect_xz_bounce();
			break;
			case 0x03:
			effect_water_torture();
			break;
			case 0x04:
			effect_full_cube();
			break;
			case 0x05:
			effect_stairs();
			break;
			case 0x06:
			effect_spiral();
			break;
			case 0x07:
			effect_compress();
			break;
			case 0x08:
			rotate = 1;
			scaled_delay(9000);
			default:
			break;
		}
		if(rotate>0)
		{
		
			rotate = (rotate +1)%9;
			if(rotate == 0)
			{
				rotate = 1;
			}
			current_pattern = 8;
		}
		clear_cube();
	}
  
	int main(void)
	{
		//Data bus all outputs
		DDRA = 0xFF;
		//isp, OE and row address all output. Bit 5 is for button input
		DDRB = 0xEF;
		//Transistor Layer all outputs
		DDRC = 0xFF;
		//3 buttons inputs on bits (7..5) where (7..6) are user buttons
		//debug LEDs and rs232 are outputs
		DDRD = 0x1F;
	
		PORTA = 0x00; //Data bus defaults to all 0s
		PORTB = 0x10; //Debug button is pull up. ISP, OE, address default 0
		PORTC = 0x00; //No layer on.
		PORTD = 0xE0; //All buttons are pullup. debug LEDs off and rs232 off
	
		current_layer = 0;
	
		//Courtesy of Mr. CHR (and Atmel datasheet)
		OCR2 = 200; //Interrupt at counter = 10
		TCCR2 |= (1<<WGM21); //prescale 128 with counter reset at OCR2
		TCCR2 |= (1<<CS20) | (1<<CS22); //prescale 128 with counter reset at OCR2
		TCNT2 = 0x00; //Counter starts at 0
		TIMSK |= (1<<OCIE2); //Enable interrupt on count reset.
		//Courtesy of Mr. CHR (and Atmel datasheet)
		
		uint8_t i = 0;
		
		srand(1);
		current_pattern = 0;
		current_speed = 0;
		rotate = 0;
		sei();
	
		while(1)
		{
			button_pattern = (PIND & D_PATTERN) >> 7;
			if(!button_pattern)
			{
				current_pattern = (current_pattern+1)%9;
				button_pattern = 0;
				if(current_pattern == 0 && rotate>0)
				{
					rotate=0;
				}
			}
			button_speed = (PIND & D_SPEED) >> 6;
			if(!button_speed)
			{
				//10 speed settings 0..9
				current_speed = (current_speed + 1)%9;
				button_speed = 0;
			}
			launch_effect();
		}
	
	}