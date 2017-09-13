#include <stdio.h>
#include <EEPROM.h>

/*
 * Constant definition
 */

// ------------------------------------------------------------
#define LOG_SAVETIME                //If defined, every time the current time is saved a message will be displayed
#define PIN_REDEFINE_TIME          4  //If LOW on startup, sets the timer according to TIME_INIT constants. Otherwise, the Arduino reads time settings from EEPROM.
#define PIN_REDEFINE_ELAPSED_HOURS 5 //If LOW on startup, sets the elapsed hours to zero.

#define TIME_INIT_SECONDS       0 //Clock's initial seconds
#define TIME_INIT_MINUTES       2 //Clock's initial minutes
#define TIME_INIT_HOURS         19 //Clock's initial hours
// ------------------------------------------------------------

#define EEPROM_ADDR_SECONDS     0 //EEPROM's address for storing the seconds
#define EEPROM_ADDR_MINUTES     1 //EEPROM's address for storing the minutes
#define EEPROM_ADDR_DHOURS      2 //EEPROM's address for storing the daily hours
#define EEPROM_ADDR_HOURS       3 //EEPROM's address for storing the total elapsed hours

#define TIME_LAMP_ONTIME        10 //The lamps will go on at 10am every day (time is in 24 hour base)

#define TIME_LAMP               6 //How much time the lamp stays on
#define TIME_PUMP               10 //How much time until the pump goes on

#define TIME_SAVETIME_MINUTES   10 //Every TIME_SAVETIME_MINUTES minutes the current time will be saved to the EEPROM

#define TIME_PUMP_SECONDS       30 //The pump will stay on for these many seconds

#define TIME_MILLI_TO_SECOND    1000 //1000 milliseconds = 1 second
#define TIME_MODULE_SECONDS     60 //60 seconds = 1 minute
#define TIME_MODULE_MINUTES     60 //60 minutes = 1 hour
#define TIME_MODULE_HOURS       24 //24 hours = 1 day

#define PIN_RELAY_LAMP          6 //Digital output pin
#define PIN_RELAY_PUMP          7 //Digital output pin

#define RELAY_LAMP_STATE_ON     LOW   //Relay pin state for turning the light on
#define RELAY_LAMP_STATE_OFF    HIGH  //Relay pin state for turning the light off
#define RELAY_PUMP_STATE_ON     LOW   //Relay pin state for turning the pump on
#define RELAY_PUMP_STATE_OFF    HIGH  //Relay pin state for turning the pump off

#define STR_TIME_LENGTH         255 //how many characters for time string

/*
 * Global variables
 */

unsigned long int counterSeconds = TIME_INIT_SECONDS;
unsigned long int counterMinutes = TIME_INIT_MINUTES;
unsigned long int counterHours = 0;
unsigned long int counterDailyHours = TIME_INIT_HOURS;
unsigned long int lastMillis = 0;

unsigned long int counterPumpSeconds = 0;
unsigned long int counterLampHours = 0;

boolean updateMinutes = false; //Indicates whether a change has happened in seconds. Used to trigger timer updates
boolean updateHours = false; //Indicates whether a change has happened in minutes. Used to trigger timer updates

boolean pumpOn = false; 
boolean lampOn = false;

char txtTime[STR_TIME_LENGTH]; //char buffer for the time string printed via Serial
//String strTime = new String(STR_TIME_LENGTH);

void setup() {
  Serial.begin(9600);
  pinMode(PIN_RELAY_LAMP,OUTPUT);
  pinMode(PIN_RELAY_PUMP,OUTPUT);
  pinMode(PIN_REDEFINE_TIME, INPUT_PULLUP);
  pinMode(PIN_REDEFINE_ELAPSED_HOURS, INPUT_PULLUP);
  lastMillis=millis() + TIME_MILLI_TO_SECOND; //Force initial increment on counterSeconds
  digitalWrite(PIN_RELAY_LAMP,HIGH); //Adicionado por Eduardo
  digitalWrite(PIN_RELAY_PUMP,HIGH); //Adicionado por Eduardo

  if (digitalRead(PIN_REDEFINE_ELAPSED_HOURS) == LOW){
    Serial.println("Elapsed hours variable was reset to zero.");
    EEPROMWriteLong(EEPROM_ADDR_HOURS, counterHours);
  } else {
    counterHours = EEPROMReadlong(EEPROM_ADDR_HOURS);
  }

  if (digitalRead(PIN_REDEFINE_TIME) == LOW){
    saveTime(); //Initial time is already set to the respective variables
    Serial.println("Initial time was reset.");
  } else {
    loadTime();
    Serial.println("Initial time loaded from memory.");
  }

  printTime();
}

