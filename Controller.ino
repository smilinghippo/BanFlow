/* Example for analogContinuousRead
   It measures continuously the voltage on pin A9,
   Write v and press enter on the serial console to get the value
   Write c and press enter on the serial console to check that the conversion is taking place,
   Write t to check if the voltage agrees with the comparison in the setup()
   Write s to stop the conversion, you can restart it writing r.
*/

//------setup for the SCPI library------------- 
#include "Vrekrer_scpi_parser.h"

SCPI_Parser my_instrument;

//------setup for the ADCs--------------------- 

#include <i2c_t3.h>
#include <ADC.h>
#include <ADC_util.h>
ADC *adc = new ADC(); // adc object

uint8_t VO_pins[6] = {23, 4, 20, A21, 17, 32};
uint8_t VC_pins[6] = {22, 13, 19, 39, 16, 35};

float flows[6] = {0.125, 0.25, 0.375, 0.5, 0.625, 0.75};
float read_flows[6] = {0.125, 0.25, 0.375, 0.5, 0.625, 0.75};


void setup() {
  //Set the LED pin as output
  my_instrument.RegisterCommand(F("*IDN?"), &Identify);   //*IDN?
  my_instrument.SetCommandTreeBase(F("SETflow"));
    my_instrument.RegisterCommand(F(":MFC1"), &Set1);
    my_instrument.RegisterCommand(F(":MFC2"), &Set2); 
    my_instrument.RegisterCommand(F(":MFC3"), &Set3);
    my_instrument.RegisterCommand(F(":MFC4"), &Set4); 
    my_instrument.RegisterCommand(F(":MFC5"), &Set5); 
    my_instrument.RegisterCommand(F(":MFC6"), &Set6); 
  
  Serial.begin(9600);

  //Setup the I2C for the DACS
  Wire.begin (I2C_MASTER, 0x00, I2C_PINS_33_34, I2C_PULLUP_EXT, 400000);
  Wire.setDefaultTimeout (200000); // 200ms
  Wire1.begin(I2C_MASTER, 0x00, I2C_PINS_37_38, I2C_PULLUP_EXT, 400000);
  Wire1.setDefaultTimeout(200000);
  Wire2.begin(I2C_MASTER, 0x00, I2C_PINS_3_4,   I2C_PULLUP_EXT, 400000);
  Wire2.setDefaultTimeout(200000);

  //Set the ADC pins to input
  pinMode(A7, INPUT);
  pinMode(A22, INPUT);
  pinMode(A4, INPUT);
  pinMode(A17, INPUT);
  pinMode(A1, INPUT);
  pinMode(A12, INPUT);

  for (auto pin : VC_pins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 1);
  }

  //Setup the two ADCs to their best conversion settings
  adc->adc0->setAveraging(32); // set number of averages
  adc->adc0->setResolution(16); // set bits of resolution
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED);
  adc->adc0->setReference(ADC_REFERENCE::REF_3V3);

  adc->adc1->setAveraging(32); // set number of averages
  adc->adc1->setResolution(16); // set bits of resolution
  adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);
  adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED);
  adc->adc1->setReference(ADC_REFERENCE::REF_3V3);

  //Let everything settle a bit.
  delay(100);
}

