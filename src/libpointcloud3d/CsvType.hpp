#ifndef _CSV_TYPE_HPP
#define _CSV_TYPE_HPP


#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <vector>

#ifndef uchar
typedef unsigned char uchar;
#endif // !uchar

namespace CSV {

#define CREATE_GRAYCODE_PATTERN 0
#define CREATE_PHASESHIFT_PATTERN 0

#define USE_OPENCV 1
#define SHOW_DEBUG_WRAPPHASE 0
#define SHOW_DEBUG_BINARY 1
#define SHOW_DEBUG_UNWRAPPHASE 0

#define SAVE_GRAYCODE_PATTERN 0
#define SAVE_PHASESHIFT_PATTERN 0
#define SHOW_GRAYCODE_PATTERN 0
#define SHOW_PHASESHIFT_PATTERN 0

//////////////////////////////////////////////////////////////////////////
/// 格雷相移算法宏定义
#define  CSV_PI		  3.14159265358979323846
#define  CSV_PI_HALF  1.57079632679489661923
#define  CSV_PI_3HALF 4.71238898038468985769
#define  CSV_PI_4HALF 6.28318530717958647692
	enum StructureLightType
	{
		CSV_GRAYCODE7_PHASE4 = 0
	};

#define IM_WIDTH_1616 1616
#define IM_HEIGHT_1240 1240



#define IM_PRJ_WIDTH_1920 1920
#define IM_PRJ_HEIGHT_1080 1080
#define IM_PRJ_SIZE 2073600

#define IM_WIDTH IM_WIDTH_1616
#define IM_HEIGHT IM_HEIGHT_1240
#define IM_SIZE 2003840

#define IM_PRJ_WIDTH IM_PRJ_WIDTH_1920
#define IM_PRJ_HEIGHT IM_PRJ_HEIGHT_1080

#define WHITE_BLACK_NUM  2  //白 黑 共 2 个
#define GRAY_CODE_NUM 7     //格雷码图 共 7 个
#define PHASE_SHIFT_NUM 4   //相移图 共 4 个

//////////////////////////////////////////////////////////////////////////
	///内参数
#define INTRINSIC_DISTORTED_MAX_LOOP_NUM		5
#define INTRINSIC_FOCUS_IDX			0
#define INTRINSIC_CENTER_IDX		2
#define INTRINSIC_SKEW_IDX			4
#define INTRINSIC_RADIAL_IDX		5
#define INTRINSIC_TANGENTIAL_IDX	8
	
#define INTRINSIC_IMSIZE_RATIO_THR      1.25 // If the image coordinate of the input point is outside the range of the image size threshold times, the calculation result before distortion correction may become unstable.

#define NOT_3DPOINT_VALUE -10000

	enum CsvStatus
	{
		CSV_OK = 0,
		CSV_ERROR = -1,
		
		// 相机内参错误信息
		ERR_ILLEGAL_INTRINSIC_PARAM			= -10001,
		ERR_INTRINSIC_CALC_DENOMINATOR_ZERO	= -10001,
		ERR_INTRINSIC_DENOMINATOR_ZERO		= -10003,
		ERR_INTRINSIC_IMWIDTH				= -10004,
		ERR_INTRINSIC_IMHEIGHT				= -10005,
		ERR_INTRINSIC_IMSIZE				= -10006,
		ERR_SAME_INTRINSIC_PARAM			= -10007,
		ERR_LOAD_CALIB_FILE_FAILED			= -10008,
		ERR_CREATE_LUT_FAILED				= -1009,

		
		ERR_IMAGE_NUM				= -30001,
		ERR_IMAGE_EMPTY				= -30002,
		ERR_IMAGE_RESOLUTION		= -30003,
		ERR_GRAYCODE_IMAGE_NUM		= -30004,
		ERR_PHASE_IMAGE_NUM			= -30005,

		ERR_PTR_NULL				= -40001
	};


	class Point
	{
	public:
		int x;
		int y;
	};

	class Point2d
	{
	public:
		Point2d(double x = 0.0, double y = 0.0)
		{
			this->x = x;
			this->y = y;
		}
		double x;
		double y;

	public:
		Point2d operator+(const Point2d& point)const
		{
			Point2d point2d;
			point2d.x = this->x + point.x;
			point2d.y = this->y + point.y;
			return point2d;
		}
		Point2d operator-(const Point2d& point)const
		{
			Point2d point2d;
			point2d.x = this->x - point.x;
			point2d.y = this->y - point.y;
			return point2d;
		}
		Point2d operator*(const Point2d& point)const
		{
			Point2d point2d;
			point2d.x = this->x * point.x;
			point2d.y = this->y * point.y;
			return point2d;
		}

