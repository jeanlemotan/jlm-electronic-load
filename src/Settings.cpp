#include "Settings.h"
#include "Util.h"
#include <string.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include <iostream>
#include <fstream>


constexpr float k_shuntResistor = 0.1f;
constexpr float k_currentAmplifierGain = 5.7f; //1 + R35/R36
constexpr float k_voltageAmplifierGain = 0.1f; //R26/R19

//decided by the R18, R21, R22, R23 dividers
constexpr float k_minVTtarget = -0.019f; //== DAC 0
constexpr float k_maxVTarget = 0.531f; //== DAC 1

Settings::Settings()
{
    float voltsPerAmp = k_shuntResistor * k_currentAmplifierGain;
    float voltsPerVolt = k_voltageAmplifierGain;

    float currentScale = 1.f / voltsPerAmp;
    float voltageScale = 1.f / voltsPerVolt;
    for (uint8_t i = 0; i < k_rangeCount; i++)
    {
        data.currentRangeScales[i] = currentScale;
        data.voltageRangeScales[i] = voltageScale;
    }

    float minCurrent = k_minVTtarget / k_shuntResistor; //-0.19A == DAC 0
    float maxCurrent = k_maxVTarget / k_shuntResistor; //5.31A == DAC 1
    float dacForZeroCurrent = (0 - minCurrent) / (maxCurrent - minCurrent); //DAC 0.034545455 will result in zero current

    data.dacScale = 1.f / (maxCurrent - minCurrent); //0.181818182: Amps to DAC scale
    data.dacBias = dacForZeroCurrent / data.dacScale; //0.190000002
}


void Settings::save() 
{
    ESP_LOGI("Settings", "Save settings...");
    uint32_t crc = 123456789ULL;
    data.version = k_version;
    hashCombine(crc, data.version);

    hashCombine(crc, *(uint32_t*)&data.temperatureBias);
    hashCombine(crc, *(uint32_t*)&data.temperatureScale);
    ESP_LOGI("Settings", "temperature bias/scale: %f/%f", data.temperatureBias, data.temperatureScale);
    hashCombine(crc, *(uint32_t*)&data.dacBias);
    hashCombine(crc, *(uint32_t*)&data.dacScale);
    ESP_LOGI("Settings", "dac bias/scale: %f/%f", data.dacBias, data.dacScale);
    for (uint8_t i = 0; i < k_rangeCount; i++)
    {
        hashCombine(crc, *(uint32_t*)&data.currentRangeBiases[i]);
        hashCombine(crc, *(uint32_t*)&data.currentRangeScales[i]);
        hashCombine(crc, *(uint32_t*)&data.voltageRangeBiases[i]);
        hashCombine(crc, *(uint32_t*)&data.voltageRangeScales[i]);
        ESP_LOGI("Settings", "current bias/scale %d: %f/%f", i, data.currentRangeBiases[i], data.currentRangeScales[i]);
        ESP_LOGI("Settings", "voltage bias/scale %d: %f/%f", i, data.voltageRangeBiases[i], data.voltageRangeScales[i]);
    }

    data.crc = crc;

    std::ofstream file;
    file.open("/spiffs/settings");
    if (!file.is_open())
    {
      ESP_LOGE("Settings", "Cannot open settings file for writing");
      return;
    }
    file.write((const char*)&data, sizeof(Data));
    file.close();
    ESP_LOGI("Settings", "done saving");
}

bool Settings::load()
{
    ESP_LOGI("Settings", "Load settings...");
    std::ifstream file;
    file.open("/spiffs/settings");
    if (!file.is_open())
    {
      ESP_LOGE("Settings", "Cannot open settings file for reading");
      return false;
    }
    Data d;
    file.read((char*)&d, sizeof(Data));
    file.close();

    if (d.version != k_version)
    {
      ESP_LOGE("Settings", "Version mismatch: expected %u, got %u", k_version, d.version);
      return false;
    }
    
    uint32_t crc = 123456789ULL;
    hashCombine(crc, d.version);

    hashCombine(crc, *(uint32_t*)&d.temperatureBias);
    hashCombine(crc, *(uint32_t*)&d.temperatureScale);
    ESP_LOGI("Settings", "temperature bias/scale: %f/%f", d.temperatureBias, d.temperatureScale);
    hashCombine(crc, *(uint32_t*)&d.dacBias);
    hashCombine(crc, *(uint32_t*)&d.dacScale);
    ESP_LOGI("Settings", "dac bias/scale: %f/%f", data.dacBias, data.dacScale);
    for (uint8_t i = 0; i < k_rangeCount; i++)
    {
    	hashCombine(crc, *(uint32_t*)&d.currentRangeBiases[i]);
    	hashCombine(crc, *(uint32_t*)&d.currentRangeScales[i]);
    	hashCombine(crc, *(uint32_t*)&d.voltageRangeBiases[i]);
    	hashCombine(crc, *(uint32_t*)&d.voltageRangeScales[i]);
        ESP_LOGI("Settings", "current bias/scale %d: %f/%f", i, d.currentRangeBiases[i], d.currentRangeScales[i]);
        ESP_LOGI("Settings", "voltage bias/scale %d: %f/%f", i, d.voltageRangeBiases[i], d.voltageRangeScales[i]);
    }

    if (d.crc != crc)
    {
        ESP_LOGE("Settings", "Crc mismatch: expected %u, got %u", crc, d.crc);
        return false;
    }
    ESP_LOGI("Settings", "done loading");
    data = d;
    return true;
}
