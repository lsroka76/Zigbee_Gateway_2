#include "ZigbeeIASZone.h"
#if SOC_IEEE802154_SUPPORTED 
//&& CONFIG_ZB_ENABLED

// Initialize the static instance of the class
ZigbeeIASZone *ZigbeeIASZone::_instance = nullptr;

ZigbeeIASZone::ZigbeeIASZone(uint8_t endpoint) : ZigbeeEP(endpoint) {
  _device_id = ESP_ZB_HA_REMOTE_CONTROL_DEVICE_ID;
  _instance = this;  // Set the static pointer to this instance

  //use custom config to avoid narrowing error -> must be fixed in zigbee-sdk
  esp_zb_configuration_tool_cfg_t ias_zone_cfg = ZB_DEFAULT_IAS_ZONE_CONFIG();

  _cluster_list = esp_zb_zcl_cluster_list_create();
  esp_zb_attribute_list_t *basic_cluster = esp_zb_basic_cluster_create(&(ias_zone_cfg.basic_cfg));
  esp_zb_cluster_list_add_basic_cluster(_cluster_list, basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  esp_zb_cluster_list_add_identify_cluster(_cluster_list, esp_zb_identify_cluster_create(&(ias_zone_cfg.identify_cfg)), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  esp_zb_cluster_list_add_identify_cluster(_cluster_list, esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
  
  esp_zb_cluster_list_add_ias_zone_cluster(_cluster_list, esp_zb_ias_zone_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);

  _ep_config = {.endpoint = _endpoint, .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID, .app_device_id = ESP_ZB_HA_REMOTE_CONTROL_DEVICE_ID, .app_device_version = 0};
}

void ZigbeeIASZone::bindCb(esp_zb_zdp_status_t zdo_status, void *user_ctx) {
  if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS) {
    if (user_ctx) {
      zb_device_params_t *sensor = (zb_device_params_t *)user_ctx;
      log_i("The IASZ sensor originating from address(0x%x) on endpoint(%d)", sensor->short_addr, sensor->endpoint);
      _instance->_bound_devices.push_back(sensor);
    } else {
      log_v("Local binding success");
    }
    _is_bound = true;
  } else {
    log_e("Binding failed!");
  }
}

void ZigbeeIASZone::findCb(esp_zb_zdp_status_t zdo_status, uint16_t addr, uint8_t endpoint, void *user_ctx) {
  
  if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS) {
    log_i("Found IASZ sensor");
    esp_zb_zdo_bind_req_param_t bind_req;
    /* Store the information of the remote device */
    zb_device_params_t *sensor = (zb_device_params_t *)malloc(sizeof(zb_device_params_t));
    sensor->endpoint = endpoint;
    sensor->short_addr = addr;
    esp_zb_ieee_address_by_short(sensor->short_addr, sensor->ieee_addr);
    log_d("IASZ sensor found: short address(0x%x), endpoint(%d)", sensor->short_addr, sensor->endpoint);

    /* 1. Send binding request to the sensor */
    bind_req.req_dst_addr = addr;
    log_d("Request IASZ sensor to bind us");

    /* populate the src information of the binding */
    memcpy(bind_req.src_address, sensor->ieee_addr, sizeof(esp_zb_ieee_addr_t));
    bind_req.src_endp = endpoint;
    bind_req.cluster_id = ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE;
    log_d("Bind IASZ sensor");

    /* populate the dst information of the binding */
    bind_req.dst_addr_mode = ESP_ZB_ZDO_BIND_DST_ADDR_MODE_64_BIT_EXTENDED;
    esp_zb_get_long_address(bind_req.dst_address_u.addr_long);
    bind_req.dst_endp = *((uint8_t *)user_ctx);  //_endpoint;

    log_i("Request sensor to bind us");
    esp_zb_zdo_device_bind_req(&bind_req, bindCb, NULL);
/*
    // 2. Send binding request to self 
    bind_req.req_dst_addr = esp_zb_get_short_address();

    // populate the src information of the binding
    esp_zb_get_long_address(bind_req.src_address);
    bind_req.src_endp = *((uint8_t *)user_ctx);  //_endpoint;
    bind_req.cluster_id = ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE;

    // populate the dst information of the binding
    bind_req.dst_addr_mode = ESP_ZB_ZDO_BIND_DST_ADDR_MODE_64_BIT_EXTENDED;
    memcpy(bind_req.dst_address_u.addr_long, sensor->ieee_addr, sizeof(esp_zb_ieee_addr_t));
    bind_req.dst_endp = endpoint;

    log_i("Bind sensor");
    esp_zb_zdo_device_bind_req(&bind_req, bindCb, (void *)sensor);
*/
  }
}

void ZigbeeIASZone::findEndpoint(esp_zb_zdo_match_desc_req_param_t *param) {
  
  uint16_t cluster_list[] = {ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE};
  param->profile_id = ESP_ZB_AF_HA_PROFILE_ID;
  param->num_in_clusters = 1;
  param->num_out_clusters = 0;
  param->cluster_list = cluster_list;
  esp_zb_zdo_match_cluster(param, findCb, &_endpoint);
}

void ZigbeeIASZone::zbAttributeRead(uint16_t cluster_id, const esp_zb_zcl_attribute_t *attribute) {
  
  
  log_i("cluster_id %d ",cluster_id);
  log_i("attribute->id %d ", attribute->id);
}

void ZigbeeIASZone::zbIASZoneStatusChangeNotification(const esp_zb_zcl_ias_zone_status_change_notification_message_t *message) {

  esp_zb_zcl_cmd_info_t info = message->info;
  esp_zb_zcl_addr_t src_address = info.src_address;
  log_i("short address %d ", src_address.u.short_addr);
  log_i("src id %d ", src_address.u.src_id);
  Serial.println("inside zbIASZoneStatusChangeNotification");
  char ieee_addr[9];
  uint8_t ieee_addr_64[8];

  ieee_addr[8] = '\0';

  esp_zb_ieee_address_by_short(src_address.u.short_addr, ieee_addr_64);

  //for (int i = 0; i < 8; i++)
    //ieee_addr[i] = ieee_addr_64[i];
    log_i("ieee addr %d:%d:%d:%d:%d:%d:%d:%d ",ieee_addr_64[7],ieee_addr_64[6],ieee_addr_64[5],ieee_addr_64[4],
    ieee_addr_64[3],ieee_addr_64[2],ieee_addr_64[1],ieee_addr_64[0]);
  //log_i("src id %s ",ieee_addr);
  log_i("zone status %d ", message->zone_status);
  if (_on_status_notification)
    _on_status_notification(message->zone_status, ieee_addr_64); 
}

#endif  //SOC_IEEE802154_SUPPORTED && CONFIG_ZB_ENABLED
