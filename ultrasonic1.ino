#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>


RF24 radio(7, 8);               // nRF24L01 (CE,CSN)

RF24Network network(radio);      // Include the radio in the network

//addresses of each Nano in octal format
const uint16_t main1 = 00;
const uint16_t ultra1 = 01;

//header for sending
RF24NetworkHeader header0(main1);

bool ok1 = false;   //main1 and ultra1


//temporary data 
RF24NetworkHeader header;
unsigned long incomingData;

int water1 =1;  //water level from ultra1
int prevwater1 =1;

//for stabilized data
int watertemp[5] = {1,2,3,4,5};
int try_index = 0;
bool water_tick =false;

//ultrasonic variables
const int trigPin = 10;
const int echoPin = 5;
long duration;
int distance = 0;
int max_distance = 0;
int waterlevel = 1;

//water level settings

//temporary
struct tempsetlevel
{
  int level1 = 3; //1st water level trigger 
  int level2 = 35;  //2nd water level trigger 
};

tempsetlevel tempsetlevel;

//final
struct setlevel
{
  int level1 = 3; //1st water level trigger 
  int level2 = 35;  //2nd water level trigger 
};

setlevel setlevel;


bool set_level = false;


void setup() {
  Serial.begin(9600);

  //ultrasonic setup
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT);

  //nRF setup
  SPI.begin();
  radio.begin();
  network.begin(90, ultra1);  //(channel, node address)
  radio.setDataRate(RF24_2MBPS);

  delay(2000); //to stabilize the ultrasonic

  for( int i = 0 ; i < 5 ; i++)
  {
        digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    // Calculating the distance
    distance= duration*0.034/2;
    
    if(max_distance < distance && distance < 300)
    {
      max_distance = distance;
    }
    
  }

  Serial.println("READY");
  Serial.println(max_distance);
  
    
}


void loop() {

  network.update();


 if (network.available())
  {
    while (network.available())
    {
     network.read(header,  &setlevel, sizeof(tempsetlevel));
     Serial.println( "Received from: " + String(header.from_node) + " Data: " );


       if (header.from_node == 0) //00 octal is 0 in decimal
      {
          setlevel.level1 = tempsetlevel.level1;
          setlevel.level2 = tempsetlevel.level2;
          Serial.println("level1 : " + String(setlevel.level1));
          Serial.println("level2 : " + String(setlevel.level2));
      }

    }
  }

//ultrasonic action
digitalWrite(trigPin, LOW);
delayMicroseconds(2);
// Sets the trigPin on HIGH state for 10 micro seconds
digitalWrite(trigPin, HIGH);
delayMicroseconds(10);
digitalWrite(trigPin, LOW);
duration = pulseIn(echoPin, HIGH);
// Calculating the distance
distance= duration*0.034/2;


delay(50);
watertemp[try_index] = distance;
try_index ++;

//stabilized data
if(try_index == 3)
{
  water_tick =true;
  for(try_index = 3; try_index >= 2; try_index--)
  {
    
    if(watertemp[try_index - 1] != watertemp[try_index - 2])
    {
      water_tick =false;
      try_index = 0;
      break;
    }
    
  }
  
  try_index = 0;

}




//only record the water level when it is read 5 times consecutively
if(water_tick)
{
  waterlevel = max_distance - distance;
  Serial.print(waterlevel);
Serial.print(" / ");
Serial.println(max_distance);
  //for water level rise of level1 - (setlevel.level2 - 1).
  if ( ((max_distance - setlevel.level1) >= distance) && ((max_distance - setlevel.level2) < distance) && (prevwater1 != setlevel.level2))
  {
    water1 = setlevel.level1;
  }
  //for water level rise of level2 or more cm
  //it needs to pass level1 cm of course, so level1 cm should be the last reading
  else if ( ((max_distance - setlevel.level2) >= distance) && (prevwater1 == setlevel.level1))
  {
    water1 = setlevel.level2;
  }
  //for when the water level drops to 0
  else if ( ( ((max_distance - setlevel.level1) < distance))  && (water1 > 1))
  {
    water1 = 0;
  }

  water_tick = false;
}

  //send the data to main if trigger water level is achieved
  if(water1 != 1 && prevwater1 != water1)
  {
    while(ok1 == false)
    {
      ok1 = network.write(header0, &water1, sizeof(water1));
    }
    ok1 = false;
    Serial.println("");
    Serial.println(water1);
    Serial.println("");

    delay(500);
    
    prevwater1 = water1;
  }
//  else
//  {
//    //see the current water level
//    if(waterlevel != setlevel.level1 && waterlevel != setlevel.level2 && waterlevel != 0)
//    {
//    while(ok1 == false)
//    {
//      ok1 = network.write(header0, &waterlevel, sizeof(waterlevel));
//    }
//    ok1 = false;
//    }
//  }


}
