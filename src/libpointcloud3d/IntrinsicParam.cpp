#include "IntrinsicParam.h"

#include <memory>
#include <math.h>

namespace CSV
{

IntrinsicParam::IntrinsicParam()
{
	init();
}

IntrinsicParam::IntrinsicParam(const IntrinsicParam& param)
{
	this->operator=(param);
}

IntrinsicParam& IntrinsicParam::operator=(const IntrinsicParam& inparam)
{
	rms = inparam.rms;
	focus = inparam.focus;
	chipw = inparam.chipw;
	chiph = inparam.chiph;
	imw = inparam.imw;
	imh = inparam.imh;

	for (int i = 0; i < INTRIN_PARAM_NUM; ++i) {
		param[i] = inparam.param[i];
	}

	return *this;
}

IntrinsicParam::~IntrinsicParam(void)
{

}

bool IntrinsicParam::operator==(const IntrinsicParam& inparam) const
{
	if (rms != inparam.rms || focus != inparam.focus || chipw != inparam.chipw || chiph != inparam.chiph ||
		imw != inparam.imw || imh != inparam.imh) {
		return false;
	}
	for (int i = 0; i < INTRIN_PARAM_NUM; ++i) {
		if (fabs(param[i] - inparam.param[i]) > CSV_EPSILON) {
			return false;
		}
	}
}

bool IntrinsicParam::operator!=(const IntrinsicParam& param) const
{
	return !(*this == param);
}

/*
*@brief  获取相机相关信息，芯片宽高，图像宽高，镜头焦距
*@author HORSETAIL
*@param[out]  double& chipw  芯片宽度，单位：mm
*@param[out]  double& chiph  芯片高度，点位：mm
*@param[out]  int& imw  图像宽，单位：pixel
*@param[out]  int& imh  图像高，单位：pixel
*@param[out]  double& focus  镜头焦距，单位：mm
*@return     int  状态值
*/
CsvStatus IntrinsicParam::getInfo(double& chipw, double& chiph, int& imw, int& imh, double& focus, double& rms)
{
	rms = this->rms;
	chipw = this->chipw;
	chiph = this->chiph;
	imw = this->imw;
	imh = this->imh;
	focus = this->focus;

	return CSV_OK;
}

/*
*@brief  设置相机相关信息，芯片宽高，图像宽高，镜头焦距
*@author HORSETAIL
*@param[in]  double chipw  芯片宽度，单位：mm
*@param[in]  double chiph  芯片高度，点位：mm
*@param[in]  int imw  图像宽，单位：pixel
*@param[in]  int imh  图像高，单位：pixel
*@param[in]  double focus  镜头焦距，单位：mm
*@return     int  状态值
*/
CsvStatus IntrinsicParam::setInfo(double chipw, double chiph, int imw, int imh, double focus, double rms)
{
	this->rms = rms;
	this->chipw = chipw;
	this->chiph = chiph;
	this->imw = imw;
	this->imh = imh;
	this->focus = focus;

	return CSV_OK;
}

/*
*@brief  设置参数
*@author HORSETAIL
*@param[out] 
*@param[in]  double * intrinsic  相机内参数
*@return     int  状态返回值
*/
CsvStatus IntrinsicParam::getParam(double* intrinsic)
{
	if (NULL == intrinsic) return CSV_ERROR;
	memcpy(intrinsic, this->param, sizeof(double)*INTRIN_PARAM_NUM);
	return CSV_ERROR;
}

/*
*@brief  设置参数
*@author HORSETAIL
*@param[out] 
*@param[in]  const double * intrinsic  相机内参数
*@return     int  
*/
CsvStatus IntrinsicParam::setParam(const double* param)
{
	if (NULL == param) return CSV_ERROR;
	memcpy(this->param, param, sizeof(double)*INTRIN_PARAM_NUM);
	return CSV_ERROR;
}

/*
*@brief  计算像素级的焦距f,计算公式：f_pixel = focus * sqrt(imw^2+imh^2)/sqrt(chipw^2+chiph^2)
*@author HORSETAIL
*@param[out] 
*@param[in]  double & f  
*@return     int  
*/
CsvStatus IntrinsicParam::focusPixel(double& f_pixel)
{
	double chip_diag_len = sqrt(chipw*chipw + chiph * chiph);
	if (chip_diag_len > 1e-10) {
		f_pixel = focus * sqrt(imw*imw + imh * imh) / chip_diag_len;
	}
	else
	{
		return CSV_ERROR;
	}
	return CSV_OK;
}

/*
*@brief  计算矫正后的内参
*@author HORSETAIL
*@param[out] 
*@param[in]  IntrinsicParam & undistParam  
*@return     CsvStatus  
*/
CsvStatus IntrinsicParam::undistortParam(IntrinsicParam& undistParam)
{
	if (imw == 0 || imh == 0)
		return ERR_INTRINSIC_IMSIZE;

	// If you set yourself to output
	if (this == &undistParam)
		return ERR_SAME_INTRINSIC_PARAM;

	const double* parm = this->param;

	// Store principal coordinates
	Point2d centerPoint(parm[2], parm[3]);
	double focuslen = parm[0];

	int samplePointNum = 8;
	Point2d samplePoint[] = { Point2d(0, 0),// Calculate the whereabouts of the upper left of the input image
		Point2d(imw, 0),					// Calculate the whereabouts of the upper right of the input image
		Point2d(0, imh),					// Calculate the whereabouts of the lower left of the input image
		Point2d(imw, imh),					// Calculate the whereabouts of the bottom right of the input image
		Point2d(centerPoint.x, 0),			// Calculate the whereabouts on the input image
		Point2d(0, centerPoint.y),			// Calculate the whereabouts of the left of the input image
		Point2d(imw, centerPoint.y),		// Calculate the right whereabouts of the input image
		Point2d(centerPoint.x, imh) };		// Calculate the whereabouts below the input image

	Point2d undistTL(1e+8, 1e+8), undistBR(-1e+8, -1e+8);
	Point2d undistPoint;					// Calculate coordinates after correction
	CsvStatus status;
	for (int i = 0; i < samplePointNum; ++i) {
		if ((status = undistort(samplePoint[i], undistPoint)) != CSV_OK) {
			return status;
		}

		if (undistTL.x > undistPoint.x) {
			undistTL.x = undistPoint.x;
		}
		if (undistTL.y > undistPoint.y) {
			undistTL.y = undistPoint.y;
		}
		if (undistBR.x < undistPoint.x) {
			undistBR.x = undistPoint.x;
		}
		if (undistBR.y < undistPoint.y) {
			undistBR.y = undistPoint.y;
		}
	}

	// Convert to digital coordinate system
	undistTL = undistTL * focuslen + centerPoint;
	undistBR = undistBR * focuslen + centerPoint;

	// Calculate optical axis points after correction
	Point2d newCenter;
	newCenter = centerPoint - undistTL;

	// initialize with internal parameters
	undistParam.init();

	// Set the optical axis point and focal length after the correction
	double parmNew[10];
	parmNew[0] = parm[0]; parmNew[1] = parm[0]; parmNew[2] = newCenter.x; parmNew[3] = newCenter.y; parmNew[4] = 0;
	parmNew[5] = 0; parmNew[6] = 0; parmNew[7] = 0; parmNew[8] = 0; parmNew[9] = 0;

	undistParam.setParam(parmNew);

	// Calculate size after correction
	undistParam.imw = (int)(undistBR.x + 0.5) - (int)(undistTL.x + 0.5);
	undistParam.imh = (int)(undistBR.y + 0.5) - (int)(undistTL.y + 0.5);
	undistParam.chiph = this->chiph;
	undistParam.chipw = this->chipw;
	undistParam.focus = this->focus;

	return CSV_OK;
}

/*
*@brief  将校正后的坐标点转化为矫正前的坐标点
*@author HORSETAIL
*@param[out] 
*@param[in]  const Point2d& undist	矫正后的坐标点
*@param[in]  Point2d & dist			矫正前的坐标点
*@return     CsvStatus  
*/
CsvStatus IntrinsicParam::distort(const Point2d &undist, Point2d &dist)
{
	// get focus length
	double focus[] = { param[INTRINSIC_FOCUS_IDX], param[INTRINSIC_FOCUS_IDX + 1] };
	if (focus[0] <= 0.0 || focus[1] <= 0.0) {
		return ERR_ILLEGAL_INTRINSIC_PARAM;
	}

	// get the optical axis point
	double center[] = { param[INTRINSIC_CENTER_IDX], param[INTRINSIC_CENTER_IDX + 1] };

	// get skew factor
	double skew = param[INTRINSIC_SKEW_IDX];

	// get circular strain factor
	double radial[] = { param[INTRINSIC_RADIAL_IDX],
		param[INTRINSIC_RADIAL_IDX + 1],
		param[INTRINSIC_RADIAL_IDX + 2]
	};

	// r^2 = x^2 + y^2
	double r2 = undist.x*undist.x + undist.y*undist.y;
	double delta_radial = radial[0] * r2 + radial[1] * r2*r2 + radial[2] * r2*r2*r2;

	// get linear strain factor
	double tangential[] = { param[INTRIN_PARAM_TANGENTIAL_IDX], param[INTRIN_PARAM_TANGENTIAL_IDX + 1] };

	// stores digital coordinates after distorting
	// u = fu*x + u0 + b*y + fu*x*(k0 + k1*r^2 + k2*r^4 + k3*r^6) + p1*(r^2+2*x^2) + 2*p2*x*y
	dist.x = focus[0] * undist.x + center[0] + skew * undist.y + focus[0] * undist.x*delta_radial
		+ tangential[0] * (r2 + 2 * undist.x*undist.x) + 2 * tangential[1] * undist.x*undist.y;
	// v = fv*y + v0 + fv*y*(k0 + k1*r^2 + k2*r^4 + k3*r^6) + p2*(r^2 + 2*y^ 2) + 2*x*p1*x*y
	dist.y = focus[1] * undist.y + center[1] + focus[1] * undist.y*delta_radial
		+ tangential[1] * (r2 + 2 * undist.y*undist.y) + 2 * tangential[0] * undist.x*undist.y;

	// Check if input points are out of range of image threshold times
	// Maximum normalized x-coordinate calculation for points in x direction
	double xmax = (imw*INTRINSIC_IMSIZE_RATIO_THR - center[0]) / focus[0];
	// -Minimum normalized x-coordinate computation of points in the x direction
	double xmin = (imw*(1.0 - INTRINSIC_IMSIZE_RATIO_THR) - center[0]) / focus[0];
	// Maximum normalized y-coordinate calculation of points in y direction
	double ymax = (imh*INTRINSIC_IMSIZE_RATIO_THR - center[1]) / focus[1];
	// -Minimum normalized y-coordinate calculation for point in y direction
	double ymin = (imh*(1.0 - INTRINSIC_IMSIZE_RATIO_THR) - center[1]) / focus[1];
	// Return warning if out of bounds
	if (undist.x > xmax || undist.x < xmin || undist.y > ymax || undist.y < ymin) {
		return CSV_ERROR;
	}

	return CSV_OK;
}

/*
*@brief  将校正后的坐标点转化为矫正前的坐标点
*@author HORSETAIL
*@param[out]
*@param[in]  const Point2d& undist	矫正后的坐标点
*@param[in]  Point2d & dist			矫正前的坐标点
*@return     CsvStatus
*@notes The coordinates before the correction are the digital(image) coordinates, 
the coordinates after the correction are normalized coordinates
*/
CsvStatus IntrinsicParam::undistort(const Point2d &dist, Point2d &undist)
{
	// Get focal length
	double focus[] = { param[INTRINSIC_FOCUS_IDX], param[INTRINSIC_FOCUS_IDX + 1] };
	if (focus[0] <= 0.0 || focus[1] <= 0.0) {
		return ERR_ILLEGAL_INTRINSIC_PARAM;
	}

	// Get the optical axis point
	double center[] = { param[INTRINSIC_CENTER_IDX], param[INTRINSIC_CENTER_IDX + 1] };

	// Get skew factor
	double skew = param[INTRINSIC_SKEW_IDX];

	// Get circular strain factor
	double radial[] = { param[INTRINSIC_RADIAL_IDX],
		param[INTRINSIC_RADIAL_IDX + 1],
		param[INTRINSIC_RADIAL_IDX + 2]
	};

	// Get linear strain factor
	double tangential[] = { param[INTRINSIC_TANGENTIAL_IDX], param[INTRINSIC_TANGENTIAL_IDX + 1] };

	// Calculating the default value of normalized coordinates
	double undistPoint[] = { (dist.x - center[0]) / focus[0], (dist.y - center[1]) / focus[1] };

	double dDelta = 1e-8;
	double dDistort[2];
	double dDiff[4];
	double dDenominator;
	double DiffPoint[2];

	for (int i = 0; i < INTRINSIC_DISTORTED_MAX_LOOP_NUM; ++i) {
		// Distorted and calculated
		double dr2 = undistPoint[0] * undistPoint[0] + undistPoint[1] * undistPoint[1];
		double radialOffset = radial[0] * dr2 + radial[1] * dr2*dr2 + radial[2] * dr2*dr2*dr2;
		dDistort[0] = dist.x - (focus[0] * undistPoint[0] + center[0] + skew * undistPoint[1] + focus[0] * undistPoint[0] * radialOffset + tangential[0] * (dr2 + 2 * undistPoint[0] * undistPoint[0]) + 2 * tangential[1] * undistPoint[0] * undistPoint[1]);
		dDistort[1] = dist.y - (focus[1] * undistPoint[1] + center[1] + focus[1] * undistPoint[1] * radialOffset + tangential[1] * (dr2 + 2 * undistPoint[1] * undistPoint[1]) + 2 * tangential[0] * undistPoint[0] * undistPoint[1]);

		// Differential calculation
		DiffPoint[0] = undistPoint[0] + dDelta;
		DiffPoint[1] = undistPoint[1];
		dr2 = DiffPoint[0] * DiffPoint[0] + DiffPoint[1] * DiffPoint[1];
		radialOffset = radial[0] * dr2 + radial[1] * dr2*dr2 + radial[2] * dr2*dr2*dr2;
		dDiff[0] = (dist.x - (focus[0] * DiffPoint[0] + center[0] + skew * DiffPoint[1] + focus[0] * DiffPoint[0] * radialOffset + tangential[0] * (dr2 + 2 * DiffPoint[0] * DiffPoint[0]) + 2 * tangential[1] * DiffPoint[0] * DiffPoint[1]) - dDistort[0]) / dDelta;
		dDiff[2] = (dist.y - (focus[1] * DiffPoint[1] + center[1] + focus[1] * DiffPoint[1] * radialOffset + tangential[1] * (dr2 + 2 * DiffPoint[1] * DiffPoint[1]) + 2 * tangential[0] * DiffPoint[0] * DiffPoint[1]) - dDistort[1]) / dDelta;

		DiffPoint[0] = undistPoint[0];
		DiffPoint[1] = undistPoint[1] + dDelta;
		dr2 = DiffPoint[0] * DiffPoint[0] + DiffPoint[1] * DiffPoint[1];
		radialOffset = radial[0] * dr2 + radial[1] * dr2*dr2 + radial[2] * dr2*dr2*dr2;
		dDiff[1] = (dist.x - (focus[0] * DiffPoint[0] + center[0] + skew * DiffPoint[1] + focus[0] * DiffPoint[0] * radialOffset + tangential[0] * (dr2 + 2 * DiffPoint[0] * DiffPoint[0]) + 2 * tangential[1] * DiffPoint[0] * DiffPoint[1]) - dDistort[0]) / dDelta;
		dDiff[3] = (dist.y - (focus[1] * DiffPoint[1] + center[1] + focus[1] * DiffPoint[1] * radialOffset + tangential[1] * (dr2 + 2 * DiffPoint[1] * DiffPoint[1]) + 2 * tangential[0] * DiffPoint[0] * DiffPoint[1]) - dDistort[1]) / dDelta;

		// Calculate determinant
		dDenominator = dDiff[0] * dDiff[3] - dDiff[1] * dDiff[2];
		if (dDenominator == 0.0) {
			return ERR_INTRINSIC_DENOMINATOR_ZERO;
		}
		undistPoint[0] += (dDiff[1] * dDistort[1] - dDiff[3] * dDistort[0]) / dDenominator;
		undistPoint[1] += (dDiff[2] * dDistort[0] - dDiff[0] * dDistort[1]) / dDenominator;
	}
	undist.x = undistPoint[0];
	undist.y = undistPoint[1];

	// Check if input points are out of range of image threshold times
	// Maximum normalized x-coordinate calculation for points in x direction
	double xmax = imw * INTRINSIC_IMSIZE_RATIO_THR;
	// -Minimum normalized x-coordinate computation of points in the x direction
	double xmin = imw - xmax;
	// Maximum normalized y-coordinate calculation of points in y direction
	double ymax = imh * INTRINSIC_IMSIZE_RATIO_THR;
	// -Minimum normalized y-coordinate calculation for point in y direction
	double ymin = imh - ymax;
	// Return warning if out of bounds
	if (dist.x > xmax || dist.x < xmin || dist.y > ymax || dist.y < ymin) {
		return CSV_ERROR;
	}

	return CSV_OK;
}


	/*
	*@brief  初始化参数
	*@author HORSETAIL
	*@param[out] 
	*@return     int  状态返回值
	*/

	CsvStatus IntrinsicParam::init()
	{
		focus = 0.0;
		chiph = 0.0;
		chipw = 0.0;
		imh = 0;
		imw = 0;

		for (int i = 0; i < INTRIN_PARAM_NUM; ++i) {
			param[i] = 0.0;
		}
		return CSV_OK;
	}

}