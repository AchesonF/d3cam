#pragma once

#include <opencv2/opencv.hpp>


using namespace cv;
using namespace std;

class CsvStereoMatch
{
public:
	CsvStereoMatch(){};

	virtual ~CsvStereoMatch(){};

	virtual void stereoMatch(const Mat& left_image, const Mat& right_image, Mat& disp){};

	int NumDisparities;
	int BlockSize;
	int UniqRatio;
	int nWidth;
	int nHeight;
};


class CsvStereoMatchBM: public CsvStereoMatch
{
public:
	CsvStereoMatchBM();
	virtual ~CsvStereoMatchBM();

	virtual void stereoMatch(const Mat& left_image, const Mat& right_image, Mat& disp);

private:
	bool loadCameraParam(const string& file_path);

private:
	Ptr<StereoBM> stereo_bm;
	Mat rmap_[2][2], cam_Q_;
	Mat leftcam_M_, leftcam_D_, leftcam_R_, leftcam_P_;
	Mat rightcam_M_, rightcam_D_, rightcam_R_, rightcam_P_;
	Rect leftcam_valid_roi_, rightcam_valid_roi_;
};

