# Makerlabvn_I2C_Motor_Driver

## Frame data (6 bytes): [ADDRESS_ID] [MODE_ID] [INDEX] [PWM] [DIR] [CHECKSUM_LOWBYTE]
```c++
void Makerlabvn_I2C_Motor_Driver::sendI2cMotorDC_Data(str_serial_data_dcMotor *_myMotor_)
{
  delayMicroseconds(DELAY_I2C_SEND);
  Wire.beginTransmission(addressDriver); // transmit to device 
  Wire.write(_myMotor_->addressId);      // sends one byte
  Wire.write(_myMotor_->modeId);         // sends one byte
  Wire.write(_myMotor_->index);          // sends one byte
  Wire.write(_myMotor_->pwm);            // sends one byte
  Wire.write(_myMotor_->dir);            // sends one byte
  Wire.write(_myMotor_->checkSum);       // sends one byte
  Wire.endTransmission();                // stop transmitting
}
```
---
### Set DC motor:
---
#### [ADDRESS_ID]: 0x64,0x65, ..., 0x68
#### [MODE_ID]: 1
#### [INDEX]: 0 -> 1
* 0: Set MA
* 1: Set MB
#### [PWM]: 0 -> 255
#### [DIR]: 0 -> 1
#### [CHECKSUM_LOWBYTE] = [ADDRESS_ID] + [MODE_ID] + [INDEX] + [PWM] + [DIR] 
```c++
// ex: Driver ADDRESS 0x64, DC MA run pwn: 127(0x7F); dir: 0.
[0x64, 1, 0, 127, 0, 0xE4]

// ex: Driver ADDRESS 0x64, DC MA run pwn: 240(0x70); dir: 1.
[0x64, 1, 1, 0xF0, 1, 0x57]
```
---
### Set RC motor:
---
#### [ADDRESS_ID]:  0x64,0x65, ..., 0x68
#### [MODE_ID]: 0
#### [INDEX]:
* 1: Set S1
* 2: Set S2
#### [PULSE_HIGH_BYTE]: 0x00 -> 0x0A
#### [PULSE_LOW_BYTE]: 0x00-> 0xFF
#### [CHECKSUM_LOWBYTE] = [ADDRESS_ID] + [MODE_ID] + [INDEX] + [PULSE_HIGH_BYTE] + [PULSE_LOW_BYTE] 
```c++
// ex: Driver ADDRESS 0x64, Servo S1 run with pulse 1400:
[0x64, 0, 1, 0x05, 0x78, 0xE2]

// ex: Driver ADDRESS 0x64, Servo S2 run with pulse 1000:
[0x64, 0, 2, 0x03, 0xE8, 0x51]
```

### Set Address:
---
#### [ADDRESS_ID]:  0x64,0x65, ..., 0x68
#### [MODE_ID]: 2
#### [INDEX]:0

#### [PWM]: 0
#### [DIR]: 0
#### [CHECKSUM_LOWBYTE] = [ADDRESS_ID] + [MODE_ID] + [INDEX] + [PWM] + [DIR] 
```c++
// ex: Set Driver ADDRESS 0x64:
[0x64, 2, 0, 0, 0, 0x66]

// ex: Set Driver ADDRESS 0x66:
[0x66, 2, 0, 0, 0, 0x68]
```
