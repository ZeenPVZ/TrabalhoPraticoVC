#include <iostream>
#include <opencv2/opencv.hpp>	
#include "vc.h"

typedef struct {
	int xc, yc;
	bool ativo;
}LaranjaVideo;

int main() {
	cv::VideoCapture cap("video.avi");
	if(!cap.isOpened()) {
		std::cerr << "Nao foi possivel abrir video" << std::endl;
		return -1;
	}

	int TotalLaranjas = 0;

	cv::Mat frame;
	while (cap.read(frame))
	{
		IVC* imgRGB = vc_image_new(frame.cols, frame.rows, 3, 255);
		IVC* imgBin = vc_image_new(frame.cols, frame.rows, 1, 255);

		for (int y = 0; y < frame.rows; y++)
		{
			for (int x = 0; x < frame.cols; x++)
			{
				int pos = (y * frame.cols + x) * 3;
				imgRGB->data[pos] = frame.data[pos+2];
				imgRGB->data[pos+1] = frame.data[pos + 1];
				imgRGB->data[pos+2] = frame.data[pos];
			}
		}
		IVC* imgHSV = vc_image_new(frame.cols, frame.rows, 3, 255);
		vc_rgb_to_hsv(imgRGB, imgHSV);

		vc_hsv_segmentation(imgHSV, imgBin, 20, 40, 50, 100, 50, 255);

		vc_binary_close(imgBin, imgBin, 7);
		vc_binary_open(imgBin, imgBin, 5);

		int nlabels = 0;
		IVC* imgLabel = vc_image_new(frame.cols, frame.rows, 1, 255);
		vc_binary_blob_labelling(imgBin, imgLabel, &nlabels);

		IVCBlob blobs[100];
		vc_binary_blob_info(imgLabel, blobs, nlabels);

		for (int i = 0; i < nlabels; i++)
		{
			if (blobs[i].area < 500) continue;

			float diametro = (blobs[i].width * 55.0f) / 280.0f;

			cv::Rect rect(blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height);
			cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 2);

			std::string info="Diametro: " + std::to_string(diametro) + " cm";
			cv::putText(frame, info, cv::Point(blobs[i].x, blobs[i].y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);

			IVC* imgLabel = vc_image_new(frame.cols, frame.rows, 1, 255);
		}

		cv::imshow("Laranjas", frame);

		vc_image_free(imgRGB);
		vc_image_free(imgBin);
		vc_image_free(imgHSV);
		vc_image_free(imgLabel);

		if (cv::waitKey(1) == 'q') break;
	}
	cap.release();
	return 0;
}