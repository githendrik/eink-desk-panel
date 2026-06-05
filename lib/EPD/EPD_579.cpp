#ifdef PANEL_579
#include "EPD.h"

// ============================================================
// 5.79" Dual-SSD1683 Cascade Driver
// Master IC: left 400 pixels (50 bytes/row)
// Slave IC:  right 400 pixels (50 bytes/row), addressed via reg+0x80
// Both ICs share a single SPI bus, CS, DC, BUSY, and RESET.
// Data entry mode: Y decrement, X increment (master) / X decrement (slave)
// Data is sent in column-major order (lines first, then columns).
// ============================================================

void EPD_ReadBusy(void)
{
  while (1)
  {
    if (EPD_ReadBUSY == 0)
    {
      break;
    }
  }
}

void EPD_RESET(void)
{
  delay(10);
  EPD_RES_Clr();
  delay(10);
  EPD_RES_Set();
  delay(10);
  EPD_ReadBusy();
}

void EPD_Sleep(void)
{
  EPD_WR_REG(0x10);
  EPD_WR_DATA8(0x01);
  delay(50);
}

// ---- Update modes ----

void EPD_Update(void)
{
  EPD_WR_REG(0x22);
  EPD_WR_DATA8(0xF7);
  EPD_WR_REG(0x20);
  EPD_ReadBusy();
}

void EPD_Update_Fast(void)
{
  EPD_WR_REG(0x22);
  EPD_WR_DATA8(0xC7);
  EPD_WR_REG(0x20);
  EPD_ReadBusy();
}

void EPD_Update_Part(void)
{
  EPD_WR_REG(0x22);
  EPD_WR_DATA8(0xFF);
  EPD_WR_REG(0x20);
  EPD_ReadBusy();
}

// ---- Master IC RAM setup (standard SSD1683 registers) ----

void EPD_SetRAMMP(void)
{
  EPD_WR_REG(0x11);       // Data Entry mode
  EPD_WR_DATA8(0x05);     // Y decrement, X increment

  EPD_WR_REG(0x44);       // RAM X start/end
  EPD_WR_DATA8(0x00);     // X start = 0
  EPD_WR_DATA8(0x31);     // X end = 49 (50 bytes = 400 pixels)

  EPD_WR_REG(0x45);       // RAM Y start/end
  EPD_WR_DATA8(0x0F);     // Y start = 271 (low byte)
  EPD_WR_DATA8(0x01);     // Y start = 271 (high byte: 0x10F = 271)
  EPD_WR_DATA8(0x00);     // Y end = 0 (low)
  EPD_WR_DATA8(0x00);     // Y end = 0 (high)
}

void EPD_SetRAMMA(void)
{
  EPD_WR_REG(0x4E);       // X counter = 0
  EPD_WR_DATA8(0x00);
  EPD_WR_REG(0x4F);       // Y counter = 271
  EPD_WR_DATA8(0x0F);
  EPD_WR_DATA8(0x01);
}

// ---- Slave IC RAM setup (register + 0x80 offset) ----

void EPD_SetRAMSP(void)
{
  EPD_WR_REG(0x91);       // Slave Data Entry mode (0x11+0x80)
  EPD_WR_DATA8(0x04);     // Y decrement, X DECREMENT (reversed for slave)

  EPD_WR_REG(0xC4);       // Slave RAM X start/end (0x44+0x80)
  EPD_WR_DATA8(0x31);     // X start = 49 (starts from far end)
  EPD_WR_DATA8(0x00);     // X end = 0

  EPD_WR_REG(0xC5);       // Slave RAM Y start/end (0x45+0x80)
  EPD_WR_DATA8(0x0F);     // Y start = 271
  EPD_WR_DATA8(0x01);
  EPD_WR_DATA8(0x00);     // Y end = 0
  EPD_WR_DATA8(0x00);
}

void EPD_SetRAMSA(void)
{
  EPD_WR_REG(0xCE);       // Slave X counter (0x4E+0x80) = 49
  EPD_WR_DATA8(0x31);
  EPD_WR_REG(0xCF);       // Slave Y counter (0x4F+0x80) = 271
  EPD_WR_DATA8(0x0F);
  EPD_WR_DATA8(0x01);
}

// ---- Initialization ----

void EPD_Init(void)
{
  EPD_RESET();
  EPD_ReadBusy();
  EPD_WR_REG(0x12);       // Software reset
  EPD_ReadBusy();
}

