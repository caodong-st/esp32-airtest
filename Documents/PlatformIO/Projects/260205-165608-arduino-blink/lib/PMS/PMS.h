#ifndef PMS_H
#define PMS_H

#include <Arduino.h>

class PMS {
public:
    struct DATA {
        uint16_t PM_AE_UG_2_5;
    };

    PMS(HardwareSerial& serial) : _serial(serial) {}

    bool read(DATA& data) {
        while (_serial.available() < 32) {
            delay(10);
            if (!wait) return false;
            wait--;
        }
        wait = 100;

        uint8_t buffer[32];
        _serial.readBytes(buffer, 32);

        // 简单校验：起始字节 0x42, 0x4D
        if (buffer[0] != 0x42 || buffer[1] != 0x4D) {
            return false;
        }

        // 计算校验和
        uint16_t sum = 0;
        for (int i = 0; i < 30; i++) sum += buffer[i];
        if (sum != (buffer[30] << 8 | buffer[31])) {
            return false;
        }

        data.PM_AE_UG_2_5 = (buffer[12] << 8) | buffer[13];
        return true;
    }

private:
    HardwareSerial& _serial;
    int wait = 100;
};

#endif
