float getCO2UART() {
  byte com[] = {0xff, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};  //Befehl zum Auslesen der CO2 Konzentration
  byte return1[9];                                                      //hier kommt der zurückgegeben Wert des ersten Sensors rein

  while (Serial1.available() > 0) {    //Hier wird der Buffer der Seriellen Schnittstelle gelehrt
    Serial1.read();
  }

  Serial1.write(com, 9);                       //Befehl zum Auslesen der CO2 Konzentration
  Serial1.readBytes(return1, 9);               //Auslesen der Antwort

  int concentration = 0;                      //CO2-Konzentration des Sensors
  if (getCheckSum(return1) != return1[8]) {
    Serial.println("Checksum error");
  } else {
    concentration = return1[2] * 256 + return1[3]; //Berechnung der Konzentration
//    Serial.print("MH-Z19 sample OK: ");
//    Serial.print(concentration); ; Serial.println(" ppm");
  }
  
  display.setCursor(80, 3);
  display.print(concentration);
  display.print(" PPM");
  display.display();

  return concentration;
}


byte getCheckSum(byte *packet)
{
  byte checksum = 0;
  for ( int i = 1; i < 8; i++)
  {
    checksum += packet[i];
  }
  checksum = 0xFF - checksum;
  checksum += 1;
  return checksum;
}
