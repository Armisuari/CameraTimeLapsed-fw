#include <Arduino.h>

#include <stdio.h>
#include <pb_encode.h>
#include <pb_decode.h>

#include "live_stream.pb.h"

/* This is the buffer where we will store our message. */
uint8_t buffer[128];
size_t message_length;
bool status;

void setup()
{
  Serial.begin(115200);
}

void loop()
{

  // ENCODE
  open_gopro_RequestSetLiveStreamMode message = open_gopro_RequestSetLiveStreamMode_init_zero;

  pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));

  message.minimum_bitrate = 720;
  strcpy(message.url, "rtmp://angkasatimelapse.com/live/mykey");
  message.maximum_bitrate = 1080;

  status = pb_encode(&ostream, open_gopro_RequestSetLiveStreamMode_fields, &message);
  message_length = ostream.bytes_written;

  if (!status)
  {
    Serial.printf("Encoding failed: %s\n", PB_GET_ERROR(&ostream));
  }
  byte set_livestream_command[] = {0xF1, 0x79};

  for (int i = 0; i < 128; i++)
  {
    Serial.print(buffer[i]);
  }

  // DECODE
  open_gopro_RequestSetLiveStreamMode message2 = open_gopro_RequestSetLiveStreamMode_init_zero;

  pb_istream_t istream = pb_istream_from_buffer(buffer, message_length);

  status = pb_decode(&istream, open_gopro_RequestSetLiveStreamMode_fields, &message2);

  if (!status)
  {
    Serial.printf("Decoding failed: %s\n", PB_GET_ERROR(&istream));
  }

  Serial.println(message.url);
  Serial.println(message.minimum_bitrate);
  Serial.println(message.maximum_bitrate);
  Serial.println(sizeof(buffer));
  Serial.println(ostream.bytes_written);
  delay(3000);
}