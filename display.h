/*
Copyright (c) 2014-  by John Chandler

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

typedef struct alt_font {
        char    offset;                 // Offset
        char    hori;                   // Horizontal size
        char    vert;                   // Vertical size
        char    bpl;                    // bytes per line of font
        char    font[5][1+9];
        } alt_font;

#define Init_display()	\
		SSD1331_init()
#define Print_icon(icon, x, y, colour) \
		SSD1331_foreground(colour);	\
		SSD1331_locate(x, y);		\
		SSD1331_set_font((unsigned char *)&symbol_font);	\
		SSD1331_print(icon);		\
		SSD1331_set_font(NULL)
#define Print_text(text, size, x, y, colour) \
		SSD1331_foreground(colour);	\
		SSD1331_locate(x, y);		\
		SSD1331_SetFontSize(size);	\
		SSD1331_print(text)
#define Print_time(size, x, y, colour)		\
	   {					\
	   time_t seconds;			\
	   char Time[10];			\
		seconds = time(NULL);		\
		strftime(Time,40,"%H:%M %a", localtime(&seconds));	\
		Print_text(Time, size, x, y, colour);\
	   }
#define	Display_on()	\
		SSD1331_on()
#define	Display_off()	\
		SSD1331_off()
#define	Display_cls()	\
		SSD1331_cls()
#define Display_clearline(line) \
		SSD1331_fillrect(0, line, width, line+Y_height, Black, Black)
#define Display_clearlinesto(line, toline) \
		SSD1331_fillrect(0, line, width, toline, Black, Black)

void	display_process();			// Main display process
