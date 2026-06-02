/*****************************************************************//**
 * \file   vc.c
 * \author DUARTE DUQUE - dduque@ipca.pt
 * \date   May 2025
 *********************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "vc.h"

 //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 //            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
 //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


IVC* vc_image_new(int width, int height, int channels, int levels)
{
	IVC* image = (IVC*)malloc(sizeof(IVC));

	if (image == NULL) return NULL;
	if ((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


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

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNCTIONS: IMAGE READING AND WRITING (PBM, PGM AND PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char* netpbm_get_token(FILE* file, char* tok, int len)
{
	char* t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)));
		if (c != '#') break;
		do c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF) break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#') ungetc(c, file);
	}

	*t = 0;

	return tok;
}


long int unsigned_char_to_bit(unsigned char* datauchar, unsigned char* databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char* p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char* databit, unsigned char* datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char* p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC* vc_read_image(char* filename)
{
	FILE* file = NULL;
	IVC* image = NULL;
	unsigned char* tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	if ((file = fopen(filename, "rb")) != NULL)
	{
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }
		else if (strcmp(tok, "P5") == 0) channels = 1;
		else if (strcmp(tok, "P6") == 0) channels = 3;
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1)
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
	}

	return image;
}


int vc_write_image(char* filename, IVC* image)
{
	FILE* file = NULL;
	unsigned char* tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL) return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}

int vc_add_image(IVC* src, IVC* dst) {
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int sPos1;
	int x, y, k = 0;


	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			sPos1 = y * bytesperline + x * channels;

			if (src->data[sPos1] != 0) {
				dst->data[sPos1] = 255;
				dst->data[sPos1 + 1] = 255;
				dst->data[sPos1 + 2] = 255;
			}
		}
	}

	return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: C�LCULO DE CALIBRE E CATEGORIA LARANJA
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float vc_calculate_diameter(int width, int height)
{
	float pixels = (width > height) ? width : height;
	float diameter_mm = (pixels / 280.0f) * 55.0f;
	return diameter_mm;
}

int vc_calculate_calibre(float diameter_mm)
{
	if (diameter_mm < 53.0f)   return -1;
	if (diameter_mm >= 100.0f) return 0;
	if (diameter_mm >= 87.0f)  return 1;
	if (diameter_mm >= 84.0f)  return 2;
	if (diameter_mm >= 81.0f)  return 3;
	if (diameter_mm >= 77.0f)  return 4;
	if (diameter_mm >= 73.0f)  return 5;
	if (diameter_mm >= 70.0f)  return 6;
	if (diameter_mm >= 67.0f)  return 7;
	if (diameter_mm >= 64.0f)  return 8;
	if (diameter_mm >= 62.0f)  return 9;
	if (diameter_mm >= 60.0f)  return 10;
	if (diameter_mm >= 58.0f)  return 11;
	if (diameter_mm >= 56.0f)  return 12;
	return 13;
}

char vc_calculate_category(OVC blob)
{
	float diameter_mm = vc_calculate_diameter(blob.width, blob.height);

	if (diameter_mm < 53.0f)
		return 'X';

	int cal = vc_calculate_calibre(diameter_mm);
	if (cal <= 2) return 'E';
	if (cal <= 6) return 'I';
	if (cal <= 10) return '2';
	return '3';
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: CONVERS�O DE CORES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int vc_gbr_rgb(IVC* src)
{
	if (src == NULL || src->channels != 3) return 0;

	for (int i = 0; i < src->width * src->height * src->channels; i += 3) {
		unsigned char temp = src->data[i];
		src->data[i] = src->data[i + 2];
		src->data[i + 2] = temp;
	}
	return 1;
}

int vc_rgb_to_hsv(IVC* src, IVC* dst)
{
	if (src == NULL || dst == NULL || src->channels != 3 || dst->channels != 3) return 0;

	for (int i = 0; i < src->height * src->width; i++) {
		float r = src->data[i * 3] / 255.0f;
		float g = src->data[i * 3 + 1] / 255.0f;
		float b = src->data[i * 3 + 2] / 255.0f;

		float maxC = MAX3(r, g, b);
		float minC = MIN3(r, g, b);
		float delta = maxC - minC;

		float h = 0;
		if (delta != 0) {
			if (maxC == r) h = 60 * (fmod((g - b) / delta, 6));
			else if (maxC == g) h = 60 * (((b - r) / delta) + 2);
			else if (maxC == b) h = 60 * (((r - g) / delta) + 4);
			if (h < 0) h += 360;
		}

		float s = (maxC == 0) ? 0 : (delta / maxC);
		float v = maxC;

		dst->data[i * 3] = (unsigned char)(h / 360.0f * 255.0f);
		dst->data[i * 3 + 1] = (unsigned char)(s * 255.0f);
		dst->data[i * 3 + 2] = (unsigned char)(v * 255.0f);
	}
	return 1;
}

int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	if (src == NULL || dst == NULL || src->channels != 3 || dst->channels != 3) return 0;

	for (int i = 0; i < src->height * src->width; i++) {
		int h = src->data[i * 3];
		int s = src->data[i * 3 + 1];
		int v = src->data[i * 3 + 2];

		if (h >= hmin && h <= hmax && s >= smin && s <= smax && v >= vmin && v <= vmax) {
			dst->data[i * 3] = 255;
			dst->data[i * 3 + 1] = 255;
			dst->data[i * 3 + 2] = 255;
		}
		else {
			dst->data[i * 3] = 0;
			dst->data[i * 3 + 1] = 0;
			dst->data[i * 3 + 2] = 0;
		}
	}
	return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: OPERA��ES MORFOL�GICAS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{
	if (src == NULL || dst == NULL || src->channels != 3 || dst->channels != 3) return 0;

	memcpy(dst->data, src->data, src->width * src->height * src->channels);

	int k = kernel / 2;
	for (int y = k; y < src->height - k; y++) {
		for (int x = k; x < src->width - k; x++) {
			int max_val = 0;
			for (int dy = -k; dy <= k; dy++) {
				for (int dx = -k; dx <= k; dx++) {
					int pos = (y + dy) * src->width * 3 + (x + dx) * 3;
					max_val = (src->data[pos] > max_val) ? src->data[pos] : max_val;
				}
			}
			int dst_pos = y * dst->width * 3 + x * 3;
			dst->data[dst_pos] = max_val;
			dst->data[dst_pos + 1] = max_val;
			dst->data[dst_pos + 2] = max_val;
		}
	}
	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{
	if (src == NULL || dst == NULL || src->channels != 3 || dst->channels != 3) return 0;

	memcpy(dst->data, src->data, src->width * src->height * src->channels);

	int k = kernel / 2;
	for (int y = k; y < src->height - k; y++) {
		for (int x = k; x < src->width - k; x++) {
			int min_val = 255;
			for (int dy = -k; dy <= k; dy++) {
				for (int dx = -k; dx <= k; dx++) {
					int pos = (y + dy) * src->width * 3 + (x + dx) * 3;
					min_val = (src->data[pos] < min_val) ? src->data[pos] : min_val;
				}
			}
			int dst_pos = y * dst->width * 3 + x * 3;
			dst->data[dst_pos] = min_val;
			dst->data[dst_pos + 1] = min_val;
			dst->data[dst_pos + 2] = min_val;
		}
	}
	return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: CONVERS�O DE CANAIS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int vc_three_to_one_channel(IVC* src, IVC* dst)
{
	if (src == NULL || dst == NULL || src->channels != 3 || dst->channels != 1) return 0;

	for (int i = 0; i < src->height * src->width; i++) {
		dst->data[i] = src->data[i * 3];
	}
	return 1;
}

int vc_one_to_three_channel(IVC* src, IVC* dst)
{
	if (src == NULL || dst == NULL || src->channels != 1 || dst->channels != 3) return 0;

	for (int i = 0; i < src->height * src->width; i++) {
		dst->data[i * 3] = src->data[i];
		dst->data[i * 3 + 1] = src->data[i];
		dst->data[i * 3 + 2] = src->data[i];
	}
	return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: LABELLING DE BLOBS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	if (src == NULL || dst == NULL || nlabels == NULL) return NULL;

	int label = 0;
	OVC* blobs = (OVC*)malloc(src->width * src->height * sizeof(OVC));
	if (blobs == NULL) return NULL;

	unsigned char* labels = (unsigned char*)calloc(src->width * src->height, sizeof(unsigned char));
	if (labels == NULL) {
		free(blobs);
		return NULL;
	}

	memcpy(dst->data, src->data, src->width * src->height);

	for (int y = 0; y < src->height; y++) {
		for (int x = 0; x < src->width; x++) {
			int pos = y * src->width + x;
			if (src->data[pos] == 255 && labels[pos] == 0) {
				label++;
				blobs[label - 1].label = label;
				blobs[label - 1].x = x;
				blobs[label - 1].y = y;
				blobs[label - 1].xf = x;
				blobs[label - 1].yf = y;
				blobs[label - 1].area = 0;

				int* queue = (int*)malloc(src->width * src->height * 2 * sizeof(int));
				int front = 0, rear = 0;
				queue[rear++] = x;
				queue[rear++] = y;
				labels[pos] = label;
				dst->data[pos] = label;

				while (front < rear) {
					int cx = queue[front++];
					int cy = queue[front++];
					int cpos = cy * src->width + cx;

					blobs[label - 1].area++;
					if (cx < blobs[label - 1].x) blobs[label - 1].x = cx;
					if (cx > blobs[label - 1].xf) blobs[label - 1].xf = cx;
					if (cy < blobs[label - 1].y) blobs[label - 1].y = cy;
					if (cy > blobs[label - 1].yf) blobs[label - 1].yf = cy;

					int dx[] = { -1, 1, 0, 0, -1, -1, 1, 1 };
					int dy[] = { 0, 0, -1, 1, -1, 1, -1, 1 };
					for (int i = 0; i < 8; i++) {
						int nx = cx + dx[i];
						int ny = cy + dy[i];
						if (nx >= 0 && nx < src->width && ny >= 0 && ny < src->height) {
							int npos = ny * src->width + nx;
							if (src->data[npos] == 255 && labels[npos] == 0) {
								labels[npos] = label;
								dst->data[npos] = label;
								queue[rear++] = nx;
								queue[rear++] = ny;
							}
						}
					}
				}
				free(queue);
			}
		}
	}

	free(labels);
	*nlabels = label;
	return blobs;
}

int vc_binary_blob_info(IVC* src, OVC* blobs, int nlabels)
{
	if (src == NULL || blobs == NULL) return 0;

	for (int i = 0; i < nlabels; i++) {
		blobs[i].width = blobs[i].xf - blobs[i].x + 1;
		blobs[i].height = blobs[i].yf - blobs[i].y + 1;
		blobs[i].xc = blobs[i].x + blobs[i].width / 2;
		blobs[i].yc = blobs[i].y + blobs[i].height / 2;

		blobs[i].perimeter = 0;
		for (int y = blobs[i].y; y <= blobs[i].yf; y++) {
			for (int x = blobs[i].x; x <= blobs[i].xf; x++) {
				int pos = y * src->width + x;
				if (src->data[pos] == blobs[i].label) {
					int neighbors = 0;
					int dx[] = { -1, 1, 0, 0 };
					int dy[] = { 0, 0, -1, 1 };
					for (int j = 0; j < 4; j++) {
						int nx = x + dx[j];
						int ny = y + dy[j];
						if (nx < 0 || nx >= src->width || ny < 0 || ny >= src->height || src->data[ny * src->width + nx] != blobs[i].label) {
							neighbors++;
						}
					}
					if (neighbors > 0) blobs[i].perimeter++;
				}
			}
		}
	}
	return 1;
}

int vc_draw_bounding_box(IVC* dest, OVC* blobs, int nlabels)
{
	if (dest == NULL || blobs == NULL) return 0;

	for (int i = 0; i < nlabels; i++) {
		for (int x = blobs[i].x; x <= blobs[i].xf && x < dest->width; x++) {
			int y_top = blobs[i].y;
			int y_bot = blobs[i].yf;

			if (y_top >= 0 && y_top < dest->height && x >= 0) {
				int pos = y_top * dest->width * dest->channels + x * dest->channels;
				dest->data[pos] = 0;
				dest->data[pos + 1] = 255;
				dest->data[pos + 2] = 0;
			}

			if (y_bot >= 0 && y_bot < dest->height && x >= 0) {
				int pos = y_bot * dest->width * dest->channels + x * dest->channels;
				dest->data[pos] = 0;
				dest->data[pos + 1] = 255;
				dest->data[pos + 2] = 0;
			}
		}

		for (int y = blobs[i].y; y <= blobs[i].yf && y < dest->height; y++) {
			int x_left = blobs[i].x;
			int x_right = blobs[i].xf;

			if (x_left >= 0 && x_left < dest->width && y >= 0) {
				int pos = y * dest->width * dest->channels + x_left * dest->channels;
				dest->data[pos] = 0;
				dest->data[pos + 1] = 255;
				dest->data[pos + 2] = 0;
			}

			if (x_right >= 0 && x_right < dest->width && y >= 0) {
				int pos = y * dest->width * dest->channels + x_right * dest->channels;
				dest->data[pos] = 0;
				dest->data[pos + 1] = 255;
				dest->data[pos + 2] = 0;
			}
		}
	}
	return 1;
}

OVC* vc_check_if_circle(OVC* blobs, int* nLabels, IVC* src)
{
	if (blobs == NULL || nLabels == NULL) return NULL;

	OVC* filtered_blobs = (OVC*)malloc(*nLabels * sizeof(OVC));
	if (filtered_blobs == NULL) return NULL;

	int count = 0;
	for (int i = 0; i < *nLabels; i++) {
		float circularity = (4.0f * 3.14159f * blobs[i].area) / (blobs[i].perimeter * blobs[i].perimeter + 0.0001f);
		if (circularity > 0.72f) {
			filtered_blobs[count++] = blobs[i];
		}
	}

	free(blobs);
	*nLabels = count;
	return filtered_blobs;
}

int vc_center(OVC* blobs, IVC* dst, int nlabels)
{
	if (blobs == NULL || dst == NULL) return 0;

	for (int i = 0; i < nlabels; i++) {
		if (blobs[i].xc >= 0 && blobs[i].xc < dst->width && blobs[i].yc >= 0 && blobs[i].yc < dst->height) {
			int pos = blobs[i].yc * dst->width * dst->channels + blobs[i].xc * dst->channels;
			if (dst->channels == 3) {
				dst->data[pos] = 255;
				dst->data[pos + 1] = 0;
				dst->data[pos + 2] = 0;
			}
		}
	}
	return 1;
}

int vc_check_collisions(OVC firstBlob, OVC secondBlob)
{
	return (firstBlob.xc >= secondBlob.x && firstBlob.xc <= secondBlob.xf &&
		firstBlob.yc >= secondBlob.y && firstBlob.yc <= secondBlob.yf) ? 1 : 0;
}

int vc_main_collisions(OVC blob, OVC* secondBlobs, int secondBlob)
{
	for (int i = 0; i < secondBlob; i++) {
		if (vc_check_collisions(blob, secondBlobs[i])) {
			return 1;
		}
	}
	return 0;
}

int vc_delete_blob(IVC* img, OVC blob)
{
	if (img == NULL) return 0;

	for (int y = blob.y; y <= blob.yf && y < img->height; y++) {
		for (int x = blob.x; x <= blob.xf && x < img->width; x++) {
			int pos = y * img->width;
			if (img->channels == 1) {
				img->data[pos + x] = 0;
			}
			else if (img->channels == 3) {
				img->data[(pos + x) * 3] = 0;
				img->data[(pos + x) * 3 + 1] = 0;
				img->data[(pos + x) * 3 + 2] = 0;
			}
		}
	}
	return 1;
}

int idCoin(int area, int perimeter)
{
	if (area == 0 || perimeter == 0) return 0;
	float circularity = (4.0f * 3.14159f * area) / (perimeter * perimeter);
	return (circularity > 0.8f) ? 1 : 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: OUTRAS OPERA��ES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int vc_gray_edge_prewitt(IVC* src, IVC* dst)
{
	return 0;
}

int vc_draw_edge(IVC* src, IVC* dst)
{
	return 0;
}

int vc_limit(IVC* src, IVC* dst, int y)
{
	return 0;
}

int vc_limit2(IVC* src, IVC* dst, int y)
{
	return 0;
}