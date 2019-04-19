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

void DeltaBitmap::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) 
	{
		return;
	}

	int16_t t;
	switch(rotation) {
	case 1:
		t = x;
		x = _width  - 1 - y;
		y = t;
	break;
	case 2:
		x = _width  - 1 - x;
		y = _height - 1 - y;
	break;
	case 3:
		t = x;
		x = y;
		y = _height - 1 - t;
	break;
	}

	int16_t cx = x >> _cellWBits;
	int16_t cy = y >> _cellHBits;
	int16_t cox = x & _cellWMask;
	int16_t coy = y & _cellHMask;

	uint16_t cellIndex = cy * _cellCountX + cx;
	Cell*& cell = _cells[cellIndex];
	if (!cell || cell == _clearCell)
	{
		cell = acquireCell();
	}
	cell->data[coy * _cellW + cox] = color;
}

void DeltaBitmap::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	int16_t t;
	switch(rotation) {
	case 1:
		t = x;
		x = _width  - 1 - y;
		y = t;
		t = w;
		w = h;
		h = t;
	break;
	case 2:
		x = _width  - 1 - x;
		y = _height - 1 - y;
	break;
	case 3:
		t = x;
		x = y;
		y = _height - 1 - t;
		t = w;
		w = h;
		h = t;
	break;
	}	

	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (y >= _height)
	{
		return;
	}
	if (y + h > _height)
	{
		h = _height - y;
	}
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (x >= _width)
	{
		return;
	}
	if (x + w > _width)
	{
		w = _width - x;
	}

	for (int16_t j = 0; j < h; j++)
	{
		int16_t yy = y + j;
		int16_t cy = yy >> _cellHBits;
		int16_t coy = yy & _cellHMask;
		uint16_t cellIndex = cy * _cellCountX;
		uint16_t cp = coy * _cellW;

		for (int16_t i = 0; i < w; i++)
		{
			int16_t xx = x + i;
			int16_t cx = xx >> _cellWBits;
			int16_t cox = xx & _cellWMask;

			Cell*& cell = _cells[cellIndex + cx];
			if (!cell || cell == _clearCell)
			{
				cell = acquireCell();
			}
			cell->data[cp + cox] = color;
		}
	}
}

void DeltaBitmap::fillScreen(uint16_t color)
{
	uint16_t cellDataSize = _cellW * _cellH;
	uint16_t* data = _clearCell->data;
	for (uint16_t i = 0; i < cellDataSize; i++)
	{
		data[i] = color;
	}
	_clearCell->hash = computeCellHash(*_clearCell);

	for (Cell*& cell: _cells)
	{
		if (cell && cell != _clearCell)
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
	size_t cellDataSize = _cellW * _cellH;
	if (_cellPool.empty())
	{
		cell = new Cell(cellDataSize);
	}
	else
	{
		cell = _cellPool.back();
		_cellPool.erase(_cellPool.end() - 1);
	}
	if (_clearCell != nullptr)
	{
		memcpy(cell->data, _clearCell->data, cellDataSize * sizeof(uint16_t));
	}
	else
	{
		memset(cell->data, 0, cellDataSize * sizeof(uint16_t));		
	}
	return cell;
}
void DeltaBitmap::releaseCell(Cell* cell)
{
	_cellPool.push_back(cell);
}

uint32_t murmur32(const void* _buffer, size_t size, uint32_t seed)
{
	const uint8_t* buffer = reinterpret_cast<const uint8_t*>(_buffer);

	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const uint32_t m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value

	uint16_t h = seed ^ static_cast<uint32_t>(size);

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

