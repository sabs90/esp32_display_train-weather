/* Includes ------------------------------------------------------------------*/
#include "Arduino.h"
#include <stdlib.h>

/* Entry point ----------------------------------------------------------------*/
void setup()
{
  Serial.begin(9600);
  Serial.println("setup! Amorrisx");
}

/* The main loop -------------------------------------------------------------*/
void loop()
{
  Serial.println("Hello world! Amorrisx");
  delay(1000);
}
