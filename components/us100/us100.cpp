#include "us100.h"
#include "esphome/core/log.h"

namespace esphome {
namespace us100 {

static const char *const TAG = "us100";

/* To start measuring the distance, output a 0x55 over the serial port and
 * read back the two byte distance in high byte, low byte format. The
 * distance returned is measured in millimeters. Use the following formula
 * to obtain the distance as millimeters:
 *
 *     Millimeters = FirstByteRead * 256 + SecondByteRead
 *
 * This module can also output the temperature when using serial output
 * mode. To read the temperature, output a 0x50 byte over the serial port
 * and read back a single temperature byte. The actual temperature is
 * obtained by using the following formula:
 *
 *     Celsius = ByteRead - 45
 */

void US100Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up US100...");
}

void US100Component::loop() {
  if (this->bytes_expected_ == 2 && this->available() >= 2) {
    // we're expecting a distance measurement to come in, and there are
    // enough bytes for it, process it
    uint8_t b1 = this->read();
    uint8_t b2 = this->read();
    uint16_t mm = (static_cast<uint16_t>(b1) << 8) | b2;
    if ((mm > 1) && (mm < 10000)) {
      ESP_LOGV(TAG, "Distance is %u mm", mm);
      if (this->distance_sensor_ != nullptr) {
        this->distance_sensor_->publish_state(mm);
      }
    }
    // finished with distance measurement, move on to temperature
    this->flush();
    this->write(0x50);  // tell the US100 to start a temperature measurement
    this->bytes_expected_ = 1;  // we should start looking for a temperature reading
  } else if (this->bytes_expected_ == 1 && this->available() >= 1) {
    // we are looking for a temperature and there are bytes to read
    uint8_t raw_temp = this->read();

    if ((raw_temp > 1) && (raw_temp < 130)) {
      int16_t temp_c = static_cast<int16_t>(raw_temp) - 45;
      ESP_LOGV(TAG, "Temperature is %d °C", temp_c);
    
      if (this->temperature_sensor_ != nullptr) {
        this->temperature_sensor_->publish_state(temp_c);
      }
    }
    this->bytes_expected_ = 0;  // stop looking for bytes
  }
}

void US100Component::update() {
  this->flush();
  this->write(0x55);  // tell the US100 to start a distance measurement
  this->bytes_expected_ = 2;  // tell loop() that it should start looking for a distance
}

void US100Component::dump_config() {
  ESP_LOGCONFIG(TAG, "US100:");
  LOG_UPDATE_INTERVAL(this);
  this->check_uart_settings(9600);
  LOG_SENSOR("  ", "Distance", this->distance_sensor_);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
}

}  // namespace us100
}  // namespace esphome
