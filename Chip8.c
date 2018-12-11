// Chip8 Emulator by g0jirasan
// Thanks to arnsa for his Chip8 emulator: https://github.com/arnsa/Chip-8-Emulator/blob/master/chip8.c
// and emulator101.com
//TODO: Error handling!, comments, Destroy SDL texture/renderer

// gcc Chip8.c -o Chip8 -lSDL2

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
 
#define	STACK_SIZE 10
#define MEM_SIZE 4096
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 320
#define WIDTH 64
#define HEIGHT 32
#define WHITE 0xFFFFFFFF
#define BLACK 0XFF000000
#define FPS 60
#define MS_PER_CYCLE 250 / FPS

unsigned char font[80] =		//Built in characters
{
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
 
typedef struct Chip8State
{
	FILE * game;

	uint8_t		V[16]; 		//Registers
	uint16_t	I;	   	//I register
	uint16_t	SP;	   	//Stack pointer	
	uint16_t	PC;	   	//Program counter (Instruction pointer)	 
	uint8_t		delay; 		//Delay timer
	uint8_t		sound;		//Sound timer
	uint16_t	stack[STACK_SIZE];
	uint8_t		memory[MEM_SIZE]; 
	uint32_t	graphics[WIDTH * HEIGHT];
	uint16_t	op;			//Opcode 
	bool 		draw;
	uint32_t 	keys[16];	

} Chip8State;


typedef struct display
{
	SDL_Texture *texture;
	SDL_Renderer *renderer;
	SDL_Window  *window;
} display;


int GetFileSize(FILE * file)
{
	fseek(file, 0L, SEEK_END);
	int fsize = ftell(file);
	fseek(file, 0L, SEEK_SET);

	return fsize;
}



Chip8State* InitChip8(FILE * file)
{
	Chip8State* s = calloc(sizeof(Chip8State), 1);

	for(int i = 0; i < 80; i++)
		s->memory[i] = font[i];

	s->SP = 0;			//Set stack pointer to 0
	s->PC = 0x200;		//Chip8 programs are loaded at address 0x200
	s->game = file;
	s->draw = false;	//Set draw flag
	
	int fsize = GetFileSize(file);

	fread(s->memory + 0x200, fsize, 1, s->game);
	memset(s->graphics, 0xFF000000, sizeof(s->graphics));
	
	printf("%s\n", "Reading Chip8 File...");
	printf("%-10s %d\n", "Game Size:", fsize);
	printf("%-10s 0x%X\n", "SP:", s->SP);
	printf("%-10s 0x%X\n\n", "PC:", s->PC);
	
	return s;
}



void timers(Chip8State * state)
{
	//TODO: Implement timers
	if(state->delay > 0)
		state->delay--;
	if(state->sound > 0)
		state->sound--;
}



void UpdateKeys(uint32_t keys[])
{
	const uint8_t *keystates = SDL_GetKeyboardState(NULL);
	keys[0x0] = keystates[SDL_SCANCODE_1];
	keys[0x1] = keystates[SDL_SCANCODE_2];
	keys[0x2] = keystates[SDL_SCANCODE_3];
	keys[0x3] = keystates[SDL_SCANCODE_4];
	keys[0x4] = keystates[SDL_SCANCODE_Q];
	keys[0x5] = keystates[SDL_SCANCODE_W];
	keys[0x6] = keystates[SDL_SCANCODE_E];
	keys[0x7] = keystates[SDL_SCANCODE_R];
	keys[0x8] = keystates[SDL_SCANCODE_A];
	keys[0x9] = keystates[SDL_SCANCODE_S];
	keys[0xA] = keystates[SDL_SCANCODE_D];
	keys[0xB] = keystates[SDL_SCANCODE_F];
	keys[0xC] = keystates[SDL_SCANCODE_Z];
	keys[0xD] = keystates[SDL_SCANCODE_X];
	keys[0xE] = keystates[SDL_SCANCODE_C];
	keys[0xF] = keystates[SDL_SCANCODE_V];
}



void Emulate(Chip8State *state)
{
	//Chip
	state->op = state->memory[state->PC] << 8 | state->memory[state->PC + 1];
	uint8_t X = (state->op & 0x0F00) >> 8;
	uint8_t Y = (state->op & 0x00F0) >> 4;
	uint16_t NN = state->op & 0x00FF;
	uint16_t NNN = state->op & 0x0FFF;
	UpdateKeys(state->keys);
	printf("Instruction: %X Address: %X I: %X SP: %X\n ", state->op, state->PC, state->I, state->SP);

	switch(state->op & 0xF000)
	{
		case 0x0000:
			switch(state->op & 0x0FFF)
			{
				case 0x00EE:
				{
					state->PC = state->stack[state->SP];
					state->SP -= 1;
					state->PC += 2;			
				}
				break;
				case 0x00E0:
				{
					memset(state->graphics, 0xFF000000, sizeof(state->graphics));
					state->PC += 2;	
				}
				break;
			} 
			break;
		case 0x1000: // JMP 1NNN
			{
				state->PC = NNN;
			} 
			break;
		case 0x2000: // CALL 2NNN
			{
				state->SP += 1;
				state->stack[state->SP] = state->PC;
				state->PC = NNN;
			} 
			break;
		case 0x3000: // SKIPE 3XNN
			{
				if (state->V[X] == NN)
				{
					state->PC += 4;
				}
				else
				{
					state->PC += 2;
				}
				
			} 
			break;
		case 0x4000: // SKIPNE 4XNN
			{
				if (state->V[X] != NN)
				{
					state->PC += 2;
				}
				state->PC += 2;
			} 
			break;
		case 0x5000: // SKIPRE 5XY0
			{
				if (state->V[X] == state->V[Y])
				{
					state->PC += 2;
				}
				state->PC += 2;
			}
			break;
		case 0x6000: // SET 6XNN
			{
				state->V[X] = NN;
				state->PC += 2;
			} 
			break;
		case 0x7000: // ADDR 7XNN
			{
				state->V[X] += NN;
				state->PC += 2;
			} 
			break;
		case 0x8000:
			{
				switch(state->op & 0x000F)
				{
					case 0x0000: // Vx=Vy Sets VX to the value of VY.
						{
							state->V[X] = state->V[Y];
							state->PC += 2;
						}
						break;
					case 0x0001: // Vx=Vx|Vy Sets VX to VX or VY. (Bitwise OR operation)
						{
							state->V[X] = state->V[X] | state->V[Y];
							state->PC += 2;
						}
						break;
					case 0x0002: // Vx=Vx&Vy Sets VX to VX and VY. (Bitwise AND operation)
						{
							state->V[X] = state->V[X] & state->V[Y];
							state->PC += 2;
						}
						break;
					case 0x0003: // Vx=Vx^Vy Sets VX to VX xor VY.
						{
							state->V[X] = state->V[X] ^ state->V[Y];
							state->PC += 2;
						}
						break;
					case 0x0004: // Vx += Vy Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
						{
							if (((int)state->V[X] + (int)state->V[Y] >> 4) < 256)
								{
									state->V[0xF] &= 0;
								}
							else
								{
									state->V[0xF] = 1;
								}
							state->V[X] += state->V[Y];
							state->PC += 2; 	
						}
						break;
					case 0x0005: // Vx -= Vy VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
						{
							if (((int)state->V[X] - (int)state->V[Y] >> 4) >= 0)
								{
									state->V[0xF] = 1;
								}
							else
								{
									state->V[0xF] &= 0;
								}
							state->V[X] -= state->V[Y];
							state->PC += 2;
						}
						break;
					case 0x0006: // Vx>>=1	Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
						{
							state->V[0xF] = state->V[X] & 7;
							state->V[X] = state->V[X] >> 1;
							state->PC += 2;
						}
						break;
					case 0x0007: // Vx=Vy-Vx Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
						{
							if (((int)state->V[X] - (int)state->V[Y]) > 0)
								{
									state->V[0xF] = 1;
								}
							else
								{
									state->V[0xF] &= 0;
								}
							state->V[X] = state->V[Y] - state->V[X];
							state->PC += 2;
						}
						break;
					case 0x000E: // Vx<<=1	Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
						{
							state->V[0xF] = state->V[X] >> 7;
							state->V[X] = state->V[X] << 1;
							state->PC += 2;
						}
						break;
				}
			}; 
			break;

		case 0x9000:
			{
				if (state->V[X] != state->V[Y])
				{
					state->PC += 2;
				}
				state->PC += 2;
			}	
			break;
		case 0xA000: 
			{
				state->I = NNN;
				state->PC += 2;
			} 
			break;
		case 0xB000:
			{
				state->PC = NNN + state->V[0];
			} 
			break;
		case 0xC000:
			{
				state->V[X] = rand() & NN;
				state->PC += 2; 
			} 
			break;
		case 0xD000: 
			{
                uint8_t vx = state->V[X];
                uint8_t vy = state->V[Y];
                unsigned height = state->op & 0x000F;  
                state->V[0xF] &= 0;
           
                for(int y = 0; y < height; y++)
                {
                    for(int x = 0; x < 8; x++)
                    {
                    	uint8_t pixel = state->memory[state->I + y];
                        if(pixel & (0x80 >> x)){
                        	int index = (state->V[X] + x) % WIDTH + ((state->V[Y] + y) % HEIGHT) * WIDTH;
                        	if (state->graphics[index] == WHITE)
                        	{
                        		state->V[0xF] = 1;
                        		state->graphics[index] = BLACK;
                        	} 
                        	else
                        	{
                        		state->graphics[index] = WHITE;
                        	}
                        	state->draw = true;
                       	}
                    }
                }
				state->PC += 2;
			}
			break;

		case 0xE000:
			switch(state->op & 0x00FF)
			{
                   	 case 0x009E: // EX9E: Skips the next instruction if the key stored in VX is pressed
				{	
					if(state->keys[X] == 1)
					{
                           			state->PC += 4;
                           			//break;
					}
                        		else
                        		{	
                            		state->PC += 2;
                            			//break;
                           		 }
                       		}
                    		break;
                             
                    	case 0x00A1: // EXA1: Skips the next instruction if the key stored in VX isn't pressed
                    		{
					if(state->keys[X] == 0)
					{
                           			state->PC += 4;
                           			break;
					}
                        		else
                        		{	
                            			state->PC += 2;
                            			break;                            
                        		}
                       		}
                    		break;                         
                	}
			break;

		case 0xF000:
			switch(state->op & 0x00FF)
			{
					
                    case 0x0007:
                    	{ // FX07: Sets VX to the value of the delay timer
                        	state->V[X] = state->delay;
                        	state->PC += 2;
                    	}
                    	break;
       
                    case 0x000A: // FX0A: A key press is awaited, and then stored in VX
                    	{
	                        state->PC -= 2;
	                        for(int i = 0; i < 16; i++)
	                        {
	                            if(state->keys[i] == 1)
	                            {
	                                state->V[X] = i;
	                                state->PC += 2;
	                                printf("%d\n", state->keys[i]);
	                                break;
	                            }
	                        }
                        }    
                    	break;
       
                    case 0x0015: // FX15: Sets the delay timer to VX
                    	{
                        	state->delay = state->V[X];
                        	state->PC += 2;
                    	}
                    	break;
       
                    case 0x0018: // FX18: Sets the sound timer to VX
                    	{
                        	state->PC += 2;
                    	}
                    	break;
       
                    case 0x001E: // FX1E: Adds VX to I
                    	{
                        	state->I += state->V[X];
                        	state->PC += 2;
                    	}
                    	break;

                    case 0x0029: // FX29: Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font
                    	{
	                        state->I = state->V[X] * 5;
	                        state->PC += 2;
                    	}
                    	break;
       
                    case 0x0033: // FX33: Stores the Binary-coded decimal representation of VX, with the most significant of three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2
                        {
	                        state->memory[state->I] = state->V[X] / 100;
	                        state->memory[state->I+1] = (state->V[X] / 10) % 10;
	                        state->memory[state->I+2] = state->V[X] % 10;
	                        state->PC += 2;
                    	}
                    	break;
       
                    case 0x0055: // FX55: Stores V0 to VX in memory starting at address I
                        {
	                        for(int i = 0; i <= (X); i++)
	                        {
	                            state->memory[state->I+i] = state->V[i];
	                        }
	                        state->PC += 2;
                    	}
       		            break;
       
                    case 0x0065: //FX65: Fills V0 to VX with values from memory starting at address I
                    	{
	                        for(int i = 0; i <= (X); i++)
	                        {
	                            state->V[i] = state->memory[state->I + i];
	                        }
	                        state->PC += 2;
                    	}
                    break;
			break;
		}
		timers(state);
	}

	
	return;
}

void GraphicsDraw(struct display display, uint32_t pixels[WIDTH * HEIGHT])
{

	SDL_RenderCopy(display.renderer, display.texture, NULL, NULL);
	SDL_UpdateTexture(display.texture, NULL, pixels, WIDTH * sizeof(uint32_t));
	SDL_RenderPresent(display.renderer);
	SDL_RenderClear(display.renderer);
	

}


int main (int argc, char *argv[])
{
	struct display display;
	uint32_t start_tick;
	uint32_t frame_speed;
	SDL_Event event;

	FILE *f = fopen(argv[1], "rb");
	Chip8State* state = InitChip8(f);
	SDL_Init(SDL_INIT_EVERYTHING);
	display.window = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	display.renderer = SDL_CreateRenderer(display.window, -1, SDL_RENDERER_ACCELERATED); 
	display.texture = SDL_CreateTexture(display.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);

	if (argc < 2)
	{
		printf("Please specify Chip8 ROM file\n");
		return 0;
	}
	
 
	int quit = 0;
	while(quit != 1)
	{
		SDL_PollEvent(&event);
		
		start_tick = SDL_GetTicks();
		frame_speed = SDL_GetTicks() - start_tick;
		
		if (frame_speed < MS_PER_CYCLE)
		{
			SDL_Delay(MS_PER_CYCLE - frame_speed);
		}
		
		switch(event.type)
		{
			case SDL_QUIT:
				quit = 1;
			//TODO: Implement other keys
			break;
		}

		Emulate(state);
		
		if (state->draw = true)
		{
			GraphicsDraw(display, state->graphics);
			state->draw = false;
		}

	}

	SDL_DestroyWindow(display.window);
	SDL_Quit(); 
	return 0;

}
