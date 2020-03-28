#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h> 


RF24 radio(7, 8);               // nRF24L01 (CE,CSN)

RF24Network network(radio);      // Include the radio in the network

//addresses of each Nano in octal format
const uint16_t main1 = 00;
const uint16_t ultra1 = 01;
const uint16_t door = 03;
const uint16_t window = 04;


//header for sending
RF24NetworkHeader header1(ultra1);
RF24NetworkHeader header3(door);
RF24NetworkHeader header4(window);

  //temporary data
RF24NetworkHeader header;
unsigned long incomingData;

bool ok1 = false;   //main1 and ultra1
bool ok3 = false;   //main1 and door
bool ok4 = false;   //main1 and window



//signal  : 0 = standby; 1 = lower  ; 2 = raise
//confirm : 0 = standby; 1 = success; 2 = fail

int door_signal = 330;
int door_signal2 = 550;
int window_signal = 440;
int door_confirm = 330;
int door_confirm2 = 550;
int window_confirm = 440;
int water1 = 1; //water level from ultra1

int doorbatt = -1; //batt from door
int windowbatt = -1; //batt from window

bool dooronly1 = false;

struct setlevel
{
  int level1 = 3; //1st water level trigger 
  int level2 = 35;  //2nd water level trigger 
};

setlevel setlevel;

//boundaries for each water level trigger
int level_1_upper = 5;
int level_1_lower = 2;
int level_2_upper = 35;
int level_2_lower = 6;

String msgsend;

bool watertick = false;
bool startsendbatt = true;


void setup() {

  Serial.begin(9600);
  
  //nrf setup
  SPI.begin();
  radio.begin();
  network.begin(90, main1);  //(channel, node address)
  radio.setDataRate(RF24_2MBPS);

   //GSM setup
   Serial3.begin(9600); // for gsm, PA9(RX of GSM) and PA10(TX of GSM)
  delay(1000);
  Serial3.println("AT");
  delay(500);
  Serial3.println("AT+CMGF=1");        //Set to text mode
  delay(500);  
  Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
  delay(500);
  Serial3.println("System activated"); //Notify the user that the system is ready
  delay(500);
  Serial3.println((char)26);
  Serial3.println("AT+CNMI=2,2,0,0,0");    //Sending message 
  delay(1000);

  Serial.println("READY");


 
}

