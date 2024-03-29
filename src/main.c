#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "ADC.h"
#include "pushbuttons.h"
#include "lcd.h"
#include "servo.h"
#include "timer.h"
#include "loadcell.h"
#include "relay.h"

#define ON 1
#define OFF 0
#define ALARM_EN 1

// GLOBAL VARIABLE DEFINITIONS

// MENU VARS
unsigned char key, c, tt;

// WEIGHT
unsigned short CURRENT_Weight = 60; // Current measured weight
unsigned char ALARM_Weight;         // This is set if weight exceeds threshold
unsigned short OCCUPANCY_Time = 0;  // Time current weight is above zero in seconds (for test purposes)
#define MAX_Weight 150              // if exceeded alarm weight is initiated

// TEMPERATURE
unsigned short BODY_Temp = 37;        // Body Temperature (sensor1 at ADC A2)
unsigned char ALARM_Fever = 0;        // This is set if body temp is above 37
unsigned short ROOM_Temp = 24;        // Room Temperature (sensor 2 at ADC A3)
unsigned short HEATER_Threshold = 10; // Temperature to be compared with ROOM_Temp for heater relay control, is set by LCD menu
unsigned char HEATER_Enable = 1;      // Is heater enabled? (done from lcd menu)
unsigned char HEATER_State = 0;       // If set, heater relay is turned on

// LAMP
unsigned char LAMP_Enable = 1; // Is lamp enabled (done from lcd menu)
unsigned char LAMP_State = 1;  // If set, lamp relay is turned on

// TODO: Decide on mode change logic

// IF MODE OLD NOT EQUAL NEW THEN MODE CHANGE, BASED ON MODE NEW
// DEFAULT MODE SITTING
unsigned char MODE_Old = 0; // 0 FOR SITTING 1 FOR SLEEPING
// 0 FOR SITTING 1 FOR SLEEPING
unsigned char MODE_New = 0; // 0 FOR SITTING 1 FOR SLEEPING

// TIMER VARS
unsigned char TIMER0_Counter = 0;  // counter to help increase the timer interrupt to 100ms
unsigned char TIMER0_Counter2 = 0; // second counter for 1 sec refresh rate
unsigned char TIMER0_Counter3 = 0;
#define TIMER0_Counter_100ms 6 // 16ms * 6 = 96ms
#define TIMER0_Counter_1s 64

// ON MODE CHANGE TO WAKE UP
void WAKE_Start(void)
{
  // SERVO FRONT
  SERVO_On(2);
  _delay_ms(500);
  SERVO_Off();

  // LIGHT ON
  LAMP_State = 1;

  // EQUATE MODES

  MODE_Old = MODE_New;
}

void SLEEP_Start(void)
{
  // SERVO LAID BACK
  SERVO_On(2);
  _delay_ms(500);
  SERVO_Off();
  // LIGHT OFF
  LAMP_State = 0;

  // EQUATE MODES
  MODE_Old = MODE_New;
}

// INTERRUPT FUNCTION EACH 16ms
ISR(TIMER0_OVF_vect)
{
  TIMER0_Counter++;
  TIMER0_Counter2++;

  // ENTERS EACH 100ms
  if (TIMER0_Counter == TIMER0_Counter_100ms)
  {
    // START

    // ------------WEIGHT------------------//
    // Refresh current weight from adc
    CURRENT_Weight = LOADCELL_ReadWeight() / 3;
    // Check if max rated weight exceeded
    if (CURRENT_Weight > MAX_Weight)
    {
      ALARM_Weight = 1;
    }
    else if (CURRENT_Weight > 10) // if weight within operating range
    {
      OCCUPANCY_Time++; // each 100ms
      ALARM_Weight = 0;
    }
    else
    {
      OCCUPANCY_Time = 0; // if not used
      ALARM_Weight = 0;
    }

    //-------------TEMPERATURE-----------//
    BODY_Temp = (unsigned char)(((ADC_Read(2) * (5.0f / 1024) * 1000)) / 10); //
    ROOM_Temp = (unsigned char)(((ADC_Read(3) * (5.0f / 1024) * 1000)) / 10); //

    if (BODY_Temp > 37)
    {
      ALARM_Fever = 1;
    }
    else
    {
      ALARM_Fever = 0;
    }

    if ((ROOM_Temp < HEATER_Threshold) && HEATER_Enable)
    {
      HEATER_State = 1;
    }
    else
    {
      HEATER_State = 0;
    }

    // END of scope (100ms refresh)
    TIMER0_Counter = 0;
  }

  // START OF 1SEC REFRESH

  if (TIMER0_Counter2 == TIMER0_Counter_1s)
  {

    // If a change in modes occurs the corresponding functions will be called
    if (MODE_Old != MODE_New)
    {
      if (MODE_New == 1)
      {
        SLEEP_Start();
      }
      if (MODE_New == 0)
      {
        WAKE_Start();
      }
    }
    // TEMPERATURE AND LIGHTING OUTPUTS
    if (HEATER_State == 1 && HEATER_Enable == 1)
    {
      RELAY_Heater(ON);
    }
    else
    {
      RELAY_Heater(OFF);
    }

    if (LAMP_State == 1 && LAMP_Enable == 1)
    {
      RELAY_Lamp(ON);
    }
    else
    {
      RELAY_Lamp(OFF);
    }
    // 10 second alarm trigger
    TIMER0_Counter3++;
    if (TIMER0_Counter3 == 10)
    {
      if (ALARM_Fever == 1 && ALARM_EN) // TODO: Add snooze counter
      {
        alarm_fever();
        ALARM_Fever = 0; // RESET ALARM FLAG
      }
      if (ALARM_Weight == 1 && ALARM_EN)
      {
        alarm_max_weight();
        ALARM_Weight = 0;
      }

      TIMER0_Counter3 = 0;
    }

    // END OF 1 SEC SCOPE
    TIMER0_Counter2 = 0;
  }
}

