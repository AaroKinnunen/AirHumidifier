#include <Arduino.h>
#include <ArduinoBLE.h>
#include <Arduino_HTS221.h>

// Function predefinitions
void changeLevel(int newLevel);
void changeMode(int newMode);

enum Mode
{
  AUTO_MODE,
  USER_MODE
};

#define atomizerr D2 // atomizer pin

Mode mode = USER_MODE; // Default mode
int level = 0;         // humidity min-level

int temperature = 0;
int humidity = 0;

// simple timer for auto-mode so humidifier works on 5 second bursts and then is off 2 seconds to check if the minimum humidity level is reached
unsigned long StartTime = 0;    // starting time for loop
unsigned long full_time = 7000; // full time for the mist cycle

BLEService speedService("180A");                                                   // BLE Service that holds all the characteristics below
BLEByteCharacteristic levelCharacteristic("2A67", BLERead | BLEWrite | BLENotify); // GATT name "Location and Speed"
BLEByteCharacteristic modeCharacteristic("2BA3", BLERead | BLEWrite);              // GATT name = "Media state"
BLEByteCharacteristic temperatureCharasteristic("2A6E", BLERead | BLENotify);      // GATT name = "Temperature"
BLEByteCharacteristic humidityCharasteristic("2A6F", BLERead | BLENotify);         // GATT name = "Humidity"
BLEByteCharacteristic ledcharasteristic("2A57", BLERead | BLEWrite);               // led to show more clearly that humidifier is on or off

void setup()
{
  Serial.begin(9600);

  if (!HTS.begin())
  {
    Serial.println("Failed to initialize humidity temperature sensor!");
  }

  if (!BLE.begin())
  {
    Serial.println("- Starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }

  BLE.setLocalName("Nano 33 BLE Sense"); // Name of the arduino. The app only searches devices with this name
  BLE.setAdvertisedService(speedService);

  // add the characteristics to the service
  speedService.addCharacteristic(levelCharacteristic);       // Minimum Humidity level
  speedService.addCharacteristic(modeCharacteristic);        // Mode (0 = AUTO, 1 = USER)
  speedService.addCharacteristic(temperatureCharasteristic); // Temperature data
  speedService.addCharacteristic(humidityCharasteristic);    // Humidity data
  speedService.addCharacteristic(ledcharasteristic);         // Led

  // add service
  BLE.addService(speedService);

  // set the initial value for the characteristic:
  levelCharacteristic.writeValue(0); // Default level = 0
  modeCharacteristic.writeValue(1);  // Default mode = USER
  ledcharasteristic.writeValue(0);

  // start advertising
  BLE.advertise();
}

void loop()
{
  BLEDevice central = BLE.central();

  // Read sensor values
  int newTemperature = HTS.readTemperature();
  int newHumidity = HTS.readHumidity();

  if (levelCharacteristic.written())
  { // User has send new level
    Serial.println("Written to level");
    int newLevel = levelCharacteristic.value();
    changeLevel(newLevel);
  }
  if (modeCharacteristic.written())
  { // User has send new mode
    Serial.println("Written to mode");
    int newMode = modeCharacteristic.value();
    changeMode(newMode);
  }
  if (temperature != newTemperature)
  { // If temperature has changed, notify master
    temperature = newTemperature;
    temperatureCharasteristic.writeValue(newTemperature);
  }
  if (humidity != newHumidity)
  { // If humidity has changed, notify master
    humidity = newHumidity;
    humidityCharasteristic.writeValue(newHumidity);
  }
  switch (ledcharasteristic.value())
  { // any value other than 0
  case 01:
    // Serial.println("LED on");
    digitalWrite(LED_BUILTIN, HIGH);  // will turn the LED on
    digitalWrite(atomizerr, HIGH);    // will turn atomizer on
    break;
  default:
    // Serial.println(F("LED off"));
    digitalWrite(LED_BUILTIN, LOW);   // will turn the LED off
    digitalWrite(atomizerr, LOW);     // will turn atomizer off
    break;
  }

  /*
   if(!central){
      digitalWrite(LED_BUILTIN, LOW);          // will turn the LED off if disconnected
      ledcharasteristic.writeValue(0);
    }
*/
  unsigned long CheckTime = millis();                // millis timer starts when powered on
  unsigned long elapsedTime = CheckTime - StartTime; // elapsed time for one loop cycle

  switch (mode)
  {
  case AUTO_MODE:
  {
    if (humidity < level)
    {
      if (elapsedTime > full_time)
      {
        StartTime = CheckTime; // "resets" the starting time after every loop
      }
      if (elapsedTime < 5000) // turn on for 5 seconds 
      {
        digitalWrite(LED_BUILTIN, HIGH); // will turn the LED on
        digitalWrite(atomizerr, HIGH);   // will turn atomizer on
      }
      if (elapsedTime >= 5000 && elapsedTime < 7000)  //turn off for 2 seconds
      {
        digitalWrite(LED_BUILTIN, LOW); // will turn the LED off
        digitalWrite(atomizerr, LOW);   // will turn atomizer off
      }
    }
    if (humidity >= level)  //when minimum level is reached then turn off the atomizer and led
    {
      digitalWrite(LED_BUILTIN, LOW); // will turn the LED off
      digitalWrite(atomizerr, LOW);   // will turn atomizer off
    }
    break;
  }

  case USER_MODE:
  {
    break;
  }
  }
}

void changeMode(int newMode)
{
  if (newMode == 0)
  {
    mode = AUTO_MODE;
  }
  else if (newMode == 1)
  {
    mode = USER_MODE;
  }
}

void changeLevel(int newLevel)
{
  if (newLevel >= 0 && newLevel < 100)
  {
    level = newLevel;
  }
}