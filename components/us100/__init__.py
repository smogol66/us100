import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import (
    CONF_ID,
    CONF_TEMPERATURE,
    CONF_DISTANCE,
    DEVICE_CLASS_DISTANCE,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_MILLIMETER,
)

DEPENDENCIES = ["uart", "sensor"]

us100_ns = cg.esphome_ns.namespace("us100")
US100Component = us100_ns.class_("US100Component", cg.PollingComponent, uart.UARTDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(US100Component),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_DISTANCE): sensor.sensor_schema(
                unit_of_measurement=UNIT_MILLIMETER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_DISTANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if temperature_config := config.get(CONF_TEMPERATURE):
        temp_sensor = await sensor.new_sensor(temperature_config)
        cg.add(var.set_temperature_sensor(temp_sensor))

    if distance_config := config.get(CONF_DISTANCE):
        dist_sensor = await sensor.new_sensor(distance_config)
        cg.add(var.set_distance_sensor(dist_sensor))