#define DEBUGMODE 0
#if DEBUGMODE

int main(void)
{
  unsigned char buttonpressed;

  ADC_Init();
  TIMER0_Init();
  LOADCELL_Init();
  PUSHBUTTONS_Init();
  LCD_Init();
  RELAY_Init();

  // dont forget to enable these when debugging because the ISR wont allow RELAY_Lamp enable without them
  // LAMP_State = 1;
  LAMP_Enable = 1;

  HEATER_Enable = 1;
  // HEATER_State = 1;
  lcd_send_number(58);
  _delay_ms(200);
  LCD_SendCommand(1);
  lcd_send_number(148);
  _delay_ms(200);
  LCD_SendCommand(1);
  lcd_send_number(5);
  _delay_ms(200);
  LCD_SendCommand(1);

  while (1)
  {
    lcd_send_number(58);
    _delay_ms(500);
    LCD_SendCommand(1);

    lcd_send_number(148);
    _delay_ms(500);
    LCD_SendCommand(1);

    lcd_send_number(5);
    _delay_ms(500);
    LCD_SendCommand(1);
  }

  return 0;
}

#else

unsigned char key, c, tt; // define variables key for pushed button//c for counting

void alarm_fever(void)
{
  LCD_SendCommand(1);
  lcd_sendstring("HIGH FEVER!");
  _delay_ms(200);
  BUZZER_Pulse_ms(500);
  LCD_SendCommand(1);
}
void alarm_max_weight(void)
{

  LCD_SendCommand(1);
  lcd_sendstring("MAX WEIGHT");
  _delay_ms(200);
  BUZZER_Pulse_ms(500);
  LCD_SendCommand(1);
}
// frame 1 in sleep mode LOADING
void sleep1(void)
{

  lcd_sendstring(" sleeping..");
  _delay_ms(2000);
  LCD_SendCommand(1);
  lcd_sendstring(" body temp:");
  lcd_send_number(BODY_Temp);
  lcd_setcursor(1, 0);
  lcd_sendstring("1:roomtmp ");
  lcd_sendstring("2:home ");
}
// frame 2 in sleep mode ROOM TEMPERATURE
void sleep2(void)
{
  // TODO: keep checking on ROOM_Temp variable
  LCD_SendCommand(1);
  lcd_sendstring(" room temp:");
  lcd_send_number(ROOM_Temp);
  lcd_setcursor(1, 0);
  lcd_sendstring(" 1:weight");
  lcd_sendstring(" 2:home ");
}
// frame 3 in sleep mode  CURENT WEIGHT
void sleep3(void)
{
  // TODO: keep checking on CURRENT_Weight variable
  LCD_SendCommand(1);
  lcd_sendstring(" weight:");
  lcd_send_number(CURRENT_Weight);
  lcd_setcursor(1, 0);
  lcd_sendstring("1:occ time");
  lcd_sendstring("2:home ");
}
void sleep4(void) // frame 4 in sleep mode SLEEP TIME (should be occupancy)
{
  // TODO: keep checking on OCCUPANCY_Time variable
  LCD_SendCommand(1);
  lcd_sendstring(" occupy time:");
  lcd_send_number(OCCUPANCY_Time);
  lcd_setcursor(1, 0);
  lcd_sendstring(" 2:home ");
}
void sit1(void) // frame 1 in sitting mode HOME MENU
{
  LCD_SendCommand(1);
  lcd_sendstring(" sitting..");

  _delay_ms(2000);
  LCD_SendCommand(1);
  lcd_sendstring(" options");
  lcd_setcursor(1, 0);
  lcd_sendstring(" 1:next");
  lcd_sendstring(" 2:home ");
}
void sit2(void) // frame 2 in sitting mode HEATER ENABLE/DISABLE
{
  LCD_SendCommand(1);
  lcd_sendstring(" heating");
  lcd_setcursor(1, 0);
  lcd_sendstring(" 1:on");
  lcd_sendstring(" 2:off ");
}
void sit3(void) // frame 3 in sitting mode HEATER ON SELECT TEMP
{
  c = 0;
  LCD_SendCommand(1);
  lcd_sendstring(" heat temp");
  lcd_setcursor(1, 0);
  lcd_sendstring(" put temp:");
}
void sit4() // frame 4 in sitting mode LAMP ENABLE
{
  LCD_SendCommand(1);
  lcd_sendstring(" lamp enable");
  lcd_setcursor(1, 0);
  lcd_sendstring(" 1:on");
  lcd_sendstring("  2:off ");
}

