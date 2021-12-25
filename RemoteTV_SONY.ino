/* HomeSpan IR Remote TV Control */

#include "HomeSpan.h"
#include "extras/RFControl.h"

#define DEVICE_NAME   "SONY TV"

#define IR_PIN        23

#define SONY_OFF      0xF50
#define SONY_ON       0x750

RFControl rf(IR_PIN);

void XMIT_SONY(uint32_t code){

  rf.enableCarrier(40000,0.5);

  int unit=600;

  rf.clear();

  rf.add(2400,unit);
  
  for(int i=11;i>=0;i--){
    rf.add(unit*((code&(1<<i))?2:1),unit);
    Serial.printf("%d %d ",unit*((code&(1<<i))?2:1),unit);
    Serial.println((code&(1<<i))?1:0);
  }

  rf.phase(45000,LOW);

  rf.start(5,1);
}  

struct TV_Source : Service::InputSource{

  SpanCharacteristic *currentState = new Characteristic::CurrentVisibilityState(0,true);
  SpanCharacteristic *targetState = new Characteristic::TargetVisibilityState(0,true);

  TV_Source(uint32_t id, const char *name) : Service::InputSource(){
    new Characteristic::Identifier(id);
    new Characteristic::ConfiguredName(name,true);
    new Characteristic::IsConfigured(1);
  }

  boolean update() override{

    if(targetState->updated()){
      Serial.printf("Old Target State = %d    New Target State = %d\n",targetState->getVal(),targetState->getNewVal());
      currentState->setVal(targetState->getNewVal());
    }

    return(true);
  }
  
};

struct TV_Control : Service::Television {

  int nSources=0;
  vector<uint32_t> inputCodes;
  uint32_t onCode;
  uint32_t offCode;

  SpanCharacteristic *active = new Characteristic::Active(0,true);
  SpanCharacteristic *activeIdentifier = new Characteristic::ActiveIdentifier(1,true);

  TV_Control(const char *name, uint32_t onCode, uint32_t offCode) : Service::Television(){
    new Characteristic::ConfiguredName(name,true);
    this->onCode=onCode;
    this->offCode=offCode;
  }

  TV_Control *addSource(const char *name, uint32_t code){
    addLink(new TV_Source(nSources++,name));
    inputCodes.push_back(code);
    return(this);
  }
    
  boolean update() override{

    uint32_t code=0;

    if(active->updated()){
      code=active->getNewVal()?onCode:offCode;
      Serial.printf("Old Active State = %d    New Active State = %d\n",active->getVal(),active->getNewVal());
      Serial.printf("Code: %08X\n",code);
    }

    if(activeIdentifier->updated()){
      code=inputCodes[activeIdentifier->getNewVal()];
      Serial.printf("Old Active Identifier = %d    New Active Identifier = %d\n",activeIdentifier->getVal(),activeIdentifier->getNewVal());
      Serial.printf("Code: %08X\n",code);
    }

    if(code)
      XMIT_SONY(code);

    return(true);
  }

};

void setup() {

  Serial.begin(115200);
 
  homeSpan.begin(Category::Television,DEVICE_NAME);

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("HomeSpan TV");
      new Characteristic::Manufacturer("HomeSpan");
      new Characteristic::SerialNumber("123-ABC");
      new Characteristic::Model("HomeSpan");
      new Characteristic::FirmwareRevision("0.1");
      new Characteristic::Identify();

  new Service::HAPProtocolInformation();
    new Characteristic::Version("1.1.0");
           
  new TV_Control("Sony TV",SONY_ON,SONY_OFF);
}

void loop() {
  homeSpan.poll();
}
