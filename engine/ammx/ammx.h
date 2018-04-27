#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// result is destination
uint8_t* pmul88_3byte(uint8_t* destination, uint8_t* op1, uint8_t* op2);
int fill_flat_texel(
    uint8_t* p, short d, int u1, int v1, int w1, int au, int av, int aw,
    uint32_t texMaskU, uint32_t texMaskV, uint32_t texSizeU, uint32_t* texPixels, uint8_t* c
);
void set_ammx_pixels(void* data, size_t bytes, uint32_t color);

#ifdef __cplusplus
}
#endif