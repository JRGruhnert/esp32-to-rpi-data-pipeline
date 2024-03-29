import re
from typing import NamedTuple

import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient

# TODO CHANGE THE VARIABLES WITH UR SETTINGS
ADDRESS = 'IP address'
INFLUXDB_USER = 'influxdb_user'
INFLUXDB_PASSWORD = 'influxdb_password'
INFLUXDB_DATABASE = 'sensor_stations'
MQTT_USER = 'mqtt_user'
MQTT_PASSWORD = 'mqtt_password'
MQTT_TOPIC = 'home/+/+'
MQTT_REGEX = 'home/([^/]+)/([^/]+)' #regex matching durch json ersetzen
MQTT_CLIENT_ID = 'mqtt_client_id'


influxdb_client = InfluxDBClient(ADDRESS, 8086, INFLUXDB_USER, INFLUXDB_PASSWORD, None)

class SensorData(NamedTuple):
    location: str
    measurement: str
    value: float

def on_connect(client, userdata, flags, rc):
    client.subscribe(MQTT_TOPIC)
    print('Connected')

def _parse_mqtt_message(topic, payload):
    match = re.match(MQTT_REGEX, topic)
    if match:
        location = match.group(1)
        measurement = match.group(2)
        if measurement == 'status':
            return None
        return SensorData(location, measurement, float(payload))
    else:
        return None

def _send_sensor_data_to_influxdb(sensor_data):
    #TODO: SensorDaten direkt als json bekommen
    as_json = [
        {
            'measurement': sensor_data.measurement,
            'tags': {
                'location': sensor_data.location
            },
            'fields': {
                'value': sensor_data.value
            }
        }
    ]
    influxdb_client.write_points(as_json)

def on_message(client, userdata, msg):
   #print(msg.topic + ' ' + str(msg.payload))
    sensor_data = _parse_mqtt_message(msg.topic, msg.payload.decode('utf-8'))
    if sensor_data is not None:
        _send_sensor_data_to_influxdb(sensor_data)

def _init_influxdb_database():
    databases = influxdb_client.get_list_database()
    if len(list(filter(lambda x: x['name'] == INFLUXDB_DATABASE, databases))) == 0:
        influxdb_client.create_database(INFLUXDB_DATABASE)
    influxdb_client.switch_database(INFLUXDB_DATABASE)

def main():
    _init_influxdb_database()

    mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    mqtt_client.connect(ADDRESS, 1883)
    mqtt_client.loop_forever()


if __name__ == '__main__':
    print('bridge running...')
    main()()
