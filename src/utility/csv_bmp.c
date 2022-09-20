#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

int gray_raw2bmp (uint8_t *pRawData, uint32_t nWidth, uint32_t nHeight, char *pBmpName)
{
	struct bitmap_file_header_t file_h;
	struct bitmap_info_header_t info_h;
	uint32_t dwBmpSize = 0,dwRawSize = 0, dwLine = 0;
	int lCount = 0, i = 0;
	FILE *fp = NULL;
	struct rgbquad_t rgbPal[256];

    fp = fopen(pBmpName, "wb");
	if (fp == NULL) {
		log_err("ERROR : open '%s'.", pBmpName);

		return -1;
	}
 
    file_h.bfType = 0x4D42;
    file_h.bfReserved1 = 0;
    file_h.bfReserved2 = 0;
    file_h.bfOffBits = sizeof(rgbPal) + sizeof(file_h) + sizeof(info_h);
    info_h.biSize = sizeof(struct bitmap_info_header_t);
    info_h.biWidth = nWidth;
    info_h.biHeight = nHeight;
    info_h.biPlanes = 1;
    info_h.biBitCount = 8;
    info_h.biCompression = 0;
    info_h.biXPelsPerMeter = 0;
    info_h.biYPelsPerMeter = 0;
    info_h.biClrUsed = 0;
    info_h.biClrImportant = 0;

    dwLine = ((((info_h.biWidth * info_h.biBitCount) + 31) & ~31) >> 3);
    dwBmpSize = dwLine * info_h.biHeight;
    info_h.biSizeImage = dwBmpSize;
    file_h.bfSize = dwBmpSize + file_h.bfOffBits + 2;

    dwRawSize = info_h.biWidth * info_h.biHeight;

    if (pRawData) {
        for (i = 0; i < 256; i++) {
            rgbPal[i].rgbRed = (uint8_t)(i % 256);
            rgbPal[i].rgbGreen = rgbPal[i].rgbRed;
            rgbPal[i].rgbBlue = rgbPal[i].rgbRed;
            rgbPal[i].rgbReserved = 0;
        }

        fwrite((char*)&file_h, 1, sizeof(file_h), fp);
        fwrite((char*)&info_h, 1, sizeof(info_h), fp);
        fwrite((char*)rgbPal, 1, sizeof(rgbPal), fp);
        lCount = dwRawSize;

		// 上下颠倒
        for (lCount -= (long)info_h.biWidth; lCount >= 0; lCount -= (long)info_h.biWidth) {
			fwrite((pRawData + lCount), 1, (long)dwLine, fp);
        }
    }

    fclose(fp);

    return 0;
}


#ifdef __cplusplus
}
#endif


