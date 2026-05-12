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

    for (y = 0; y < height; y++) {
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


int vc_binary_erosion(IVC* src, IVC* dst, int size) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y, kx, ky;
	int offset = size / 2;
	int is_all_white;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

    for ( y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            is_all_white = 1;
        }
    }

    for (ky = -offset; ky <= offset; ky++) {
        for (kx = -offset; kx <= offset; kx++)
        {
            int ny = y + ky;
            int nx = x + kx;

            if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                long int pos_neighbor = ny * bytesperline + nx * channels;
                if (datasrc[pos_neighbor] == 0) {
                    is_all_white = 0;
                    break;
                }
            }
            else
            {
                is_all_white = 0;
                break;
            }
        }
        if (!is_all_white) {
            break;
        }

        long int pos = y * bytesperline + x * channels;
        if (is_all_white)
        {
            datadst[pos] = 255;
        }
        else
        {
            datadst[pos] = 0;
        }
    }
}

int vc_binary_dilation(IVC* src, IVC* dst, int size) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y, kx, ky;
	int offset = size / 2;
	int has_white;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (channels != 1) return 0;
        
        for(y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                has_white = 0;


                for (ky = -offset; ky < offset; ky++)
                {
                    for (kx = -offset; kx < offset; kx++)
                    {
                        int ny = y + ky;
                        int nx = x + kx;
                        if (ny >= 0 && ny < height && nx < width) {
                            long int pos_neighbor = ny * bytesperline + nx * channels;


                            if (datasrc[pos_neighbor] == 255) {
                                has_white = 1;
                                break;
                            }
                        }
                    }
                }

                long int pos = y * bytesperline + x * channels;

                if (has_white)
                {
                    datadst[pos] = 255;
                }
                else
                {
                    datadst[pos] = 0;
                }
            }
        }
		return 1;
}
int vc_binary_open(IVC* src, IVC* dst, int size) {
    int ret = 1;

    if (src->width <= 0 || src->height <= 0 || src->data == NULL) return 0;
    if (src->channels != 1 || dst->channels != 1) return 0;

    IVC* tmp = vc_image_new(src->width, src->height, src->channels, src->levels);
    if (tmp == NULL) return 0;

    ret &= vc_binary_erosion(src, tmp, size);

    ret &= vc_binary_dilation(tmp, dst, size);

    vc_image_free(tmp);

    return ret;
}

int vc_binary_close(IVC* src, IVC* dst, int size) {
    int ret = 1;

    if (src->width <= 0 || src->height <= 0 || src->data == NULL) return 0;
    if (src->channels != 1 || dst->channels != 1) return 0;

    IVC* tmp = vc_image_new(src->width, src->height, src->channels, src->levels);
    if (tmp == NULL) return 0;

    ret &= vc_binary_dilation(tmp, dst, size);

    ret &= vc_binary_erosion(src, tmp, size);

    vc_image_free(tmp);

    return ret;
}

int vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y, i;
	long int pos, pos_neighbor;
	int label = 1;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1||dst->channels!=1) return 0;

    for(i = 0; i < width * height; i++)
    {
        datadst[i] = 0;
	}
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;

            if (datasrc[pos] != 0 && datadst[pos] == 0) {
                _vc_flood_fill(src, dst, x, y, label++);
            }
        }
    }

	*nlabels = label - 1;
    return 1;
}

//fazer vc_flood_fill para auxiliar na função de rotulagem de blobs
void vc_flood_fill(IVC* src, IVC* dst, int x, int y, int label) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels = src->channels;

	int *stack_x = (int*)malloc(sizeof(int) * width * height);
	int* stack_y = (int*)malloc(sizeof(int) * width * height);
	int stack_ptr = 0;

    stack_x[stack_ptr] = x;
	stack_y[stack_ptr] = y;
	stack_ptr++;

    while (stack_ptr>0)
    {
        stack_ptr--;
		int cx = stack_x[stack_ptr];
		int cy = stack_y[stack_ptr];

		long int pos = cy * width * channels + cx * channels;

        if (datasrc[pos] == 255 && datadst[pos] == 0) {
            datadst[pos] == (unsigned char)label;

			int dx[] = { -1, 0, 1, 0 };
            int dy[] = { 0, -1, 0, 1 };
            for (int i = 0; i < 4; i++)
            {
                int nx = cx + dx[i];
                int ny = cy + dy[i];

                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    stack_x[stack_ptr] = nx;
                    stack_y[stack_ptr] = ny;
                    stack_ptr++;
                }
            }
        }
    }
	free(stack_x);
	free(stack_y);
}
//fazer vc_image_free para liberar a memória alocada para as imagens
IVC* vc_image_free(IVC* image)
{
    if (image != NULL)
    {
        if (image->data != NULL)
        {
            free(image->data);
            image->data = NULL;
        }

        free(image);
        image = NULL;
    }

    return image;
}