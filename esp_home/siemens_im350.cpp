// https://stackoverflow.com/questions/38034197/compile-error-stoi-is-not-a-member-of-std
#define _GLIBCXX_USE_C99 1
#include "siemens_im350.h"
#include "esphome/core/log.h"
#include <iostream>
#include <Arduino.h>
#include <Crypto.h>
#include <AES.h>
#include <GCM.h>
#include <time.h>

namespace esphome {
    namespace siemens_im350 {
 
        static const char *TAG = "siemens_im350.sensor";
        byte message[123];

        struct Vector_GCM {
            const char *name;
            uint8_t key[16];
            uint8_t ciphertext[90];
            uint8_t authdata[16];
            uint8_t iv[12];
            uint8_t tag[12];
            size_t authsize;
            size_t datasize;
            size_t tagsize;
            size_t ivsize;
            size_t keysize;
        };
        
        Vector_GCM vector_sm;
        GCM<AES128> *gcmaes128 = 0;

        int const decrypt_buffer_size = 90;
        byte decrypt_buffer[decrypt_buffer_size];

        uint32_t counter_reading_p_in_temp;
        uint32_t counter_reading_p_out_temp;
        uint32_t counter_reading_q_in_temp;
        uint32_t counter_reading_q_out_temp;
        uint32_t current_power_usage_in_temp;
        uint32_t current_power_usage_out_temp;


        // https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
        // NTP
        struct tm ntpTime;
        uint8_t current_time_day;
        uint8_t current_time_month;
        uint16_t current_time_year;

        // message date variables
        uint16_t message_year;
        uint8_t message_month;
        uint8_t message_day;
        uint8_t message_hour;
        uint8_t message_minute;
        uint8_t message_second;

        void SmartMeterSensorComponent::setup() {
            ESP_LOGCONFIG(TAG, "Setting up SmartMeter Sensor...");
            this->trigger_pin_->setup();
            this->trigger_pin_->digital_write(false);
            this->builtin_led_pin_->setup();
            this->trigger_pin_->digital_write(false);
            
            Serial2.begin(115200, SERIAL_8N1, this->uart_rx_pin_->get_pin(), this->uart_tx_pin_->get_pin());
        }
        void SmartMeterSensorComponent::update() {
            //init and get the time
            ESP_LOGD(TAG, "Try to get time from NTP..");
            configTime(ntp_gmt_offset_sec_, ntp_daylight_offset_sec_, ntp_server_);
            this->get_local_time();

            this->request_data();
            this->init_vector();

            uint8_t empty_message[123] = {};
            if (std::equal(std::begin(message), std::end(message), std::begin(empty_message))) {
                ESP_LOGD(TAG, "Message empty, skip message");
            }
            else {
                if (this->message_valid()) {
                    this->decrypt_text();
                    this->parse_decrypted_data();
                    
                    this->parse_timestamp();
                    if (this->message_date_valid()) {
                        this->publish_data();
                    }
                    else {
                        ESP_LOGD(TAG, "Message date invalid, skip message");
                    }
                }
            }
        }
        void SmartMeterSensorComponent::dump_config() {
            LOG_SENSOR("", "SmartMeter Sensor", this);
            ESP_LOGCONFIG(TAG, "  Decryption Key: %s", hexencode(this->decryption_key_, 16).c_str());
            LOG_PIN("  Trigger Pin: ", this->trigger_pin_);
            LOG_PIN("  UART RX Pin: ", this->uart_rx_pin_);
            LOG_PIN("  UART TX Pin: ", this->uart_tx_pin_);
            LOG_PIN("  Builtin LED Pin: ", this->builtin_led_pin_);
            ESP_LOGCONFIG(TAG, "  Delay befor reading data : %d", this->delay_before_reading_data_);
            ESP_LOGCONFIG(TAG, "  Maximum time for reading data : %d", this->max_wait_time_for_reading_data_);
            ESP_LOGCONFIG(TAG, "  NTP Server : %s", this->ntp_server_);
            ESP_LOGCONFIG(TAG, "  NTP GMT Offset : %d", this->ntp_gmt_offset_sec_);
            ESP_LOGCONFIG(TAG, "  NTP Daylight Offset : %d", this->ntp_daylight_offset_sec_);
            ESP_LOGCONFIG(TAG, "  Use Test Data : %s", this->use_test_data_ ? "true" : "false");
            ESP_LOGCONFIG(TAG, "  Test Data : %s", hexencode(this->test_data_, 16).c_str());
            LOG_UPDATE_INTERVAL(this);
        }

