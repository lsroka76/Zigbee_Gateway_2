#pragma once

//#include "ZigbeeIASZone.h"
//#include "ZigbeeTempMeas.h"
#include "ZigbeeGateway.h"

#include "esp_coexist.h"

//#include "esp_zigbee_aps.h"
//#include "c:\Users\lsrok\AppData\Local\Arduino15\packages\esp32\tools\esp32-arduino-libs\idf-release_v5.1-632e0c2a\esp32c6\include\espressif__esp-zigbee-lib\include\aps\esp_zigbee_aps.h"
//#include <SuplaDevice.h>
//#include <supla/sensor/virtual_binary.h>
//#include <supla/network/esp_wifi.h>

//#define IASZONE_ENDPOINT_NUMBER 1
//#define TEMPMEAS_ENDPOINT_NUMBER 2
#define GATEWAY_ENDPOINT_NUMBER 1

#define BUTTON_PIN                  9  //Boot button for C6/H2

//ZigbeeIASZone zbIASZone = ZigbeeIASZone(IASZONE_ENDPOINT_NUMBER);
//ZigbeeTempMeas zbTempMeas = ZigbeeTempMeas(TEMPMEAS_ENDPOINT_NUMBER);

ZigbeeGateway zbGateway = ZigbeeGateway(GATEWAY_ENDPOINT_NUMBER);

//Supla::ESPWifi wifi("xxx", "xxx");

//Supla::Sensor::VirtualBinary *ZBS_TS0203_1;
//Supla::Sensor::VirtualBinary *ZBS_TS0203_2;

/*static bool zb_aps_data_indication_handler(esp_zb_apsde_data_ind_t msg)
{
    ESP_LOGI("ZB_APS", "profile Id %d status %d Dst endp %d Cluster %d: %ld - Security %d", 
                         msg.profile_id, msg.status,  msg.dst_endpoint, msg.cluster_id, 
                         msg.asdu_length,  msg.security_status);
    return false;
}

static void zb_aps_data_confirm_handler(esp_zb_apsde_data_confirm_t confirm)
{
    ESP_LOGI("ZB_APS", "Confirm status %d dst endp %d <- src endp %d, length %ld", confirm.status,
                        confirm.dst_endpoint,confirm.src_endpoint,confirm.asdu_length);
}
*/

void sz_ias_zone_notification(int status, uint8_t *ieee_addr_64)
{
    Serial.println("in sz_ias_zone_nitification");
    Serial.println(status);
    Serial.println(ieee_addr_64[0]);
}

//void onBoundDeviceCb(zb_device_params_t * device) {
//  zbGateway.addGatewayDevice (device);
//} 

uint32_t startTime = 0;
uint32_t printTime = 0;
uint32_t zbInit_delay = 0;

bool zbInit = true;

void setup() {

  pinMode(BUTTON_PIN, INPUT);

  /*char GUID[SUPLA_GUID_SIZE] = {0xA4,0x45,0xDF,0xAF,0x7F,0xB1,0x16,0x39,0xB1,0xBB,0xB8,0x48,0x18,0x65,0x81,0xF8};
  char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {0x09,0xB6,0xF1,0x47,0x59,0xD7,0x73,0x5E,0xCE,0x43,0x9C,0x96,0xF1,0x43,0xF2,0xEC};
  
  ZBS_TS0203_1 = new Supla::Sensor::VirtualBinary();
  ZBS_TS0203_1->setDefaultFunction(SUPLA_CHANNELFNC_OPENINGSENSOR_WINDOW);
  ZBS_TS0203_1->set();
  
  ZBS_TS0203_2 = new Supla::Sensor::VirtualBinary();
  ZBS_TS0203_2->setDefaultFunction(SUPLA_CHANNELFNC_OPENINGSENSOR_WINDOW);
  ZBS_TS0203_2->set();
*/
  //  Zigbee

  //zbIASZone.onStatusNotification(sz_ias_zone_notification);
  zbGateway.onStatusNotification(sz_ias_zone_notification);
  //zbGateway.onBoundDevice(onBoundDeviceCb);

 // put your setup code here, to run once:
  //zbIASZone.setManufacturerAndModel("Espressif", "MyZigbeeIASZone");
  //zbTempMeas.setManufacturerAndModel("Espressif", "MyZigbeeTempMeas");
  zbGateway.setManufacturerAndModel("Espressif", "MyZigbeeGateway");
  zbGateway.allowMultipleBinding(true);

  //Add endpoint to Zigbee Core
  //Zigbee.addEndpoint(&zbIASZone);
  //Zigbee.addEndpoint(&zbTempMeas);
  Zigbee.addEndpoint(&zbGateway);

  //Open network for 180 seconds after boot
  Zigbee.setRebootOpenNetwork(180);

  //Supla
  
  //SuplaDevice.begin(GUID,              // Global Unique Identifier 
    //                "svr3.supla.org",  // SUPLA server address
      //              "l.sroka@gmail.com",   // Email address used to login to Supla Cloud
        //            AUTHKEY);          // Authorization key
  
  startTime = millis();
  printTime = millis();
  zbInit_delay = millis();
}
zb_device_params_t *gateway_device;
zb_device_params_t *joined_device;
char zbd_model_name[64];

