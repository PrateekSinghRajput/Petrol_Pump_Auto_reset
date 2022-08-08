
//Auto Reset

#include <Wire.h>
#include<LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);
#include "RTClib.h"
RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

//-------Pins-----//
int Relay = 4;                 //Solenoid valve open/close
int start_stop = 13;             //Start/Stop button
int rst_sp = 12;                 // Reset Set Point Button
int rst_cnt = 11;               // Reset counter button
int unit = 10;                  // Change Unit Button
const int sensor_pulse = A0;     // Sensor Pulse In
//----Analog as Input-----//
int add_one = 9;               // +1 Button
int add_ten = 8;               // +10 Button
int add_cien = 7;              // +100 Button
int add_mil = 6;               // +1000 Buton

//-----Variables for debouncing-----//
boolean currentstart_stop = LOW;
boolean laststart_stop = LOW;
boolean lastsensor_pulse = LOW;
boolean currentsensor_pulse = LOW;
boolean lastunit = LOW;
boolean currentunit = LOW;
boolean lastrst_sp = LOW;
boolean currentrst_sp = LOW;
boolean lastrst_cnt = LOW;
boolean currentrst_cnt = LOW;
boolean lastadd_one = LOW;
boolean currentadd_one = LOW;
boolean lastadd_ten = LOW;
boolean currentadd_ten = LOW;
boolean lastadd_cien = LOW;
boolean currentadd_cien = LOW;
boolean lastadd_mil = LOW;
boolean currentadd_mil = LOW;

//-----Storage state for toggle function---//
boolean unitState = LOW;                  //storage for the current state of the unit
boolean RelayState = LOW;                //storage for the current state of the Relay (off/on)

//-------You have to put your pulses x liters here-----//
float cal_1 = 2.5;                      //Calibrate ml x pulse (cal_1 = 1000/400)
int cal_2 = 40;                        //Calibrate pulses x liters
//-----------------------------------------------------//

float counter_1 = 0;
int counter_2 = 0;
int TotalCount_1 = 0;
int TotalCount_2 = 0;
int set_point_1 = 0;
int set_point_2 = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  pinMode(Relay, OUTPUT);     //Pin 13
  pinMode(add_one, INPUT);    //A2 as Input
  pinMode(add_ten, INPUT);    //A3 as Input
  pinMode(add_cien, INPUT);   //A4 as Input
  pinMode(add_mil, INPUT);    //A5 as Input

  lcd.setCursor(0, 2); //Show "SP" on the LCD
  lcd.print("SET V");
  lcd.setCursor(0, 3); //Show "CNT" on the LCD
  lcd.print("OUT V");

  if (! rtc.begin())
  {
    lcd.print("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning())
  {
    lcd.print("RTC is NOT running!");
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));//auto update from computer time
  //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));// to set the time manualy

}
//----Debouncing function for all buttons----//
boolean debounce(boolean last, int pin)
{
  boolean current = digitalRead(pin);
  if (last != current)
  {
    delay(5);
    current = digitalRead(pin);
  }
  return current;
}