void loop() {


  
  network.update(); //update nrf network
  

  
  Setlevel(); //check if admin changes the level1 and level2 of water level trigger
  Checkbatt();  //check battery of both motors and send sms when they are in critical condition
  String msgsend = "";
  

  //NRF listens for incoming data
  if (network.available())
  {
    while (network.available())
    {
      network.read(header,  &incomingData, sizeof(incomingData));
      Serial.print( "Received from: " + String(header.from_node) + " Data: " );

      if (header.from_node == 1) //01 octal is 1 in decimal
      {
        if(incomingData == setlevel.level1 || incomingData == setlevel.level2 || incomingData == 0)
        {
        water1 = incomingData;
        Serial.println(water1);
        Serial.println("");
        watertick = true;
        }
        else
        {
          Serial.println(incomingData);
        }
      }

      else if (header.from_node == 3)
      {
        if(incomingData > 330 && incomingData < 400) //it must be the door confirm of 1st barrier
        {
          door_confirm = incomingData;
          Serial.println(door_confirm);
          Serial.println("");
        }
        else if(incomingData > 550) //it must be the door confirm of 2nd barrier
        {
          door_confirm2 = incomingData;
          Serial.println(door_confirm2);
          Serial.println("");
        }
        else  //it is the battery condition
        {
          doorbatt = incomingData;
          Serial.println(doorbatt);
          Serial.println("");
        }
      }
      else if (header.from_node == 4)
      {
        if(incomingData > 440) //it must be the window confirm
        {
          window_confirm = incomingData;
          Serial.println(window_confirm);
          Serial.println("");
        }
        else //it is the battery condition
        {
          windowbatt = incomingData;
          Serial.println(windowbatt);
          Serial.println("");
        }
      }

    }
  }

  
//lower or raise the barricades only when it has received true water1 and water2
if(watertick)
{
  watertick = false;
  
  if (water1 == setlevel.level1) //lower the door barricade
  {
    dooronly1 = true;
    
    door_signal = 331;
    while (ok3 == false)
    {
      ok3 = network.write(header3, &door_signal, sizeof(door_signal));
    }
    ok3 = false;
    Serial.println("Sent msg about trying to lower the door barricade");
    
    //notify user that water level has risen
    msgsend = "The system has detected the water level to be about ";
    msgsend += setlevel.level1;
    msgsend += " cm. Commanding to lower the 1st door barricade.";
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);
    Serial3.println(msgsend); 
    delay(50);
    Serial3.write(26);
    delay(5000); //wait for the confirmation first
  }

  else if (water1 == setlevel.level2) //lower the 2nd door barricade and window barricade
  {
    dooronly1 = false;

    //raise 2nd door barricade

    door_signal2 = 551;
    while (ok3 == false)
    {
      ok3 = network.write(header3, &door_signal2, sizeof(door_signal2));
    }
    ok3 = false;
    Serial.println("Sent msg about trying to lower the 2nd door barricade");

    window_signal = 441;
    while (ok4 == false)
    {
      ok4 = network.write(header4, &window_signal, sizeof(window_signal));  
    }
    Serial.println("Sent msg about trying to lower the window barricade");
    ok4 = false;
    
    //notify user that water level has risen
    msgsend = "The system has detected the water level to be about ";
    msgsend += setlevel.level2;
    msgsend += " cm. Commanding to lower the 2nd door barricade.";
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);
    Serial3.println(msgsend); 
    delay(50);
    Serial3.write(26);
    delay(5000); //wait for the confirmation first
    
    //notify user that water level has risen
    msgsend = "The system has detected the water level to be about ";
    msgsend += setlevel.level2;
    msgsend += " cm. Commanding to lower the window barricade.";
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);
    Serial3.println(msgsend); 
    delay(50);
    Serial3.write(26);
    delay(5000); //wait for the confirmation first
  }

  else if(water1 == 0)
  {

    if(dooronly1 == false)  //raise window when both door barricades are lowered
    {
      window_signal = 442;
      while (ok4 == false)
      {
        ok4 = network.write(header4, &window_signal, sizeof(window_signal));
      }
      ok4 = false;
      Serial.println("Sent msg about trying to raise the window barricade");
  
      //notify user that water level has dropped
      Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
      delay(50);
      Serial3.println("The system has detected the water level to be about 0 cm. Commanding to raise the window barricade."); 
      delay(50);
      Serial3.write(26);
      
      delay(1000);

    Serial.println("Sent msg about trying to raise the 2nd door barricade");
    door_signal2 = 552;
    while (ok3 == false)
    {
      ok3 = network.write(header3, &door_signal2, sizeof(door_signal2));
    }
    ok3 = false;

    //notify user that water level has dropped
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);
    Serial3.println("The system has detected the water level to be about 0 cm. Commanding to raise the 2nd door barricade."); 
    delay(50);
    Serial3.write(26);
    delay(5000); //wait for the confirmation first
    
    Serial.println("Sent msg about trying to raise the 1st door barricade");
    door_signal = 332;
    while (ok3 == false)
    {
      ok3 = network.write(header3, &door_signal, sizeof(door_signal));
    }
    ok3 = false;

    //notify user that water level has dropped
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);
    Serial3.println("The system has detected the water level to be about 0 cm. Commanding to raise the 1st door barricade."); 
    delay(50);
    Serial3.write(26);
    delay(2000); //wait for the confirmation first

    }
    else if(dooronly1 == true)
    {
      dooronly1 = false;
      
    Serial.println("Sent msg about trying to raise the 1st door barricade");
    door_signal = 332;
    while (ok3 == false)
    {
      ok3 = network.write(header3, &door_signal, sizeof(door_signal));
    }
    ok3 = false;

    //notify user that water level has dropped
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);
    Serial3.println("The system has detected the water level to be about 0 cm. Commanding to raise the 1st door barricade."); 
    delay(50);
    Serial3.write(26);
    delay(5000); //wait for the confirmation first
    }
  }

}