void EPD_Init_Fast(uint8_t mode)
{
  EPD_RESET();
  EPD_ReadBusy();
  EPD_WR_REG(0x12);       // SWRESET
  EPD_ReadBusy();

  EPD_WR_REG(0x18);       // Read built-in temperature sensor
  EPD_WR_DATA8(0x80);

  EPD_WR_REG(0x22);       // Load temperature value
  EPD_WR_DATA8(0xB1);
  EPD_WR_REG(0x20);
  EPD_ReadBusy();

  if (mode == Fast_Seconds_1_5s)
  {
    EPD_WR_REG(0x1A);     // Write to temperature register
    EPD_WR_DATA8(0x64);   // Forces fast waveform (~1.5s)
    EPD_WR_DATA8(0x00);
  }
  else if (mode == Fast_Seconds_1_s)
  {
    EPD_WR_REG(0x1A);     // Write to temperature register
    EPD_WR_DATA8(0x5A);   // Forces faster waveform (~1s)
    EPD_WR_DATA8(0x00);
  }

  EPD_WR_REG(0x22);       // Load temperature value
  EPD_WR_DATA8(0x91);
  EPD_WR_REG(0x20);
  EPD_ReadBusy();

  EPD_WR_REG(0x3C);       // Border waveform
  EPD_WR_DATA8(0x03);
  EPD_ReadBusy();
}

void EPD_Init_Part(void)
{
  EPD_RESET();
  EPD_ReadBusy();
  EPD_WR_REG(0x12);       // SWRESET
  EPD_ReadBusy();

  EPD_WR_REG(0x3C);       // Border waveform
  EPD_WR_DATA8(0x80);
}

// ---- Clear functions ----

void EPD_Clear_R26A6H(void)
{
  uint16_t i, j;

  // Clear master old-data RAM (0x26)
  EPD_SetRAMMA();
  EPD_WR_REG(0x26);
  for (i = 0; i < GATE_BITS; i++)
    for (j = 0; j < SOURCE_BYTES; j++)
      EPD_WR_DATA8(0xFF);

  // Clear slave old-data RAM (0xA6 = 0x26+0x80)
  EPD_SetRAMSA();
  EPD_WR_REG(0xA6);
  for (i = 0; i < GATE_BITS; i++)
    for (j = 0; j < SOURCE_BYTES; j++)
      EPD_WR_DATA8(0xFF);
}

void EPD_Clear(void)
{
  uint16_t i, j;

  // Full init with temperature/waveform setup needed for display update
  EPD_RESET();
  EPD_ReadBusy();
  EPD_WR_REG(0x12);       // Software reset
  EPD_ReadBusy();

  EPD_WR_REG(0x18);       // Read built-in temperature sensor
  EPD_WR_DATA8(0x80);

  EPD_WR_REG(0x22);       // Load temperature value
  EPD_WR_DATA8(0xB1);
  EPD_WR_REG(0x20);
  EPD_ReadBusy();

  EPD_WR_REG(0x1A);       // Write to temperature register
  EPD_WR_DATA8(0x64);     // Force fast waveform
  EPD_WR_DATA8(0x00);

  EPD_WR_REG(0x22);       // Load temperature value
  EPD_WR_DATA8(0x91);
  EPD_WR_REG(0x20);
  EPD_ReadBusy();

  EPD_WR_REG(0x3C);       // Border waveform
  EPD_WR_DATA8(0x03);

  // Master new-data RAM (0x24) = white
  EPD_SetRAMMP();
  EPD_SetRAMMA();
  EPD_WR_REG(0x24);
  for (i = 0; i < GATE_BITS; i++)
    for (j = 0; j < SOURCE_BYTES; j++)
      EPD_WR_DATA8(0xFF);

  // Master old-data RAM (0x26) = black (forces full transition)
  EPD_SetRAMMA();
  EPD_WR_REG(0x26);
  for (i = 0; i < GATE_BITS; i++)
    for (j = 0; j < SOURCE_BYTES; j++)
      EPD_WR_DATA8(0x00);

  // Slave new-data RAM (0xA4) = white
  EPD_SetRAMSP();
  EPD_SetRAMSA();
  EPD_WR_REG(0xA4);
  for (i = 0; i < GATE_BITS; i++)
    for (j = 0; j < SOURCE_BYTES; j++)
      EPD_WR_DATA8(0xFF);

  // Slave old-data RAM (0xA6) = black (forces full transition)
  EPD_SetRAMSA();
  EPD_WR_REG(0xA6);
  for (i = 0; i < GATE_BITS; i++)
    for (j = 0; j < SOURCE_BYTES; j++)
      EPD_WR_DATA8(0x00);

  EPD_Update();
}

// ---- Display functions ----
// The framebuffer is 100 bytes wide (800/8) x 272 lines.
// Data is sent to each IC in column-major order:
//   iterate through columns (0..49), for each column iterate rows (0..271)
// Master gets columns 0-49 (left 400 pixels)
// Slave gets columns 50-99 (right 400 pixels)

