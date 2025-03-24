#include "matter.h"
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>
#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

#define TAG "GHOME_MATTER"

#define DEFAULT_ON true
#define DEFAULT_BRIGHTNESS 255

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

constexpr auto k_timeout_seconds = 300;

struct matter_state_t
{
    node_t *node;
    endpoint_t *light_endpoint;
    uint16_t light_endpoint_id;
    void *light_handle;
};
matter_state_t state{};

#if CONFIG_ENABLE_ENCRYPTED_OTA
extern const char decryption_key_start[] asm("_binary_esp_image_encryption_key_pem_start");
extern const char decryption_key_end[] asm("_binary_esp_image_encryption_key_pem_end");

static const char *s_decryption_key = decryption_key_start;
static const uint16_t s_decryption_key_len = decryption_key_end - decryption_key_start;
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type)
    {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address changed");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
    {
        ESP_LOGI(TAG, "Fabric removed successfully");
        break;
    }

    case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved:
        ESP_LOGI(TAG, "Fabric will be removed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricUpdated:
        ESP_LOGI(TAG, "Fabric is updated");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricCommitted:
        ESP_LOGI(TAG, "Fabric is committed");
        break;

    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
        ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
        break;

    default:
        break;
    };
}

// This callback is invoked when clients interact with the Identify Cluster.
// In the callback implementation, an endpoint can identify itself. (e.g., by flashing an LED or light).
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

// This callback is called for every attribute update. The callback implementation shall
// handle the desired attributes and return an appropriate error code. If the attribute
// is not of your interest, please do not return an error code and strictly return ESP_OK.
static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    esp_err_t err = ESP_OK;

    if (type == PRE_UPDATE)
    {
        /* Driver update */
        // app_driver_handle_t driver_handle = (app_driver_handle_t)priv_data;
        // err = app_driver_attribute_update(driver_handle, endpoint_id, cluster_id, attribute_id, val);
        ESP_LOGI(TAG, "Attribute update callback: type: %u, cluster: %lu, attribute: %lu", type, cluster_id, attribute_id);
    }

    return err;
}

bool ghome_matter_init()
{
    ESP_LOGI(TAG, "Initializing GHome Matter");
    esp_err_t err = ESP_OK;

    do
    {
        err = nvs_flash_init();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialize NVS flash: %s", esp_err_to_name(err));
            break;
        }
        ESP_LOGI(TAG, "NVS flash initialized");

        node::config_t node_config;

        // node handle can be used to add/modify other endpoints.
        state.node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
        if (state.node == nullptr)
        {
            ESP_LOGE(TAG, "Failed to create Matter node");
            break;
        }
        ESP_LOGI(TAG, "Matter node created");

        extended_color_light::config_t light_config;
        light_config.on_off.on_off = DEFAULT_ON;
        light_config.on_off.lighting.start_up_on_off = nullptr;
        light_config.level_control.current_level = DEFAULT_BRIGHTNESS;
        light_config.level_control.on_level = DEFAULT_BRIGHTNESS;
        light_config.level_control.lighting.start_up_current_level = DEFAULT_BRIGHTNESS;
        light_config.color_control.color_mode = (uint8_t)ColorControl::ColorMode::kColorTemperature;
        light_config.color_control.enhanced_color_mode = (uint8_t)ColorControl::ColorMode::kColorTemperature;
        light_config.color_control.color_temperature.startup_color_temperature_mireds = nullptr;

        // endpoint handles can be used to add/modify clusters.
        state.light_endpoint = extended_color_light::create(state.node, &light_config, ENDPOINT_FLAG_NONE, state.light_handle);
        if (state.light_endpoint == nullptr)
        {
            ESP_LOGE(TAG, "Failed to create extended color light endpoint");
            break;
        }
        ESP_LOGI(TAG, "Extended color light endpoint created");

        state.light_endpoint_id = endpoint::get_id(state.light_endpoint);
        ESP_LOGI(TAG, "Light created with endpoint_id %d", state.light_endpoint_id);

        /* Mark deferred persistence for some attributes that might be changed rapidly */
        attribute_t *current_level_attribute = attribute::get(state.light_endpoint_id, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id);
        attribute::set_deferred_persistence(current_level_attribute);

        attribute_t *current_x_attribute = attribute::get(state.light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentX::Id);
        attribute::set_deferred_persistence(current_x_attribute);
        attribute_t *current_y_attribute = attribute::get(state.light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentY::Id);
        attribute::set_deferred_persistence(current_y_attribute);
        attribute_t *color_temp_attribute = attribute::get(state.light_endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id);
        attribute::set_deferred_persistence(color_temp_attribute);

        ESP_LOGI(TAG, "Deferred persistence set for some attributes");

        err = esp_matter::start(app_event_cb);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to start Matter: %s", esp_err_to_name(err));
            break;
        }
        ESP_LOGI(TAG, "Matter started");

#if CONFIG_ENABLE_CHIP_SHELL
        esp_matter::console::diagnostics_register_commands();
        esp_matter::console::wifi_register_commands();
#if CONFIG_OPENTHREAD_CLI
        esp_matter::console::otcli_register_commands();
#endif
        esp_matter::console::init();
#endif

    } while (false);

    return err == ESP_OK;
}

void ghome_matter_deinit()
{
}