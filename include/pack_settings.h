uint8_t getBrigtness(void);

uint16_t getScreenChangeSeconds_x5(void);

void packSettings(uint8_t brigtnness, uint16_t screenChangeSeconds_x5, uint8_t& data0, uint8_t& data1);