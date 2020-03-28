#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>


RF24 radio(7, 8);               // nRF24L01 (CE,CSN)

RF24Network network(radio);      // Include the radio in the network

//addresses of each Nano in octal format
const uint16_t main1 = 00;
const uint16_t door = 03;   

//header for sending
RF24NetworkHeader header0(main1);

bool ok3 = false;   //main and door


//temporary data 
RF24NetworkHeader header;
unsigned long incomingData;


//signal 0 = standby; 1 = lower; 2 = raise
//confirm 0 = standby; 1 =success ; 2 = fail
int door_signal = 330;
int door_confirm = 330;
int door_signal2 = 550;
int door_confirm2 = 550;

//1st barrier
const int droppin= 2;
const int raisepin = 4;

//2nd barrier
const int droppin2= 9;
const int raisepin2 = 10;


//for the voltage sensor
const int SensorValue = A2;
float volt = 0.0;
int doorbatt = -1;
int prevbatt = -1;
bool startsendbatt = true;

bool raise2 = false;



void setup() {
  Serial.begin(9600);
  pinMode(droppin, OUTPUT);
  pinMode(raisepin, OUTPUT);
  pinMode(droppin2, OUTPUT);
  pinMode(raisepin2, OUTPUT);
  pinMode(SensorValue, INPUT);


  //HIGH is off for the relay
  //LOW is on for the relay
  digitalWrite(droppin, HIGH);
  digitalWrite(raisepin, HIGH);
  digitalWrite(droppin2, HIGH);
  digitalWrite(raisepin2, HIGH);
  
  SPI.begin();
  radio.begin();
  network.begin(90, door);  //(channel, node address)
  radio.setDataRate(RF24_2MBPS);

}

void loop() {
  network.update();

  
   
    

   critical_battery(); //battery check

  if(network.available())
  {
    while (network.available())
    {
        network.read(header,  &incomingData, sizeof(incomingData));
        Serial.print( "Received from: " + String(header.from_node) + " Data: " );
       
  
      if(header.from_node == 0) //00 octal is 0 in decimal
      {
        if(incomingData <500) //it must be for the 1st barrier
        {
        door_signal = incomingData;
         Serial.println(door_signal);
        }
        else  //it must be for the 2nd barrier
        {
          door_signal2 = incomingData;
         Serial.println(door_signal2);
        }
        
      }
    }
  }

//lower or raise the barricade
  if(door_signal == 331)
  {
    //Serial.println("lowering barricade");
    digitalWrite(droppin, LOW);  //lower the barricade 
    delay(15700); //delay to finish
    digitalWrite(droppin, HIGH); //stop lowering the barricade
    door_confirm = 331;
  }
  
  else if(door_signal2 == 551)
  {
    //Serial.println("lowering barricade");
    digitalWrite(droppin2, LOW);  //lower the barricade 
    delay(14500); //delay to finish
    digitalWrite(droppin2, HIGH); //stop lowering the barricade
    door_confirm2 = 551;
  }
  else if(door_signal2 == 552 )
  {
    //Serial.println("raising barricade");
    digitalWrite(raisepin2, LOW);  //raise the barricade 
    delay(17500); //delay to finish
    digitalWrite(raisepin2, HIGH); //stop raising the barricade
    door_confirm2 = 551;

    digitalWrite(raisepin, LOW);  //raise the barricade 
    delay(16500); //delay to finish
    digitalWrite(raisepin, HIGH); //stop raising the barricade
    door_confirm = 331;

    raise2 = true;
  }
  else if(door_signal == 332)
  {
    if(raise2 = false)
    {
    //Serial.println("raising barricade");
    digitalWrite(raisepin, LOW);  //raise the barricade 
    delay(16500); //delay to finish
    digitalWrite(raisepin, HIGH); //stop raising the barricade
    door_confirm = 331;
    }
    raise2 =false;
  }
  else{

    if(door_signal == 331 || door_signal == 332)
    {
    door_confirm = 332;
    }
    if(door_signal2 == 551 || door_signal2 == 552)
    {
    door_confirm2 = 552;
    }
  }

//send acknowledgement about the status of the barricade
  if(door_signal == 331 || door_signal == 332)
  {
    while(ok3 == false)
    {
      ok3 = network.write(header0, &door_confirm, sizeof(door_confirm));
    }

    //Serial.println("Sent acknowledgement");

    //flush
    ok3 =false;
    door_signal = 330;
    door_confirm = 330;
  }

  if(door_signal2 == 551 || door_signal2 == 552)
  {
    while(ok3 == false)
    {
      ok3 = network.write(header0, &door_confirm2, sizeof(door_confirm2));
    }


    if(raise2)
    {
      while(ok3 == false)
    {
      ok3 = network.write(header0, &door_confirm, sizeof(door_confirm));
    }

    }
    //Serial.println("Sent acknowledgement");

    //flush
    ok3 =false;
    door_signal2 = 550;
    door_confirm2 = 550;
  }

}

void critical_battery(){                       //for critical condition of the battery
  
  int i = 0;

  while(i < 5)
  {
  int voltage = analogRead(SensorValue);
  Serial.print(voltage);
  Serial.print("   ");
  volt = ((24 * voltage)/1024.0);
  doorbatt = volt;
  Serial.print(doorbatt);
  Serial.println(" V");
  i++;
  }
  
  

  
if(startsendbatt)   //send battery condition only once for startup
{
  while(ok3 == false)
    {
      ok3 = network.write(header0, &doorbatt, sizeof(doorbatt));
    }
    ok3 =false;

    startsendbatt =false;
}
else 
{
  if(doorbatt<=6 && prevbatt != doorbatt)
  {
  prevbatt = doorbatt;
  while(ok3 == false)
    {
      ok3 = network.write(header0, &doorbatt, sizeof(doorbatt));
    }
    ok3 = false;
  }
}

}
