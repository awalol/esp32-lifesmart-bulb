#define BLINKER_WIFI

#include <Blinker.h>
#include <NimBLEDevice.h>

char auth[] = "";
char ssid[] = "";
char pwd[] = "";

// BULB Data
uint8_t turnOnData[] = {0x0e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x02,0x01,0x81};
uint8_t turnOffData[] = {0x0e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x02,0x01,0x80};

// 新建组件对象
BlinkerButton button("toggle");
BlinkerRGB rgb("color");
BlinkerText status("status");

NimBLERemoteCharacteristic *pCharacteristic;

void button1_callback(const String & state)
{
    BLINKER_LOG("get button state: ", state);

    if (state == BLINKER_CMD_ON) {
        button.print("on");
        pCharacteristic->writeValue(turnOnData, sizeof(turnOnData));
    } else if (state == BLINKER_CMD_OFF){
        button.print("off");
        pCharacteristic->writeValue(turnOffData, sizeof(turnOffData));
    }
}

// 改变颜色
void color_callback(uint8_t red,uint8_t green,uint8_t blue,uint8_t brightness){
    BLINKER_LOG("R value: ", red);
    BLINKER_LOG("G value: ", green);
    BLINKER_LOG("B value: ", blue);
    BLINKER_LOG("Brightness value: ", brightness);

    uint8_t data[] = {0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x00,0x08,0x01,0xff,
                      static_cast<uint8_t>(255 - brightness),
                      red,
                      green,
                      blue,
                      0x02,0x80};

    pCharacteristic->writeValue(data, sizeof(data));
    button.print("on");
}

void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);
    Blinker.vibrate();
}

class ClientCallbacks : public NimBLEClientCallbacks{
    void onConnect(NimBLEClient* pClient) override{
        BLINKER_LOG("Connected BLE Client:");
        BLINKER_LOG(pClient->getPeerAddress().toString().c_str());
        status.icon("fad fa-link");
        status.color("#03A9F4");
        status.print("已连接");
    }

    void onDisconnect(NimBLEClient* pClient) override{
        status.icon("fad fa-unlink");
        status.color("#757575");
        status.print("未连接");
        BLINKER_LOG("Disconnected - Starting scan");

        NimBLEDevice::getScan()->start(10);
    }
};

void setup()
{
    // 初始化串口
    Serial.begin(115200);
    BLINKER_DEBUG.stream(Serial);
    BLINKER_DEBUG.debugAll();

    // 初始化blinker
    Blinker.begin(auth, ssid, pwd);
    Blinker.attachData(dataRead);
    button.attach(button1_callback);
    rgb.attach(color_callback);

    // 初始化 BLE
    BLINKER_LOG("Initialize BLE");
    NimBLEDevice::init("");
    NimBLEScan *pScan = NimBLEDevice::getScan();
    NimBLEScanResults results = pScan->start(10);

    // scan BULB by UUID
    NimBLEUUID serviceUuid("9fdc9c81-fffe-51a1-e511-5a38c414c2f9");

    for(int i = 0;i < results.getCount();i++){
        NimBLEAdvertisedDevice device = results.getDevice(i);

        if(device.isAdvertisingService(serviceUuid)){
            NimBLEClient *pClient = NimBLEDevice::createClient();
            static ClientCallbacks clientCB;
            pClient->setClientCallbacks(&clientCB, false);

            if(!pClient ->connect(&device)){
                if(!pClient ->connect(&device)){
                    BLINKER_LOG("Connect fail");
                    return;
                }
            }

            BLINKER_LOG("Connected");
            NimBLERemoteService *pService = pClient->getService(serviceUuid);

            if(pService != nullptr){
                NimBLEUUID characteristicUuid("ac7bc836-6b69-74b6-d64c-451cc52b476e");
                pCharacteristic = pService->getCharacteristic(characteristicUuid);
            }
        }
    }
}

void loop() {
    Blinker.run();
}