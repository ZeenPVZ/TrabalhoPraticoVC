#ifdef __cplusplus
extern "C" {
#endif

// Estrutura de Imagem [cite: 148, 160]
typedef struct {
    unsigned char* data;
    int width, height;
    int channels;
    int levels;
    int bytesperline;
} IVC;

// Estrutura de Informação de Objetos 
typedef struct {
    int x, y;           // Posição superior esquerda (Bounding Box) [cite: 469]
    int width, height;  // Dimensões da Bounding Box [cite: 469]
    int area;           // Número de pixéis do objeto 
    int perimeter;      // Número de pixéis de contorno [cite: 469]
    float xc, yc;       // Centro de massa [cite: 469]
} IVCBlob;

IVC* vc_image_free(IVC* image);
IVC* vc_image_new(int width, int height, int channels, int levels);

int vc_rgb_to_hsv(IVC* src, IVC* dst);
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);
int vc_binary_erosion(IVC* src, IVC* dst, int size);
int vc_binary_dilation(IVC* src, IVC* dst, int size);
int vc_binary_open(IVC* src, IVC* dst, int size);
int vc_binary_close(IVC* src, IVC* dst, int size);
int vc_flood_fill(IVC* src, IVC* dst, int x, int y, int label);
int vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info(IVC* src, IVCBlob* blobs, int nlabels);

#ifdef __cplusplus
}
#endif

