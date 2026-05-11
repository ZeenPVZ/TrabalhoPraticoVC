#define VC_DEBUG
typedef struct {
	unsigned char* data;
	int width, height;
	int channels;
	int levels;
}IVC;


int vc_rgb_to_hsv(IVC* src, IVC* dst);
int vc_hsv_segmentation(IVC* src, IVC* dst, int h, int s, int v, int hmax, int smax, int vmax);
int vc_binary_erosion(IVC* src, IVC* dst, int size);
int vc_binary_dilation(IVC* src, IVC* dst, int size);
int vc_binary_open(IVC* src, IVC* dst, int size);
int vc_binary_close(IVC* src, IVC* dst, int size);
int vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);