        void SmartMeterSensorComponent::set_decryption_key(const std::string &decryption_key) {
            uint8_t buffer[16];
            int splitLength=2;

            int NumSubstrings = decryption_key.length() / splitLength;

            for (auto i = 0; i < NumSubstrings; i++)
            {
                buffer[i] = std::stoi(decryption_key.substr(i * splitLength, splitLength), 0, 16);
            }

            // If there are leftover characters, create a shorter item at the end.
            if (decryption_key.length() % splitLength != 0)
            {
                buffer[-1] = std::stoi(decryption_key.substr(splitLength * NumSubstrings), 0, 16);
            }
            for (int i = 0; i < 16; i++) {
                decryption_key_[i] = buffer[i];
            }
        }

        void SmartMeterSensorComponent::set_test_data(const std::string &test_data) {
            uint8_t buffer[123];
            int splitLength=2;

            int NumSubstrings = test_data.length() / splitLength;

            for (auto i = 0; i < NumSubstrings; i++)
            {
                buffer[i] = std::stoi(test_data.substr(i * splitLength, splitLength), 0, 123);
            }

            // If there are leftover characters, create a shorter item at the end.
            if (test_data.length() % splitLength != 0)
            {
                buffer[-1] = std::stoi(test_data.substr(splitLength * NumSubstrings), 0, 123);
            }
            for (int i = 0; i < 123; i++) {
                test_data_[i] = buffer[i];
            }
        }

        void SmartMeterSensorComponent::request_data() {
            unsigned short serial_cnt = 0;
            if (use_test_data_ == true) {
                ESP_LOGW(TAG, "USE TEST DATA IS ACTIVE!");
                serial_cnt = 123;
                for (unsigned int i = 0; i < 123; i++) {
                    message[i] = test_data_[i];
                }
            }
            else {
                ESP_LOGD(TAG, "Try to read data from serial port.");
                memset(message, 0, 123);

                int cnt = 0;
                int readBuffer = 250;

                this->builtin_led_pin_->digital_write(true);
                this->trigger_pin_->digital_write(true);
                delay(delay_before_reading_data_);
                unsigned long requestMillis = millis();
                while ((Serial2.available()) && (cnt < readBuffer) && (millis()-requestMillis <= max_wait_time_for_reading_data_)) {
                message[cnt] = Serial2.read();
                if (message[0] != 0x7E && cnt == 0) {
                    continue;
                }
                else {
                    cnt++;
                }
                }

                this->builtin_led_pin_->digital_write(false);
                this->trigger_pin_->digital_write(false);
            }
        ESP_LOGD(TAG, "Done with reading data.");
        ESP_LOGD(TAG, "Received message: %s", hexencode(message, 123).c_str());
        // digitalWrite(data_request_gpio,LOW);
        }

        void SmartMeterSensorComponent::init_vector() {
            vector_sm.name = "Vector_SM";  // vector name
            for (unsigned int i = 0; i < 16; i++) {
                vector_sm.key[i] = this->decryption_key_[i];
            }

            for (unsigned int i = 0; i < 90; i++) {
                vector_sm.ciphertext[i] = message[i+30];
            }

            // i took this value from the gurux director tool
            byte AuthData[] = {0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf};
            for (int i = 0; i < 16; i++) {
                vector_sm.authdata[i] = AuthData[i];
            }

            for (int i = 0; i < 8; i++) {
                vector_sm.iv[i] = message[16+i]; // manufacturer + serialnumber 8 bytes
            }
            for (int i = 8; i < 12; i++) {
                vector_sm.iv[i] = message[18+i]; // frame counter
            }

            byte tag[12]; // 12x zero
            for (int i = 0; i < 12; i++) {
                vector_sm.tag[i] = tag[i];
            }

            vector_sm.authsize = 16;
            vector_sm.datasize = 90;
            vector_sm.tagsize = 12;
            vector_sm.ivsize  = 12;
            vector_sm.keysize  = 16;

            ESP_LOGVV(TAG, "IV: %s", hexencode(vector_sm.iv, vector_sm.ivsize).c_str());
            ESP_LOGVV(TAG, "Key: %s", hexencode(vector_sm.key, vector_sm.keysize).c_str());
            ESP_LOGVV(TAG, "Authdata: %s", hexencode(vector_sm.authdata, vector_sm.authsize).c_str());
            ESP_LOGVV(TAG, "Tag: %s", hexencode(vector_sm.tag, vector_sm.tagsize).c_str());
            ESP_LOGVV(TAG, "Encrypted Data (Ciphertext): %s", hexencode(vector_sm.iv, vector_sm.datasize).c_str());
        }

