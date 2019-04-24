#pragma once

#include <stdint.h>

struct Image
{
  uint16_t  width;
  uint16_t  height;
  uint16_t  bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  const char*  pixel_data;
};

extern Image k_imgOk;
extern Image k_imgCancel;
extern Image k_imgCardOk;
extern Image k_imgCardError;
extern Image k_img2Wire;
extern Image k_img4Wire;
extern Image k_imgTarget;


