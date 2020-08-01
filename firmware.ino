#define emonTxV3                                                      
#define RF69_COMPAT 1                                                

#include <JeeLib.h>                                                 
#include <avr/pgmspace.h>
#include <util/parity.h>
ISR(WDT_vect) { Sleepy::watchdogEvent(); }                           

#include "EmonLib.h"                                                 
EnergyMonitor ct1, ct2;       

#include <OneWire.h>                                                 
#include <DallasTemperature.h>                                        

#include <Wire.h>                                                    
#include <LiquidCrystal_I2C.h>                                        
LiquidCrystal_I2C lcd(0x27,16,2);                                    
const byte firmware_version = 25;                                   

//----------------------------Settings---------------------------------------------------------------------------------------------------------------
boolean debug =                   true; 
const unsigned long BAUD_RATE=    38400;
const byte Vrms_EU=               230;                              
const byte Vrms_USA=              110;                          
const int TIME_BETWEEN_READINGS=  5000;                         
const int RF_RESET_PERIOD=        60000;                           


const float Ical1=                90.9;                           
const float Ical2=                90.9;                                 
float Vcal_EU=                    275.8;                           
const float Vcal_USA=             130.0;                          
boolean USA=                      false; 
const byte min_pulsewidth= 110;                            

const float phase_shift=          1.7;
const int no_of_samples=          1480; 
const byte no_of_half_wavelengths= 20;
const int timeout=                2000;                             
const int ACAC_DETECTION_LEVEL=   3000;

const byte TEMPERATURE_PRECISION=  12;                              
const byte MaxOnewire=             6;                                         
boolean RF_STATUS=0;                            

//-------------------------------------------------------------------------------------------------------------------------------------------


const byte LEDpin=                     9;              
const byte shutdown_switch_pin =       8;              
const byte emonpi_GPIO_pin=            5;         

const byte emonPi_int1=                1;             
const byte emonPi_int1_pin=            3;             
#define ONE_WIRE_BUS                   4               
//-------------------------------------------------------------------------------------------------------------------------------------------

//Setup DS128B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
byte allAddress [MaxOnewire][8]; 
byte numSensors;

byte RF_freq=RF12_433MHZ;                                       
byte nodeID = 5;                                               
int networkGroup = 210;  

typedef struct { 
int power1;
int power2;
int power1_plus_2;                                                    
int Vrms; 
int temp[MaxOnewire]; 
unsigned long pulseCount;  
} PayloadTX;                                                   
PayloadTX emonPi; 
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------

double Vcal, vrms;
boolean CT1, CT2, ACAC, DS18B20_STATUS;
byte CT_count, Vrms;                                             
unsigned long last_sample=0;                                     
byte flag;                                                       
volatile byte pulseCount = 0;
unsigned long now =0;
unsigned long pulsetime=0;                                        
unsigned long last_rf_rest=0;                                 

// RF Global Variables 
static byte stack[RF12_MAXDATA+4], top, sendLen, dest;           
static char cmd;
static word value;                                              
long unsigned int start_press=0;                                

const char helpText1[] PROGMEM =                                
"\n"
"Available commands:\n"
"  <nn> i     - set node IDs (standard node ids are 1..30)\n"
"  <n> b      - set MHz band (4 = 433, 8 = 868, 9 = 915)\n"
"  <nnn> g    - set network group (RFM12 only allows 212, 0 = any)\n"
"  <n> c      - set collect mode (advanced, normally 0)\n"
"  ...,<nn> a - send data packet to node <nn>, request ack\n"
"  ...,<nn> s - send data packet to node <nn>, no ack\n"
"  ...,<n> p  - Set AC Adapter Vcal 1p = UK, 2p = USA\n"
"  v          - Show firmware version\n"
;