unsigned char choose(void) // polling function to w8 user to press key
{
  do
  {

    key = PUSHBUTTONS_Read();

  } while (key == 0xff);
  return key;
}
int main(void)
{
  ADC_Init();
  TIMER0_Init();
  LOADCELL_Init();
  PUSHBUTTONS_Init();
  LCD_Init();
  RELAY_Init();
  SERVO_Init();
  BUZZER_Init();

  unsigned char mode = 5, Pass = 0, ff = 0;
  lcd_setcursor(0, 4);
  lcd_sendstring("WELCOME!");
  _delay_ms(300);
  LCD_SendCommand(1);
  lcd_setcursor(0, 10);
  lcd_sendstring(" For Login");
  lcd_setcursor(1, 2);
  lcd_sendstring("Press :  1");
  key = choose(); // wait to check pressed button from the user
  if (key == 1)
  {
    LCD_SendCommand(1);
    while (Pass != 4) // will be in the loop while the password is not correct
    {
      ff = 0;
      LCD_SendCommand(1);
      lcd_sendstring(" USER : Hassan");
      lcd_setcursor(1, 0);
      lcd_sendstring("PASS : ");
      while (ff != 4) // make password from 4 digit
      {
        key = choose();
        LCD_SendData(key + '0'); // to recive correct number in ascii code
        Pass += key;
        ff++;
        if (ff == 4)
          _delay_ms(200);
      }
      ff = 0;
      if (Pass != 4)
      {
        Pass = 0;
        LCD_SendCommand(1);
        lcd_sendstring(" wrong pass");
        lcd_setcursor(1, 3);
        lcd_sendstring(" try again");
        _delay_ms(200);
      }
    }
    LCD_SendCommand(1);
    mode = 5;
  }

  else // if user does not want to login so end the program
  {
    LCD_SendCommand(1);
    mode = 4;
    lcd_sendstring(" good bye");
    _delay_ms(200);
    LCD_SendCommand(1);
  }
  // _delay_ms(200);//////////////////////////////////////

  // LCD_Print("hello");
  //_delay_ms(200);
  // LCD_SendCommand(1);
  // _delay_ms(200);

  while (mode == 5) // main function after user is allowed in
  {
    LCD_SendCommand(1); // make user choose between 2 modes we have in our program
    lcd_sendstring(" 1:for sleep mode");
    lcd_setcursor(1, 0);
    lcd_sendstring(" 2:for sit mode");
    mode = choose();
    LCD_SendCommand(1);
    if (mode == 1) // if user choose sleep mode
    {
      MODE_New = 1;
      sleep1();
      mode = choose();
      if (mode == 1) // user decides to proceed 1
      {
        sleep2();
        mode = choose();

        if (mode == 1) // user decides to proceed 2
        {
          sleep3();
          mode = choose();
          if (mode == 1) // user decides to proceed 3
          {
            sleep4();
            mode = choose();
            if (mode == 2) // user want to return home 3
            {
              mode = 5;
            }
          }
          else if (mode == 2) // user want to return home 2
          {
            mode = 5;
          }
        }

        else if (mode == 2) // user want to return home 1
        {

          mode = 5;
        }
      }

      else if (mode == 2) // if user choose home 0
      {
        mode = 5;
      }
    }
    else if (mode == 2) // user choose sitting mode
    {                   // last if condition
      MODE_New = 0;     // enable sitting globally (will be read by interrupt)
      sit1();
      mode = choose();
      if (mode == 1) // user want to proceed 1
      {

        sit2();
        mode = choose();
        if (mode == 1) // user want to proceed 2
        {
          HEATER_Enable = 1;
          LCD_SendCommand(1);
          lcd_sendstring("heater on");
          _delay_ms(200);
          LCD_SendCommand(1);
          sit3();
          while (c != 2) // wait user to set temp then proceed auto to next step so here user has no option to return home
          {
            key = choose();
            LCD_SendData(key + '0');
            HEATER_Threshold = 0;
            if (c == 0)
            {
              HEATER_Threshold = key * 10;
              tt = (key + '0') * 10;
            }
            _delay_ms(200);
            if (c == 1)
            {
              HEATER_Threshold += key;
              tt += (key + '0');
            }
            c++;
            if (c == 2)
            {
              _delay_ms(100);
            }
          }
          sit4(); // proceed to final frame in sitting mode
          mode = choose();
          if (mode == 1)
          {
            LAMP_Enable = 1;
            LAMP_State = 1;
            LCD_SendCommand(1);
            mode = 5; // make mode 5 to return home after outing from this function
            lcd_sendstring("lamp on");
            _delay_ms(200);
          }
          else if (mode == 2)
          {
            LAMP_Enable = 0;
            LAMP_State = 0;
            LCD_SendCommand(1);
            mode = 5; // make mode 5 to return home after outing from this function
            lcd_sendstring("lamp off");

            _delay_ms(200);
          }
        }
        else if (mode == 2)
        {
          HEATER_Enable = 0;
          LCD_SendCommand(1);
          lcd_sendstring("heater off");
          _delay_ms(200);
          LCD_SendCommand(1);
          sit4(); // proceed to final frame in sitting mode
          mode = choose();
          if (mode == 1)
          {
            LAMP_Enable = 1;
            LAMP_State = 1;
            LCD_SendCommand(1);
            mode = 5; // make mode 5 to return home after outing from this function
            lcd_sendstring("lamp on");
            _delay_ms(200);
          }
          else if (mode == 2)
          {
            LAMP_Enable = 0;
            LAMP_State = 0;
            LCD_SendCommand(1);
            mode = 5; // make mode 5 to return home after outing from this function
            lcd_sendstring("lamp off");
            _delay_ms(200);
          }
        }
      }
      else if (mode == 2)
      {
        mode = 5;
      }
    }
  }
}

