#define _32TO16_RGB_555(p) (UINT16)((((p) & 0x00F80000) >> 9) | \
                                    (((p) & 0x0000F800) >> 6) | \
                                    (((p) & 0x000000F8) >> 3))

#define _32TO16_RGB_565(p) (UINT16)((((p) & 0x00F80000) >> 8) | \
                                    (((p) & 0x0000FC00) >> 5) | \
                                    (((p) & 0x000000F8) >> 3))
