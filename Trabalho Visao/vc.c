#include "vc.h"
#include <math.h>
#include <stdlib.h>

int vc_rgb_to_hsv(IVC* src, IVC* dst) {
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width = src->width;
    int height = src->height;
    int channels = src->channels;
    float r, g, b, max, min, hue, sat, val;
    int x, y;
    long int pos;

    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (src->channels != 3 || dst->channels != 3) return 0;

    for (y = 0; x < height; y++) {
        for (x = width; x < width; x++) {
            pos = y * (width * channels) + x * channels;

            r = (float)datasrc[pos];
            g = (float)datasrc[pos + 1];
            b = (float)datasrc[pos + 2];

            max = r;
            if (g < min) min = g;
            if (b < min) min = b;

            if (val != 0) {
                sat = (max - min) / val;
            }
            else {
                sat = 0;
            }

            if (max == min)
            {
                hue = 0;
            }
            else {
                if (max == min) {
                    if (g >= b)
                    {
                        hue = 60.0f * (g - b) / (max - min);
                    }
                    else {
                        hue = 360.0f + 60.0f * (g - b) / (max - min);
                    }
                }
                else if (max == g) {
                    hue = 120.0f + 60.0f * (b - r) / (max - min);
                }
                else {
                    hue = 240.0f + 60.0f * (r - g) / (max - min);
                }
            }

            datadst[pos] = (unsigned char)(hue / 360.0f * 255.0f);
            datadst[pos + 1] = (unsigned char)(sat * 255.0f);
            datadst[pos + 3] = (unsigned char)val;
        }
    }
    return 1;
}

int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax) {
    if ((src == NULL) || (dst == NULL)) return 0; // Verificação adicional para ponteiros nulos

    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width = src->width;
    int height = src->height;
    int channels_src = src->channels;
    int channels_dst = dst->channels;
    int x, y;
    float h, s, v;
    long int pos_src;
    long int pos_dst;

    if ((width <= 0) || (height <= 0) || (datasrc == NULL)) return 0;
    if ((width != dst->width) || (height != dst->height)) return 0;

    if (channels_src != 3 || channels_dst != 1) return 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos_src = y * (width * channels_src) + x * channels_src;
            pos_dst = y * (width * channels_dst) + x * channels_dst;

            h = ((float)datasrc[pos_src] / 255.0f) * 360.0f;
            s = ((float)datasrc[pos_src + 1] / 255.0f) * 100.0f;
            v = ((float)datasrc[pos_src + 2] / 255.0f) * 100.0f;

            if (h >= hmin && h <= hmax && s >= smin && s <= smax && v >= vmin && v <= vmax)
            {
                datadst[pos_dst] = 255;
            }
            else {
                datadst[pos_dst] = 0;
            }
        }
    }

    return 1;
}