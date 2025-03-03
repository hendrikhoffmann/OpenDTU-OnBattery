// SPDX-License-Identifier: GPL-2.0-or-later

#include "PylontechCanReceiver.h"
#include "Battery.h"
#include "MqttHandleBatteryHass.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "Utils.h"

MqttHandleBatteryHassClass MqttHandleBatteryHass;

void MqttHandleBatteryHassClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandleBatteryHassClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();
}

void MqttHandleBatteryHassClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if (!config.Battery.Enabled) { return; }

    if (!config.Mqtt.Hass.Enabled) { return; }

    // TODO(schlimmchen): this cannot make sure that transient
    // connection problems are actually always noticed.
    if (!MqttSettings.getConnected()) {
        _doPublish = true;
        return;
    }

    // only publish HA config once when (re-)connecting
    // to the MQTT broker or on config changes.
    if (!_doPublish) { return; }

    // the MQTT battery provider does not re-publish the SoC under a different
    // known topic. we don't know the manufacture either. HASS auto-discovery
    // for that provider makes no sense.
    if (config.Battery.Provider != 2) {
        publishSensor("Manufacturer",          "mdi:factory",        "manufacturer");
        publishSensor("Data Age",              "mdi:timer-sand",     "dataAge",       "duration", "measurement", "s");
        publishSensor("State of Charge (SoC)", "mdi:battery-medium", "stateOfCharge", "battery",  "measurement", "%");
    }

    switch (config.Battery.Provider) {
        case 0: // Pylontech Battery
            publishSensor("Battery voltage", NULL, "voltage", "voltage", "measurement", "V");
            publishSensor("Battery current", NULL, "current", "current", "measurement", "A");
            publishSensor("Temperature", NULL, "temperature", "temperature", "measurement", "°C");
            publishSensor("State of Health (SOH)", "mdi:heart-plus", "stateOfHealth", NULL, "measurement", "%");
            publishSensor("Charge voltage (BMS)", NULL, "settings/chargeVoltage", "voltage", "measurement", "V");
            publishSensor("Charge current limit", NULL, "settings/chargeCurrentLimitation", "current", "measurement", "A");
            publishSensor("Discharge current limit", NULL, "settings/dischargeCurrentLimitation", "current", "measurement", "A");

            publishBinarySensor("Alarm Discharge current", "mdi:alert", "alarm/overCurrentDischarge", "1", "0");
            publishBinarySensor("Warning Discharge current", "mdi:alert-outline", "warning/highCurrentDischarge", "1", "0");

            publishBinarySensor("Alarm Temperature low", "mdi:thermometer-low", "alarm/underTemperature", "1", "0");
            publishBinarySensor("Warning Temperature low", "mdi:thermometer-low", "warning/lowTemperature", "1", "0");

            publishBinarySensor("Alarm Temperature high", "mdi:thermometer-high", "alarm/overTemperature", "1", "0");
            publishBinarySensor("Warning Temperature high", "mdi:thermometer-high", "warning/highTemperature", "1", "0");

            publishBinarySensor("Alarm Voltage low", "mdi:alert", "alarm/underVoltage", "1", "0");
            publishBinarySensor("Warning Voltage low", "mdi:alert-outline", "warning/lowVoltage", "1", "0");

            publishBinarySensor("Alarm Voltage high", "mdi:alert", "alarm/overVoltage", "1", "0");
            publishBinarySensor("Warning Voltage high", "mdi:alert-outline", "warning/highVoltage", "1", "0");

            publishBinarySensor("Alarm BMS internal", "mdi:alert", "alarm/bmsInternal", "1", "0");
            publishBinarySensor("Warning BMS internal", "mdi:alert-outline", "warning/bmsInternal", "1", "0");

            publishBinarySensor("Alarm High charge current", "mdi:alert", "alarm/overCurrentCharge", "1", "0");
            publishBinarySensor("Warning High charge current", "mdi:alert-outline", "warning/highCurrentCharge", "1", "0");

            publishBinarySensor("Charge enabled", "mdi:battery-arrow-up", "charging/chargeEnabled", "1", "0");
            publishBinarySensor("Discharge enabled", "mdi:battery-arrow-down", "charging/dischargeEnabled", "1", "0");
            publishBinarySensor("Charge immediately", "mdi:alert", "charging/chargeImmediately", "1", "0");
            break;
        case 1: // JK BMS
            //            caption              icon                    topic                       dev. class     state class    unit
            publishSensor("Voltage",           "mdi:battery-charging", "BatteryVoltageMilliVolt",  "voltage",     "measurement", "mV");
            publishSensor("Current",           "mdi:current-dc",       "BatteryCurrentMilliAmps",  "current",     "measurement", "mA");
            publishSensor("BMS Temperature",   "mdi:thermometer",      "BmsTempCelsius",           "temperature", "measurement", "°C");
            publishSensor("Cell Voltage Diff", "mdi:battery-alert",    "CellDiffMilliVolt",        "voltage",     "measurement", "mV");
            publishSensor("Charge Cycles",     "mdi:counter",          "BatteryCycles");
            publishSensor("Cycle Capacity",    "mdi:battery-sync",     "BatteryCycleCapacity");

            publishBinarySensor("Charging Possible",    "mdi:battery-arrow-up",   "status/ChargingActive",    "1", "0");
            publishBinarySensor("Discharging Possible", "mdi:battery-arrow-down", "status/DischargingActive", "1", "0");
            publishBinarySensor("Balancing Active",     "mdi:scale-balance",      "status/BalancingActive",   "1", "0");

#define PBS(a, b, c) publishBinarySensor("Alarm: " a, "mdi:" b, "alarms/" c, "1", "0")
            PBS("Low Capacity",                "battery-alert-variant-outline", "LowCapacity");
            PBS("BMS Overtemperature",         "thermometer-alert",             "BmsOvertemperature");
            PBS("Charging Overvoltage",        "fuse-alert",                    "ChargingOvervoltage");
            PBS("Discharge Undervoltage",      "fuse-alert",                    "DischargeUndervoltage");
            PBS("Battery Overtemperature",     "thermometer-alert",             "BatteryOvertemperature");
            PBS("Charging Overcurrent",        "fuse-alert",                    "ChargingOvercurrent");
            PBS("Discharging Overcurrent",     "fuse-alert",                    "DischargeOvercurrent");
            PBS("Cell Voltage Difference",     "battery-alert",                 "CellVoltageDifference");
            PBS("Battery Box Overtemperature", "thermometer-alert",             "BatteryBoxOvertemperature");
            PBS("Battery Undertemperature",    "thermometer-alert",             "BatteryUndertemperature");
            PBS("Cell Overvoltage",            "battery-alert",                 "CellOvervoltage");
            PBS("Cell Undervoltage",           "battery-alert",                 "CellUndervoltage");
#undef PBS
            break;
        case 2: // SoC from MQTT
            break;
        case 3: // Victron SmartShunt
            break;
    }

    _doPublish = false;
}

