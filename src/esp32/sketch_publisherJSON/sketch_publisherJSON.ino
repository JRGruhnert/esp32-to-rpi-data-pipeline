#include "PubSubClient.h"; 
#include "WiFi.h"; 
#include "bsec.h";
#include "time.h";

//TODO CHANGE VARIABLES WITH YOUR ACTUAL SETTINGS
// WiFi
const char* ssid = "ssid_here";
const char* wifi_password = "wifi_password_here";

// MQTT
const char* mqtt_server = "server_ip_here";  // IP of the MQTT broker (rasperry pi) local ip
const char* topic = "home/bedroom";
const char* location = "bedroom";
const char* mqtt_username = "mqtt_username_here";
const char* mqtt_password = "mqtt_password_here";
const char* clientID = "client_bedroom"; // MQTT client ID

//Instances of included librarys
WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); // 1883 is the listener port for the Broker
Bsec sensor; //sensor class

//SPI Pins
#define BME_SCK 18
#define BME_MISO 19
#define BME_MOSI 23
#define BME_CS 5


void setup() {
  Serial.begin(115200);
  SPI.begin();
  connect_Wifi();
  connect_MQTT();
  connect_Sensor();
}

void loop() {

  float humidity;
  float temperature;
  float gas;
  float pressure;

  unsigned long time_trigger = millis();
  if (sensor.run()) { // If new data is available
    temperature = sensor.rawTemperature;
    humidity = sensor.rawHumidity;
    gas = sensor.gasResistance;
    pressure = sensor.pressure;
  } else {
    checkSensorStatus();
  }

  unsigned int timestamp = getTime();

  send_JSON_message(humidity, temperature, pressure, gas, timestamp);

  delay(1000);   // print new values every 1 sec
}




void connect_Sensor() {
  sensor.begin(BME_CS, SPI);
  checkSensorStatus();

  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  sensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkSensorStatus();
}

void connect_Wifi(){
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Connect to the WiFi
  WiFi.begin(ssid, wifi_password);

  // Wait until the connection has been confirmed before continuing
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}


void connect_MQTT() {
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker!");
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }
}


void checkSensorStatus()
{
  if (sensor.status != BSEC_OK) {
    if (sensor.status < BSEC_OK) {
      Serial.println("BSEC error code : " + String(sensor.status));
    } else {
      Serial.println("BSEC warning code : " + String(sensor.status));
    }
  }

  if (sensor.bme680Status != BME680_OK) {
    if (sensor.bme680Status < BME680_OK) {
      Serial.println("BME680 error code : " + String(sensor.bme680Status));
    } else {
      Serial.println("BME680 warning code : " + String(sensor.bme680Status));
    }
  }
}

void send_JSON_message(float humidity, float temperature, float pressure, float gas, unsigned int timestamp) {

  // Create the message that will be send using mqtt
  String message = "SensorData,location=" + String(location) +" humidity=" + String(humidity) 
  + ",temperature=" + String(temperature) 
  + ",pressure=" + String(pressure) 
  + ",gas=" + String(gas);
  //+ " " + String(timestamp);

  //send to server
   while(!client.publish(topic, message.c_str())) {
    Serial.println("Measurments failed to send. Trying again...");
    connect_MQTT();
    delay(10);
  }
  Serial.println(message.c_str());
  Serial.println("Measurements sent!");
  
}

// Function that gets current epoch time
// TODO: I want to do it here but arduino only supports a local clock.
unsigned int getTime() {
  return (unsigned int) time(NULL);
}
