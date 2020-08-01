
void onPulse() {  
  if ( (millis() - pulsetime) > min_pulsewidth) {
    pulseCount++;					                         
  }
  pulsetime=millis(); 	
}
