#include "Settings.h"
#include "Util.h"
#include <string.h>
#include "esp_log.h"

#include <iostream>
#include <fstream>

/*
temperature bias/scale: 0.000000/126.800003
current bias/scale 0: 0.005415/1.711373
voltage bias/scale 0: -0.001118/9.991604
current bias/scale 1: 0.005461/1.711530
voltage bias/scale 1: -0.001108/9.995218
current bias/scale 2: 0.005306/1.712632
voltage bias/scale 2: -0.001080/9.995083
current bias/scale 3: 0.005076/1.715677
voltage bias/scale 3: -0.001109/9.995955
current bias/scale 4: 0.005886/1.708356
voltage bias/scale 4: -0.001094/9.993112
*/


void saveSettings(Settings const& _settings) 
{
    Settings settings = _settings;
    ESP_LOGI("Settings", "Save settings...");
    uint32_t crc = 123456789ULL;
    settings.version = Settings::k_version;
    hashCombine(crc, settings.version);

    hashCombine(crc, *(uint32_t*)&settings.temperatureBias);
    hashCombine(crc, *(uint32_t*)&settings.temperatureScale);
    ESP_LOGI("Settings", "temperature bias/scale: %f/%f", settings.temperatureBias, settings.temperatureScale);
    for (uint8_t i = 0; i < Settings::k_rangeCount; i++)
    {
        hashCombine(crc, *(uint32_t*)&settings.currentRangeBiases[i]);
        hashCombine(crc, *(uint32_t*)&settings.currentRangeScales[i]);
        hashCombine(crc, *(uint32_t*)&settings.voltageRangeBiases[i]);
        hashCombine(crc, *(uint32_t*)&settings.voltageRangeScales[i]);
        ESP_LOGI("Settings", "current bias/scale %d: %f/%f", i, settings.currentRangeBiases[i], settings.currentRangeScales[i]);
        ESP_LOGI("Settings", "voltage bias/scale %d: %f/%f", i, settings.voltageRangeBiases[i], settings.voltageRangeScales[i]);
    }
    for (size_t i = 0; i < settings.dac2CurrentTable.size(); i++)
    {
        float v = settings.dac2CurrentTable[i];
        hashCombine(crc, *(uint32_t*)&v);
        ESP_LOGI("Settings", "dac2current %d: %f", i, v);
    }


    settings.crc = crc;

    std::ofstream file;
    file.open("/spiffs/settings");
    if (!file.is_open())
    {
      ESP_LOGE("Settings", "Cannot open settings file for writing");
      return;
    }
    file.write((const char*)&settings, sizeof(Settings));
    file.close();
    ESP_LOGI("Settings", "done saving");
}

bool loadSettings(Settings& settings)
{
    ESP_LOGI("Settings", "Load settings...");
    std::ifstream file;
    file.open("/spiffs/settings");
    if (!file.is_open())
    {
      ESP_LOGE("Settings", "Cannot open settings file for reading");
      return false;
    }
    file.read((char*)&settings, sizeof(Settings));
    file.close();

    if (settings.version != Settings::k_version)
    {
      ESP_LOGE("Settings", "Version mismatch: expected %u, got %u", Settings::k_version, settings.version);
      return false;
    }
    
    uint32_t crc = 123456789ULL;
    hashCombine(crc, settings.version);

    hashCombine(crc, *(uint32_t*)&settings.temperatureBias);
    hashCombine(crc, *(uint32_t*)&settings.temperatureScale);
    ESP_LOGI("Settings", "temperature bias/scale: %f/%f", settings.temperatureBias, settings.temperatureScale);
    for (uint8_t i = 0; i < Settings::k_rangeCount; i++)
    {
    	hashCombine(crc, *(uint32_t*)&settings.currentRangeBiases[i]);
    	hashCombine(crc, *(uint32_t*)&settings.currentRangeScales[i]);
    	hashCombine(crc, *(uint32_t*)&settings.voltageRangeBiases[i]);
    	hashCombine(crc, *(uint32_t*)&settings.voltageRangeScales[i]);
        ESP_LOGI("Settings", "current bias/scale %d: %f/%f", i, settings.currentRangeBiases[i], settings.currentRangeScales[i]);
        ESP_LOGI("Settings", "voltage bias/scale %d: %f/%f", i, settings.voltageRangeBiases[i], settings.voltageRangeScales[i]);
    }

    for (size_t i = 0; i < settings.dac2CurrentTable.size(); i++)
    {
        float v = settings.dac2CurrentTable[i];
        hashCombine(crc, *(uint32_t*)&v);
        ESP_LOGI("Settings", "dac2current %d: %f", i, v);
    }

    if (settings.crc != crc)
    {
        ESP_LOGE("Settings", "Crc mismatch: expected %u, got %u", crc, settings.crc);
        settings = Settings();
        return false;
    }
    ESP_LOGI("Settings", "done loading");
    return true;
}
