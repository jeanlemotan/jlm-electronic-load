#pragma once

#include "Adafruit_GFX.h"
#include "Adafruit_SPITFT.h"

class DeltaBitmap : public Adafruit_GFX 
{
public:
  DeltaBitmap(uint16_t w, uint16_t h, uint16_t cellWBits, uint16_t cellHBits);
  ~DeltaBitmap();

  void setOpacity(uint8_t opacity);
  void setClipRect(int16_t x, int16_t y, int16_t w, int16_t h);

  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void fillScreen(uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

  void blit(Adafruit_SPITFT& dst);

private:
  void writePixel(int16_t x, int16_t y, uint16_t color);
  void writePixel(int16_t x, int16_t y, uint16_t color, uint8_t alpha);
  void _fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

  uint16_t _cellWBits = 0;
  uint16_t _cellHBits = 0;
  uint16_t _cellWMask = 0;
  uint16_t _cellHMask = 0;
  uint16_t _cellW = 0;
  uint16_t _cellH = 0;
  uint16_t _cellCountX = 0;
  uint16_t _cellCountY = 0;

  typedef uint32_t CellHash;

  struct Cell
  {
    Cell(size_t size);
    ~Cell();

    Cell(Cell&&) = default;
    Cell& operator=(Cell&&) = default;

    CellHash hash = 0;
    uint16_t* data = nullptr;
  };

  Cell* acquireCell();
  inline void releaseCell(Cell* cell);
  inline void fillCell(Cell& cell, uint16_t color, uint8_t opacity);

  CellHash computeCellHash(Cell& cell) const;

  std::vector<Cell*> _cellPool; //owning
  std::vector<Cell*> _cells; //non-owning
  Cell* _clearCell = nullptr; //non-owning
  uint16_t _clearColor = 0;
  uint8_t _opacity = 0xFF;
  int16_t _clipX = 0;
  int16_t _clipY = 0;
  int16_t _clipX2 = 1;
  int16_t _clipY2 = 1;

  std::vector<CellHash> _prevCellHashes;
};
