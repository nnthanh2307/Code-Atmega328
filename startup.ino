double calc_rms(int pin, int samples)                 
{
  unsigned long sum = 0;
  for (int i=0; i<samples; i++)                           
    int raw = (analogRead(pin)-512);
    sum += (unsigned long)raw * raw;
  }
  double rms = sqrt((double)sum / samples);
  return rms;
}

void emonPi_startup()                                                     
{
  pinMode(LEDpin, OUTPUT); 
  digitalWrite(LEDpin,HIGH); 

  pinMode(shutdown_switch_pin,INPUT_PULLUP);            

  pinMode(emonpi_GPIO_pin, OUTPUT);                    
  digitalWrite(emonpi_GPIO_pin, LOW);
  
  pinMode(emonPi_int1_pin, INPUT_PULLUP);              

  Serial.begin(BAUD_RATE);
  Serial.print(F("emonPi V")); Serial.println(firmware_version*0.1); 
  Serial.println(F("Ngoc Thanh"));
  Serial.println(F("Loading..."));
}


 
void CT_Detect(){
//--------------------------------------------------Check for connected CT sensors--------------------------------------------------------------------------------------------------------- 
if (analogRead(1) > 0) {CT1 = 1; CT_count++;} else CT1=0;            
if (analogRead(2) > 0) {CT2 = 1; CT_count++;} else CT2=0;              
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

digitalWrite(LEDpin,LOW); 

pinMode(A0, OUTPUT);
analogWrite(A0, 255);   
analogWrite(A0, 0);     
pinMode(A0,INPUT);
delay(500);            

vrms = calc_rms(0,1780) * 0.87;     
if (vrms>90) ACAC = 1; else ACAC=0;
//Serial.print(vrms);
}
