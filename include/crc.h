#ifndef __CRC_H__
#define __CRC_H__

#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t crc16_ccitt_tab (const uint8_t *buf, uint32_t len);

extern uint16_t crc16_ccitt_calc (const uint8_t *data, uint32_t len);


#ifdef __cplusplus
}
#endif


#endif /* __CRC_H__ */

