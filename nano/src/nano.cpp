#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "Servo.h"

#define CHAN_NUM 0x7A // radio channel (transmitter = receiver)
#define LEFT_OUT 2
#define RIGHT_OUT 3
#define PITCH_OUT 4
#define YAW_OUT 5
#define THRUST_OUT 6 // PWM pin

RF24 radio(9, 10); // radio module pins
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; // possible pipes numbers

Servo engine;
Servo left_servo;
Servo right_servo;
Servo pitch_servo;
Servo yaw_servo;

int min_thrust = 1000; // old: 800, VGOOD: 1000, SunnySky: 1000
int max_thrust = 2000; // old: 2270, VGOOD: 2500, SunnySky: 2000
byte received_data[5];

void outputs_init()
{
  engine.attach(THRUST_OUT);
  left_servo.attach(LEFT_OUT);
  right_servo.attach(RIGHT_OUT);
  pitch_servo.attach(PITCH_OUT);
  yaw_servo.attach(YAW_OUT);

  // engine calibration
  engine.writeMicroseconds(max_thrust);
  delay(2000);
  engine.writeMicroseconds(min_thrust);
  delay(5000);
}

void receiver_init()
{
  radio.begin(); // radio module activation
  radio.setAutoAck(true); // respond to the received packet (transmitter = receiver)
  radio.setRetries(0, 15); // number of retry attempts and delay
  radio.enableAckPayload();
  radio.setPayloadSize(32); // packet size (bytes)

  radio.openReadingPipe(1, address[0]); // receiver
  radio.setChannel(CHAN_NUM);

  radio.setPALevel(RF24_PA_MAX); // RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX (transmitter = receiver)
  // the lower the data rate, the higher the range
  radio.setDataRate(RF24_250KBPS); // RF24_2MBPS, RF24_1MBPS, RF24_250KBPS (transmitter = receiver)

  radio.powerUp();
  radio.startListening(); // receiver
}

void setup()
{
  Serial.begin(9600);

  outputs_init();
  receiver_init();
}

void print_data()
{
  size_t data_size = sizeof(received_data) / sizeof(received_data[0]);
  // mode thrust roll pitch yaw
  for (size_t i = 0; i < data_size; i++)
  {
    Serial.print(received_data[i]);
    Serial.print(' ');
  }

  Serial.println();
}

void write_data()
{
  // int mode = received_data[0];

  int thrust_val = map(received_data[1], 0, 255, min_thrust, max_thrust);
  thrust_val = constrain(thrust_val, min_thrust, max_thrust);
  engine.writeMicroseconds(thrust_val);

  int roll_val = received_data[2];
  left_servo.write(roll_val);
  right_servo.write(roll_val);

  int pitch_val = received_data[3];
  pitch_servo.write(pitch_val);

  int yaw_val = received_data[4];
  yaw_servo.write(yaw_val);
}

void loop()
{
  byte pipeNo;
  while (radio.available(&pipeNo))
  {
    radio.read(&received_data, sizeof(received_data)); // receive packet
    print_data();
    write_data();
  }
}
