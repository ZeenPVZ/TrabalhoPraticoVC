#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <opencv2/opencv.hpp>
#include "vc.h"

#define PIXELS_PER_MM (280.0f / 55.0f)

float pixels_to_mm(float pixels) {
    return pixels / PIXELS_PER_MM;
}

int get_calibre(float diameter_mm) {
    if (diameter_mm >= 100)           return 0;
    else if (diameter_mm >= 87)            return 1;
    else if (diameter_mm >= 84)            return 2;
    else if (diameter_mm >= 81)            return 3;
    else if (diameter_mm >= 77)            return 4;
    else if (diameter_mm >= 73)            return 5;
    else if (diameter_mm >= 70)            return 6;
    else if (diameter_mm >= 67)            return 7;
    else if (diameter_mm >= 64)            return 8;
    else if (diameter_mm >= 62)            return 9;
    else if (diameter_mm >= 60)            return 10;
    else if (diameter_mm >= 58)            return 11;
    else if (diameter_mm >= 56)            return 12;
    else if (diameter_mm >= 53)            return 13;
    else                                   return -1; 
}

const char* get_categoria(float width_px, float height_px) {
    float maior = width_px > height_px ? width_px : height_px;
    float menor = width_px < height_px ? width_px : height_px;
    if (maior == 0) return "III";
    float deformacao = (maior - menor) / maior;
    if (deformacao < 0.05f) return "Extra";
    else if (deformacao < 0.10f) return "I";
    else if (deformacao < 0.20f) return "II";
    else                         return "III";
}

