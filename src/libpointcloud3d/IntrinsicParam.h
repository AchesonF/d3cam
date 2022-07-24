#ifndef _INTRINSIC_PARAM_H
#define _INTRINSIC_PARAM_H


#include "CsvType.hpp"

namespace CSV
{
	#define INTRIN_PARAM_NUM			10
	#define INTRIN_PARAM_FOCUS_IDX		0
	#define INTRIN_PARAM_PRINCIPAL_IDX	2
	#define INTRIN_PARAM_SKEW_IDX		4
	#define INTRIN_PARAM_RADIAL_IDX		5
	#define INTRIN_PARAM_TANGENTIAL_IDX	8

	#define CSV_EPSILON 1e-14
	class IntrinsicParam
	{
	public:
		IntrinsicParam();
		IntrinsicParam(const IntrinsicParam& param);
		IntrinsicParam& operator=(const IntrinsicParam& param);
		virtual ~IntrinsicParam(void);

		bool operator==(const IntrinsicParam& param) const;
		bool operator!=(const IntrinsicParam& param) const;

		CsvStatus getInfo(double& chipw, double& chiph, int& imw, int& imh, double& focus, double& rms);
		CsvStatus setInfo(double chipw, double chiph, int imw, int imh, double focus, double rms);

		CsvStatus getParam(double* param);
		CsvStatus setParam(const double* param);

		CsvStatus focusPixel(double& f);
		CsvStatus undistortParam(IntrinsicParam& undistParam);

		CsvStatus distort(const Point2d& undist, Point2d& dist);
		CsvStatus undistort(const Point2d& dist, Point2d& undist);

	private:
		CsvStatus init();
	public:
		double	rms;	// �����ͶӰ���
		double	chiph;	// оƬ�߶ȣ���λ��mm
		double	chipw;	// оƬ��ȣ���λ��mm
		int		imh;	// ͼ��߶ȣ���λ��pixel
		int		imw;	// ͼ���ȣ���λ��pixel
		double	focus;	// ��ͷ���࣬��λ��mm
		// -          -
		// | fx s  cx |
		// | 0  fx cy |
		// | 0  0  1  |
		// -          - 
		// 
		double	param[INTRIN_PARAM_NUM];	// �ڲΣ�����˳��fx, fy, cx, cy, s, k1, k2, k3, p1, p2
	};
}

#endif // !_INTRINSIC_PARAM_H