void readFlows(float o[6]) {
  adc->adc0->wait_for_cal();
  adc->adc1->wait_for_cal();
  
  uint16_t readings_int[6];
  //The mapping of ADC pins (1st arg) to ADC modules (2nd arg) are given here (https://forum.pjrc.com/attachment.php?attachmentid=10666&d=1479644486)

  //We perfom synchronised reads as this does them in parallel.
  ADC::Sync_result result;
  result = adc->analogSynchronizedRead(A7, A22);
  readings_int[0] = (uint16_t)result.result_adc0;
  readings_int[1] = (uint16_t)result.result_adc1;
  result = adc->analogSynchronizedRead(A4, A17);
  readings_int[2] = (uint16_t)result.result_adc0;
  readings_int[3] = (uint16_t)result.result_adc1;
  result = adc->analogSynchronizedRead(A1, A12);
  readings_int[4] = (uint16_t)result.result_adc0;
  readings_int[5] = (uint16_t)result.result_adc1;

  for (int i(0); i < 6; ++i)
    o[i] = readings_int[i] * (1.0 / 65535); //Make it a fraction of the output range

  // Print errors, if any.
  if (adc->adc0->fail_flag != ADC_ERROR::CLEAR) {
    Serial.print("ADC0: "); Serial.println(getStringADCError(adc->adc0->fail_flag));
  }

  if (adc->adc1->fail_flag != ADC_ERROR::CLEAR) {
    Serial.print("ADC1: "); Serial.println(getStringADCError(adc->adc1->fail_flag));
  }
}

void setFlows(float o[6]) {
  uint8_t packet[3];
  packet[0] = 0x40; //Write to the DAC, not the EEPROM

  for (int i(0); i < 6; ++i) {
    uint16_t output;
    if (o[i] > 1.0)
      output = 4095; 
    else if (o[i] < 0.0)
      output = 0;
    else
      output = uint16_t(o[i] * 4095 + 0.49999); //rounding conversion
      
    packet[1] = output / 16;        // Upper data bits (D11.D10.D9.D8.D7.D6.D5.D4)
    packet[2] = (output % 16) << 4; // Lower data bits (D3.D2.D1.D0.x.x.x.x)
    
    //Figure out which Wire we should be using
    //First, this is a lazy way to get the type right
    decltype(&Wire) wire_ptr;
    uint8_t addr;
    //We know that flow 0&2 go to Wire2, flow 4&1 go to Wire1, and flow 3&5 go to Wire, so do some switchery to get that. 
    switch (i) {
      case 0:
        wire_ptr = &Wire2;
        addr = 0x60 + 0;
        break;
      case 1:
        wire_ptr = &Wire1;
        addr = 0x60 + 1;
        break;
      case 2:
        wire_ptr = &Wire2;
        addr = 0x60 + 1;
        break;
      case 3:
        wire_ptr = &Wire;
        addr = 0x60 + 0;
        break;
      case 4:
        wire_ptr = &Wire1;
        addr = 0x60 + 0;
        break;
      case 5:
        wire_ptr = &Wire;
        addr = 0x60 + 1;
        break;
      }
    wire_ptr->beginTransmission(addr);
    wire_ptr->write(packet, 3);
    wire_ptr->endTransmission();
  }
}

void Identify(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.println(F("MFC Controller: Rhyme-noceros"));
}

//changes the flow for each MFC

void Set1(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() > 0) {
    flows[1] = constrain(String(parameters[0]).toFloat(), 0.0, 3.3);
  }
}
void Set2(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() > 0) {
    flows[2] =  constrain(String(parameters[0]).toFloat(), 0.0, 3.3);
  }
}
void Set3(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() > 0) {
    flows[3] = constrain(String(parameters[0]).toFloat(), 0.0, 3.3);
  }
}
void Set4(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() > 0) {
    flows[4] = constrain(String(parameters[0]).toFloat(), 0.0, 3.3);
  }
}
void Set5(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() > 0) {
    flows[5] = constrain(String(parameters[0]).toFloat(), 0.0, 3.3);
  }
}
void Set6(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() > 0) {
    flows[6] = constrain(String(parameters[0]).toFloat(), 0.0, 3.3);
  }
}


void loop() {
  my_instrument.ProcessInput(Serial, "\n");
  
  setFlows(flows);
  
  readFlows(read_flows);
  Serial.print("Flows");
  for (int i(0); i < 6; ++i) {
    Serial.print(" "); Serial.print(read_flows[i]);
  }
  Serial.println("");

  delay(100);
}
