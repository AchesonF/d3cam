#ifndef _CSV_CAMERA_PARAM_H
#define _CSV_CAMERA_PARAM_H


#include <string>
#include <vector>
#include "IntrinsicParam.h"
#include "ExtrinsicParam.h"

namespace CSV {
	class CameraParam
	{
	public:
		CameraParam();
		
		/*
		*@brief  �����������
		*@author HORSETAIL
		*@param[out] 
		*@param[in]  const CameraParam & param  
		*@return     int  
		*/
		int set(const CameraParam& param);

		/*
		*@brief  �����������
		*@author HORSETAIL
		*@param[out] 
		*@param[in]  const IntrinsicParam & inparam  
		*@param[in]  const IntrinsicParam & exparam  
		*@return     int  
		*/
		int set(const IntrinsicParam& inparam, const ExtrinsicParam& exparam);

		/*
		*@brief  
		*@author HORSETAIL
		*@param[out] 
		*@param[in]  IntrinsicParam & inparam  
		*@return     int  
		*/
		int setIntr(const IntrinsicParam& inparam);

		/*
		*@brief  
		*@author HORSETAIL
		*@param[out] 
		*@param[in]  ExtrinsicParam & exparam  
		*@return     int  
		*/
		int setExtr(const ExtrinsicParam& exparam);

	public:
		// ����ڲ�
		IntrinsicParam inparam;
		// ������
		ExtrinsicParam exparam;
	};
}

#endif // !_CSV_CAMERA_PARAM_H