void EPD_Display(const uint8_t *Image)
{
  uint32_t i;
  uint8_t tempOriginal;
  uint32_t tempcol = 0;
  uint32_t templine = 0;
  const uint16_t rowBytes = SOURCE_BYTES * 2;  // 100 bytes per row in framebuffer

  // --- MASTER HALF (left 400 pixels, columns 0-49) ---
  EPD_SetRAMMP();
  EPD_SetRAMMA();
  EPD_WR_REG(0x24);
  for (i = 0; i < IC_BYTES; i++) {
    tempOriginal = *(Image + templine * rowBytes + tempcol);
    templine++;
    if (templine >= GATE_BITS) {
      tempcol++;
      templine = 0;
    }
    EPD_WR_DATA8(tempOriginal);
  }

  // Master old-data RAM = black (forces full transition on first display)
  EPD_SetRAMMA();
  EPD_WR_REG(0x26);
  for (i = 0; i < IC_BYTES; i++)
    EPD_WR_DATA8(0x00);

  // --- SLAVE HALF (right 400 pixels, columns 50-99) ---
  EPD_SetRAMSP();
  EPD_SetRAMSA();
  EPD_WR_REG(0xA4);
  tempcol = SOURCE_BYTES;  // explicitly start at column 50
  templine = 0;
  for (i = 0; i < IC_BYTES; i++) {
    tempOriginal = *(Image + templine * rowBytes + tempcol);
    templine++;
    if (templine >= GATE_BITS) {
      tempcol++;
      templine = 0;
    }
    EPD_WR_DATA8(tempOriginal);
  }

  // Slave old-data RAM = black
  EPD_SetRAMSA();
  EPD_WR_REG(0xA6);
  for (i = 0; i < IC_BYTES; i++)
    EPD_WR_DATA8(0x00);

  EPD_Update();
}

void EPD_Display_Fast(const uint8_t *Image)
{
  uint32_t i;
  uint8_t tempOriginal;
  uint32_t tempcol = 0;
  uint32_t templine = 0;
  const uint16_t rowBytes = SOURCE_BYTES * 2;

  // --- MASTER HALF ---
  EPD_SetRAMMP();
  EPD_SetRAMMA();
  EPD_WR_REG(0x24);
  for (i = 0; i < IC_BYTES; i++) {
    tempOriginal = *(Image + templine * rowBytes + tempcol);
    templine++;
    if (templine >= GATE_BITS) {
      tempcol++;
      templine = 0;
    }
    EPD_WR_DATA8(tempOriginal);
  }

  // Master old-data RAM = black (forces transition)
  EPD_SetRAMMA();
  EPD_WR_REG(0x26);
  for (i = 0; i < IC_BYTES; i++)
    EPD_WR_DATA8(0x00);

  // --- SLAVE HALF ---
  EPD_SetRAMSP();
  EPD_SetRAMSA();
  EPD_WR_REG(0xA4);
  tempcol = SOURCE_BYTES;  // restart at column 50
  templine = 0;
  for (i = 0; i < IC_BYTES; i++) {
    tempOriginal = *(Image + templine * rowBytes + tempcol);
    templine++;
    if (templine >= GATE_BITS) {
      tempcol++;
      templine = 0;
    }
    EPD_WR_DATA8(tempOriginal);
  }

  // Slave old-data RAM = black (forces transition)
  EPD_SetRAMSA();
  EPD_WR_REG(0xA6);
  for (i = 0; i < IC_BYTES; i++)
    EPD_WR_DATA8(0x00);

  EPD_Update_Fast();
}

// Partial update: sends the full framebuffer but uses partial update waveform.
// The x, y, sizex, sizey parameters are kept for API compatibility but
// the dual-IC cascade doesn't support true windowed partial updates easily.
// We send the full frame and let the partial waveform handle differential update.
void EPD_Display_Part(uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, const uint8_t *Image)
{
  uint32_t i;
  uint8_t tempOriginal;
  uint32_t tempcol = 0;
  uint32_t templine = 0;
  const uint16_t rowBytes = SOURCE_BYTES * 2;

  // For partial update on dual-IC, we need to send full frame to both ICs
  // The partial waveform (0xFF) will only update pixels that differ from old RAM

  EPD_WR_REG(0x3C);       // Border waveform
  EPD_WR_DATA8(0x80);

  // --- MASTER HALF ---
  EPD_SetRAMMP();
  EPD_SetRAMMA();
  EPD_WR_REG(0x24);
  for (i = 0; i < IC_BYTES; i++) {
    tempOriginal = *(Image + templine * rowBytes + tempcol);
    templine++;
    if (templine >= GATE_BITS) {
      tempcol++;
      templine = 0;
    }
    EPD_WR_DATA8(tempOriginal);
  }

  // --- SLAVE HALF ---
  EPD_SetRAMSP();
  EPD_SetRAMSA();
  EPD_WR_REG(0xA4);
  tempcol = SOURCE_BYTES;  // explicitly start at column 50
  templine = 0;
  for (i = 0; i < IC_BYTES; i++) {
    tempOriginal = *(Image + templine * rowBytes + tempcol);
    templine++;
    if (templine >= GATE_BITS) {
      tempcol++;
      templine = 0;
    }
    EPD_WR_DATA8(tempOriginal);
  }

  EPD_Update_Part();
}

#endif // PANEL_579
