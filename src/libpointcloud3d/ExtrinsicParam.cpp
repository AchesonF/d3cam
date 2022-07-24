#include "ExtrinsicParam.h"

namespace CSV
{
	ExtrinsicParam::ExtrinsicParam(void)
	{
	}

	ExtrinsicParam::ExtrinsicParam(const Mat33& rotMat, const Point3d& transVec)
	{
		rotmat = rotMat;
		trans = transVec;
	}

	ExtrinsicParam::ExtrinsicParam(const ExtrinsicParam& param)
	{
		*this = param;
	}

	ExtrinsicParam::~ExtrinsicParam(void)
	{
	}

	bool ExtrinsicParam::operator==(const ExtrinsicParam& param) const
	{
		return (rotmat == param.rotmat) && (trans == param.trans);
	}

	bool ExtrinsicParam::operator!=(const ExtrinsicParam& param) const
	{
		return !(*this == param);
	}


	void ExtrinsicParam::init(void)
	{
		rotmat.init();
	}

	ExtrinsicParam& ExtrinsicParam::operator=(const ExtrinsicParam& param)
	{
		if (this != &param) {
			(*this).rotmat = param.rotmat;
			(*this).trans = param.trans;
		}
		return *this;
	}

	const Mat33& ExtrinsicParam::getRotMat() const
	{
		return this->rotmat;
	};

	void ExtrinsicParam::setRotMat(const Mat33& mat)
	{
		rotmat = mat;
	};

	const Point3d& ExtrinsicParam::getTrans() const
	{
		return trans;
	};

	void ExtrinsicParam::setTrans(const Point3d& vec)
	{
		trans = vec;
	};

	CsvStatus ExtrinsicParam::transform(const Point3d& source, Point3d& dest) const
	{
		if (source.x == NOT_3DPOINT_VALUE && source.y == NOT_3DPOINT_VALUE && source.z == NOT_3DPOINT_VALUE) {
			// The point that is not calculated is output as it is
			dest = source;
		}
		else {
			dest = getRotMat()*source + getTrans();
		}
		return CSV_OK;
	}

	int ExtrinsicParam::set(double *extrinsic)
	{
		if (extrinsic == NULL)
		{
			return CSV_ERROR;
		}
		rotmat.init(extrinsic[0], extrinsic[1], extrinsic[2],
			extrinsic[3], extrinsic[4], extrinsic[5],
			extrinsic[6], extrinsic[7], extrinsic[8]);
		trans.x = extrinsic[9];
		trans.y = extrinsic[10];
		trans.z = extrinsic[11];

		return 0;
	}

}