        void SmartMeterSensorComponent::decrypt_text() {
            gcmaes128 = new GCM<AES128>();
            gcmaes128->setKey(vector_sm.key, gcmaes128->keySize());
            gcmaes128->setIV(vector_sm.iv, vector_sm.ivsize);
            gcmaes128->decrypt((byte*)decrypt_buffer, vector_sm.ciphertext, vector_sm.datasize);
            delete gcmaes128;
            ESP_LOGD(TAG, "Decrypted Data: %s", hexencode(decrypt_buffer, decrypt_buffer_size).c_str());
        }

        uint32_t SmartMeterSensorComponent::byte_to_uint32t(byte array[], unsigned int startByte)
        {
            // https://stackoverflow.com/questions/12240299/convert-bytes-to-int-uint-in-c
            // convert 4 bytes to uint32
            uint32_t result;
            result = (uint32_t) array[startByte] << 24;
            result |=  (uint32_t) array[startByte+1] << 16;
            result |= (uint32_t) array[startByte+2] << 8;
            result |= (uint32_t) array[startByte+3];

            return result;
        }

        void SmartMeterSensorComponent::parse_decrypted_data() {
            ESP_LOGD(TAG, "Parsing decrypted data.");
            counter_reading_p_in_temp = this->byte_to_uint32t(decrypt_buffer, 57);
            counter_reading_p_out_temp = this->byte_to_uint32t(decrypt_buffer, 62);
            counter_reading_q_in_temp = this->byte_to_uint32t(decrypt_buffer, 67);
            counter_reading_q_out_temp = this->byte_to_uint32t(decrypt_buffer, 72);
            current_power_usage_in_temp = this->byte_to_uint32t(decrypt_buffer, 77);
            current_power_usage_out_temp = this->byte_to_uint32t(decrypt_buffer, 82);
        }

        void SmartMeterSensorComponent::publish_data() {
            ESP_LOGD(TAG, "Publish data");
            this->counter_reading_p_in_->publish_state(counter_reading_p_in_temp);
            this->counter_reading_p_out_->publish_state(counter_reading_p_out_temp);
            this->counter_reading_q_in_->publish_state(counter_reading_q_in_temp);
            this->counter_reading_q_out_->publish_state(counter_reading_q_out_temp);
            this->current_power_usage_in_->publish_state(current_power_usage_in_temp);
            this->current_power_usage_out_->publish_state(current_power_usage_out_temp);
        }

        void SmartMeterSensorComponent::parse_timestamp() {
            message_year = (decrypt_buffer[6] << 8) + (decrypt_buffer[7]);
            message_month = decrypt_buffer[8];
            message_day = decrypt_buffer[9];
            message_hour = decrypt_buffer[11];
            message_minute = decrypt_buffer[12];
            message_second = decrypt_buffer[13];
        }

        // used to see if decryption was successful...cause we cannot use the tag for validation?!
        bool SmartMeterSensorComponent::message_date_valid() {
            ESP_LOGVV(TAG, "DATE FROM NTP: %02d-%02d-%02d", current_time_year, current_time_month, current_time_day);
            
            ESP_LOGVV(TAG, "DATE FROM MESSAGE: %02d-%02d-%02d", message_year, message_month, message_day);

            if (current_time_year == message_year and current_time_month == message_month and current_time_day == message_day){
                ESP_LOGD(TAG, "Message Date is VALID!, ntp_date: %02d-%02d-%02d == message_date: %02d-%02d-%02d", current_time_year, current_time_month, current_time_day, message_year, message_month, message_day);
                return true;
            }
            else {
                ESP_LOGD(TAG, "Message Date is INVALID!, ntp_date: %02d-%02d-%02d !=  message_date: %02d-%02d-%02d", current_time_year, current_time_month, current_time_day, message_year, message_month, message_day);
                return false;
            }
        }

        // check if received message is starting and stopping with 7E
        bool SmartMeterSensorComponent::message_valid() {
            if (message[0] == 0x7E and message[sizeof(message)-1] == 0x7E) {
                ESP_LOGD(TAG, "Message Start/Stop Byte looks ok! (0x7E)");
                return true;
            }
            else {
                ESP_LOGD(TAG, "Message Start/Stop Byte is not 0x7E, skip this message!");
                return false;
            }
        }

        bool SmartMeterSensorComponent::get_local_time() {
            if(!getLocalTime(&ntpTime)){
                ESP_LOGD(TAG, "Failed to obtain time");
                return false;
            }
            else {
                current_time_day = ntpTime.tm_mday;
                current_time_month = ntpTime.tm_mon + 1; // Month is 0 - 11, add 1 to get a jan-dec 1-12 concept
                current_time_year = ntpTime.tm_year + 1900; // Year is # years since 1900

                ESP_LOGD(TAG, "Got time from NTP: %d-%02d-%02d", current_time_year, current_time_month, current_time_day);
                return true;
            }
        }

    }  // namespace siemens_im350
}  // namespace esphome
