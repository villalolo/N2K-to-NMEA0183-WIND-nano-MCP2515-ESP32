#include <mcp_can.h>  // MCP_CAN_lib-master.zip
#include <SPI.h>
#include <Arduino.h>

/*
MCP2515    ESP32

INT -------21
SCK -------18
SI --------23
SO --------19
CS --------5
GND -------GND
VCC -------3.3V

MCP2515    Nano

INT -------D2
SCK -------D13
SI --------D11
SO --------D12
CS --------D10
GND -------GND
VCC -------5V


*/

const int SPI_CS_PIN = 10; // marche avec nano
// const int SPI_CS_PIN = 5; // marche avec esp32

MCP_CAN CAN(SPI_CS_PIN);

void setup() {
  Serial.begin(115200);
  
  while (CAN_OK != CAN.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ)) {
    Serial.println("Initialisation CAN échouée. Réessai...");
    delay(100);
  }
  Serial.println("Initialisation CAN réussie!");
  
  CAN.setMode(MCP_NORMAL);
}

void loop() {
  unsigned char len = 0;
  unsigned char buf[8];
  unsigned long canId;


  unsigned int windangleint  = 0;
  unsigned int windspeedint  = 0;

  unsigned int windangleintLSB  = 0;
  unsigned int windangleintMSB  = 0;
  unsigned int windspeedintLSB  = 0;
  unsigned int windspeedintMSB  = 0;

  char strstnmea[90]="";
  char strstnmeaspeed[10]="";
  char strstnmeaangle[10]="";
  String chcksumHEX;
  char chcksumHEXchar[10]="";



float windangle;
float windspeed;
float Pi = 3.141592653589793238462643;

int val;
char CS;

  if (CAN_MSGAVAIL == CAN.checkReceive()) {
    CAN.readMsgBuf(&canId, &len, buf);
    
    // Serial.print("Message reçu - ID: 0x");
    // Serial.print(canId, HEX);
    // Serial.print("  Longueur: ");
    // Serial.print(len);
    // Serial.print("  Données: ");



    
    for(int i = 0; i < len; i++) {
      // Serial.print(buf[i], HEX);
      // Serial.print(" ");
    }
    // Serial.println();



windangle = 0;
windspeed = 0;

windspeedintLSB  = buf[1];
windspeedintMSB  = buf[2];
windangleintLSB  = buf[3];
windangleintMSB  = buf[4];

windspeed  = (windspeedintLSB+windspeedintMSB*256)*0.01*1.94384*10; // en Noeuds * 10 pour les decimales
val = windspeed;
windspeed  = val / 10;

windangle  = (windangleintLSB+256*windangleintMSB)*0.0001*0.318309886184*180;
val = windangle;
windangle  = val ;

// NMEA 0183 message

/*

MWV - Wind Speed and Angle
       1 2 3 4 5
       | | | | |
$--MWV,x.x,a,x.x,a*hh<CR><LF>
Field Number:
1) Wind Angle, 0 to 360 degrees
2) Reference, R = Relative, T = True
3) Wind Speed
4) Wind Speed Units, K/M/N
5) Status, A = Data Valid
6) Checksum

$WIMWV,320,R,15.0,M,A*0B<CR><LF>

$WIMWV,250,R,0.0,K,A*0B

a moi
WIMWV,239,R,0.0,K,A
CS = *30

WIMWV,251,R,0.0,K,A
CS = *3E

WIMWV,264,R,0.0,K,A
CS = *38
$WIMWV,264,R,0.0,K,A*38  ... ca marche

To check online Checksum https://www.meme.au/nmea-checksum.html





*/

strcat (strstnmea,"$WIMWV,"); // begining of the new sentence

dtostrf(windangle, 3, 0, strstnmeaangle); // cree une char array strstnmeaangle depuis la valeur du float windangle avec 0 decimale

// snprintf(strstnmeaspeed, sizeof(strstnmeaspeed),"%f", windspeed);

dtostrf(windspeed, 3, 1, strstnmeaspeed); // convert windspeed to a string of 1 decimal

strcat (strstnmea,strstnmeaangle); 
strcat (strstnmea,",R,");
strcat (strstnmea,strstnmeaspeed);
strcat (strstnmea,",K,");
strcat (strstnmea,"A*");

// Serial.println("---------------");
// Serial.println(strstnmea);

CS = checkSum(strstnmea);
// Serial.println(CS, HEX);


// Convert CS into an char array de hex value of checksum
chcksumHEX =  String(CS, HEX);
if(chcksumHEX.length()==1)
  {
  chcksumHEX = "0" + chcksumHEX;
  // chcksumcalcstr= pad1;    
}

chcksumHEX.toUpperCase();

chcksumHEX.toCharArray(chcksumHEXchar, 3);


strcat (strstnmea,chcksumHEXchar);

// Serial.println(CS,HEX);

Serial.println(strstnmea);
// Serial.println(chcksumHEX);

// Serial.println("$WIMWV,264,R,0.0,K,A*38");



  }
}




// ******************   calculate the checksum      ***************
// ****************************************************************


char checkSum(String theseChars) 
{
char check = 0;
int firststar = theseChars.indexOf('*');
String messagecore=theseChars.substring(1,firststar);  // get the core message between $ and *<CR><LF>

// Serial.print("csmsg:");
// Serial.print(messagecore);
// Serial.print(".");

// iterate over the string, XOR each byte with the total sum:
for (int c = 1; c < theseChars.length()-1; c++) 
  {
  // check = char(check ^ theseChars.charAt(c));
 
  // Serial.print(theseChars.charAt(c));
  // Serial.print(":");
  
  check = check xor theseChars.charAt(c);

  } 

// Serial.print("  CS value:");
// Serial.println(check, HEX);

return check;
}





// ****************************************************************
// ******************   check message checksum      ***************
// ****************************************************************


boolean checksumcheck (String messagestring)
{
int msglenth=messagestring.length();
int chcksumcalc = 0;
String chcksumread;      // checksum read from the string
String chcksumcalcstr;   // checksum calculated
String pad1 ="0";

int firststar = messagestring.indexOf('*');  
// Serial.println("starposition :" );
// Serial.println(firststar);

String messagecore=messagestring.substring(1,firststar);  // get the core message between $ and *<CR><LF>

// Debug only
//
// Serial.print("CS Test > ");
// Serial.print("Core ");
// Serial.println(messagecore);
// 

String messagechsm=messagestring.substring(firststar+1,firststar+3); // get the HEX code after the * sign
// Serial.print("Read hex: ");
// Serial.print(messagechsm);
// Serial.println(messagecore);


boolean result=0;

chcksumcalc = checkSum(messagecore);
chcksumread = messagechsm;

// Put calc in Hex, in Upper case, and pad with 0's
chcksumcalcstr =  String(chcksumcalc, HEX);
chcksumcalcstr.toUpperCase();

if(chcksumcalcstr.length()==1)
  {
  pad1 = pad1 + chcksumcalcstr;
  chcksumcalcstr= pad1;    
}

// Debug only
//
//
// Serial.print(" CCS ");
// Serial.print(chcksumcalcstr);

// Serial.print(" RCS ");
// Serial.print(messagechsm);
// 
//
//

if (chcksumcalcstr==messagechsm)
  {
  result=1;
  }
return result;
}
