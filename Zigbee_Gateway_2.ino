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

static void Z2S_active_ep_req(esp_zb_zdp_status_t zdo_status, uint8_t ep_count, uint8_t *ep_id_list, void *user_ctx) {
  if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS) {
      
      uint16_t short_addr = *((uint16_t*)(user_ctx));
      esp_zb_zdo_simple_desc_req_param_t cl_cmd_req;

      log_i("Z2S active_ep_req: device address %d, endpoints count %d", short_addr, ep_count);
      
      for (int i = 0; i < ep_count; i++) {
        log_i("Endpont # %d, id %d ", i+1, *(ep_id_list+i));
        cl_cmd_req.addr_of_interest = short_addr;
        cl_cmd_req.endpoint = *(ep_id_list+i);
        esp_zb_zdo_simple_desc_req(&cl_cmd_req, Z2S_simple_desc_req, &short_addr);
    }
  }
  else log_i("Z2S active_ep_req failed");
}

static void Z2S_simple_desc_req(esp_zb_zdp_status_t zdo_status, esp_zb_af_simple_desc_1_1_t *simple_desc, void *user_ctx) {
  if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS) {
    
    uint16_t short_addr = *((uint16_t*)(user_ctx));
    
    log_i("Z2S simple_desc_req: device address %d, endpoint # %d", short_addr, simple_desc->endpoint);
    log_i("Z2S simple_desc_req: in clusters # %d, out clusters # %d", simple_desc->app_input_cluster_count, simple_desc->app_output_cluster_count);
    for (int i = 0; i < simple_desc->app_input_cluster_count; i++) {
      
      log_i("In cluster # %d, id %d ", i+1, *(simple_desc->app_cluster_list+i));
    }
    for (int i = 0; i < simple_desc->app_output_cluster_count; i++) {
      log_i("Out cluster # %d, id %d ", i+1, *(simple_desc->app_cluster_list + simple_desc->app_input_cluster_count + i));
    }
  }
  else log_i("Z2S simple desc failed");
}

uint16_t short_addr_req;


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
          (strcmp(zbd_model_name,"TS0201") == 0)) {
          esp_zb_lock_acquire(portMAX_DELAY);
          zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT);
          esp_zb_lock_release();
      } else
      if ((strcmp(zbd_model_name,"TS0203") == 0)||
          (strcmp(zbd_model_name,"TS0202") == 0)) {
          esp_zb_lock_acquire(portMAX_DELAY);
          zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE);
          esp_zb_lock_release();
      } else 
          if (strcmp(zbd_model_name,"TS0044") == 0) {
          esp_zb_lock_acquire(portMAX_DELAY);
          zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF);
          esp_zb_lock_release();
      } else
          if (strcmp(zbd_model_name,"TS011F") == 0) {
          esp_zb_lock_acquire(portMAX_DELAY);
          zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF);
          zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_METERING);
          zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_ELECTRICAL_MEASUREMENT);    
          esp_zb_lock_release();
          }
        else
          if (strcmp(zbd_model_name,"TS0601") == 0) {
          esp_zb_lock_acquire(portMAX_DELAY);
          zbGateway.bindDeviceCluster(joined_device, 61184);
          //zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT);
          //zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_THERMOSTAT);
          //zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_METERING);
          //zbGateway.bindDeviceCluster(joined_device, ESP_ZB_ZCL_CLUSTER_ID_ELECTRICAL_MEASUREMENT);    
          esp_zb_lock_release();
          }
      else
      log_d("Unknown model %s, no binding is possible", zbd_model_name);
    }
  }
  if (millis() - printTime > 10000) {
      zbGateway.printGatewayDevices();
      zbGateway.printJoinedDevices();
      //if (zbGateway.getGatewayDevices().size() > 0) zbGateway.setIASZReporting(0, 0);
      //if (zbGateway.getGatewayDevices().size() > 0) {
      for ([[maybe_unused]]
       const auto &device : zbGateway.getGatewayDevices()) {
  
        esp_zb_zdo_active_ep_req_param_t ep_cmd_req;
        
        esp_zb_zcl_disc_attr_cmd_t disc_attr_cmd_req;

        short_addr_req = device->short_addr; //zbGateway.getGatewayDevices().front()->short_addr;
        //log_i("address before zdo_active_ep_req %d", short_addr_req);
        ep_cmd_req.addr_of_interest = short_addr_req; 
        

        /*disc_attr_cmd_req.zcl_basic_cmd.dst_endpoint = 1;
        disc_attr_cmd_req.zcl_basic_cmd.dst_addr_u.addr_short = short_addr_req;
        disc_attr_cmd_req.zcl_basic_cmd.src_endpoint = 1;
        disc_attr_cmd_req.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        disc_attr_cmd_req.cluster_id = 61184;
        disc_attr_cmd_req.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV;
        disc_attr_cmd_req.start_attr_id = 0;
        disc_attr_cmd_req.max_attr_number = 99;*/

        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zdo_active_ep_req(&ep_cmd_req, Z2S_active_ep_req, &short_addr_req);
        //esp_zb_zdo_simple_desc_req(&cl_cmd_req, Z2S_simple_desc_req, &short_addr_req);
        //esp_zb_zcl_disc_attr_cmd_req(&disc_attr_cmd_req);
        esp_zb_lock_release();
      }
      printTime = millis();
  }
}
