#include "DeltaBitmap.h"

extern "C" {
	uint16_t crc16_le(uint16_t crc, uint8_t const *buf, uint32_t len);
}

DeltaBitmap::DeltaBitmap(uint16_t w, uint16_t h, uint16_t cellWBits, uint16_t cellHBits)
	: Adafruit_GFX(w, h)
	, _cellWBits(cellWBits)
	, _cellHBits(cellHBits)
	, _cellWMask((1 << cellWBits) - 1)
	, _cellHMask((1 << cellHBits) - 1)
	, _cellW(1 << cellWBits)
	, _cellH(1 << cellHBits)
	, _clipX2(w - 1)
	, _clipY2(h - 1)
{
	_cellCountX = w / _cellW;
	if (w % _cellW != 0)
	{
		_cellCountX++;
	}
	_cellCountY = h / _cellH;
	if (h % _cellH != 0)
	{
		_cellCountY++;
	}

	_cells.resize(_cellCountX * _cellCountY);
	_prevCellHashes.resize(_cells.size());

	_clearCell = acquireCell();
}

DeltaBitmap::~DeltaBitmap()
{
	releaseCell(_clearCell);
	for (Cell* cell: _cellPool)
	{
		delete cell;
	}
}

void DeltaBitmap::setOpacity(uint8_t opacity)
{
	_opacity = opacity;
} 

void DeltaBitmap::setClipRect(int16_t x, int16_t y, int16_t w, int16_t h)
{
	_clipX = x >= 0 ? x : 0;
	_clipY = y >= 0 ? y : 0;
	_clipX2 = w >= 1 ? _clipX + w - 1 : _clipX;
	_clipY2 = h >= 1 ? _clipY + h - 1 : _clipY;
}

// inline uint16_t blend(uint16_t c0, uint16_t c1, uint8_t a)
// {
// 	//565
// 	//rrrrrggggggbbbbb
// 	//rrrrr-----gggggg------bbbbb-----
// }

inline uint16_t blend(uint32_t fg, uint32_t bg, uint8_t alpha)
{
    // Alpha converted from [0..255] to [0..31]
    alpha = ( alpha + 4 ) >> 3;

    // Converts  0000000000000000rrrrrggggggbbbbb
    //     into  00000gggggg00000rrrrr000000bbbbb
    // with mask 00000111111000001111100000011111
    // This is useful because it makes space for a parallel fixed-point multiply
    bg = (bg | (bg << 16)) & 0b00000111111000001111100000011111;
    fg = (fg | (fg << 16)) & 0b00000111111000001111100000011111;

    // This implements the linear interpolation formula: result = bg * (1.0 - alpha) + fg * alpha
    // This can be factorized into: result = bg + (fg - bg) * alpha
    // alpha is in Q1.5 format, so 0.0 is represented by 0, and 1.0 is represented by 32
    uint32_t result = (fg - bg) * alpha; // parallel fixed-point multiply of all components
    result >>= 5;
    result += bg;
    result &= 0b00000111111000001111100000011111; // mask out fractional parts
    return (int16_t)((result >> 16) | result); // contract result
}

//alpha is [0..31]
//fg32 is 00000gggggg00000rrrrr000000bbbbb
inline uint16_t blendRaw(uint32_t fg32, uint32_t bg, uint8_t alpha32)
{
    // Converts  0000000000000000rrrrrggggggbbbbb
    //     into  00000gggggg00000rrrrr000000bbbbb
    // with mask 00000111111000001111100000011111
    // This is useful because it makes space for a parallel fixed-point multiply
    bg = (bg | (bg << 16)) & 0b00000111111000001111100000011111;

    // This implements the linear interpolation formula: result = bg * (1.0 - alpha) + fg * alpha
    // This can be factorized into: result = bg + (fg - bg) * alpha
    // alpha is in Q1.5 format, so 0.0 is represented by 0, and 1.0 is represented by 32
    uint32_t result = (fg32 - bg) * alpha32; // parallel fixed-point multiply of all components
    result >>= 5;
    result += bg;
    result &= 0b00000111111000001111100000011111; // mask out fractional parts
    return (int16_t)((result >> 16) | result); // contract result
}

inline uint8_t encodeAlpha(uint8_t alpha)
{
    return (alpha + 4) >> 3;
}
inline uint8_t encodeColor(uint16_t color)
{
    return (color | (color << 16)) & 0b00000111111000001111100000011111;
}

