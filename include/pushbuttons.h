#ifndef _PUSHBUTTONS_H
#define _PUSHBUTTONS_H

// PIN DEFINITIONS
#define PUSHBUTTON_PRT 'B'
#define PUSHBUTTON_PIN_UP 0 //6
#define PUSHBUTTON_PIN_DN 1 //5
#define PUSHBUTTON_PIN_LEFT 2 //4
#define PUSHBUTTON_PIN_RIGHT 3 //2

#define DEBOUNCE_DELAY_MS 20
#define KEYPAD_NO_PRESSED_KEY 0xff
void PUSHBUTTONS_init(void);

unsigned char PUSHBUTTONS_read(void);

#endif