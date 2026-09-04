//board=MiniCore:avr:88:baudrate=default,BOD=disabled,bootloader=no_bootloader,clock=8MHz_internal,eeprom=erase,LTO=Os,variant=modelP

#include <avr/wdt.h>      // ***fuse watchdog always on must be unprogrammed to make wtd+int working together ***        
#include "src/OneWireNoRes/OneWire.h"  // *** no-resistor one-wire protocol needed *** *** 1-wire needs at least 8Mhz clock ***
#include <EEPROM.h>
#include <avr/sleep.h>            // sleep mode library https://www.nongnu.org/avr-libc/user-manual/group__avr__sleep.html
#include <avr/power.h>            // power control library https://www.nongnu.org/avr-libc/user-manual/group__avr__power.html 


//#define PIN_LED_RED 10    
//#define PIN_LED_GREEN 9   

#define PIN_1WIRE 6 
#define PIN_555_VCC A2     
#define PIN_555_OUT 0    
#define PIN_SWITCH_1WIRE 5  
#define PIN_SWITCH_VCC_DS18B20 A5
#define PIN_WTD_DONE 3
#define PIN_WTD_WAKE 2 //INT0

#define TEMP_CALIBRATION_MODE 0    //0=no temperature calibration 1=one point calibration 2=three points calibration
#define TEMP_REF_1 160        //riferimento certo della temp x 16 a 10, 25 e 50 gradi
#define TEMP_REF_2 400
#define TEMP_REF_3 800
#define TEMP_1 160            //valori rilevati per ogni riferimento x 16
#define TEMP_2 400
#define TEMP_3 800

int temperature = 25;
byte temperatureSensorAddress[8];
OneWire oneWire(PIN_1WIRE); 
int moisture_calibration;
int temp_calibration;
bool firstLoop = true;
long ctt=0;
byte clk;
byte b=0;

// EEPROM Errors Legenda
// byte 4 = DS18B20 not found
// byte 5 = 555 timeout
// byte 6 = get temp error
// byte 7 = set scratchpad error
// byte 8 = requestTemperatureConversion error

void setup() {
  pinMode(PIN_WTD_DONE, OUTPUT);  
  digitalWrite(PIN_WTD_DONE, LOW);  
  pinMode(PIN_WTD_WAKE, INPUT_PULLUP);
  /*
 
  pinMode(PIN_LED_RED, OUTPUT); 
  digitalWrite(PIN_LED_RED, LOW); 
  pinMode(PIN_LED_GREEN, OUTPUT); 
  digitalWrite(PIN_LED_GREEN, LOW); 
  */


  pinMode(PIN_555_VCC, OUTPUT);  
  digitalWrite(PIN_555_VCC, LOW); 
  pinMode(PIN_555_OUT, INPUT_PULLUP);  

  pinMode(PIN_SWITCH_VCC_DS18B20, OUTPUT);  
  digitalWrite(PIN_SWITCH_VCC_DS18B20, HIGH); //reset ds18b20 and i-wire analog switch
  delayMicroseconds(100);
  digitalWrite(PIN_SWITCH_VCC_DS18B20, LOW);
  delayMicroseconds(500);
  pinMode(PIN_SWITCH_1WIRE, OUTPUT);  
  digitalWrite(PIN_SWITCH_1WIRE, HIGH);
  delayMicroseconds(100);
  oneWire.target_search(0x28);
  if (!oneWire.search(temperatureSensorAddress)) 
  {
    b=EEPROM.read(4);
    EEPROM.write(4, b+1);
    waitWTD();
  }    

  requestTemperatureConversion();
  setScratchpadBytes (0, 0);   
  digitalWrite(PIN_SWITCH_1WIRE, LOW);
  delayMicroseconds(100);
  moisture_calibration = EEPROM.read(0); // Read the signed calibration value (0-255)
  moisture_calibration = ((moisture_calibration==0 || moisture_calibration==255) ? 128 : moisture_calibration)  - 128; // Convert back to signed (-128 to +127)
  temp_calibration = EEPROM.read(1); // Read the signed calibration value (0-255)
  temp_calibration = ((temp_calibration==0 || temp_calibration==255) ? 128 : temp_calibration)  - 128; // Convert back to signed (-128 to +127)  
  attachInterrupt(digitalPinToInterrupt(PIN_WTD_WAKE), ClockInterrupt, FALLING); 
}


