
byte check_for_DS18B20()                                      
{
  sensors.begin();
  numSensors=(sensors.getDeviceCount()); 

  byte j=0; 
  

  while ( (j < numSensors) && (sensors.getAddress(allAddress[j], j)) )  j++;
  for(byte j=0;j<numSensors;j++) sensors.setResolution(allAddress[j], TEMPERATURE_PRECISION);      
  
  if (numSensors==0) DS18B20_STATUS=0; 
    else DS18B20_STATUS=1;

  if (numSensors>MaxOnewire) 
    numSensors=MaxOnewire;                                          
    
 return numSensors;   
}

int get_temperature(byte sensor) {
  {
    float temp=(sensors.getTempC(allAddress[sensor]));
    if ((temp<125.0) && (temp>-55.0)) return(temp*10);          
}