void loop() {
  
//  SuplaDevice.iterate();
  // put your main code here, to run repeatedly:

  //if (zbInit && wifi.isReady())
  if (zbInit)
  {
    Serial.println("zbInit");
    
  //  esp_coex_wifi_i154_enable();
  
    //esp_zb_aps_data_indication_handler_register(zb_aps_data_indication_handler);
    //esp_zb_aps_data_confirm_handler_register(zb_aps_data_confirm_handler);
  
    // When all EPs are registered, start Zigbee with ZIGBEE_COORDINATOR mode
    if (!Zigbee.begin(ZIGBEE_COORDINATOR)) {
      Serial.println("Zigbee failed to start!");
      Serial.println("Rebooting...");
      ESP.restart();
    }
    zbInit = false;
 }
  
  if (digitalRead(BUTTON_PIN) == LOW) {  // Push button pressed
    // Key debounce handling
    delay(100);
    
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        // If key pressed for more than 3secs, factory reset Zigbee and reboot
        Serial.printf("Resetting Zigbee to factory settings, reboot.\n");
        Zigbee.factoryReset();
      }
    }
    Zigbee.openNetwork(180);
  }
  delay(100);

  if (zbGateway.isNewDeviceJoined()) {

    zbGateway.clearNewDeviceJoined();
    zbGateway.printJoinedDevices();

    while (!zbGateway.getJoinedDevices().empty())
    {
      joined_device = zbGateway.getLastJoinedDevice();
      log_d("getJoinedDEvices %d %d %d before", zbGateway.getJoinedDevices().empty(), zbGateway.getJoinedDevices().size(),zbGateway.getJoinedDevices().max_size());
      //zbGateway.getJoinedDevices().pop_back();
      //zbGateway.getJoinedDevices().clear();
      log_d("getJoinedDEvices %d %d %d after", zbGateway.getJoinedDevices().empty(), zbGateway.getJoinedDevices().size(),zbGateway.getJoinedDevices().max_size());
      zbGateway.printJoinedDevices();

      log_i("manufacturer %s ", zbGateway.readManufacturer(joined_device->endpoint, joined_device->short_addr, joined_device->ieee_addr));
      strcpy(zbd_model_name,zbGateway.readModel(joined_device->endpoint, joined_device->short_addr, joined_device->ieee_addr));
      log_i("model %s ", zbd_model_name);
      if ((strcmp(zbd_model_name,"ZigbeeTempSensor") == 0)||
      strcmp(zbd_model_name,"TS0201") {
          esp_zb_lock_acquire(portMAX_DELAY);
          zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT);
          esp_zb_lock_release();
      } else
      if ((strcmp(zbd_model_name,"TS0203") == 0)||
          strcmp(zbd_model_name,"TS0202") == 0) {
          esp_zb_lock_acquire(portMAX_DELAY);
          zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE);
          esp_zb_lock_release();
      } else
          if (strcmp(zbd_model_name,"TS0044") == 0) {
          esp_zb_lock_acquire(portMAX_DELAY);
          zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF);
          esp_zb_lock_release();
          }
      else
      log_d("Unknown model %s, no binding is possible", zbd_model_name);
    }
  }
  if (millis() - printTime > 60000) {
      zbGateway.printGatewayDevices();
      zbGateway.printJoinedDevices();
      if (zbGateway.getGatewayDevices().size() > 0) zbGateway.setIASZReporting(10, 15);
      printTime = millis();
  }
}