void DeltaBitmap::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	writePixel(x, y, color);
}
void DeltaBitmap::writePixel(int16_t x, int16_t y, uint16_t color, uint8_t alpha)
{
	if (x < _clipX || y < _clipY || x > _clipX2 || y >= _clipY2) 
	{
		return;
	}

	int16_t cx = x >> _cellWBits;
	int16_t cy = y >> _cellHBits;
	int16_t cox = x & _cellWMask;
	int16_t coy = y & _cellHMask;

	uint16_t cellIndex = cy * _cellCountX + cx;
	Cell*& cell = _cells[cellIndex];
	cell = cell ? cell : acquireCell();

	uint16_t& dst = cell->data[coy * _cellW + cox];
	alpha = ((uint32_t)alpha * (uint32_t)_opacity) >> 8;
	if (alpha == 0xFF)
	{
		dst = color;
	}
	else
	{
		dst = blend(color, dst, alpha);		
	}
}

void DeltaBitmap::writePixel(int16_t x, int16_t y, uint16_t color)
{
	writePixel(x, y, color, _opacity);
}

void DeltaBitmap::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (x > _clipX2 || y > _clipY2)
	{
		return;
	}
    int16_t x2 = x + w - 1;
	int16_t y2 = y + h - 1;
    if (x2 < _clipX || y2 < _clipY)
	{
		return;
	}

    // Clip left/top
    if (x < _clipX) 
	{
        x = _clipX;
        w = x2 - x + 1;
    }
    if (y < _clipY) 
	{
        y = _clipY;
        h = y2 - y + 1;
    }

    // Clip right/bottom
    if (x2 > _clipX2)
	{
		w = _clipX2 - x + 1;
	}
    if (y2 >= _clipY2) 
	{
		h = _clipY2 - y + 1;
	}

	if (w < _cellW || h < _cellH) //rect too small, go for the slow routine
	{
		_fillRect(x, y, w, h, color);
	}
	else
	{
		//these are the areas. 0, 1, 2, 3 are partial cells, 4 are full cells
		//  000000000000
		//  144444444442
		//  144444444442
		//  133333333332

		//Area 0
		if ((y & _cellHMask) != 0)
		{
			int16_t hh = _cellH - (y & _cellHMask);
			_fillRect(x, y, w, hh, color);
			y += hh;
			h -= hh;
		}
		//Area 1
		if ((x & _cellWMask) != 0)
		{
			int16_t ww = _cellW - (x & _cellWMask);
			_fillRect(x, y, ww, h, color);
			x += ww;
			w -= ww;
		}
		//Area 2
		if (((x + w) & _cellWMask) != 0)
		{
			int16_t ww = (x + w) & _cellWMask;
			_fillRect(x + w - ww, y, ww, h, color);
			w -= ww;
		}
		//Area 3
		if (((y + h) & _cellHMask) != 0)
		{
			int16_t hh = (y + h) & _cellHMask;
			_fillRect(x, y + h - hh, w, hh, color);
			h -= hh;
		}

		//Area 4 - full cells only
		uint16_t cellDataSize = _cellW * _cellH;
		int16_t cx = x >> _cellWBits;
		int16_t cy = y >> _cellHBits;
		int16_t cw = w >> _cellWBits;
		int16_t ch = h >> _cellHBits;
		for (int16_t j = 0; j < ch; j++)
		{
			uint16_t cellIndex = (cy + j) * _cellCountX;
			for (int16_t i = 0; i < cw; i++)
			{
				Cell*& cell = _cells[cellIndex + cx + i];
				cell = cell ? cell : acquireCell();
				fillCell(*cell, color, _opacity);
			}
		}
	}
}
void DeltaBitmap::_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	uint8_t a32 = encodeAlpha(_opacity);
	uint32_t c32 = encodeColor(color);

	for (int16_t j = 0; j < h; j++)
	{
		int16_t yy = y + j;
		int16_t cy = yy >> _cellHBits; //cell Y
		int16_t coy = yy & _cellHMask; //cell Offset Y
		uint16_t cellIndex = cy * _cellCountX;
		uint16_t cp = coy * _cellW;

		if (_opacity == 0xFF)
		{
			for (int16_t i = 0; i < w; i++)
			{
				int16_t xx = x + i;
				int16_t cx = xx >> _cellWBits; //cell X
				int16_t cox = xx & _cellWMask; //cell Offset X
				Cell*& cell = _cells[cellIndex + cx];
				cell = cell ? cell : acquireCell();
				cell->data[cp + cox] = color;
			}
		}
		else
		{
			for (int16_t i = 0; i < w; i++)
			{
				int16_t xx = x + i;
				int16_t cx = xx >> _cellWBits; //cell X
				int16_t cox = xx & _cellWMask; //cell Offset X
				Cell*& cell = _cells[cellIndex + cx];
				cell = cell ? cell : acquireCell();
				uint16_t& dst = cell->data[cp + cox];
				dst = blendRaw(c32, dst, a32);		
			}
		}
	}
}

