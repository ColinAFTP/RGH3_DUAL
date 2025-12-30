#include <functions.h>

int led = 13;

void setup()
{
  pinMode(led, OUTPUT);
  Wire2.begin(0x40);                  // Join I2C bus with address #8
  Wire2.onRequest(writeGapPatterns);   // Register event
  Wire2.setClock(1000000);

  Serial.begin(9600);                 // Start serial for output
  while(!Serial)
  {

  }
  
  // Clear the PuTTY terminal
  Serial.print("\033[2J");   // Clear screen
  Serial.print("\033[H");    // Move cursor to home position

  Serial.println("CPU 1 online and waiting... ");
  digitalWrite(led, HIGH); 

}

void loop()
{

   
  
}