//confirmation msg about lowering or raising barricades

if (door_confirm == 331 && door_signal != 330)
{
  if (door_signal == 331)
  {
    Serial.println("send msg about door success in lowering");
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);  
    Serial3.println("1st Door barricade was successfully lowered."); 
    delay(50);
    Serial3.write(26);
  }
  else if (door_signal == 332)
  {
    Serial.println("send msg about door success in raising");
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);  
    Serial3.println("1st Door barricade was successfully raised."); 
    delay(50);
    Serial3.write(26);
  }

}
if (window_confirm == 441 && window_signal != 440)
{
  if (window_signal == 441)
  {
    Serial.println("send msg about window success in lowering");
    Serial.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);  
    Serial3.println("Window barricade was successfully lowered."); 
    delay(50);
    Serial3.write(26);
  }
  else if (window_signal == 442)
  {
    Serial.println("send msg about window success in raising");
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);  
    Serial3.println("Window barricade was successfully raised."); 
    delay(50);
    Serial3.write(26);
  }

}
if (door_confirm2 == 551 && door_signal2 != 550)
{
  if (door_signal2 == 551)
  {
    Serial.println("send msg about door success in lowering");
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);  
    Serial3.println("2nd Door barricade was successfully lowered."); 
    delay(50);
    Serial3.write(26);
  }
  else if (door_signal2 == 552)
  {
    Serial.println("send msg about door success in raising");
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);  
    Serial3.println("2nd Door barricade was successfully raised."); 
    delay(50);
    Serial3.write(26);
  }
}
if (door_confirm == 332 || window_confirm == 442 || door_confirm2 == 552)
{ //for failed operation
    Serial.println("send msg about door success in lowering");
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(50);  
    Serial3.println("The operation was unsuccessful. Trying again"); 
    delay(50);
    Serial3.write(26);
    watertick = true;
}


  if( door_confirm == 331 || window_confirm == 441 || door_confirm2 == 551)
  {
    //flush all data only when the operation is successful
      water1 = 1;
      door_signal = 330;
      door_confirm = 330;
      door_signal2 = 550;
      door_confirm2 = 550;
      window_signal = 440;
      window_confirm = 440;
  }


}

