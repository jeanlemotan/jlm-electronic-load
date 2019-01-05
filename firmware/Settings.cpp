#include "Settings.h"
#include "Util.h"
#include <string.h>

/*
static constexpr uint8_t SETTINGS_VERSION = 7;
static constexpr size_t BIAS_SCALE_SIZE = k_rangeCount * 4 * sizeof(float);

enum eeprom_settings : uint16_t
{
  version           = E2END - 128,
  biasScale			= E2END - BIAS_SCALE_SIZE - 2,
  crc               = biasScale + BIAS_SCALE_SIZE + 1
};


void resetSettings()
{
    if (eeprom_read_byte(&eeprom_settingsVersion) != 0)
    {
        eeprom_write_byte(&eeprom_settingsVersion, 0);
    }
}

void saveSettings(Settings const& settings)
{
    LOG(PSTR("Save settings..."));
    uint32_t crc = 123456789ULL;
    eeprom_write_byte((uint8_t*)eeprom_settings::version, SETTINGS_VERSION);
    hashCombine(crc, SETTINGS_VERSION);

    uint32_t* address = (uint32_t*)eeprom_settings::biasScale;
    for (uint8_t i = 0; i < k_rangeCount; i++)
    {
    	uint32_t v = *(uint32_t*)&settings.currentRangeBiases[i];
    	eeprom_write_dword(address++, v);
    	hashCombine(crc, v);

    	v = *(uint32_t*)&settings.currentRangeScales[i];
    	eeprom_write_dword(address++, v);
    	hashCombine(crc, v);

    	v = *(uint32_t*)&settings.voltageRangeBiases[i];
    	eeprom_write_dword(address++, v);
    	hashCombine(crc, v);

    	v = *(uint32_t*)&settings.voltageRangeScales[i];
    	eeprom_write_dword(address++, v);
    	hashCombine(crc, v);
	}

    eeprom_write_dword((uint32_t*)eeprom_settings::crc, crc);
    LOG(PSTR("done\n"));
}

bool loadSettings(Settings& settings)
{
    LOG(PSTR("Load settings..."));
    if (eeprom_read_byte((const uint8_t*)eeprom_settings::version) == SETTINGS_VERSION)
    {
        uint32_t crc = 123456789ULL;
        hashCombine(crc, SETTINGS_VERSION);

	    const uint32_t* address = (const uint32_t*)eeprom_settings::biasScale;
	    for (uint8_t i = 0; i < k_rangeCount; i++)
	    {
	    	uint32_t v;;
	    	eeprom_read_dword(address++, v);
	    	hashCombine(crc, v);
	    	*(uint32_t*)&settings.currentRangeBiases[i] = v;

	    	eeprom_read_dword(address++, v);
	    	hashCombine(crc, v);
	    	*(uint32_t*)&settings.currentRangeScales[i] = v;

	    	eeprom_read_dword(address++, v);
	    	hashCombine(crc, v);
	    	*(uint32_t*)&settings.voltageRangeBiases[i] = v;

	    	eeprom_read_dword(address++, v);
	    	hashCombine(crc, v);
	    	*(uint32_t*)&settings.voltageRangeScales[i] = v;
		}

        uint32_t storedCrc = eeprom_read_dword((const uint32_t*)eeprom_settings::crc);
        if (storedCrc != crc)
        {
            LOG(PSTR("failed: crc\n"));
            settings = Settings();
            resetSettings();
            return false;
        }
        LOG(PSTR("done\n"));
        return true;
    }
    else
    {
        LOG(PSTR("failed: old\n"));
        settings = Settings();
        resetSettings();
        return false;
    }
}
*/