void MqttHandleBatteryHassClass::publishSensor(const char* caption, const char* icon, const char* subTopic, const char* deviceClass, const char* stateClass, const char* unitOfMeasurement )
{
    String sensorId = caption;
    sensorId.replace(" ", "_");
    sensorId.replace(".", "");
    sensorId.replace("(", "");
    sensorId.replace(")", "");
    sensorId.toLowerCase();

    String configTopic = "sensor/dtu_battery_" + serial
        + "/" + sensorId
        + "/config";

    String statTopic = MqttSettings.getPrefix() + "battery/";
    // omit serial to avoid a breaking change
    // statTopic.concat(serial);
    // statTopic.concat("/");
    statTopic.concat(subTopic);

    DynamicJsonDocument root(1024);
    if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
        return;
    }
    root["name"] = caption;
    root["stat_t"] = statTopic;
    root["uniq_id"] = serial + "_" + sensorId;

    if (icon != NULL) {
        root["icon"] = icon;
    }

    if (unitOfMeasurement != NULL) {
        root["unit_of_meas"] = unitOfMeasurement;
    }

    JsonObject deviceObj = root.createNestedObject("dev");
    createDeviceInfo(deviceObj);

    if (Configuration.get().Mqtt.Hass.Expire) {
        root["exp_aft"] = Battery.getStats()->getMqttFullPublishIntervalMs() * 3;
    }
    if (deviceClass != NULL) {
        root["dev_cla"] = deviceClass;
    }
    if (stateClass != NULL) {
        root["stat_cla"] = stateClass;
    }

    char buffer[512];
    serializeJson(root, buffer);
    publish(configTopic, buffer);

}

void MqttHandleBatteryHassClass::publishBinarySensor(const char* caption, const char* icon, const char* subTopic, const char* payload_on, const char* payload_off)
{
    String sensorId = caption;
    sensorId.replace(" ", "_");
    sensorId.replace(".", "");
    sensorId.replace("(", "");
    sensorId.replace(")", "");
    sensorId.replace(":", "");
    sensorId.toLowerCase();

    String configTopic = "binary_sensor/dtu_battery_" + serial
        + "/" + sensorId
        + "/config";

    String statTopic = MqttSettings.getPrefix() + "battery/";
    // omit serial to avoid a breaking change
    // statTopic.concat(serial);
    // statTopic.concat("/");
    statTopic.concat(subTopic);

    DynamicJsonDocument root(1024);
    if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
        return;
    }
    root["name"] = caption;
    root["uniq_id"] = serial + "_" + sensorId;
    root["stat_t"] = statTopic;
    root["pl_on"] = payload_on;
    root["pl_off"] = payload_off;

    if (icon != NULL) {
        root["icon"] = icon;
    }

    JsonObject deviceObj = root.createNestedObject("dev");
    createDeviceInfo(deviceObj);

    char buffer[512];
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleBatteryHassClass::createDeviceInfo(JsonObject& object)
{
    object["name"] = "Battery(" + serial + ")";

    auto& config = Configuration.get();
    if (config.Battery.Provider == 1) {
        object["name"] = "JK BMS (" + Battery.getStats()->getManufacturer() + ")";
    }

    object["ids"] = serial;
    object["cu"] = String("http://") + NetworkSettings.localIP().toString();
    object["mf"] = "OpenDTU";
    object["mdl"] = Battery.getStats()->getManufacturer();
    object["sw"] = AUTO_GIT_HASH;
}

void MqttHandleBatteryHassClass::publish(const String& subtopic, const String& payload)
{
    String topic = Configuration.get().Mqtt.Hass.Topic;
    topic += subtopic;
    MqttSettings.publishGeneric(topic.c_str(), payload.c_str(), Configuration.get().Mqtt.Hass.Retain);
}