		Point2d operator*(double factor)const
		{
			Point2d point2d;
			point2d.x = this->x * factor;
			point2d.y = this->y * factor;
			return point2d;
		}
	};

	class Point3d
	{
	public:
		Point3d(double x = 0.0, double y = 0.0, double z = 0.0)
		{
			this->x = x;
			this->y = y;
			this->z = z;
		}

		Point3d operator+(const Point3d& point)const
		{
			Point3d point3d;
			point3d.x = this->x + point.x;
			point3d.y = this->y + point.y;
			point3d.z = this->z + point.z;
			return point3d;
		}
		Point3d operator-(const Point3d& point)const
		{
			Point3d point3d;
			point3d.x = this->x - point.x;
			point3d.y = this->y - point.y;
			point3d.z = this->z - point.z;
			return point3d;
		}
		Point3d operator*(const Point3d& point)const
		{
			Point3d point3d;
			point3d.x = this->x * point.x;
			point3d.y = this->y * point.y;
			point3d.z = this->z * point.z;
			return point3d;
		}

		Point3d operator*(double factor)const
		{
			Point3d point3d;
			point3d.x = this->x * factor;
			point3d.y = this->y * factor;
			point3d.z = this->z * factor;
			return point3d;
		}

		bool operator==(const Point3d& point) const {
			return point.x == this->x && point.y == this->y && point.z == this->z;
		}

	public:
		double x;
		double y;
		double z;
	};

	class PointCloud
	{
	public:
		int width;		// 图像宽度
		int height;		// 图像高度
		std::vector<int> x, y;		// 坐标索引x，y
		std::vector<double> data;	// 深度图
		std::vector<int> alpah;		// 透明度

	private:

	};

	class Mat33
	{
	public:
		Mat33(void) { init(); };
		Mat33(double m00, double m01, double m02, double m10, double m11, double m12, double m20, double m21, double m22) { data[0] = m00; data[1] = m01; data[2] = m02; data[3] = m10; data[4] = m11; data[5] = m12; data[6] = m20; data[7] = m21; data[8] = m22; };
		Mat33(const double mat[9]) { memcpy(data, mat, 9 * sizeof(double)); };
		Mat33(const Mat33& mat) { memcpy(data, mat.data, 9 * sizeof(double)); };
		Mat33& operator=(const Mat33& mat) { memmove(data, mat.data, 9 * sizeof(double)); return *this; };
		~Mat33(void) {};

		void init(void) { data[0] = data[4] = data[8] = 1.0; data[1] = data[2] = data[3] = data[5] = data[6] = data[7] = 0.0; };
		void init(double* data) { memmove(this->data, data, 9 * sizeof(double)); }
		void init(double m00, double m01, double m02, double m10, double m11, double m12, double m20, double m21, double m22) { data[0] = m00; data[1] = m01; data[2] = m02; data[3] = m10; data[4] = m11; data[5] = m12; data[6] = m20; data[7] = m21; data[8] = m22; };

		const double& operator()(int row, int col) const { if (row < 0 || row >= 3 || col < 0 || col >= 3) { printf("const double& Mat33::operator()(int row, int col) const : Index Out of Range"); } return data[row * 3 + col]; };
		double& operator()(int row, int col) { if (row < 0 || row >= 3 || col < 0 || col >= 3) { printf("double& Mat33::operator()(int row, int col) : Index Out of Range"); } return data[row * 3 + col]; };
		const double& operator()(int index) const { if (index < 0 || index >= 9) { printf("const double& Mat33::operator()(int index) const : Index Out of Range"); } return data[index]; };
		double& operator()(int index) { if (index < 0 || index >= 9) { printf("double& Mat33::operator()(int index) : Index Out of Range"); } return data[index]; };

		Point3d getRow(int rowIndex)const { if (0 <= rowIndex && 2 >= rowIndex) { return Point3d(data[3 * rowIndex], data[3 * rowIndex + 1], data[3 * rowIndex + 2]); } return Point3d(data[0], data[1], data[2]); }
		Point3d getCol(int colIndex)const { if (0 <= colIndex && 2 >= colIndex) { return Point3d(data[colIndex], data[colIndex + 3], data[colIndex + 6]); } return Point3d(data[0], data[3], data[6]); }
		Point3d getDiag()const { return Point3d(data[0], data[4], data[8]); }