//-------------------------------------------------------------------------------------------------------------------------------------------
// SETUP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{ 
  delay(100);
  if (USA==true) 
  {
    Vcal = Vcal_USA;                                              
    Vrms = Vrms_USA;
  }
  else 
  {
    Vcal = Vcal_EU;
    Vrms = Vrms_EU;
  }
  
  emonPi_startup();                                                    
  emonPi_LCD_Startup(); 
  delay(1500);  
  CT_Detect();
  serial_print_startup();
   
  attachInterrupt(emonPi_int1, onPulse, FALLING); 
  emonPi.pulseCount = 0;                                                  
   
  

  ct1.current(1, Ical1);                                    
  ct2.current(2, Ical2);                             

  if (ACAC)                                               
  {
    ct1.voltage(0, Vcal, phase_shift);                       
    ct2.voltage(0, Vcal, phase_shift);                      
  }
 
}


//-------------------------------------------------------------------------------------------------------------------------------------------
// LOOP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------

void loop()
{
  now = millis();
 
  if (USA==true) 
  {
    Vcal = Vcal_USA;                                                       
    Vrms = Vrms_USA;
  }
  else 
  {
    Vcal = Vcal_EU;
    Vrms = Vrms_EU;
  }
  
  // Update Vcal
  ct1.voltage(0, Vcal, phase_shift);                  
  ct2.voltage(0, Vcal, phase_shift);                     

  if (digitalRead(shutdown_switch_pin) == 0 ) 
    digitalWrite(emonpi_GPIO_pin, HIGH);                                         
  else 
    digitalWrite(emonpi_GPIO_pin, LOW);
  
  if (Serial.available()){
    handleInput(Serial.read());                                             
    double_LED_flash();
  }                                             
      


  if ((now - last_sample) > TIME_BETWEEN_READINGS)
  {
    single_LED_flash();                                                           
    
    if (ACAC && CT1)                                                                    
    {
      ct1.calcVI(no_of_half_wavelengths,timeout); 
      emonPi.power1=ct1.realPower;

      ct1.calcVI(no_of_half_wavelengths,timeout);
      emonPi.Vrms=ct1.Vrms*100;

   }
    else 
    {
      if (CT1) emonPi.power1 = ct1.calcIrms(no_of_samples)*Vrms;                               
   }  
  
   if (ACAC && CT2)                                                                      
   {
     ct2.calcVI(no_of_half_wavelengths,timeout); 
     emonPi.power2=ct2.realPower;

    ct2.calcVI(no_of_half_wavelengths,timeout);
    emonPi.Vrms=ct1.Vrms*100;

   }
   else 
   {
     if (CT2) emonPi.power2 = ct2.calcIrms(no_of_samples)*Vrms;                             
   }
   
   emonPi.power1_plus_2=emonPi.power1 + emonPi.power2;                                     
   
   if ((ACAC==0) && (CT_count > 0)) emonPi.Vrms=Vrms*100;                                       
  
   if ((ACAC==1) && (CT_count==0)) {                                                                     
     ct1.calcVI(no_of_half_wavelengths,timeout);
     emonPi.Vrms=ct1.Vrms*100;
   }
    
    if (pulseCount)                                                     
    {
      cli();                                                             
      emonPi.pulseCount += pulseCount;
      pulseCount = 0;
      sei();                                                         
    }     
    
    // Serial.print(CT1); Serial.print(" "); Serial.print(CT2); Serial.print(" "); Serial.print(ACAC); Serial.print(" "); Serial.println  (CT_count);
    // Serial.print(emonPi.power1); Serial.print(" ");
    // Serial.print(emonPi.power2); Serial.print(" ");
    // Serial.print(emonPi.Vrms); Serial.print(" ");
    // Serial.println(emonPi.temp[1]);
    
    send_emonpi_serial();                                           
    
    last_sample = now;                                          
    
    }
    
    
} // end loop---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void single_LED_flash()
{
  digitalWrite(LEDpin, HIGH);  delay(50); digitalWrite(LEDpin, LOW);
}

void double_LED_flash()
{
  digitalWrite(LEDpin, HIGH);  delay(25); digitalWrite(LEDpin, LOW);
  digitalWrite(LEDpin, HIGH);  delay(25); digitalWrite(LEDpin, LOW);
}
