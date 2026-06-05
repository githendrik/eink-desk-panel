#ifndef _EPD_H_
#define _EPD_H_

#include "EPD_SPI.h"

// 5.79" Dual-SSD1683 Panel: 792x272 visible, 800x272 virtual (8-pixel gap at IC junction)
// Each IC drives 396 columns x 272 rows (50 bytes x 272 lines = 13600 bytes per IC)
#define EPD_W 800          // Virtual width (includes 8-pixel gap)
#define EPD_W_VISIBLE 792  // Actual visible width
#define EPD_H 272          // Height

#define SOURCE_BYTES  50   // 400/8 = 50 bytes per IC row
#define GATE_BITS     272  // Number of gate lines
#define IC_BYTES      (SOURCE_BYTES * GATE_BITS)  // 13600 bytes per IC

#define Fast_Seconds_1_5s 0
#define Fast_Seconds_1_s  1

void EPD_ReadBusy(void);
void EPD_RESET(void);
void EPD_Sleep(void);

void EPD_Update(void);
void EPD_Update_Fast(void);
void EPD_Update_Part(void);

// Master IC RAM setup
void EPD_SetRAMMP(void);   // Master RAM Position (address range)
void EPD_SetRAMMA(void);   // Master RAM Address counter

// Slave IC RAM setup (register + 0x80 offset, cascade mode)
void EPD_SetRAMSP(void);   // Slave RAM Position
void EPD_SetRAMSA(void);   // Slave RAM Address counter

void EPD_Display(const uint8_t *Image);
void EPD_Display_Fast(const uint8_t *Image);
void EPD_Display_Part(uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, const uint8_t *Image);

void EPD_Init(void);
void EPD_Init_Fast(uint8_t mode);
void EPD_Init_Part(void);
void EPD_Clear_R26A6H(void);

void EPD_Clear(void);
#endif