		bool operator==(const Mat33& mat) const { return (data[0] == mat.data[0]) && (data[1] == mat.data[1]) && (data[2] == mat.data[2]) && (data[3] == mat.data[3]) && (data[4] == mat.data[4]) && (data[5] == mat.data[5]) && (data[6] == mat.data[6]) && (data[7] == mat.data[7]) && (data[8] == mat.data[8]); };
		bool operator!=(const Mat33& mat) const { return (data[0] != mat.data[0]) || (data[1] != mat.data[1]) || (data[2] != mat.data[2]) || (data[3] != mat.data[3]) || (data[4] != mat.data[4]) || (data[5] != mat.data[5]) || (data[6] != mat.data[6]) || (data[7] != mat.data[7]) || (data[8] != mat.data[8]); };

		Mat33& operator+=(const Mat33& mat) { data[0] += mat.data[0]; data[1] += mat.data[1]; data[2] += mat.data[2]; data[3] += mat.data[3]; data[4] += mat.data[4]; data[5] += mat.data[5]; data[6] += mat.data[6]; data[7] += mat.data[7]; data[8] += mat.data[8]; return *this; };
		Mat33& operator-=(const Mat33& mat) { data[0] -= mat.data[0]; data[1] -= mat.data[1]; data[2] -= mat.data[2]; data[3] -= mat.data[3]; data[4] -= mat.data[4]; data[5] -= mat.data[5]; data[6] -= mat.data[6]; data[7] -= mat.data[7]; data[8] -= mat.data[8]; return *this; };
		Mat33& operator*=(double s) { data[0] *= s; data[1] *= s; data[2] *= s; data[3] *= s; data[4] *= s; data[5] *= s; data[6] *= s; data[7] *= s; data[8] *= s; return *this; };
		Mat33& operator*=(const Mat33& mat) {
			Mat33 temp(*this);
			data[0] = temp.data[0] * mat.data[0] + temp.data[1] * mat.data[3] + temp.data[2] * mat.data[6];
			data[1] = temp.data[0] * mat.data[1] + temp.data[1] * mat.data[4] + temp.data[2] * mat.data[7];
			data[2] = temp.data[0] * mat.data[2] + temp.data[1] * mat.data[5] + temp.data[2] * mat.data[8];
			data[3] = temp.data[3] * mat.data[0] + temp.data[4] * mat.data[3] + temp.data[5] * mat.data[6];
			data[4] = temp.data[3] * mat.data[1] + temp.data[4] * mat.data[4] + temp.data[5] * mat.data[7];
			data[5] = temp.data[3] * mat.data[2] + temp.data[4] * mat.data[5] + temp.data[5] * mat.data[8];
			data[6] = temp.data[6] * mat.data[0] + temp.data[7] * mat.data[3] + temp.data[8] * mat.data[6];
			data[7] = temp.data[6] * mat.data[1] + temp.data[7] * mat.data[4] + temp.data[8] * mat.data[7];
			data[8] = temp.data[6] * mat.data[2] + temp.data[7] * mat.data[5] + temp.data[8] * mat.data[8];
			return *this;
		};
		Mat33& operator/=(double s) {
			if (s != 0.0) {
				double inv = 1.0 / s;
				inv = 2.0 * inv - s * inv * inv;
				inv = 2.0 * inv - s * inv * inv;
				data[0] *= inv; data[1] *= inv; data[2] *= inv; data[3] *= inv; data[4] *= inv; data[5] *= inv; data[6] *= inv; data[7] *= inv; data[8] *= inv;
				return *this;
			}
			else {
				printf("Mat33& Mat33::operator/=(double s) : Zero Division");
				return *this;
			}
		};
		//friend Mat33 operator*(double s, const Mat33& mat);
		Mat33 operator+(const Mat33& mat) const { return Mat33(data[0] + mat.data[0], data[1] + mat.data[1], data[2] + mat.data[2], data[3] + mat.data[3], data[4] + mat.data[4], data[5] + mat.data[5], data[6] + mat.data[6], data[7] + mat.data[7], data[8] + mat.data[8]); };
		Mat33 operator-(const Mat33& mat) const { return Mat33(data[0] - mat.data[0], data[1] - mat.data[1], data[2] - mat.data[2], data[3] - mat.data[3], data[4] - mat.data[4], data[5] - mat.data[5], data[6] - mat.data[6], data[7] - mat.data[7], data[8] - mat.data[8]); };
		Mat33 operator*(const Mat33& mat) const {
			return Mat33(data[0] * mat.data[0] + data[1] * mat.data[3] + data[2] * mat.data[6], data[0] * mat.data[1] + data[1] * mat.data[4] + data[2] * mat.data[7], data[0] * mat.data[2] + data[1] * mat.data[5] + data[2] * mat.data[8],
				data[3] * mat.data[0] + data[4] * mat.data[3] + data[5] * mat.data[6], data[3] * mat.data[1] + data[4] * mat.data[4] + data[5] * mat.data[7], data[3] * mat.data[2] + data[4] * mat.data[5] + data[5] * mat.data[8],
				data[6] * mat.data[0] + data[7] * mat.data[3] + data[8] * mat.data[6], data[6] * mat.data[1] + data[7] * mat.data[4] + data[8] * mat.data[7], data[6] * mat.data[2] + data[7] * mat.data[5] + data[8] * mat.data[8]);
		};
		Point3d operator*(const Point3d& vec) const { return Point3d(data[0] * vec.x + data[1] * vec.y + data[2] * vec.z, data[3] * vec.x + data[4] * vec.y + data[5] * vec.z, data[6] * vec.x + data[7] * vec.y + data[8] * vec.z); };
		Mat33 operator*(double s) const { return Mat33(data[0] * s, data[1] * s, data[2] * s, data[3] * s, data[4] * s, data[5] * s, data[6] * s, data[7] * s, data[8] * s); };
		Mat33 operator/(double s) const {
			if (s != 0.0) {
				double inv = 1.0 / s;
				inv = 2.0 * inv - s * inv * inv;
				inv = 2.0 * inv - s * inv * inv;
				return Mat33(data[0] * inv, data[1] * inv, data[2] * inv, data[3] * inv, data[4] * inv, data[5] * inv, data[6] * inv, data[7] * inv, data[8] * inv);
			}
			else {
				printf("Mat33 Mat33::operator/(double s) const : Zero Division");
				return *this;
			}
		};
		Mat33 operator-(void) const { return Mat33(-data[0], -data[1], -data[2], -data[3], -data[4], -data[5], -data[6], -data[7], -data[8]); };

