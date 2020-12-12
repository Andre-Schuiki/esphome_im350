import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.config_validation import string_strict, Invalid, boolean, int_, string
from esphome import pins
from esphome.components import sensor
from esphome.const import CONF_ECHO_PIN, CONF_ID, CONF_TRIGGER_PIN, \
    CONF_TIMEOUT, UNIT_WATT, UNIT_WATT_HOURS, UNIT_VOLT_AMPS_REACTIVE, CONF_BINDKEY, CONF_RX_PIN, CONF_TX_PIN, ICON_POWER

siemens_im350_ns = cg.esphome_ns.namespace('siemens_im350')
SmartMeterSensorComponent = siemens_im350_ns.class_('SmartMeterSensorComponent', sensor.Sensor, cg.PollingComponent)

def validate_decryption_key(value):
    value = string_strict(value)
    parts = [value[i:i+2] for i in range(0, len(value), 2)]
    if len(parts) != 16:
        raise Invalid("Decryption key must consist of 16 hexadecimal numbers")
    parts_int = []
    if any(len(part) != 2 for part in parts):
        raise Invalid("Decryption key must be format XX")
    for part in parts:
        try:
            parts_int.append(int(part, 16))
        except ValueError:
            raise Invalid("Decryption key must be hex values from 00 to FF")

    return ''.join(f'{part:02X}' for part in parts_int)

def validate_test_data(value):
    value = string_strict(value)
    parts = [value[i:i+2] for i in range(0, len(value), 2)]
    if len(parts) != 123:
        raise Invalid("Test Data must consist of 123 hexadecimal numbers, starting and ending with 7E")
    parts_int = []
    if any(len(part) != 2 for part in parts):
        raise Invalid("Test Data must be format XX")
    for part in parts:
        try:
            parts_int.append(int(part, 16))
        except ValueError:
            raise Invalid("Test Data must be hex values from 00 to FF")

    return ''.join(f'{part:02X}' for part in parts_int)

