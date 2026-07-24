#include "us100.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cmath>

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
  this->reset_waiting_state();
}

void US100Component::publish_unavailable(sensor::Sensor *sensor) {
  if (sensor != nullptr) {
    sensor->publish_state(NAN);
  }
}

void US100Component::reset_waiting_state() {
  this->bytes_expected_ = 0;
  this->request_started_ms_ = 0;
}

void US100Component::loop() {
  if (this->bytes_expected_ != 0 && this->request_started_ms_ != 0 &&
      (millis() - this->request_started_ms_) > kResponseTimeoutMs) {
    ESP_LOGW(TAG, "No response from US100 within %lu ms; resetting UART state",
             static_cast<unsigned long>(kResponseTimeoutMs));
    this->publish_unavailable(this->distance_sensor_);
    this->publish_unavailable(this->temperature_sensor_);
    this->flush();
    this->reset_waiting_state();
    return;
  }

  if (this->bytes_expected_ == 2 && this->available() >= 2) {
    // we're expecting a distance measurement to come in, and there are
    // enough bytes for it, process it
    uint8_t b1 = this->read();
    uint8_t b2 = this->read();
    uint16_t mm = (static_cast<uint16_t>(b1) << 8) | b2;
    
    ESP_LOGD(TAG, "Distance RAW: b1=0x%02X (%u), b2=0x%02X (%u) -> %u mm", 
             b1, b1, b2, b2, mm);
    
    if ((mm > 1) && (mm < 10000)) {
      ESP_LOGI(TAG, "Distance VALID: %u mm", mm);
      if (this->distance_sensor_ != nullptr) {
        this->distance_sensor_->publish_state(mm);
      }
    } else {
      ESP_LOGW(TAG, "Distance OUT OF RANGE: %u mm (ignored)", mm);
      this->publish_unavailable(this->distance_sensor_);
    }
    
    // finished with distance measurement, move on to temperature
    this->flush();
    delay(kBetweenMeasurementsDelayMs);
    this->write(0x50);  // tell the US100 to start a temperature measurement
    this->bytes_expected_ = 1;  // we should start looking for a temperature reading
    this->request_started_ms_ = millis();
  } else if (this->bytes_expected_ == 1 && this->available() >= 1) {
    // we are looking for a temperature and there are bytes to read
    uint8_t raw_temp = this->read();
    int16_t temp_c = static_cast<int16_t>(raw_temp) - 45;
    
    ESP_LOGD(TAG, "Temperature RAW: raw=0x%02X (%u) -> %d °C", 
             raw_temp, raw_temp, temp_c);

    if ((raw_temp > 1) && (raw_temp < 130)) {
      ESP_LOGI(TAG, "Temperature VALID: %d °C", temp_c);
    
      if (this->temperature_sensor_ != nullptr) {
        this->temperature_sensor_->publish_state(temp_c);
      }
    } else {
      ESP_LOGW(TAG, "Temperature OUT OF RANGE: raw=0x%02X (%d °C) (ignored)", 
               raw_temp, temp_c);
      this->publish_unavailable(this->temperature_sensor_);
    }
    this->reset_waiting_state();
  }
}

void US100Component::update() {
  this->flush();
  this->write(0x55);  // tell the US100 to start a distance measurement
  this->bytes_expected_ = 2;  // tell loop() that it should start looking for a distance
  this->request_started_ms_ = millis();
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