void DeltaBitmap::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	fillRect(x, y, 1, h, color);
}
void DeltaBitmap::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	fillRect(x, y, w, 1, color);
}

void DeltaBitmap::fillScreen(uint16_t color)
{
	fillCell(*_clearCell, color, 0xFF);
	_clearCell->hash = computeCellHash(*_clearCell);
	_clearColor = color;

	for (Cell*& cell: _cells)
	{
		if (cell)
		{
			releaseCell(cell);
		}
		cell = nullptr;
	}
}

void DeltaBitmap::blit(Adafruit_SPITFT& dst)
{
	dst.startWrite();

	uint16_t cellDataSize = _cellW * _cellH;
	uint16_t cellIndex = 0;
	for (uint16_t j = 0; j < _cellCountY; j++)
	{
		uint16_t y = j * _cellH;
		for (uint16_t i = 0; i < _cellCountX; i++)
		{
			CellHash& prevCellHash = _prevCellHashes[cellIndex];
			CellHash cellHash;

			uint16_t x = i * _cellW;
			Cell* cell = _cells[cellIndex];
			if (!cell)
			{
				cell = _clearCell;
				cellHash = _clearCell->hash;
			}
			else
			{
				cellHash = computeCellHash(*cell);
			}

			if (cellHash != prevCellHash)
			{
				dst.setAddrWindow(x, y, _cellW, _cellH); // Clipped area
				dst.writePixels(cell->data, cellDataSize);
				prevCellHash = cellHash;
			}
			cellIndex++;
		}
	}
	dst.endWrite();
}

DeltaBitmap::Cell::Cell(size_t size)
	: data(new uint16_t[size])
{
}
DeltaBitmap::Cell::~Cell()
{
	delete data;
}

DeltaBitmap::Cell* DeltaBitmap::acquireCell()
{
	Cell* cell;
	if (_cellPool.empty())
	{
		size_t cellDataSize = _cellW * _cellH;
		cell = new Cell(cellDataSize);
	}
	else
	{
		cell = _cellPool.back();
		_cellPool.erase(_cellPool.end() - 1);
	}
	fillCell(*cell, _clearColor, 0xFF);
	return cell;
}
void DeltaBitmap::releaseCell(Cell* cell)
{
	_cellPool.push_back(cell);
}
void DeltaBitmap::fillCell(Cell& cell, uint16_t color, uint8_t opacity)
{
	size_t cellDataSize = _cellW * _cellH;
	if (opacity != 0xFF)
	{
		uint8_t a32 = encodeAlpha(opacity);
		uint32_t c32 = encodeColor(color);
		uint16_t* ptr = cell.data;
		size_t sz = cellDataSize;
		while (sz-- > 0)
		{
			*ptr = blendRaw(c32, *ptr, a32);
			ptr++;
		}
	}
	else if (color != 0)
	{
		//initialize the cell with the clear color
		uint16_t* ptr = cell.data;
		size_t sz = cellDataSize;
		while (sz-- > 0)
		{
			*ptr++ = color;
		}
	}
	else
	{
		memset(cell.data, 0, cellDataSize * sizeof(uint16_t));		
	}
}

uint32_t murmur32(const void* _buffer, size_t size, uint32_t seed)
{
	const uint8_t* buffer = reinterpret_cast<const uint8_t*>(_buffer);

	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const uint32_t m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value

	uint32_t h = seed ^ static_cast<uint32_t>(size);

	// Mix 4 bytes at a time into the hash

	while (size >= 4)
	{
		uint32_t k = *(uint32_t*)buffer;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		buffer += 4;
		size -= 4;
	}

	// Handle the last few bytes of the input array

	switch (size)
	{
	case 3: h ^= (buffer[2]) << 16;
	case 2: h ^= (buffer[1]) << 8;
	case 1: h ^= (buffer[0]);
		h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

DeltaBitmap::CellHash DeltaBitmap::computeCellHash(Cell& cell) const
{
	CellHash hash = 0;
	size_t cellDataSize = _cellW * _cellH;
	//hash = crc16_le(0, (const uint8_t*)cell.data, cellDataSize * sizeof(uint16_t));
	hash = murmur32(cell.data, cellDataSize * sizeof(uint16_t), 0);
//	for (size_t i = 0; i < cellDataSize; i++)
//	{
//		hash ^= cell.data[i];
//	}
	return hash;
}