int main(void) {
    char videofile[20] = "video.avi";
    cv::VideoCapture capture;

    struct {
        int width, height;
        int ntotal_frames;
        int fps;
        int nframe;
    } video;

    std::string str;
    int key = 0;

    int total_laranjas = 0;

    capture.open(videofile);
    if (!capture.isOpened()) {
        std::cerr << "Erro ao abrir o video: " << videofile << std::endl;
        return 1;
    }

    video.ntotal_frames = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    cv::namedWindow("TrabalhoPratico VisaoPorComputador", cv::WINDOW_AUTOSIZE);

    cv::Mat frame;
    while (key != 'q') {
        capture.read(frame);
        if (frame.empty()) break;

        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

        IVC* src_rgb = vc_image_new(video.width, video.height, 3, 255);
        IVC* src_hsv = vc_image_new(video.width, video.height, 3, 255);
        IVC* bin = vc_image_new(video.width, video.height, 1, 255);
        IVC* morph = vc_image_new(video.width, video.height, 1, 255);
        IVC* labels = vc_image_new(video.width, video.height, 1, 255);

        if (!src_rgb || !src_hsv || !bin || !morph || !labels) {
            std::cerr << "Erro ao alocar imagens IVC" << std::endl;
            break;
        }

        for (int y = 0; y < video.height; y++) {
            for (int x = 0; x < video.width; x++) {
                long int pos = y * video.width * 3 + x * 3;
                cv::Vec3b pixel = frame.at<cv::Vec3b>(y, x);
                src_rgb->data[pos] = pixel[2]; 
                src_rgb->data[pos + 1] = pixel[1]; 
                src_rgb->data[pos + 2] = pixel[0]; 
            }
        }

        vc_rgb_to_hsv(src_rgb, src_hsv);
        vc_hsv_segmentation(src_hsv, bin, 15, 30, 50, 100, 40, 100);

        vc_binary_open(bin, morph, 5);
        vc_binary_close(morph, bin, 15);

        int nlabels = 0;
        vc_binary_blob_labelling(bin, labels, &nlabels);

        int laranjas_frame = 0;

        if (nlabels > 0) {
            IVCBlob* blobs = (IVCBlob*)malloc(sizeof(IVCBlob) * nlabels);
            if (blobs != NULL) {
                vc_binary_blob_info(labels, blobs, nlabels);

                for (int i = 0; i < nlabels; i++) {
                    if (blobs[i].area < 35000) continue;

                    laranjas_frame++;

                    float diam_px = (blobs[i].width + blobs[i].height) / 2.0f;
                    float diam_mm = pixels_to_mm(diam_px);

                    int calibre = get_calibre(diam_mm);
                    const char* categoria = get_categoria(
                        (float)blobs[i].width,
                        (float)blobs[i].height
                    );

                    float maior = blobs[i].width > blobs[i].height
                        ? blobs[i].width : blobs[i].height;
                    float menor = blobs[i].width < blobs[i].height
                        ? blobs[i].width : blobs[i].height;
                    float deformacao = (maior > 0) ? (maior - menor) / maior * 100.0f : 0.0f;

                    bool aprovado = (calibre >= 0) && (std::string(categoria) != "III");

                    cv::Scalar cor_box = aprovado
                        ? cv::Scalar(0, 255, 0)  
                        : cv::Scalar(0, 0, 255); 
                    cv::rectangle(frame,
                        cv::Point(blobs[i].x, blobs[i].y),
                        cv::Point(blobs[i].x + blobs[i].width, blobs[i].y + blobs[i].height),
                        cor_box, 2);

                    cv::circle(frame,
                        cv::Point((int)blobs[i].xc, (int)blobs[i].yc),
                        4, cv::Scalar(255, 0, 255), -1);

                    int tx = blobs[i].x;
                    int ty = blobs[i].y - 5;
                    if (ty < 60) ty = blobs[i].y + blobs[i].height + 60;

                    char buf[256];

                    sprintf(buf, "LABEL:%d AREA:%d PERIMETRO:%d", i + 1, blobs[i].area, blobs[i].perimeter);
                    cv::putText(frame, buf, cv::Point(tx, ty - 45),
                        cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 0, 0), 2);
                    cv::putText(frame, buf, cv::Point(tx, ty - 45),
                        cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);

                    sprintf(buf, "CALIBRE:%d DIAMETRO:%.0fmm", calibre, diam_mm);
                    cv::putText(frame, buf, cv::Point(tx, ty - 30),
                        cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 0, 0), 2);
                    cv::putText(frame, buf, cv::Point(tx, ty - 30),
                        cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);

                    sprintf(buf, "DEFORMACAO:%.1f%% CATEGORIA:%s", deformacao, categoria);
                    cv::putText(frame, buf, cv::Point(tx, ty - 15),
                        cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 0, 0), 2);
                    cv::putText(frame, buf, cv::Point(tx, ty - 15),
                        cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);

                    sprintf(buf, "APROVADO:%s", aprovado ? "SIM" : "NAO");
                    cv::putText(frame, buf, cv::Point(tx, ty),
                        cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 0, 0), 2);
                    cv::putText(frame, buf, cv::Point(tx, ty),
                        cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);
                }

                total_laranjas += laranjas_frame;

                free(blobs);
            }
        }

        cv::rectangle(frame, cv::Point(0, 0), cv::Point(420, 115),
            cv::Scalar(0, 0, 0), -1);

        str = std::string("RESOLUCAO: ")
            .append(std::to_string(video.width))
            .append("x")
            .append(std::to_string(video.height));
        cv::putText(frame, str, cv::Point(5, 20),
            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);

        str = std::string("FRAME: ")
            .append(std::to_string(video.nframe))
            .append(" / ")
            .append(std::to_string(video.ntotal_frames))
            .append("  FPS: ")
            .append(std::to_string(video.fps));
        cv::putText(frame, str, cv::Point(5, 40),
            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);

        str = std::string("LARANJAS NA FRAME: ").append(std::to_string(laranjas_frame));
        cv::putText(frame, str, cv::Point(5, 65),
            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 1);

        str = std::string("TOTAL ACUMULADO: ").append(std::to_string(total_laranjas));
        cv::putText(frame, str, cv::Point(5, 90),
            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 1);

        vc_image_free(src_rgb);
        vc_image_free(src_hsv);
        vc_image_free(bin);
        vc_image_free(morph);
        vc_image_free(labels);

        cv::imshow("TrabalhoPratico VisaoPorComputador", frame);
        key = cv::waitKey(1);
    }

    cv::destroyWindow("TrabalhoPratico VisaoPorComputador");
    capture.release();
    return 0;
}