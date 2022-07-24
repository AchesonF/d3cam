#ifndef _EXTRINSIC_PARAM_H
#define _EXTRINSIC_PARAM_H


#include "CsvType.hpp"
namespace CSV
{
	class ExtrinsicParam
	{
	public:
		ExtrinsicParam(void);
		ExtrinsicParam(const Mat33&, const Point3d&);
		ExtrinsicParam(const ExtrinsicParam&);
		ExtrinsicParam& operator=(const ExtrinsicParam&);
		virtual ~ExtrinsicParam(void);

		virtual void init(void);

		bool operator==(const ExtrinsicParam& param) const;
		bool operator!=(const ExtrinsicParam& param) const;

		const Mat33& getRotMat() const;
		void setRotMat(const Mat33& mat);
		int set(double *extrinsic);

		const Point3d& getTrans() const;
		void setTrans(const Point3d& vec);

		CsvStatus transform(const Point3d& source, Point3d& dest) const;
		
	private:
		Mat33 rotmat;
		Point3d trans;
	};

}

#endif // !_EXTRINSIC_PARAM_H
