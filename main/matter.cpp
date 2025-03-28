#include "matter.h"
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>
#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>
#include <setup_payload/SetupPayload.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/ManualSetupPayloadGenerator.h>
#include <lib/support/CodeUtils.h>
#include "matter_callback.h"

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

//
//  Generate QR code with https://github.com/project-chip/connectedhomeip/tree/master/src/setup_payload/python
//
static void get_setup_code(void)
{
    uint32_t setupCode = 0;
    uint16_t setupDescriminator = 0;

    CHIP_ERROR err = chip::DeviceLayer::GetCommissionableDataProvider()->GetSetupPasscode(setupCode);
    if (err == CHIP_NO_ERROR)
    {
        ESP_LOGI(TAG, "Setup code retrieved from commissionable data provider: %lu", setupCode);

        err = chip::DeviceLayer::GetCommissionableDataProvider()->GetSetupDiscriminator(setupDescriminator);
        if (err != CHIP_NO_ERROR)
        {
            ESP_LOGE(TAG, "Failed to retrieve setup discriminator from commissionable data provider: %s", chip::ErrorStr(err));
            return;
        }

        ESP_LOGI(TAG, "Setup discriminator retrieved from commissionable data provider: %u", setupDescriminator);

        uint16_t product_id, vendor_id;
        err = chip::DeviceLayer::GetDeviceInstanceInfoProvider()->GetProductId(product_id);
        err = chip::DeviceLayer::GetDeviceInstanceInfoProvider()->GetVendorId(vendor_id);

        ESP_LOGI(TAG, "Product ID: %u, Vendor ID: %u", product_id, vendor_id);

        chip::SetupPayload payload;
        payload.setUpPINCode = setupCode;
        payload.discriminator.SetLongValue(setupDescriminator);
        payload.productID = product_id;
        payload.vendorID = vendor_id;
        // payload.version = 1;

        std::string manual_pairing_code;
        chip::ManualSetupPayloadGenerator manualGenerator(payload);
        err = manualGenerator.payloadDecimalStringRepresentation(manual_pairing_code);
        if (err == CHIP_NO_ERROR)
        {
            ESP_LOGI(TAG, "Manual pairing code: %s", manual_pairing_code.c_str());
        }
        else
        {
            ESP_LOGE(TAG, "Failed to generate manual pairing code: %s", chip::ErrorStr(err));
        }

#if 0
        std::string qr_code;
        //qr_code.reserve(51200);
        
        chip::QRCodeSetupPayloadGenerator qr_generator(payload);
        err = qr_generator.payloadBase38Representation(qr_code);
        if (err == CHIP_NO_ERROR)
        {
            ESP_LOGI(TAG, "QR code: %s", qr_code.c_str());
        }
        else
        {
            ESP_LOGE(TAG, "Failed to generate QR code: %s", chip::ErrorStr(err));
        }
#endif
    }
    else
    {
        ESP_LOGE(TAG, "Failed to retrieve setup code from commissionable data provider: %s", chip::ErrorStr(err));
    }
}

static bool is_provisioned(void)
{
    return chip::DeviceLayer::ConfigurationMgr().IsFullyProvisioned();
}

static void reset_provisioning(void)
{
    ESP_LOGW(TAG, "Resetting provisioning - nvs flash erase");
    nvs_flash_erase();
    ESP_LOGW(TAG, "Resetting provisioning - InitiateFactoryReset");
    chip::DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
    ESP_LOGW(TAG, "Factory reset completed");
}

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
        get_setup_code();
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
    ESP_LOGI(TAG, "[CBC] Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
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
        //esp_matter_val_type_t val_type = val->type;

        if (cluster_id == OnOff::Id && attribute_id == OnOff::Attributes::OnOff::Id)
        {
            ESP_LOGI(TAG, "[CBC] We got a boolean value: for type %u cluster %lu attribute: %lu -> %d", type, cluster_id, attribute_id, val->val.b);
            matter_ui_update_msg_t msg = {LIGHT_ON_CHANGE, val->val.b == 1};
            ghome_matter_events_enqueue(msg);
        }
        else if (cluster_id == LevelControl::Id && attribute_id == LevelControl::Attributes::CurrentLevel::Id)
        {
            ESP_LOGI(TAG, "[CBC] We got a int16_t level value: for type %u cluster %lu attribute: %lu -> %d", type, cluster_id, attribute_id, val->val.i16);
            matter_ui_update_msg_t msg = {BRIGTHNESS_CHANGE, val->val.i16};
            ghome_matter_events_enqueue(msg);
        }

        ESP_LOGI(TAG, "[CBC] Attribute update callback: type: %u, cluster: %lu, attribute: %lu", type, cluster_id, attribute_id);
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

        /* Mark deferred persistence for some attributes that might be changed rapidly 
        attribute_t *current_level_attribute = attribute::get(state.light_endpoint_id, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id);
        attribute::set_deferred_persistence(current_level_attribute);

        attribute_t *current_x_attribute = attribute::get(state.light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentX::Id);
        attribute::set_deferred_persistence(current_x_attribute);
        attribute_t *current_y_attribute = attribute::get(state.light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentY::Id);
        attribute::set_deferred_persistence(current_y_attribute);
        attribute_t *color_temp_attribute = attribute::get(state.light_endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id);
        attribute::set_deferred_persistence(color_temp_attribute);

        ESP_LOGI(TAG, "Deferred persistence set for some attributes");
        */

        err = esp_matter::start(app_event_cb);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to start Matter: %s", esp_err_to_name(err));
            break;
        }
        ESP_LOGI(TAG, "Matter started");

        if (is_provisioned())
        {
            ESP_LOGI(TAG, "Device is already provisioned");
        }

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