void loop() {

  unsigned long tot = 0, minS = 65535, maxS = 0, t1 = 0;
  unsigned long long sample[6];
  
  byte ct=0, errCt = 0;  
  
  //doBlink(1, 2, 0, 1);
  if (firstLoop)
  {
    t1 = millis();
  }
    
  digitalWrite(PIN_555_VCC, HIGH); // switch ON 555
  delayMicroseconds(1000);
  for (byte x = 0; x < 6; x++ ) { //Read 6 pulses from 555
    sample[x]=pulseInLong(PIN_555_OUT, LOW, 200000); 
    if (sample[x]==0) // timeout
    {
      errCt++;
      if (errCt==4)
      {
        delayMicroseconds(1000);
        b=EEPROM.read(5);
        EEPROM.write(5, b+1);
        digitalWrite(PIN_555_VCC, LOW);
        digitalWrite(PIN_SWITCH_1WIRE, HIGH); // disconnect from the 1-wire network to transmit a total of 39 bytes (~25 microseconds)
        delayMicroseconds(1000); // the analog switch has a max turn-off time of 15 ns
        setScratchpadBytes (0, 0); //zero value of humidity means timeout error
        digitalWrite(PIN_SWITCH_1WIRE, LOW); // reconnect to the 1-wire network          
        waitWTD();
      }
    }
  }
  digitalWrite(PIN_555_VCC,LOW); // switch OFF 555
  if (firstLoop)
  {
    long df=800-(millis()-t1);
    delay(df>0 && df<801 ? df : 0); //give time to the DS18B20 to complete the first temperature conversion
    temperature=getTemp();
    firstLoop=false;
  }


  for (byte x = 0; x < 6; x++ ) {
    if (sample[x]>0)
    {
      ct++;
      sample[x]=(sample[x]*(1600+((temperature<-64 ? -64 : temperature) - 320)*4))/1600; //temperature compensation
      sample[x] = (sample[x] * (1000 + moisture_calibration * 2)) / 1000; // 555 capacitor calibration applied with 0.2% steps
      sample[x]=(65535-((sample[x]*3)>65535 ? 65535 : (sample[x]*3)))+1; //revert the logic to: lower values lower humidity and magnify by 3x
      
      //sample[x]=(65535-(sample[x]>65535 ? 65535 : sample[x]))+1; //revert the logic to: lower values lower humidity
      //sample[x]=((sample[x]*sample[x]*sample[x])/4294967296); // logarithmic transformation  4294967296=65536^2     
      
      if (sample[x]==65536)
      {
        sample[x]=65535;
      }
      if (sample[x]<minS)
      {
        minS=sample[x];
      }
      if (sample[x]>maxS)
      {
        maxS=sample[x];
      }        
      if (sample[x]==0) //0 is reserved to signal a timeout error
      {
        sample[x]=1;
      }        
      tot=tot+sample[x];
    }
  }
  
  
  tot=(tot-minS-maxS)/(ct-2);


  digitalWrite(PIN_SWITCH_1WIRE, HIGH); // disconnect from the 1-wire network to transmit a total of 39 bytes including getTemp (~25 ms)
  delayMicroseconds(1); // the analog switch has a max turn-off time of 15 ns
  clk++;
  if (clk==17)
  {
    clk=0;
    temperature=getTemp();
  }
  setScratchpadBytes ((unsigned int)tot/256, (unsigned int)tot%256);   
  requestTemperatureConversion();
  digitalWrite(PIN_SWITCH_1WIRE, LOW); // reconnect to the 1-wire network

  resetWtd(); 
  delayMicroseconds(1000);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  cli();
  power_all_disable(); //deep sleep
  sleep_bod_disable();
  sei();
  sleep_cpu ();  // Sleep here 

  sleep_disable(); //return here after WDT interrupt INT0
  power_all_enable();  


}

void ClockInterrupt() { // wake up signal from the WTD
}


void resetWtd()
{
  digitalWrite(PIN_WTD_DONE, HIGH); // send done to WTD
  delayMicroseconds(1);
  digitalWrite(PIN_WTD_DONE, LOW);    

}