void Setlevel()
{
 char admin_msg[100];
 int inChar =0;


  bool resetu1 = false;
  bool resetu2 = false;
  bool endmsg= false;


        
while(Serial3.available())        //when the gsm receives a msg
{
  Serial3.println("AT+CMGR=1");     //read the msg received
  delay(500);
  if(Serial3.find("U@ "))
  {
    while(Serial3.available())
    {
        
        admin_msg[inChar] = Serial3.read();
        Serial.print(admin_msg[inChar]);
        inChar++;
        resetu1 =true;
    }
  }
}




  if(resetu1)
{



//convert digit character into its integer value by subtracting 48
//bcs '0' is 48 in integer and 0 - 9 digit characters are in succession
//therefore subtracting char '0' to any digit character will 
//give its correct integer value

//admin_msg could be [level1][space][level2 tens][level2 ones][#] or
//                   [level1][space][level2 ones][#] or
//                   [level1 tens][level1 ones][space][level2 ones][#] or
//                   [level1 tens][level1 ones][space][level2 tens][level2 ones][#][#]

//level1 should only be 2 - 5 cm
//level2 should only be 6 - 35 cm
//when the admin selects a value greater or lesser than the range boundaries,
//the level is automatically set to upper boundary or lower boundary respectively


  
  if(admin_msg[2] == ' ')//when the admin selects a two digit number for level1
  {
    setlevel.level1 = (10*(admin_msg[0] - 48)) + (admin_msg[1] - 48);
  }
  else    //if only one digit
  {
    setlevel.level1 = admin_msg[0] - 48;
  }
  

  if(admin_msg[4] == '#') //one digit for level1 and two digit for level2
  {
    setlevel.level2 = (10*(admin_msg[2] - 48)) + (admin_msg[3] - 48);
  }
  else if(admin_msg[5] == '#') //both level1 and level2 are two digit
  {
    setlevel.level2 = (10*(admin_msg[3] - 48)) + (admin_msg[4] - 48);
  }
  else if (admin_msg[2] == ' ')   //when its only a one digit for level2 but two digit for level1
  {
    setlevel.level2 = admin_msg[3] - 48;
  }
  else  //when both level 1&2 are one digit
  {
    setlevel.level2 = admin_msg[2] - 48;
  }

  //if the admin chooses values that are out of boundaries
  //set it automatically to the boundaries

  if(setlevel.level1 > level_1_upper)
  {
    setlevel.level1 = level_1_upper;
  }
  if(setlevel.level1 < level_1_lower)
  {
    setlevel.level1 = level_1_lower;
  }
  
  if(setlevel.level2 > level_2_upper)
  {
    setlevel.level2 = level_2_upper;
  }
  if(setlevel.level2 < level_2_lower)
  {
    setlevel.level2 = level_2_lower;
  }

  //send the struct data to ultrasonic
  while (ok1 == false)
   {
      ok1= network.write(header1, &setlevel, sizeof(setlevel));
   }
   ok1 = false;
  delay(500);
    
      Serial.println("----");
      resetu1 = false;
      Serial.println("----");

    //send confirmation of the new levels
    msgsend = "NEW WATER LEVEL TRIGGER\r\n";
    msgsend += "LEVEL 1 : ";
    msgsend += setlevel.level1;
    msgsend += " cm \r\n";
    msgsend += "LEVEL 2 : ";
    msgsend += setlevel.level2;
    msgsend += " cm \r\n";
    
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(500);
    Serial3.println(msgsend); //Notify the user that the system is ready
    delay(500);
    Serial3.println((char)26);
    delay(2000);
  }
    

    Serial3.println("AT+CMGDA=\"DEL ALL\"");    //delete all msgs
    delay(500);
  
}

void Checkbatt()
{

    //send sms only once to the admin aboout the startup condition of the batteries
  if(windowbatt >= 0 && doorbatt >= 0 && startsendbatt == true)
  {
    startsendbatt = false;
    
    msgsend = "Battery conditions update \r\n";
    msgsend += "Door battery : ";
    msgsend += doorbatt;
    msgsend += " V \r\n";
    msgsend += "Window battery : ";
    msgsend += windowbatt;
    msgsend += " V \r\n";
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(500);
    Serial3.println(msgsend); //Notify the user that the system is ready
    delay(500);
    Serial3.println((char)26);
  }
  
  if(windowbatt <= 6 && windowbatt >=0)
  {
    msgsend = "WINDOW BATTERY IS IN CRITICAL CONDITION \r\n";
    msgsend += "Window battery : ";
    msgsend += windowbatt;
    msgsend += " V \r\n";
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(500);
    Serial3.println(msgsend); //Notify the user that the system is ready
    delay(500);
    Serial3.println((char)26);
    windowbatt = -1; //flush
  }
  if(doorbatt <= 6 && doorbatt >=0)
  {
    msgsend = "DOOR BATTERY IS IN CRITICAL CONDITION \r\n";
    msgsend += "Door battery : ";
    msgsend += doorbatt;
    msgsend += " V \r\n";
    Serial3.println("AT+CMGS=\"+639432073281\"\r"); //phone number with country code
    delay(500);
    Serial3.println(msgsend); //Notify the user that the system is ready
    delay(500);
    Serial3.println((char)26);
    doorbatt = -1; // flush
    
  }
}
