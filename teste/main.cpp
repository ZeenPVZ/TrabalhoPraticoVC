#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <vector>

extern "C" {
#include "vc.h"
}

int main(void) {
	char videofile[50] = "video.avi";
	cv::VideoCapture capture;
	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;
	std::string str;
	int key = 0;

	capture.open(videofile);

	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de video!\n";
		return 1;
	}

	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	cv::namedWindow("VC - ORANGE QUALIFIER", cv::WINDOW_AUTOSIZE);

	IVC* image = vc_image_new(video.width, video.height, 3, 255);
	IVC* imageA = vc_image_new(video.width, video.height, 3, 255);
	IVC* imageB = vc_image_new(video.width, video.height, 3, 255);
	IVC* imageC = vc_image_new(video.width, video.height, 3, 255);
	IVC* imageD = vc_image_new(video.width, video.height, 3, 255);
	IVC* imageI = vc_image_new(video.width, video.height, 3, 255);
	IVC* imageE = vc_image_new(video.width, video.height, 3, 255);
	IVC* imageF = vc_image_new(video.width, video.height, 1, 255);
	IVC* imageH = vc_image_new(video.width, video.height, 1, 255);

	int total_oranges_counted = 0;
	// Contadores acumulados (total desde o inicio do video)
	int category_extra = 0, category_i = 0, category_ii = 0, category_iii = 0;
	int calibre_count[14] = { 0 };
	// Contadores do frame atual (quantas detetadas neste frame)
	int frame_extra = 0, frame_i = 0, frame_ii = 0, frame_iii = 0;

	OVC* blobBuffer1 = NULL;
	OVC* blobBuffer2 = NULL;
	std::vector<OrangeTrack> orange_tracks;
	int next_orange_id = 0;

	cv::Mat frame;
	cv::Mat frameA;
	int current_frame_count = 0;

	int line_y1 = video.height / 5;        // ~144px em 720p
	int line_y2 = (video.height * 4) / 5; // ~576px em 720p

	while (key != 'q') {
		capture.read(frame);
		if (frame.empty()) break;
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);
		current_frame_count = 0;

		if (blobBuffer2 != NULL) {
			free(blobBuffer2);
			blobBuffer2 = NULL;
		}
		blobBuffer2 = (OVC*)calloc(100, sizeof(OVC));
		cv::medianBlur(frame, frameA, 5);
		memcpy(image->data, frameA.data, video.width * video.height * 3);
		memcpy(imageI->data, frameA.data, video.width * video.height * 3);

		int nlabels = 0;
		vc_gbr_rgb(image);
		vc_rgb_to_hsv(image, imageB);

		// Segmentacao HSV: laranja e tons alaranjados (inclui laranjas mais amareladas)
		vc_hsv_segmentation(imageB, imageC, 5, 28, 80, 255, 70, 255);
		// Laranja avermelhada / tons quentes proximos do vermelho
		vc_hsv_segmentation(imageB, imageA, 0, 10, 90, 255, 70, 255);
		vc_add_image(imageC, imageA);

		vc_binary_dilate(imageA, imageC, 3);
		vc_binary_erode(imageC, imageA, 3);
		vc_binary_dilate(imageA, imageC, 3);

		vc_three_to_one_channel(imageC, imageF);
		OVC* blobs = vc_binary_blob_labelling(imageF, imageH, &nlabels);

		if (blobs != NULL) {
			vc_binary_blob_info(imageH, blobs, nlabels);
			blobs = vc_check_if_circle(blobs, &nlabels, imageF);

			if (blobs != NULL) {
				// Reset apenas dos contadores do frame atual
				frame_extra = frame_i = frame_ii = frame_iii = 0;

				for (int i = 0; i < nlabels; i++) {
					if (blobs[i].area < 500) continue;

					// Processar blobs que intersectam a zona (bounding box toca a zona)
					if (blobs[i].yf < line_y1 || blobs[i].y > line_y2) continue;

					blobs[i].diameter_mm = vc_calculate_diameter(blobs[i].width, blobs[i].height);
					blobs[i].calibre = vc_calculate_calibre(blobs[i].diameter_mm);
					blobs[i].category = vc_calculate_category(blobs[i]);

					bool found_match = false;

					for (auto& track : orange_tracks) {
						int dx = blobs[i].xc - track.blob.xc;
						int dy = blobs[i].yc - track.blob.yc;
						int distance = (int)sqrt(dx * dx + dy * dy);

						if (distance < 60) {
							int prev_y = track.blob.yc;
							int curr_y = blobs[i].yc;

							// Conta ao cruzar a linha de contagem (line_y1), a descer
							if (prev_y < line_y1 && curr_y >= line_y1) {
								if (!track.already_counted) {
									total_oranges_counted++;
									track.already_counted = true;
									// Acumular categoria no momento em que e contada
									switch (blobs[i].category) {
									case 'E': category_extra++; break;
									case 'I': category_i++;     break;
									case '2': category_ii++;    break;
									default:  category_iii++;   break;
									}
									if (blobs[i].calibre >= 0 && blobs[i].calibre < 14)
										calibre_count[blobs[i].calibre]++;
								}
							}

							track.blob = blobs[i];
							track.last_seen_frame = video.nframe;
							found_match = true;
							break;
						}
					}

					if (!found_match) {
						OrangeTrack new_track;
						new_track.blob = blobs[i];
						new_track.frame_detected = video.nframe;
						new_track.last_seen_frame = video.nframe;
						new_track.id = next_orange_id++;
						new_track.already_counted = false;
						// Se o blob ja entra com yc abaixo de line_y1, conta imediatamente
						if (blobs[i].yc >= line_y1) {
							total_oranges_counted++;
							new_track.already_counted = true;
							switch (blobs[i].category) {
							case 'E': category_extra++; break;
							case 'I': category_i++;     break;
							case '2': category_ii++;    break;
							default:  category_iii++;   break;
							}
							if (blobs[i].calibre >= 0 && blobs[i].calibre < 14)
								calibre_count[blobs[i].calibre]++;
						}
						orange_tracks.push_back(new_track);
					}

					// Contadores apenas do frame atual (para FRAME ATUAL no HUD)
					switch (blobs[i].category) {
					case 'E': frame_extra++; break;
					case 'I': frame_i++;     break;
					case '2': frame_ii++;    break;
					default:  frame_iii++;   break;
					}

					current_frame_count++;
					blobBuffer2[current_frame_count - 1] = blobs[i];

					vc_draw_bounding_box(imageI, &blobs[i], 1);
					vc_center(blobs, imageI, 1);

					int radius = (int)((blobs[i].width + blobs[i].height) / 4);
					cv::circle(frame, cv::Point(blobs[i].xc, blobs[i].yc), radius, cv::Scalar(0, 255, 255), 2);

					str = std::string("D: ").append(std::to_string((int)blobs[i].diameter_mm)).append("mm");
					cv::putText(frame, str, cv::Point(blobs[i].xc - 50, blobs[i].yc - 60), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 1);

					str = std::string("C").append(std::to_string(blobs[i].calibre));
					cv::putText(frame, str, cv::Point(blobs[i].xc - 50, blobs[i].yc - 45), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 0), 1);

					str = std::string("Cat: ").append(1, blobs[i].category);
					cv::putText(frame, str, cv::Point(blobs[i].xc - 50, blobs[i].yc - 30), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 165, 255), 1);

					str = std::string("A: ").append(std::to_string(blobs[i].area));
					cv::putText(frame, str, cv::Point(blobs[i].xc - 50, blobs[i].yc - 15), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 0, 255), 1);

					str = std::string("P: ").append(std::to_string(blobs[i].perimeter));
					cv::putText(frame, str, cv::Point(blobs[i].xc - 50, blobs[i].yc), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 1);
				}

			}
		}

		str = std::string("FRAME: ").append(std::to_string(video.nframe)).append("/").append(std::to_string(video.ntotalframes));
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);

		cv::line(frame, cv::Point(0, line_y1), cv::Point(video.width, line_y1), cv::Scalar(0, 200, 255), 2);
		cv::line(frame, cv::Point(0, line_y2), cv::Point(video.width, line_y2), cv::Scalar(0, 0, 255), 2);

		str = std::string("TOTAL LARANJAS: ").append(std::to_string(total_oranges_counted));
		cv::putText(frame, str, cv::Point(20, 55), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);

		str = std::string("FRAME ATUAL: ").append(std::to_string(current_frame_count));
		cv::putText(frame, str, cv::Point(20, 85), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);

		str = std::string("Cat.Extra: ").append(std::to_string(category_extra));
		cv::putText(frame, str, cv::Point(20, 120), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);

		str = std::string("Cat.I: ").append(std::to_string(category_i));
		cv::putText(frame, str, cv::Point(20, 140), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);

		str = std::string("Cat.II: ").append(std::to_string(category_ii));
		cv::putText(frame, str, cv::Point(20, 160), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);

		str = std::string("Cat.III: ").append(std::to_string(category_iii));
		cv::putText(frame, str, cv::Point(20, 180), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);

		cv::imshow("VC - ORANGE QUALIFIER", frame);
		key = cv::waitKey(1);
	}

	vc_image_free(image);
	vc_image_free(imageA);
	vc_image_free(imageB);
	vc_image_free(imageC);
	vc_image_free(imageD);
	vc_image_free(imageE);
	vc_image_free(imageF);
	vc_image_free(imageH);
	vc_image_free(imageI);

	cv::destroyWindow("VC - ORANGE QUALIFIER");
	capture.release();
	return 0;
}