void waitWTD()
{
  while (1){
    };
}


//18 bytes transmitted
int getTemp() {
  int r = 400; //=25' celsius.  temperature is kept multiplied by 16 to have an integer
  byte data[12];
  byte i = 0;
  while (i < 3) {
    if (oneWire.reset()) {
      oneWire.select(temperatureSensorAddress);
      oneWire.write(0xBE, 0);
      oneWire.read_bytes(data, 9);
      int c = oneWire.crc8(data, 8);
      if (c == data[8]) {
        int16_t raw = (int16_t)(((uint16_t)data[1] << 8) | data[0]);
        //r = (float)r / 16; 
        byte tmpSign = data[1] & 0b11111000;
  
        // -41' +86' temp is valid between these bounds
        if ((raw > -656) && (raw < 1376) && (tmpSign == 0 || tmpSign == 0b11111000)) { 
          r = raw;

          if (TEMP_CALIBRATION_MODE==1)
          {
            //One point temperature calibration. This offset is calculated using the reference sensor.
            r=r+temp_calibration;
          }
          
          if (TEMP_CALIBRATION_MODE==2)
          {
            //Three points temperature calibration using the Lagrange interpolation. Use to build the reference sensor.
            int32_t Z = r;
            //Lagrange denominators
            int64_t d12 = int64_t(TEMP_1 - TEMP_2) * (TEMP_1 - TEMP_3);
            int64_t d23 = int64_t(TEMP_2 - TEMP_1) * (TEMP_2 - TEMP_3);
            int64_t d31 = int64_t(TEMP_3 - TEMP_1) * (TEMP_3 - TEMP_2);
            // Lagrange terms
            int64_t L1 = (int64_t(Z - TEMP_2) * (Z - TEMP_3)) * TEMP_REF_1  / d12;
            int64_t L2 = (int64_t(Z - TEMP_1) * (Z - TEMP_3)) * TEMP_REF_2  / d23;
            int64_t L3 = (int64_t(Z - TEMP_1) * (Z - TEMP_2)) * TEMP_REF_3  / d31;
            r=L1+L2+L3;
          }
          i=3; // exit the while with 4 
        }
      }   
    }
    else
    {
      delayMicroseconds(1000);
    }
    i++;
  }
  if (i==3)
  {
    b=EEPROM.read(6);
    EEPROM.write(6, b+1);    
    waitWTD();
  }
  return r; 
}

//12 bytes transmitted
void setScratchpadBytes(byte byte1, byte byte2){
  if (oneWire.reset()){
    oneWire.select(temperatureSensorAddress);    
    oneWire.write(0x4E, 0);
    oneWire.write(byte1, 0);
    oneWire.write(byte2, 0);
    oneWire.write(0b01100000, 0); // third byte must be written even if not used 
  }  
  else
  {
    b=EEPROM.read(7);
    EEPROM.write(7, b+1);       
    waitWTD();
  }
}

//9 bytes transmitted    
void requestTemperatureConversion(){
  if (oneWire.reset()) {
    oneWire.select(temperatureSensorAddress);
    oneWire.write(0x44, 0);
  }
  else
  {
    b=EEPROM.read(8);
    EEPROM.write(8, b+1);       
    waitWTD();
  }
}

/*
void setTempSensorResolution(byte address[8]){
  byte data[12];
  if (oneWire.reset()){
    oneWire.select(address);    
    oneWire.write(0xBE, 0);
    oneWire.read_bytes(data, 5);

    if (oneWire.reset()){
      oneWire.select(address);    
      oneWire.write(0x4E, 0);
      oneWire.write(data[2], 0);
      oneWire.write(data[3], 0);
      byte r=data[4] & 0b10011111; //9 bits resolution preserving the other bits
      // r = r | 0b00100000; //10 bits
      // r = r | 0b01000000; //11 bits
      r = r | 0b01100000; //12 bits
      oneWire.write(r, 0);
      //copy scratchpad to eeprom
      if (oneWire.reset()){
        oneWire.select(temperatureSensorAddress);    
        oneWire.write(0x48, 0);  
        oneWire.reset();  
      }
    }     
  }
}
 */

 
