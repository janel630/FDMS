#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>


RF24 radio(7, 8);               // nRF24L01 (CE,CSN)

RF24Network network(radio);      // Include the radio in the network

//addresses of each Nano in octal format
const uint16_t main1 = 00;
const uint16_t window = 04;   

//header for sending
RF24NetworkHeader header0(main1);

bool ok4 = false;   //main and window


//temporary data 
RF24NetworkHeader header;
unsigned long incomingData;


//signal 0 = standby; 1 = lower; 2 = raise
//confirm 0 = standby; 1 =success ; 2 = fail
int window_signal = 440;
int window_confirm = 440;


const int droppin= 3;
const int raisepin = 5;

//for the voltage sensor
const int SensorValue = A2;
float volt = 0.0;
int windowbatt = -1;
int prevbatt = -1;
bool startsendbatt = true;

void setup() {
  Serial.begin(9600);
  pinMode(droppin, OUTPUT);
  pinMode(raisepin, OUTPUT);
  pinMode(SensorValue, INPUT);
  SPI.begin();
  radio.begin();
  network.begin(90, window);  //(channel, node address)
  radio.setDataRate(RF24_2MBPS);

}

void loop() {
  network.update();

critical_battery(); // check battery

  if(network.available())
  {
    while (network.available())
    {
        network.read(header,  &incomingData, sizeof(incomingData));
  
      if(header.from_node == 0) //00 octal is 0 in decimal
      {
        window_signal = incomingData;
        Serial.print( "Received from: " + String(header.from_node) + " Data: " );
        Serial.println(window_signal);
      }
    }
  }

//lower or raise the barricade
  if(window_signal == 441)
  {
    Serial.println("lowering barricade");
    digitalWrite(droppin, HIGH);  //lower the barricade 
    delay(5000); //delay to finish
    digitalWrite(droppin, LOW); //stop lowering the barricade
    window_confirm = 441;
  }
  else if(window_signal == 8306)
  {
    Serial.println("raising barricade");
    digitalWrite(raisepin, HIGH);  //raise the barricade 
    delay(5000); //delay to finish
    digitalWrite(raisepin, LOW); //stop raising the barricade
    window_confirm = 441;
  }
  else{
    window_confirm = 442;
  }

  //send acknowledgement about the status of the barricade
  if(window_signal == 441 || window_signal == 8306)
  {
    while(ok4 == false)
    {
      ok4 = network.write(header0, &window_confirm, sizeof(window_confirm));
    }

    Serial.println("Sent acknowledgement");

    //flush
    ok4 =false;
    window_signal = 440;
    window_confirm = 440;
  }

}

void critical_battery(){                       //for critical condition of the battery
  
  int i=0;
  
  while(i < 5)
  {
    int voltage = analogRead(SensorValue);
    Serial.print(voltage);
    Serial.print("   ");
    volt = ((24 * voltage)/1024.0);
    i++;
  }  

  windowbatt = volt;
  Serial.print(windowbatt);
  Serial.println(" V");

  
if(startsendbatt)   //send battery condition only once for startup
{
  while(ok4 == false)
    {
      ok4 = network.write(header0, &windowbatt, sizeof(windowbatt));
    }
    ok4 = false;

    startsendbatt =false;
}
else 
{
  if(windowbatt<=6 && prevbatt != windowbatt)
  {
  prevbatt = windowbatt;
  while(ok4 == false)
    {
      ok4 = network.write(header0, &windowbatt, sizeof(windowbatt));
    }
    ok4 = false;
  }
}

}
