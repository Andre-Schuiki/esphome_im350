#pragma once
#include <iostream>
#include <esp_attr.h>
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"
#include <Arduino.h>


namespace esphome {
    namespace siemens_im350 {

        class SmartMeterSensorComponent : public sensor::Sensor, public PollingComponent {
        public:
            void set_decryption_key(const std::string &decryption_key);
            
            void set_use_test_data(bool use_test_data) { use_test_data_ = use_test_data; };
            void set_test_data(const std::string &test_data);

            void set_delay_before_reading_data(int delay_before_reading_data) { delay_before_reading_data_ = delay_before_reading_data; };
            void set_max_wait_time_for_reading_data(int max_wait_time_for_reading_data) { max_wait_time_for_reading_data_ = max_wait_time_for_reading_data; };

            void set_ntp_server(char* ntp_server) { ntp_server_ = ntp_server; };
            void set_ntp_gmt_offset(int ntp_gmt_offset_sec) { ntp_gmt_offset_sec_ = ntp_gmt_offset_sec; };
            void set_ntp_daylight_offset(int ntp_daylight_offset_sec) { ntp_daylight_offset_sec_ = ntp_daylight_offset_sec; };
            
            void set_trigger_pin(InternalGPIOPin *trigger_pin) { trigger_pin_ = trigger_pin; }
            void set_uart_rx_pin(InternalGPIOPin *uart_rx_pin) { uart_rx_pin_ = uart_rx_pin; }
            void set_uart_tx_pin(InternalGPIOPin *uart_tx_pin) { uart_tx_pin_ = uart_tx_pin; }
            void set_uart_inverted(bool invert_serial) { invert_serial_ = invert_serial; };
            void set_builtin_led_pin(InternalGPIOPin *builtin_led_pin) { builtin_led_pin_ = builtin_led_pin; }

            void set_counter_reading_p_in(sensor::Sensor *counter_reading_p_in) { counter_reading_p_in_ = counter_reading_p_in; }
            void set_counter_reading_p_out(sensor::Sensor *counter_reading_p_out) { counter_reading_p_out_ = counter_reading_p_out; }
            void set_counter_reading_q_in(sensor::Sensor *counter_reading_q_in) { counter_reading_q_in_ = counter_reading_q_in; }
            void set_counter_reading_q_out(sensor::Sensor *counter_reading_q_out) { counter_reading_q_out_ = counter_reading_q_out; }
            void set_current_power_usage_in(sensor::Sensor *current_power_usage_in) { current_power_usage_in_ = current_power_usage_in; }
            void set_current_power_usage_out(sensor::Sensor *current_power_usage_out) { current_power_usage_out_ = current_power_usage_out; }

            void request_data();
            void init_vector();
            void decrypt_text();
            uint32_t byte_to_uint32t(byte array[], unsigned int startByte);
            void parse_decrypted_data();
            void publish_data();
            void parse_timestamp();
            bool message_date_valid();
            bool message_valid();
            bool getLocalTime(struct tm * info, uint32_t ms);
            bool get_local_time();

            // ========== INTERNAL METHODS ==========
            // (In most use cases you won't need these)
            /// Set up pins and register interval.
            void setup() override;
            void dump_config() override;
            void update() override;
        protected:
            char* ntp_server_;
            int  ntp_gmt_offset_sec_;
            int   ntp_daylight_offset_sec_;
            int delay_before_reading_data_;
            int max_wait_time_for_reading_data_;
            bool use_test_data_;
            uint8_t test_data_[123];
            uint8_t decryption_key_[16];
            InternalGPIOPin *trigger_pin_;
            InternalGPIOPin *uart_rx_pin_;
            InternalGPIOPin *uart_tx_pin_;
            bool invert_serial_;
            InternalGPIOPin *builtin_led_pin_;
            sensor::Sensor *counter_reading_p_in_;
            sensor::Sensor *counter_reading_p_out_;
            sensor::Sensor *counter_reading_q_in_;
            sensor::Sensor *counter_reading_q_out_;
            sensor::Sensor *current_power_usage_in_;
            sensor::Sensor *current_power_usage_out_;
        };

    }  // namespace siemens_im350
}  // namespace esphome
