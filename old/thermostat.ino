#include <RCSwitch.h>
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

int sensorPin = A0;
int sensorValue = 0;
float temp = 0;
float tempTarget = 21.5;
boolean heatingOn = false;
RCSwitch mySwitch = RCSwitch();
YunServer server;

void setup() {
  Bridge.begin();
  Console.begin();
  mySwitch.enableTransmit(10);
  server.listenOnLocalhost();
  server.begin(); 
}

void turn_on_heating() {
  heatingOn = true;
  mySwitch.send(5592407, 24);
  Console.println("heating on");
}

void turn_off_heating() {
  heatingOn = false;
  Console.println("heating off");
  mySwitch.send(5592412, 24);
}

void loop() {
  YunClient client = server.accept();
  if (client) {
    String command = client.readStringUntil('/');
    tempTarget = command.toFloat();
  }
  sensorValue = analogRead(sensorPin);
  temp = (sensorValue / 4.095) * 0.2222;
  delay(3000);
  Console.print("temperature: ");
  Console.println(temp);
  if((temp < tempTarget) && !heatingOn) {
    turn_on_heating();
  }
  else if((temp > tempTarget) && heatingOn) {
    turn_off_heating();
  }
}
