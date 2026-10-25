/*
 * MIT License
 *
 * Copyright (c) 2023-2024 CNLohr <lohr85@gmail.com>, et. al.
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Copyright (c) 2023-2024 E. Brombaugh
 * Copyright (c) 2023-2024 A. Mandera
 * Copyright (c) 2005-2020 Rich Felker, et al.
 * Copyright (c) 2013,2014 Michal Ludvig <michal@logix.cz>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Adapted from ch32fun 6b247c7; see THIRD_PARTY_NOTICES.md. */
/*
 * Example for using I2C with 128x32 graphic OLED
 * 03-29-2023 E. Brombaugh
 */

// what type of OLED - uncomment just one
// #define SSD1306_64X32
// #define SSD1306_72X40
#define SSD1306_128X32
//#define SSD1306_128X64

#include "esp32fun.h"
#define printf fun_printf
#include "ssd1306_i2c.h"
#include "ssd1306.h"

#include "bomb.h"

int main()
{
	// 48MHz internal clock
	SystemInit();

	Delay_Ms( 100 );
	printf("\r\r\n\ni2c_oled example\n\r");

	// init i2c and oled
	Delay_Ms( 100 );	// give OLED some more time
	printf("initializing i2c oled...");
	if(!ssd1306_i2c_init())
	{
		if (ssd1306_init()) { fun_puts("OLED initialization failed"); return 1; }
		printf("done.\n\r");

		printf("Looping on test modes...");
		while(1)
		{
			for(uint8_t mode=0;mode<(SSD1306_H>32?9:8);mode++)
			{
				// clear buffer for next mode
				ssd1306_setbuf(0);

				switch(mode)
				{
					case 0:
						printf("buffer fill with binary\n\r");
						for(int i=0;i<(int)sizeof(ssd1306_buffer);i++)
							ssd1306_buffer[i] = i;
						break;

					case 1:
						printf("pixel plots\n\r");
						for(int i=0;i<SSD1306_W;i++)
						{
							ssd1306_drawPixel(i, i/(SSD1306_W/SSD1306_H), 1);
							ssd1306_drawPixel(i, SSD1306_H-1-(i/(SSD1306_W/SSD1306_H)), 1);
						}
						break;

					case 2:
						{
							printf("Line plots\n\r");
							uint8_t y= 0;
							for(uint8_t x=0;x<SSD1306_W;x+=16)
							{
								ssd1306_drawLine(x, 0, SSD1306_W, y, 1);
								ssd1306_drawLine(SSD1306_W-x, SSD1306_H, 0, SSD1306_H-y, 1);
								y+= SSD1306_H/8;
							}
						}
						break;

					case 3:
						printf("Circles empty and filled\n\r");
						for(uint8_t x=0;x<SSD1306_W;x+=16)
							if(x<64)
								ssd1306_drawCircle(x, SSD1306_H/2, 15, 1);
							else
								ssd1306_fillCircle(x, SSD1306_H/2, 15, 1);
						break;
					case 4:
						printf("Image\n\r");
						ssd1306_drawImage(0, 0, bomb_i_stripped, 32, 32, 0);
						break;
					case 5:
						printf("Unscaled Text\n\r");
						ssd1306_drawstr(0,0, "This is a test", 1);
						ssd1306_drawstr(0,8, "of the emergency", 1);
						ssd1306_drawstr(0,16,"broadcasting", 1);
						ssd1306_drawstr(0,24,"system.",1);
						if(SSD1306_H>32)
						{
							ssd1306_drawstr(0,32, "Lorem ipsum", 1);
							ssd1306_drawstr(0,40, "dolor sit amet,", 1);
							ssd1306_drawstr(0,48,"consectetur", 1);
							ssd1306_drawstr(0,56,"adipiscing",1);
						}
						ssd1306_xorrect(SSD1306_W/2, 0, SSD1306_W/2, SSD1306_W);
						break;

					case 6:
						printf("Scaled Text 1, 2\n\r");
						ssd1306_drawstr_sz(0,0, "sz 8x8", 1, fontsize_8x8);
						ssd1306_drawstr_sz(0,16, "16x16", 1, fontsize_16x16);
						break;

					case 7:
						printf("Scaled Text 4\n\r");
						ssd1306_drawstr_sz(0,0, "32x32", 1, fontsize_32x32);
						break;


					case 8:
						printf("Scaled Text 8\n\r");
						ssd1306_drawstr_sz(0,0, "64", 1, fontsize_64x64);
						break;

					default:
						break;
				}
				ssd1306_refresh();

				Delay_Ms(2000);
			}
		}
	}
	else
		printf("failed.\n\r");

	printf("Stuck here forever...\n\r");
	while(1);
}
