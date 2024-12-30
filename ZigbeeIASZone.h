/* Class of Zigbee Temperature sensor endpoint inherited from common EP class */

#pragma once

#include "soc/soc_caps.h"
#include "sdkconfig.h"
#if SOC_IEEE802154_SUPPORTED 
//&& CONFIG_ZB_ENABLED

#include "ZigbeeEP.h"
#include "ha/esp_zigbee_ha_standard.h"

//define the thermostat configuration to avoid narrowing conversion issue in zigbee-sdk
#define ZB_DEFAULT_IAS_ZONE_CONFIG()                                                               \
  {                                                                                                  \
    .basic_cfg =                                                                                     \
      {                                                                                              \
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,                                   \
        .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,                                 \
      },                                                                                             \
    .identify_cfg =                                                                                  \
      {                                                                                              \
        .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,                            \
      }                                                                                        \
  }
class ZigbeeIASZone : public ZigbeeEP {
public:
  ZigbeeIASZone(uint8_t endpoint);
  ~ZigbeeIASZone();

  void onStatusNotification(void (*callback)(int, uint8_t *)) {
    _on_status_notification = callback;
}

private:
  // save instance of the class in order to use it in static functions
  static ZigbeeIASZone *_instance;

  void (*_on_status_notification)(int, uint8_t *);
 
  void findEndpoint(esp_zb_zdo_match_desc_req_param_t *cmd_req);
  static void bindCb(esp_zb_zdp_status_t zdo_status, void *user_ctx);
  static void findCb(esp_zb_zdp_status_t zdo_status, uint16_t addr, uint8_t endpoint, void *user_ctx);

  void zbAttributeRead(uint16_t cluster_id, const esp_zb_zcl_attribute_t *attribute) override;
  void zbIASZoneStatusChangeNotification(const esp_zb_zcl_ias_zone_status_change_notification_message_t *message) override;
};

#endif  //SOC_IEEE802154_SUPPORTED && CONFIG_ZB_ENABLED
