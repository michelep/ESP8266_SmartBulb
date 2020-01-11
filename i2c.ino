// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

enum i2c_status   {I2C_WAITING,     // stopped states
                   I2C_TIMEOUT,     //  |
                   I2C_ADDR_NAK,    //  |
                   I2C_DATA_NAK,    //  |
                   I2C_ARB_LOST,    //  |
                   I2C_BUF_OVF,     //  |
                   I2C_NOT_ACQ,     //  |
                   I2C_DMA_ERR,     //  V
                   I2C_SENDING,     // active states
                   I2C_SEND_ADDR,   //  |
                   I2C_RECEIVING,   //  |
                   I2C_SLAVE_TX,    //  |
                   I2C_SLAVE_RX};

bool i2c_status() {
  switch(Wire.status()) {
    case I2C_WAITING:  DEBUG("I2C waiting, no errors"); return true;
    case I2C_ADDR_NAK: DEBUG("Slave addr not acknowledged"); break;
    case I2C_DATA_NAK: DEBUG("Slave data not acknowledged\n"); break;
    case I2C_ARB_LOST: DEBUG("Bus Error: Arbitration Lost\n"); break;
    case I2C_TIMEOUT:  DEBUG("I2C timeout\n"); break;
    case I2C_BUF_OVF:  DEBUG("I2C buffer overflow\n"); break;
    default:           DEBUG("I2C busy\n"); break;
  }
  return false;
}

void i2c_scanner() {
  byte error, address;
  uint8_t nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)     {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    } else if (error==4) {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
}