/*unsigned char key;

void sleep1(void)
{
  lcd_sendstring("sleeping...");
  _delay_ms(200);
  LCD_SendCommand(1);
  lcd_sendstring("body temp:37");
  lcd_setcursor(1, 0);
  lcd_sendstring("1:roomtemp");
  lcd_sendstring("2:home ");
}
void sleep2(void)
{
  LCD_SendCommand(1);
  lcd_sendstring("room temp:27");
  lcd_setcursor(1, 0);
  lcd_sendstring("1:weight");
  lcd_sendstring(" 2:home ");
}
void sleep3(void)
{
  LCD_SendCommand(1);
  lcd_sendstring("weight:60");
  lcd_setcursor(1, 0);
  lcd_sendstring("1:sleeptime");
  lcd_sendstring("2:home ");
}
void sleep4(void)
{
  LCD_SendCommand(1);
  lcd_sendstring("sleepingtime:8hours");
  lcd_setcursor(1, 0);
  lcd_sendstring("2:home ");
}

// temporary wrapper function for keypad/pushbuttons

unsigned char choose(void)
{
  do
  {
    key = PUSHBUTTONS_read();

  } while (key == 0xff);
  return key;
}

int main(void)
{
  //_delay_ms(200);
  unsigned char mode = 5;
  LCD_Init();
  PUSHBUTTONS_init();

  lcd_sendstring("Starting up.");
  _delay_ms(2000);
  LCD_SendCommand(1);
  _delay_ms(100);

  lcd_sendstring("Starting up..");
  _delay_ms(2000);
  LCD_SendCommand(1);

  while (mode == 5)
  {
    LCD_SendCommand(1);
    lcd_sendstring("1:for sleep mode");
    lcd_setcursor(1, 0);
    lcd_sendstring("2:for sit mode");
    mode = choose();
    LCD_SendCommand(1);
    if (mode == 1)
    {
      sleep1();
      mode = choose();
      if (mode == 1)
      {
        sleep2();
        mode = choose();

        if (mode == 1)
        {
          sleep3();
          mode = choose();
          if (mode == 1)
          {
            sleep4();
            mode = choose();
            if (mode == 2)
            {
              mode = 5;
            }
          }
          else if (mode == 2)
          {
            mode = 5;
          }
        }

        else if (mode == 2)
        {

          mode = 5;
        }
      }

      else if (mode == 2)
      {
        mode = 5;
      }
    }
    else if (mode == 2)
    { // last if condition
      mode = 5;
    }
  }
}*/

#endif