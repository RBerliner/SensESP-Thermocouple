#include <Adafruit_MAX31856.h>
#include <Arduino.h>

#include "sensesp_app.h"
#include "sensesp_app_builder.h"
#include "sensors/max31856_thermocouple.h"
#include "signalk/signalk_output.h"
#include "transforms/linear.h"

#define SPI_CS_PIN 15
#define SPI_MOSI_PIN 13
#define SPI_MISO_PIN 12
#define SPI_CLK_PIN 14
#define DRDY_PIN 5

// SensESP builds upon the ReactESP framework. Every ReactESP application
// defines an "app" object vs defining a "main()" method.
ReactESP app([]() {

// Some initialization boilerplate when in debug mode...
#ifndef SERIAL_DEBUG_DISABLED
  SetupSerialDebug(115200);
#endif

// Create a builder object
  SensESPAppBuilder builder;

  // Create the global SensESPApp() object. If you add the line ->set_wifi("your ssid", "your password") you can specify
  // the wifi parameters in the builder. If you do not do that, the SensESP device wifi configuration hotspot will appear and you can use a web
  // browser pointed to 192.168.4.1 to configure the wifi parameters.

  sensesp_app = builder.set_hostname("EngineTemp")
                    ->set_standard_sensors(IP_ADDRESS)
                    ->set_sk_server("192.168.0.1", 3000)
                    ->get_app();

  // The "Signal K path" identifies this sensor to the Signal K server. Leaving
  // this blank would indicate this particular sensor (or transform) does not
  // broadcast Signal K data.
  // To find valid Signal K Paths that fits your need you look at this link:
  // https://signalk.org/specification/1.4.0/doc/vesselsBranch.html
  const char* sk_path = "propulsion.engine.temperature";

  // The "Configuration path" is combined with "/config" to formulate a URL
  // used by the RESTful API for retrieving or setting configuration data.
  // It is ALSO used to specify a path to the SPIFFS file system
  // where configuration data is saved on the MCU board. It should
  // ALWAYS start with a forward slash if specified. If left blank,
  // that indicates this sensor or transform does not have any
  // configuration to save, or that you're not interested in doing
  // run-time configuration.

  const char* exhaust_temp_config_path =
      "/propulsion/engine/temperature/read_delay";
  const char* linear_config_path =
      "/propulsion/engine/temperature/linear";

  // Create a sensor that is the source of our data, that will be read every
  // 1000 ms.
  const uint readDelay = 1000;
  // tcType:  MAX31856_TCTYPE_K;   // other types can be B, E, J, N, R, S, T
  auto* max31856tc = new MAX31856Thermocouple(
      SPI_CS_PIN, SPI_MOSI_PIN, SPI_MISO_PIN, SPI_CLK_PIN, DRDY_PIN,
      MAX31856_TCTYPE_K, readDelay, exhaust_temp_config_path);

  // A Linear transform takes its input, multiplies it by the multiplier, then
  // adds the offset, to calculate its output. The MAX31856TC produces
  // temperatures in degrees Celsius. We need to change them to Kelvin for
  // compatibility with Signal K.

  const float multiplier = 1.0;
  const float offset = 273.16;

  // Wire up the output of the analog input to the Linear transform,
  // and then output the results to the Signal K server.
  max31856tc->connect_to(new Linear(multiplier, offset, linear_config_path))
      ->connect_to(new SKOutputNumber(sk_path));

  // Start the SensESP application running
  sensesp_app->enable();
});