void loop() {
  updateMinutes = false;
  updateHours = false;
  
  //TODO Check for overflow on millis()
  if(millis() - lastMillis >= TIME_MILLI_TO_SECOND){
    lastMillis=millis();
    counterSeconds++;
    counterSeconds%=TIME_MODULE_SECONDS;
    updateMinutes = true;
    printTime();

    if(pumpOn)
      controlPump();
  }

  if((!counterSeconds) && (updateMinutes)){
    counterMinutes++;
    counterMinutes%=TIME_MODULE_MINUTES;
    updateHours = true;

    if(counterMinutes % TIME_SAVETIME_MINUTES == 0){
      saveTime();
    }
  }

  if((!counterMinutes) && (updateHours)){
    counterHours++;
    counterDailyHours++;
    counterDailyHours%=TIME_MODULE_HOURS;
    //Let the hours go by indefinitely
    //TODO Check for overflow on counterHours

    if(!(counterHours % TIME_PUMP) || counterHours == 1){ //It's time to activate the pump //Alterado por Eduardo adicionei a condição "ou" couterHours == 1 para a bomba ligar na primeira hora.
      pumpOn = true;
      turnPumpOn();
      counterPumpSeconds = 0;
      Serial.println("Turning pump on");
    }

    if(lampOn)
      controlLamp();

    if(counterDailyHours == TIME_LAMP_ONTIME){ //It's that hour of the day in which the turn the lamp on
      //Turn lamp on
      turnLampOn();
      lampOn = true;
      counterLampHours = 0;
      Serial.println("Turning lamp on");
    }
  }
  
}

void turnLampOn(){
  digitalWrite(PIN_RELAY_LAMP, RELAY_LAMP_STATE_ON);
}

void turnLampOff(){
  digitalWrite(PIN_RELAY_LAMP, RELAY_LAMP_STATE_OFF);
}

void turnPumpOn(){
  digitalWrite(PIN_RELAY_PUMP, RELAY_PUMP_STATE_ON);
}

void turnPumpOff(){
  digitalWrite(PIN_RELAY_PUMP, RELAY_PUMP_STATE_OFF);
}

void printTime(){
  sprintf(txtTime, "%.2lu:%.2lu:%.2lu\nElapsed hours: %lu", counterDailyHours, counterMinutes, counterSeconds, counterHours);
  Serial.println(txtTime);
}

void controlPump(){
  counterPumpSeconds++;
  counterPumpSeconds%=TIME_PUMP_SECONDS;

  if(!counterPumpSeconds){
    pumpOn = false;
    turnPumpOff();
    Serial.println("Turning pump off");
  }  
}

void controlLamp(){
  counterLampHours++;
  counterLampHours%=TIME_LAMP;

  if(!counterLampHours){
    lampOn = false;
    turnLampOff();
    Serial.println("Turning lamp off");
  }  
}

void saveTime(){  
  EEPROM.update(EEPROM_ADDR_SECONDS, counterSeconds);
  EEPROM.update(EEPROM_ADDR_MINUTES, counterMinutes);
  EEPROM.update(EEPROM_ADDR_DHOURS, counterDailyHours);
  EEPROMWriteLong(EEPROM_ADDR_HOURS, counterHours);

  #ifdef LOG_SAVETIME
    Serial.println("Current time saved to EEPROM.");
  #endif
}

void loadTime(){
  counterSeconds = EEPROM.read(EEPROM_ADDR_SECONDS);
  counterMinutes = EEPROM.read(EEPROM_ADDR_MINUTES);
  counterDailyHours = EEPROM.read(EEPROM_ADDR_DHOURS);
  counterHours = EEPROMReadlong(EEPROM_ADDR_HOURS);
}

void EEPROMWriteLong(unsigned int address, unsigned long int value)
{
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
  
  //Write the 4 bytes into the eeprom memory.
  EEPROM.update(address, four);
  EEPROM.update(address + 1, three);
  EEPROM.update(address + 2, two);
  EEPROM.update(address + 3, one);
}

unsigned long int EEPROMReadlong(unsigned int address)
{
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  
  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
