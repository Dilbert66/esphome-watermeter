#include "esphome.h"
#include "Wire.h"

class WaterMeterSensor : public PollingComponent , public Sensor {
 public:
 private:
 unsigned long last_publish = 0;
 unsigned long now = 0;
 unsigned int crossings = 0;

 int new_val = 0;
 int old_val = 0;
 
#define hmc5883l_address 0x1E
#define publish_delay 5000

boolean changed = false;
boolean last = false;

int16_t y;

  void setup() override {
    set_update_interval(10); 
    Wire.begin();
    Wire.beginTransmission(hmc5883l_address);
    Wire.write(0x00); // Select Configuration Register A
    Wire.write(0x38); // 2 Averaged Samples at 75Hz
    Wire.endTransmission();

    Wire.beginTransmission(hmc5883l_address);
    Wire.write(0x01); // Select Configuration Register B
    Wire.write(0x20); // Set Default Gain
    Wire.endTransmission();

    Wire.beginTransmission(hmc5883l_address);
    Wire.write(0x02); // Select Mode Register
    Wire.write(0x00); // Continuous Measurement Mode
    Wire.endTransmission();    

  }
  void update() override {
    now = millis();

    Wire.beginTransmission(hmc5883l_address);
    Wire.write(0x03); // Select register 3, X MSB Register
    Wire.endTransmission();

    Wire.requestFrom(hmc5883l_address, 6); delay(4);
    if(Wire.available() >= 6) {
        Wire.read(); Wire.read(); Wire.read(); Wire.read(); // Ignore X and Z Registers
        y  = Wire.read() << 8; // Y MSB
        y |= Wire.read();      // Y LSB
    }
    old_val = new_val;
    new_val = map(y, -1000, -250, -350, 350);
    changed = (old_val < 0 && new_val > 0) || (old_val > 0 && new_val < 0);

    if(changed) {
        crossings += 1;
    }
    
    char s[30];

    if((crossings > 0 || last) && (now - last_publish) >= publish_delay) {
        sprintf(s,"WaterValue: %u,%d,%d,%d",crossings,y,new_val,old_val);
        publish_state(crossings);
        ESP_LOGD("info","%s",s);
        Serial.println(s);
        if (crossings > 0) 
            last=true;
        else
            last=false;
        crossings = 0;
        last_publish = now;
    } 
  }
};