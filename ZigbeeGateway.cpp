#include "ZigbeeGateway.h"
#if SOC_IEEE802154_SUPPORTED 
//&& CONFIG_ZB_ENABLED


// Initialize the static instance of the class
ZigbeeGateway *ZigbeeGateway::_instance = nullptr;

findcb_userdata_t ZigbeeGateway::findcb_userdata;
bool ZigbeeGateway::_last_bind_success = false;
bool ZigbeeGateway::_in_binding = false;
bool ZigbeeGateway::_new_device_joined = false;
//


SemaphoreHandle_t ZigbeeGateway::gt_lock;

ZigbeeGateway::ZigbeeGateway(uint8_t endpoint) : ZigbeeEP(endpoint) {
  _device_id = ESP_ZB_HA_HOME_GATEWAY_DEVICE_ID; 
  _instance = this;  // Set the static pointer to this instance
  
  _new_device_joined = false;
  _last_bind_success = false;

  _joined_devices.clear();
  _gateway_devices.clear();

  #if !CONFIG_DISABLE_HAL_LOCKS
  if (!gt_lock) {
    gt_lock = xSemaphoreCreateBinary();
    if (gt_lock == NULL) {
      log_e("Semaphore creation failed");
    }
  }
  #endif


  //use custom config to avoid narrowing error -> must be fixed in zigbee-sdk
  esp_zb_configuration_tool_cfg_t gateway_cfg = ZB_DEFAULT_GATEWAY_CONFIG();

  esp_zb_ias_zone_cluster_cfg_t zone_cfg = {
        .zone_state   = 0,
        .zone_type    = 0x0016,// SS_IAS_ZONE_TYPE_DOOR_WINDOW_HANDLE
        .zone_status  = 0,
        .ias_cie_addr = ESP_ZB_ZCL_ZONE_IAS_CIE_ADDR_DEFAULT,
        .zone_id      = 0xFF,
    };

  
  _cluster_list = esp_zb_zcl_cluster_list_create();
  esp_zb_attribute_list_t *basic_cluster = esp_zb_basic_cluster_create(&(gateway_cfg.basic_cfg));
  esp_zb_cluster_list_add_basic_cluster(_cluster_list, basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  esp_zb_cluster_list_add_basic_cluster(_cluster_list, esp_zb_basic_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
  esp_zb_cluster_list_add_identify_cluster(_cluster_list, esp_zb_identify_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  esp_zb_cluster_list_add_identify_cluster(_cluster_list, esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
  
  esp_zb_cluster_list_add_ias_zone_cluster(_cluster_list, esp_zb_ias_zone_cluster_create(&zone_cfg), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
  esp_zb_cluster_list_add_temperature_meas_cluster(_cluster_list, esp_zb_temperature_meas_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
  esp_zb_cluster_list_add_on_off_cluster(_cluster_list, esp_zb_on_off_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  esp_zb_cluster_list_add_on_off_cluster(_cluster_list, esp_zb_on_off_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
  esp_zb_cluster_list_add_electrical_meas_cluster(_cluster_list, esp_zb_electrical_meas_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
  esp_zb_cluster_list_add_metering_cluster(_cluster_list, esp_zb_metering_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
  esp_zb_cluster_list_add_thermostat_cluster(_cluster_list, esp_zb_thermostat_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
  
  _ep_config = {.endpoint = _endpoint, .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID, .app_device_id = ESP_ZB_HA_REMOTE_CONTROL_DEVICE_ID, .app_device_version = 0};
}

void ZigbeeGateway::bindCb(esp_zb_zdp_status_t zdo_status, void *user_ctx) {
  if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS) {
    if (user_ctx) {
      zb_device_params_t *sensor = (zb_device_params_t *)user_ctx;
      _instance->_gateway_devices.push_back(sensor);
      log_v("Binding success (ZC side)");
    } else
      log_v("Binding success (ED side");
    
      _is_bound = true;
      _last_bind_success = true;
    
  } else {
    log_e("Binding failed!");
    _last_bind_success = false;
  }
  _in_binding = false;
}

void ZigbeeGateway::find_Cb(esp_zb_zdp_status_t zdo_status, uint16_t addr, uint8_t endpoint, void *user_ctx) {
  
  if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS) {
    
    log_i("Found sensor");
    
    esp_zb_zdo_bind_req_param_t bind_req;
    
    /* Store the information of the remote device */
    zb_device_params_t *sensor = (zb_device_params_t *)malloc(sizeof(zb_device_params_t));
    sensor->endpoint = endpoint;
    sensor->short_addr = addr;
    esp_zb_ieee_address_by_short(sensor->short_addr, sensor->ieee_addr);
    log_d("Sensor found: short address(0x%x), endpoint(%d)", sensor->short_addr, sensor->endpoint);

    _new_device_joined = true;
    _instance->_joined_devices.push_back(sensor);
  }
}

void ZigbeeGateway::bindDeviceCluster(zb_device_params_t * device,int16_t cluster_id ){

  esp_zb_zdo_bind_req_param_t bind_req;
    
    bind_req.req_dst_addr = device->short_addr;
    log_d("Request sensor to bind us");

    /* populate the src information of the binding */
    memcpy(bind_req.src_address, device->ieee_addr, sizeof(esp_zb_ieee_addr_t));
    bind_req.src_endp = device->endpoint;
    bind_req.cluster_id = cluster_id; 
    
    bind_req.dst_addr_mode = ESP_ZB_ZDO_BIND_DST_ADDR_MODE_64_BIT_EXTENDED;
    esp_zb_get_long_address(bind_req.dst_address_u.addr_long);
    bind_req.dst_endp = _instance->getEndpoint(); 
    
    esp_zb_zdo_device_bind_req(&bind_req, bindCb, NULL);

    bind_req.req_dst_addr = esp_zb_get_short_address();

    esp_zb_get_long_address(bind_req.src_address);
    bind_req.src_endp = _instance->getEndpoint();
    bind_req.cluster_id = cluster_id;
    
    bind_req.dst_addr_mode = ESP_ZB_ZDO_BIND_DST_ADDR_MODE_64_BIT_EXTENDED;
    memcpy(bind_req.dst_address_u.addr_long, device->ieee_addr, sizeof(esp_zb_ieee_addr_t));
    bind_req.dst_endp = device->endpoint;

    esp_zb_zdo_device_bind_req(&bind_req, bindCb, (void *)device);

}

void ZigbeeGateway::findEndpoint(esp_zb_zdo_match_desc_req_param_t *param) {
   
  uint16_t cluster_list[] = {ESP_ZB_ZCL_CLUSTER_ID_BASIC};
    
  param->profile_id = ESP_ZB_AF_HA_PROFILE_ID;
  param->num_in_clusters = 1;
  param->num_out_clusters = 0;
  param->cluster_list = cluster_list;
  findcb_userdata._endpoint = _endpoint;
  findcb_userdata._cluster_id = ESP_ZB_ZCL_CLUSTER_ID_BASIC;
  
  esp_zb_zdo_match_cluster(param, find_Cb, &findcb_userdata);
}

void ZigbeeGateway::printGatewayDevices() {
  log_i("gateway devices:");
  for ([[maybe_unused]]
       const auto &device : _gateway_devices) {
    log_i("Device on endpoint %d, short address: 0x%x", device->endpoint, device->short_addr);
    //print_ieee_addr(device->ieee_addr);
  }
}

void ZigbeeGateway::printJoinedDevices() {
  log_i("joined devices:");
  for ([[maybe_unused]]
       const auto &device : _joined_devices) {
    log_i("Device on endpoint %d, short address: 0x%x", device->endpoint, device->short_addr);
    //print_ieee_addr(device->ieee_addr);
  }
}

void ZigbeeGateway::zbAttributeRead(esp_zb_zcl_addr_t src_address, uint16_t src_endpoint, uint16_t cluster_id, const esp_zb_zcl_attribute_t *attribute) {
  
  log_i("zbAttributeRead from %d, endpoint %d, cluster %d, attribute %d ", src_address.u.short_addr, src_endpoint, cluster_id, attribute->id);
 
  if (cluster_id == ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT) {
    if (attribute->id == ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID && attribute->data.type == ESP_ZB_ZCL_ATTR_TYPE_S16) {
      int16_t value = attribute->data.value ? *(int16_t *)attribute->data.value : 0;
      log_i("zbAttributeRead temperature measurement %f",((float)value)/100);
      }
    }
  if (cluster_id == ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT) {
    if (attribute->id == ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID && attribute->data.type == ESP_ZB_ZCL_ATTR_TYPE_U16) 
    {
      int16_t value = attribute->data.value ? *(int16_t *)attribute->data.value : 0;
      log_i(" data type %d",attribute->data.type);
      log_i("zbAttributeRead humidity measurement %f",((float)value)/100);
      }
    }  
}

void ZigbeeGateway::zbIASZoneStatusChangeNotification(const esp_zb_zcl_ias_zone_status_change_notification_message_t *message) {

  esp_zb_zcl_cmd_info_t info = message->info;
  esp_zb_zcl_addr_t src_address = info.src_address;
  log_i("short address %d, src id %d ", src_address.u.short_addr, src_address.u.src_id);

  char ieee_addr[9];
  uint8_t ieee_addr_64[8];

  ieee_addr[8] = '\0';

  esp_zb_ieee_address_by_short(src_address.u.short_addr, ieee_addr_64);

  log_i("ieee addr %d:%d:%d:%d:%d:%d:%d:%d ",ieee_addr_64[7],ieee_addr_64[6],ieee_addr_64[5],ieee_addr_64[4],
    ieee_addr_64[3],ieee_addr_64[2],ieee_addr_64[1],ieee_addr_64[0]);
  //log_i("src id %s ",ieee_addr);
  log_i("zone status %d ", message->zone_status);
  if (_on_status_notification)
    _on_status_notification(message->zone_status, ieee_addr_64);  
}

void ZigbeeGateway::addBoundDevice(zb_device_params_t *device){
    
    device->short_addr = esp_zb_address_short_by_ieee(device->ieee_addr);
    _gateway_devices.push_back(device);
}

void ZigbeeGateway::setIASZReporting(uint16_t min_interval, uint16_t max_interval) {
  /* Send "configure report attribute" command to the bound sensor */
  esp_zb_zcl_config_report_cmd_t report_cmd;
  report_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
  report_cmd.zcl_basic_cmd.src_endpoint = _instance->getEndpoint();
  report_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE;

  int16_t report_change = 1;
  esp_zb_zcl_config_report_record_t records[] = {
    {
      .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV, //0x00, //ESP_ZB_ZCL_REPORT_DIRECTION_SEND,
      .attributeID = 0,
      .attrType = ESP_ZB_ZCL_ATTR_TYPE_S16,
      .min_interval = min_interval,
      .max_interval = max_interval,
      .reportable_change = &report_change,
    },
  };
  report_cmd.record_number = ZB_ARRAY_LENTH(records);
  report_cmd.record_field = records;

  //log_i("Sending 'configure reporting' command");
  esp_zb_lock_acquire(portMAX_DELAY);
  esp_zb_zcl_config_report_cmd_req(&report_cmd);
  esp_zb_lock_release();
}


#endif  //SOC_IEEE802154_SUPPORTED && CONFIG_ZB_ENABLED
