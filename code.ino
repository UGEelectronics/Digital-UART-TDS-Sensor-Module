#include <SoftwareSerial.h>

SoftwareSerial TDS_Serial(2, 3);// Create software serial port object (RX, TX)


void setup() {
  pinMode(14, LOW);
 //Hardware serial port initialization
  Serial.begin(9600);
  // Initialize the serial port
  TDS_Uart_init();
  delay(50);
  TDS_calibration();


}

void loop() {

  TDS_Getsensordata();
  delay(1000);
  TDS_ParseData();
  delay(1000);

}


// TDS Chip initialization
void TDS_Uart_init()
{
  TDS_Serial.begin(9600);
  delay(10);
}

// TDS Chip calibration function, this function can realize TDS sensor calibration, be sure to adjust the parameters according to the manual
void TDS_calibration()
{
  Serial.println("Start calibration");
  TDS_Calibration(); 
  TDS_Check_callback();
  TDS_Set_res(); 
  TDS_Check_callback();
  TDS_Set_B_NTC();
  TDS_Check_callback();
  Serial.println("Calibration completed"); 
}

//detect TDS instruction
void TDS_Getsensordata()
{
  String Detection_data = "A000000000A0"; 
  TDS_sendHexData(Detection_data);
  TDS_ParseData();

}

// TDS Chip baseline calibration
void TDS_Calibration()
{
  String Calibration_data = "A600000000A6";
  TDS_sendHexData(Calibration_data);
}

//TDS Chip sets NTC resistance value at room temperature
//Default 000186A0 10K resistance
void TDS_Set_res()
{
  String Set_res = "A3000186A0CA";
  TDS_sendHexData(Set_res);
}

//TDS Chip sets NTC B value
//Default 0F0A
void TDS_Set_B_NTC()
{
  String Set_B_NTC = "A50F0A0000BE";
  TDS_sendHexData(Set_B_NTC);
}

//Chip return judgment
void TDS_Check_callback()
{
  while(true)
  {
    if(TDS_Check_DataReceived())
    {
      Serial.println("Set successfully ");
      break;
    }  
  }
}

//TDS Chip sends hexadecimal data
void TDS_sendHexData(String hexString) {
  int hexStringLength = hexString.length();
  if (hexStringLength % 2 != 0) {
   // Ensure that the length of the hexadecimal string is an even number
    hexString = "0" + hexString;
    hexStringLength++;
  }
    for (int i = 0; i < hexStringLength; i += 2) {
    // Extract a pair of characters from a hexadecimal string
    String hexPair = hexString.substring(i, i + 2);
    // Convert hexadecimal string to bytes
    byte hexValue = (byte)strtol(hexPair.c_str(), NULL, 16);
    // Send bytes
    TDS_Serial.write(hexValue);
  }
}

/* Check if the returned data is normal
* Set successfully and return AC 00 00 00 00 AC
* Return when the above instruction is executed abnormally
* *AC XX 00 00 00 AE
* Abnormal code XX:
* * 01: Command frame abnormality
* * 02: Busy
* * 03: Calibration failed
* * 04: Detection temperature is out of range
*/
// Receive the returned result
bool TDS_Check_DataReceived() {

  if(TDS_Serial.available() >= 6) {
    
    byte resp[6];
    for (int i=0; i<6; i++) {
      resp[i] = TDS_Serial.read();
    }
    
    if (resp[0] == 0xAC && 
        resp[1] == 0x00 &&
        resp[2] == 0x00 &&
        resp[3] == 0x00 &&  
        resp[4] == 0x00 &&
        resp[5] == 0xAC) {
       return true;    
    }else if(resp[0] == 0xAC)
    {
      switch(resp[1]) 
      {
        case 0x01:Serial.println("TDS command frame abnormality");break;
        case 0x02:Serial.println("TDS device busy");break;  
        case 0x03:Serial.println("TDS calibration failed");break; 
        case 0x04:Serial.println("TDS detects temperature out of range");break;
        default: Serial.println("Unknown Error");
      }
        return false;         
    }
    // return false;
    
  }
  return false;

}

// Read and parse data
void TDS_ParseData() {

  if (TDS_Serial.available() > 0) {

    byte start = TDS_Serial.read();
    
    if (start == 0xAA) {  

      byte tdsHi = TDS_Serial.read();
      byte tdsLo = TDS_Serial.read();
      int tdsValue = (tdsHi<<8) + tdsLo;

      byte tempHi = TDS_Serial.read();
      byte tempLo = TDS_Serial.read();  
      int tempAdc = (tempHi<<8) + tempLo;
      float temp = tempAdc / 100.0;

      byte checksum = TDS_Serial.read();
      byte sum = start + tdsHi + tdsLo + tempHi + tempLo;
      if ((sum & 0xFF) == checksum) {
        
        Serial.print("TDS: ");
        Serial.print(tdsValue);
        Serial.print("ppm   Temp: ");
        Serial.print(temp);
        Serial.println("°C");
      }
    }
  }
}