void loop() {
  //-----Debounce Buttons-----//
  DateTime now = rtc.now();
  lcd.setCursor(6, 0);
  lcd.print(now.hour());
  lcd.print(':');
  lcd.print(now.minute());
  lcd.print(':');
  lcd.print(now.second());
  lcd.print("   ");

  lcd.setCursor(4, 1);
  lcd.print(daysOfTheWeek[now.dayOfTheWeek()]);
  lcd.print(" ,");
  lcd.print(now.day());
  lcd.print('/');
  lcd.print(now.month());
  lcd.print('/');
  lcd.print(now.year());
  currentstart_stop = debounce(laststart_stop, start_stop);    //Debounce for Start/Stop Button
  currentsensor_pulse = debounce(lastsensor_pulse, sensor_pulse); //Debounce for Sensor
  currentunit = debounce(lastunit, unit);             //Debounce for unit Button
  currentrst_sp = debounce(lastrst_sp, rst_sp);       //Debounce for reset set point Button
  currentrst_cnt = debounce(lastrst_cnt, rst_cnt);    //Debounce for reset counter Button
  currentadd_one = debounce(lastadd_one, add_one);    //Debounce for +1 Button
  currentadd_ten = debounce(lastadd_ten, add_ten);    //Debounce for +10 Button
  currentadd_cien = debounce(lastadd_cien, add_cien); //Debounce for +100 Button
  currentadd_mil = debounce(lastadd_mil, add_mil);    //Debounce for +1000 Button


  //-----Start/Stop toggle function----//
  if (currentstart_stop == HIGH && laststart_stop == LOW) {

    if (RelayState == HIGH) {        //Toggle the state of the Relay
      digitalWrite(Relay, LOW);
      RelayState = LOW;
    }
    else {
      digitalWrite(Relay, HIGH);
      RelayState = HIGH;
    }
  }

  laststart_stop = currentstart_stop;

  //-------Unit toggle function----//
  if (RelayState == LOW) {          //You only can change unit while system is not running!

    //------ Lt/ml unit toggle function----//
    if (currentunit == HIGH && lastunit == LOW) {
      lcd.setCursor(6, 3);        //Clear lcd(CNT area) between unit change,keeping last count
      lcd.print("          ");
      lcd.setCursor(6, 2);       //Clear lcd (SP area) between unit change, keeping last SP
      lcd.print("           ");

      if (unitState == HIGH) {        //Toggle the state of the unit (L/ml)
        digitalWrite(unit, LOW);
        unitState = LOW;
      }
      else {
        digitalWrite(unit, HIGH);
        unitState = HIGH;
      }
    }
    lastunit = currentunit;
  }
  //------Print unit state-----//
  if (unitState == HIGH) {    //Unit state HIGH = L
    lcd.setCursor(14, 2);
    lcd.print("Litter");
    lcd.setCursor(14, 3);
    lcd.print("Litter");

  }
  else {                      //Unit state LOW = ml
    lcd.setCursor(15, 2);
    lcd.print("Price");
    lcd.setCursor(15, 3);
    lcd.print("Price");
  }//End Print unit state

  //--------------------------//
  //------------Ml Counter-----//
  //---------------------------//
  if (unitState == LOW) {  // LOW= Ml state

    //-----------------------//
    //-----Settings----------//
    //----------------------//

    if (RelayState == LOW) {   // You only can change settings while system is not running!

      //-----Adders Buttons (set point_1)---//

      if (currentadd_ten == HIGH && lastadd_ten == LOW) { // Add +10
        set_point_1 = set_point_1 + 10;
      }
      lastadd_ten = currentadd_ten;

      if (currentadd_cien == HIGH && lastadd_cien == LOW) { // Add +100
        set_point_1 = set_point_1 + 100;
      }
      lastadd_cien = currentadd_cien;

      if (currentadd_mil == HIGH && lastadd_mil == LOW) { // Add +1000
        set_point_1 = set_point_1 + 1000;
      }
      lastadd_mil = currentadd_mil;

      //-------Reset Buttons----//
      if (currentrst_sp == HIGH && lastrst_sp == LOW) { //Reset Set Point
        lcd.setCursor(6, 2);         // Clear SP area
        lcd.print("          ");
        set_point_1 = 0;
      }
      lastrst_sp = currentrst_sp;
      if (currentrst_cnt == HIGH && lastrst_cnt == LOW) { //Reset Counter
        lcd.setCursor(6, 3);         // Clear CNT area
        lcd.print("         ");
        counter_1 = 0;
        TotalCount_1 = 0;
      }
      lastrst_cnt = currentrst_cnt;
    }//-----End Settings-----//

    //----Start Counter------//
    if (RelayState == HIGH) {  // Only counts while relay is HIGH
      if (lastsensor_pulse == LOW && currentsensor_pulse == HIGH) {
        counter_1 = counter_1 + cal_1;
      }
    }
    lastsensor_pulse = currentsensor_pulse;

    //-------Counter function-----//
    if (counter_1 >= 10) {
      TotalCount_1 = TotalCount_1 + 10;
      counter_1 = 0;                   //Counter  reset
    }

    lcd.setCursor(6, 2);           //Show set point
    lcd.print(set_point_1);
    lcd.setCursor(6, 3);           // Show counter
    lcd.print(TotalCount_1);

    //--Stop Counter.You can´t start if set point is lower or equal to counter--//
    if (set_point_1 <= TotalCount_1) {
      RelayState = LOW;
      digitalWrite(Relay, LOW);
    }

  }//End unit state LOW (ml)

  //--------------------------//
  //------------Lt Counter-----//
  //---------------------------//

  if (unitState == HIGH) {   //HIGH = Lt state

    //-----------------------//
    //-----Settings----------//
    //----------------------//

    if (RelayState == LOW) {   // You only can change settings while system is not running!

      //-----Adders Buttons (set point_2)---//
      if (currentadd_one == HIGH && lastadd_one == LOW) { // Add +1
        set_point_2 = set_point_2 + 1;
      }
      lastadd_one = currentadd_one;

      if (currentadd_ten == HIGH && lastadd_ten == LOW) { // Add +10
        set_point_2 = set_point_2 + 10;
      }
      lastadd_ten = currentadd_ten;

      if (currentadd_cien == HIGH && lastadd_cien == LOW) { // Add +100
        set_point_2 = set_point_2 + 100;
      }
      lastadd_cien = currentadd_cien;

      if (currentadd_mil == HIGH && lastadd_mil == LOW) { // Add +1000
        set_point_2 = set_point_2 + 1000;
      }
      lastadd_mil = currentadd_mil;

      //-------Reset Buttons----//
      if (currentrst_sp == HIGH && lastrst_sp == LOW) { //Reset Set Point
        lcd.setCursor(6, 2);         // Clear SP area
        lcd.print("          ");
        set_point_2 = 0;
      }
      lastrst_sp = currentrst_sp;
      if (currentrst_cnt == HIGH && lastrst_cnt == LOW) { //Reset Counter
        lcd.setCursor(6, 3);         // Clear CNT area
        lcd.print("         ");
        counter_2 = 0;
        TotalCount_2 = 0;
      }
      lastrst_cnt = currentrst_cnt;
    }//-----End Settings-----//

    //----Start Counter------//
    if (RelayState == HIGH) {  // Only counts while relay is HIGH
      if (lastsensor_pulse == LOW && currentsensor_pulse == HIGH) {
        counter_2 = counter_2 + 1;
      }
    }
    lastsensor_pulse = currentsensor_pulse;

    //-------Counter function-----//
    if (counter_2 == cal_2) {
      TotalCount_2 = TotalCount_2 + 1;
      counter_2 = 0;                     //Counter  reset
    }

    lcd.setCursor(6, 2);           //Show set point
    lcd.print(set_point_2);
    lcd.setCursor(6, 3);           // Show counter
    lcd.print(TotalCount_2);

    //--Stop Counter.You can´t start if set point is lower or equal to counter--//
    if (set_point_2 <= TotalCount_2) {
      RelayState = LOW;
      digitalWrite(Relay, LOW);
    }


  }//End unit state HIGH (L)



}//End Void Loop
