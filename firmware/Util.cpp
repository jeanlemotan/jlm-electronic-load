#include "Util.h"

void hashCombine(uint32_t& seed, uint32_t v)
{
    seed ^= v + 0x9e3779b9 + (seed<<6) + (seed>>2);
}
