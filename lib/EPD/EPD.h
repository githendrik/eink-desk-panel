#ifndef _EPD_H_
#define _EPD_H_

#include "EPD_SPI.h"

// Panel-specific resolution and constants
#ifdef PANEL_579
  // 5.79" Dual-SSD1683: 792x272 visible, 800x272 virtual (8-pixel gap at IC junction)
  #define EPD_W 800
  #define EPD_W_VISIBLE 792
  #define EPD_H 272
  #define SOURCE_BYTES  50    // 400/8 = 50 bytes per IC row
  #define GATE_BITS     272
  #define IC_BYTES      (SOURCE_BYTES * GATE_BITS)  // 13600 bytes per IC
  #define EPD_BUF_SIZE  27200 // 100 * 272
#else
  // 4.2" Single-SSD1683: 400x300
  #define EPD_W 400
  #define EPD_H 300
  #define EPD_BUF_SIZE  15000 // 50 * 300
#endif

#define Fast_Seconds_1_5s 0
#define Fast_Seconds_1_s  1

void EPD_ReadBusy(void);
void EPD_RESET(void);
void EPD_Sleep(void);

void EPD_Update(void);
void EPD_Update_Fast(void);
void EPD_Update_Part(void);

void EPD_Display(const uint8_t *Image);
void EPD_Display_Fast(const uint8_t *Image);
void EPD_Display_Part(uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, const uint8_t *Image);

void EPD_Init(void);
void EPD_Init_Fast(uint8_t mode);
void EPD_Init_Part(void);
void EPD_Clear_R26A6H(void);
void EPD_Clear(void);

#ifdef PANEL_579
  // Dual-IC RAM setup functions (5.79" only)
  void EPD_SetRAMMP(void);
  void EPD_SetRAMMA(void);
  void EPD_SetRAMSP(void);
  void EPD_SetRAMSA(void);
#else
  // Single-IC address functions (4.2" only)
  void EPD_Address_Set(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye);
  void EPD_SetCursor(uint16_t xs, uint16_t ys);
  void EPD_Update_4Gray(void);
#endif

#endif