		double getNorm(void) const { return sqrt(data[0] * data[0] + data[1] * data[1] + data[2] * data[2] + data[3] * data[3] + data[4] * data[4] + data[5] * data[5] + data[6] * data[6] + data[7] * data[7] + data[8] * data[8]); };
		double getNormInv(void) const {
			double norm = getNorm();
			if (norm != 0.0) {
				double inv = 1.0 / norm;
				inv = 2.0 * inv - norm * inv * inv;
				return (2.0 * inv - norm * inv * inv);
			}
			else {
				printf("double Mat33::::getNormInv(void) const : Zero Division");
				return 0.0;
			}
		};
		double getTrace(void) const { return data[0] + data[4] + data[8]; };
		double getDeterminant(void) const { return data[0] * data[4] * data[8] + data[1] * data[5] * data[6] + data[2] * data[3] * data[7] - data[0] * data[5] * data[7] - data[1] * data[3] * data[8] - data[2] * data[4] * data[6]; };

		const Mat33 &norm(void) { *this *= getNormInv(); return *this; };
		bool getNormMat(Mat33 &nmx)const { double invNorm = getNormInv(); if (invNorm <= 0.0) return false; nmx = *this * invNorm; return true; }
		Mat33 getNormMat(void) const { return *this * getNormInv(); };

		const Mat33 & t(void){ double temp = data[1]; data[1] = data[3]; data[3] = temp; temp = data[2]; data[2] = data[6]; data[6] = temp; temp = data[5]; data[5] = data[7]; data[7] = temp; return *this; };
		Mat33 t(void) const { return Mat33(data[0], data[3], data[6], data[1], data[4], data[7], data[2], data[5], data[8]); };

		//const Mat33 &inv(void);
		//bool getInv(Mat33& rInvMat) const;
		//Mat33 getInv(void) const;

		//CSV_STATE calEigenValueVector(CsvVectorND* eigenValue, CsvVectorND* eigenVector) const;
		//CSV_STATE SVD(Mat33* leftOrthogonalMatrix, Mat33* diagonalMatrix, Mat33* rightOrthogonalMatrix) const;

	private:
		double data[9];
	};

}

#endif // !_CSV_TYPE_HPP