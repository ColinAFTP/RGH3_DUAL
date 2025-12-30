#include <functions.h>

int led = LED_BUILTIN;

void setup()
{
  pinMode(led, OUTPUT);
  Wire2.begin();                         // Join I2C bus

  Serial.begin(9600);                    // Start serial for output
  while(!Serial)
  {

  }
  
  // Clear the PuTTY terminal
  Serial.print("\033[2J");   // Clear screen
  Serial.print("\033[H");    // Move cursor to home position
}

void loop()
{
  readGapPatterns();
  delay(5000);
}