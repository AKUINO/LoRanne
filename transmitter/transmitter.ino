#include <SPI.h>
#include <RH_RF95.h>
RH_RF95 rf95(8, 3); // Rocket Scream Mini Ultra Pro with the RFM95W

int ledPin = 13;

void setup() 
{
  // Ensure serial flash is not interfering with radio communication on SPI bus
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available
  if (!rf95.init()){
    Serial.println("init failed");
    digitalWrite(ledPin, LOW);
  }
  else{
    Serial.println("init successed");
    digitalWrite(ledPin, HIGH);
  }
  rf95.setFrequency(433.0);
}
 
void loop() {
  uint8_t data[] = "Hello, this is device 1";
  rf95.send(data, sizeof(data));
  rf95.waitPacketSent();
  Serial.println("envoi de données");
  delay(1000);
}

