#ifndef __CSV_BMP_H__
#define __CSV_BMP_H__


#ifdef __cplusplus
extern "C" {
#endif


struct bitmap_file_header_t {
	uint16_t		bfType;
	uint32_t		bfSize;
	uint16_t		bfReserved1;
	uint16_t		bfReserved2;
	uint32_t		bfOffBits;
};

struct bitmap_info_header_t {
	uint32_t		biSize;
	uint32_t		biWidth;
	uint32_t 		biHeight;
	uint16_t		biPlanes;
	uint16_t		biBitCount;
	uint32_t		biCompression;
	uint32_t		biSizeImage;
	uint32_t		biXPelsPerMeter;
	uint32_t		biYPelsPerMeter;
	uint32_t		biClrUsed;
	uint32_t		biClrImportant;
};

struct rgbquad_t {
	uint8_t			rgbBlue;
	uint8_t			rgbGreen;
	uint8_t			rgbRed;
	uint8_t			rgbReserved;
};

extern int gray_raw2bmp (uint8_t *pRawData, uint32_t nWidth, uint32_t nHeight, char *pBmpName);

#ifdef __cplusplus
}
#endif

#endif /* __CSV_BMP_H__ */