CONF_DECRYPTION_KEY = 'decryption_key'
CONF_BUILTIN_LED_GPIO = 'led_builtin_gpio'
CONF_COUNTER_READING_P_IN = 'counter_reading_p_in'
CONF_COUNTER_READING_P_OUT = 'counter_reading_p_out'
CONF_COUNTER_READING_Q_IN = 'counter_reading_q_in'
CONF_COUNTER_READING_Q_OUT = 'counter_reading_q_out'
CONF_POWER_USAGE_IN = 'current_power_usage_in'
CONF_POWER_USAGE_OUT = 'current_power_usage_out'
UNIT_VOLT_AMPS_REACTIVE_HOURS = 'varh'
CONF_TEST_DATA = 'test_data'
CONF_USE_TEST_DATA = 'use_test_data'
CONF_DELAY_BEFORE_READING_DATA = 'delay_before_reading_data'
CONF_MAX_WAIT_TIME_FOR_READING_DATA = 'max_wait_time_for_reading_data'
CONF_NTP_SERVER = 'ntp_server'
CONF_NTP_GMT_OFFSET = 'ntp_gmt_offset'
CONF_NTP_DAYLIGHT_OFFSET = 'ntp_daylight_offset'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(SmartMeterSensorComponent),
    cv.Required(CONF_DECRYPTION_KEY): validate_decryption_key,
    cv.Required(CONF_TRIGGER_PIN): pins.gpio_output_pin_schema,
    cv.Required(CONF_RX_PIN): pins.gpio_output_pin_schema,
    cv.Required(CONF_TX_PIN): pins.gpio_output_pin_schema,

    cv.Optional(CONF_USE_TEST_DATA, default=False): boolean,
    cv.Optional(CONF_TEST_DATA): validate_test_data,
    cv.Optional(CONF_DELAY_BEFORE_READING_DATA, default=1000): int_,
    cv.Optional(CONF_MAX_WAIT_TIME_FOR_READING_DATA, default=1100): int_,

    cv.Optional(CONF_NTP_SERVER, default="pool.ntp.org"): string,
    cv.Optional(CONF_NTP_GMT_OFFSET, default=3600): int_,
    cv.Optional(CONF_NTP_DAYLIGHT_OFFSET, default=3600): int_,
    
    cv.Optional(CONF_BUILTIN_LED_GPIO, default=2): pins.gpio_output_pin_schema,

    cv.Optional(CONF_COUNTER_READING_P_IN): sensor.sensor_schema(UNIT_WATT_HOURS, ICON_POWER, 2),
    cv.Optional(CONF_COUNTER_READING_P_OUT): sensor.sensor_schema(UNIT_WATT_HOURS, ICON_POWER, 2),
    cv.Optional(CONF_COUNTER_READING_Q_IN): sensor.sensor_schema(UNIT_VOLT_AMPS_REACTIVE_HOURS, ICON_POWER, 2),
    cv.Optional(CONF_COUNTER_READING_Q_OUT): sensor.sensor_schema(UNIT_VOLT_AMPS_REACTIVE_HOURS, ICON_POWER, 2),
    cv.Optional(CONF_POWER_USAGE_IN): sensor.sensor_schema(UNIT_WATT, ICON_POWER, 2),
    cv.Optional(CONF_POWER_USAGE_OUT): sensor.sensor_schema(UNIT_WATT, ICON_POWER, 2),
}).extend(cv.polling_component_schema('5s')).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    cg.add(var.set_decryption_key(config[CONF_DECRYPTION_KEY]))

    trigger = yield cg.gpio_pin_expression(config[CONF_TRIGGER_PIN])
    cg.add(var.set_trigger_pin(trigger))

    rx_pin = yield cg.gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_uart_rx_pin(rx_pin))

    tx_pin = yield cg.gpio_pin_expression(config[CONF_TX_PIN])
    cg.add(var.set_uart_tx_pin(tx_pin))

    builtin_led_pin = yield cg.gpio_pin_expression(config[CONF_BUILTIN_LED_GPIO])
    cg.add(var.set_builtin_led_pin(builtin_led_pin))

    if CONF_NTP_SERVER in config:
        cg.add(var.set_ntp_server(config[CONF_NTP_SERVER]))
    if CONF_NTP_GMT_OFFSET in config:
        cg.add(var.set_ntp_gmt_offset(config[CONF_NTP_GMT_OFFSET]))
    if CONF_NTP_DAYLIGHT_OFFSET in config:
        cg.add(var.set_ntp_daylight_offset(config[CONF_NTP_DAYLIGHT_OFFSET]))


    if CONF_DELAY_BEFORE_READING_DATA in config:
        cg.add(var.set_delay_before_reading_data(config[CONF_DELAY_BEFORE_READING_DATA]))
    if CONF_MAX_WAIT_TIME_FOR_READING_DATA in config:
        cg.add(var.set_max_wait_time_for_reading_data(config[CONF_MAX_WAIT_TIME_FOR_READING_DATA]))
    if CONF_TEST_DATA in config:
        cg.add(var.set_test_data(config[CONF_TEST_DATA]))
    if CONF_COUNTER_READING_P_IN in config:
        sens = yield sensor.new_sensor(config[CONF_COUNTER_READING_P_IN])
        cg.add(var.set_counter_reading_p_in(sens))
    if CONF_COUNTER_READING_P_OUT in config:
        sens = yield sensor.new_sensor(config[CONF_COUNTER_READING_P_OUT])
        cg.add(var.set_counter_reading_p_out(sens))
    if CONF_COUNTER_READING_Q_IN in config:
        sens = yield sensor.new_sensor(config[CONF_COUNTER_READING_Q_IN])
        cg.add(var.set_counter_reading_q_in(sens))
    if CONF_COUNTER_READING_Q_OUT in config:
        sens = yield sensor.new_sensor(config[CONF_COUNTER_READING_Q_OUT])
        cg.add(var.set_counter_reading_q_out(sens))
    if CONF_POWER_USAGE_IN in config:
        sens = yield sensor.new_sensor(config[CONF_POWER_USAGE_IN])
        cg.add(var.set_current_power_usage_in(sens))
    if CONF_POWER_USAGE_OUT in config:
        sens = yield sensor.new_sensor(config[CONF_POWER_USAGE_OUT])
        cg.add(var.set_current_power_usage_out(sens))