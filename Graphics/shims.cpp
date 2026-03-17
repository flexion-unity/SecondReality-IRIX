#define _CRT_SECURE_NO_WARNINGS

#include "shims.h"

#include "Parts/Common.h"

namespace Shim
{
    unsigned char vram[VRAM_X * VRAM_Y] = { 0 };
    unsigned int palette[PaletteColorCount];
    unsigned int startpixel = 0;

    namespace
    {
        int paletteReadIndex = 0;
        int paletteReadComponent = 0;
        int paletteIndex = 0;
        int paletteComponent = 0;

        bool isFirstPart = 1;
    }

    void clearScreen()
    {
        CLEAR(vram);
    }

    void setpal(int idx, unsigned char r, unsigned char g, unsigned char b)
    {
        
        // Write bytes individually so the layout [B,G,R,0] at offsets [0,1,2,3]
        // is the same on both little-endian and big-endian systems.
        unsigned char * pal8 = reinterpret_cast<unsigned char *>(palette);
#ifdef __sgi
        // GL_RGBA on big-endian: byte[0]=R, byte[1]=G, byte[2]=B, byte[3]=A
        pal8[idx * 4 + 0] = static_cast<unsigned char>(r << 2);
        pal8[idx * 4 + 1] = static_cast<unsigned char>(g << 2);
        pal8[idx * 4 + 2] = static_cast<unsigned char>(b << 2);
        pal8[idx * 4 + 3] = 0xFF;
#else
        // GL_RGBA on little-endian: byte[0]=R, byte[1]=G, byte[2]=B, byte[3]=A
        pal8[idx * 4 + 0] = static_cast<unsigned char>(b << 2);
        pal8[idx * 4 + 1] = static_cast<unsigned char>(g << 2);
        pal8[idx * 4 + 2] = static_cast<unsigned char>(r << 2);
        pal8[idx * 4 + 3] = 0xFF;
#endif
    }

    void outp(int reg, unsigned int value)
    {
        switch (reg)
        {
            case 0x3c7: {
                paletteReadIndex = value;
            }
            break;
            case 0x3c8: {
                paletteIndex = value & 0xFF;
                paletteComponent = 0;
            }
            break;
            case 0x3c9: {
                unsigned char * pal8 = (unsigned char *)palette;
#ifdef __sgi
                // GL_RGBA fixed-function (no shader): bytes must be [R,G,B,A]
                pal8[paletteIndex * 4 + paletteComponent] = static_cast<unsigned char>(value << 2);
#else
                // GLES path: store [B,G,R,A]; shader swizzles .bgr on sample
                pal8[paletteIndex * 4 + (2 - paletteComponent)] = static_cast<unsigned char>(value << 2);
#endif
                paletteComponent++;
                if (paletteComponent == 3)
                {
                    paletteIndex++;
                    paletteComponent = 0;
                }
                if (paletteIndex >= PaletteColorCount)
                {
                    paletteIndex = 0;
                }
            }
            break;
        }
    }

    unsigned char inp(int reg)
    {
        unsigned char result = 0;

        switch (reg)
        {
            case 0x3c9: {
                unsigned char * pal8 = (unsigned char *)palette;
#ifdef __sgi
                result = pal8[paletteReadIndex * 4 + paletteReadComponent] >> 2;
#else
                result = pal8[paletteReadIndex * 4 + (2 - paletteReadComponent)] >> 2;
#endif

                paletteReadComponent++;

                if (paletteReadComponent == 3)
                {
                    paletteReadIndex++;
                    paletteReadComponent = 0;
                }

                if (paletteReadIndex > PaletteColorCount)
                {
                    paletteReadIndex = 0;
                }
            }
            break;
        }

        return result;
    }

    void setstartpixel(int reg)
    {
        startpixel = reg;
    }

    bool isDemoFirstPart()
    {
        return isFirstPart;
    }

    void finishedDemoFirstPart()
    {
        isFirstPart = false;
    }
}
