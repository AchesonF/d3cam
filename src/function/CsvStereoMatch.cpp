//#include <iostream>
#include "CsvStereoMatch.hpp"

//#include <syslog.h>
//#include "csv_utility.h"
//#include "log.h"



CsvStereoMatchBM::CsvStereoMatchBM()
{
	if (loadCameraParam("camera_param.yml")) {
//		log_info("OK : load camera param.");
    } else {
 //       log_info("ERROR : load camera param.");
        return;
    }

    initUndistortRectifyMap(leftcam_M_, leftcam_D_, leftcam_R_, leftcam_P_, 
    	Size(nWidth, nHeight), CV_16SC2, rmap_[0][0], rmap_[0][1]);
    initUndistortRectifyMap(rightcam_M_, rightcam_D_, rightcam_R_, rightcam_P_, 
    	Size(nWidth, nHeight), CV_16SC2, rmap_[1][0], rmap_[1][1]);
    //int ndisparities = 192;   // Range of disparity
    int SADWindowSize = 21; // Size of the block window. Must be odd
    
    stereo_bm = cv::StereoBM::create(NumDisparities, SADWindowSize);

    Rect leftROI, rightROI;
    stereo_bm->setPreFilterSize(9);
    stereo_bm->setPreFilterCap(31);
    //stereo_bm->setBlockSize(180);
    stereo_bm->setMinDisparity(-1);
    stereo_bm->setNumDisparities(NumDisparities);
    stereo_bm->setTextureThreshold(30);
    stereo_bm->setUniquenessRatio(20);
    stereo_bm->setSpeckleWindowSize(100);
    stereo_bm->setSpeckleRange(32);
    stereo_bm->setDisp12MaxDiff(-1);
 	stereo_bm->setROI1(leftcam_valid_roi_);
	stereo_bm->setROI2(rightcam_valid_roi_);
}

CsvStereoMatchBM::~CsvStereoMatchBM()
{
}

bool CsvStereoMatchBM::loadCameraParam(const string& file_path)
{
	FileStorage fs(file_path, FileStorage::READ);

	if (!fs.isOpened())
		return false;

	fs["leftcam_M"] >> leftcam_M_;
	fs["leftcam_D"] >> leftcam_D_;
	fs["rightcam_M"] >> rightcam_M_;
	fs["rightcam_D"] >> rightcam_D_;
	fs["leftcam_R"] >> leftcam_R_;
	fs["rightcam_R"] >> rightcam_R_;
	fs["leftcam_P"] >> leftcam_P_;
	fs["rightcam_P"] >> rightcam_P_;
	fs["cam_Q"] >> cam_Q_;
	fs["leftcam_valid_roi"] >> leftcam_valid_roi_;
	fs["rightcam_valid_roi"] >> rightcam_valid_roi_;
	fs.release();

	nWidth = 1920;
	nHeight = 1024;

	return true;
}

void CsvStereoMatchBM::stereoMatch(const Mat& left_image, const Mat& right_image, Mat& disp)
{

	Mat rleft_image, rright_image;
	remap(left_image, rleft_image, rmap_[0][0], rmap_[0][1], INTER_LINEAR);
	remap(right_image, rright_image, rmap_[1][0], rmap_[1][1], INTER_LINEAR);

	stereo_bm->setNumDisparities(NumDisparities);
	stereo_bm->setMinDisparity(-(stereo_bm->getMinDisparity() + NumDisparities) + 1);

	if (NumDisparities == 0) {
		NumDisparities = 16*51;
	}

	stereo_bm->setUniquenessRatio(UniqRatio);
	//stereo_bm->setTextureThreshold(TextureThreshold);
	stereo_bm->setBlockSize(BlockSize);
	stereo_bm->setMinDisparity(0);

	//log_info("compute disp.");
	stereo_bm->compute(rleft_image, rright_image, disp);
	//log_info("disp:cols=%d rows=%d deep=%d datatype=%d", disp.cols, disp.rows, disp.channels(), disp.type());

}




