#include "Settings.h"
#include "Util.h"
#include <string.h>
#include "esp_log.h"

#include <iostream>
#include <fstream>

void saveSettings(Settings const& _settings) 
{
    Settings settings = _settings;
    ESP_LOGI("Settings", "Save settings...");
    uint32_t crc = 123456789ULL;
    settings.version = Settings::k_version;
    hashCombine(crc, settings.version);

    hashCombine(crc, *(uint32_t*)&settings.temperatureBias);
    hashCombine(crc, *(uint32_t*)&settings.temperatureScale);
    for (uint8_t i = 0; i < Settings::k_rangeCount; i++)
    {
      hashCombine(crc, *(uint32_t*)&settings.currentRangeBiases[i]);
      hashCombine(crc, *(uint32_t*)&settings.currentRangeScales[i]);
      hashCombine(crc, *(uint32_t*)&settings.voltageRangeBiases[i]);
      hashCombine(crc, *(uint32_t*)&settings.voltageRangeScales[i]);
    }

    settings.crc = crc;

    std::ofstream file;
    file.open("settings");
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
    file.open("settings");
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
    for (uint8_t i = 0; i < Settings::k_rangeCount; i++)
    {
    	hashCombine(crc, *(uint32_t*)&settings.currentRangeBiases[i]);
    	hashCombine(crc, *(uint32_t*)&settings.currentRangeScales[i]);
    	hashCombine(crc, *(uint32_t*)&settings.voltageRangeBiases[i]);
    	hashCombine(crc, *(uint32_t*)&settings.voltageRangeScales[i]);
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
