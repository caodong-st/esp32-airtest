#include <Arduino.h>
#include <Wire.h>
#include <SSD1306Wire.h>

// ==================== 数据结构 ====================
struct SensorData {
    uint16_t PM25;
    uint16_t PM10;
    float temperature;
    float humidity;
    bool valid;
};

SensorData data;
uint8_t buffer[20];
int idx = 0;
bool syncing = true;

// ==================== OLED 配置 ====================
#define OLED_SDA 18
#define OLED_SCL 17
SSD1306Wire display(0x3C, OLED_SDA, OLED_SCL);

// ==================== 函数声明 ====================
void parseFrame();
void updateDisplay();
String getAirQuality(uint16_t pm25);

// ==================== Setup ====================
void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("PM2.5 Monitor Starting...");
    
    if (!display.init()) {
        Serial.println("OLED init failed");
    } else {
        display.flipScreenVertically();
        display.setFont(ArialMT_Plain_16);
        display.setTextAlignment(TEXT_ALIGN_LEFT);
    }
    
    Serial2.begin(9600, SERIAL_8N1, 16, 15);
    Serial.println("Sensor ready");
    delay(1000);
}

// ==================== Loop ====================
void loop() {
    while (Serial2.available()) {
        uint8_t byte = Serial2.read();
        
        if (syncing) {
            if (byte == 0x3C) {
                buffer[0] = byte;
                idx = 1;
                syncing = false;
            }
        } else {
            buffer[idx++] = byte;
            
            if (idx == 17) {
                parseFrame();
                updateDisplay();
                syncing = true;
                idx = 0;
            }
        }
    }
    delay(100);
}

// ==================== 解析函数 ====================
void parseFrame() {
    if (buffer[0] != 0x3C || buffer[1] != 0x02) {
        data.valid = false;
        return;
    }
    
    uint8_t sum = 0;
    for (int i = 0; i < 16; i++) {
        sum += buffer[i];
    }
    
    if (sum != buffer[16]) {
        data.valid = false;
        return;
    }
    
    data.PM25 = buffer[8] * 256 + buffer[9];
    data.PM10 = buffer[10] * 256 + buffer[11];
    
    uint8_t tempRaw = buffer[12];
    uint8_t tempDec = buffer[13];
    int8_t tempInt = (tempRaw & 0x80) ? -(tempRaw & 0x7F) : (tempRaw & 0x7F);
    data.temperature = tempInt + (tempDec * 0.1);
    
    data.humidity = buffer[14] + (buffer[15] * 0.1);
    data.valid = true;
    
    Serial.printf("PM2.5: %d | Temp: %.1f | Humi: %.1f\n", 
                  data.PM25, data.temperature, data.humidity);
}

// ==================== 显示函数 ====================
void updateDisplay() {
    if (!data.valid) return;
    
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "Air Quality");
    display.drawLine(0, 18, 128, 18);
    
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 25, String(data.PM25) + " ug/m3");
    
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 52, getAirQuality(data.PM25));
    
    display.display();
}

// ==================== 空气质量等级 ====================
String getAirQuality(uint16_t pm25) {
    if (pm25 <= 35) return "Good";
    else if (pm25 <= 75) return "Fair";
    else if (pm25 <= 115) return "Poor";
    else return "Bad";
}