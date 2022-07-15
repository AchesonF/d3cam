#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static uint32_t xml_strlcpy (char *destination, const char *source, const uint32_t size)
{
	if (0 < size) {
		snprintf(destination, size, "%s", source);
		destination[size - 1] = '\0';
	}

	return strlen(source);
}

static void xml_unload_file (xmlDocPtr pDoc)
{
	struct csv_xml_t *pXML = &gCSV->xml;
	if (pXML->SaveFile) {
		pXML->SaveFile = false;
		xmlSaveFormatFileEnc(pXML->file, pDoc, "UTF-8", 1);
		sync();
	}

	xmlFreeDoc(pDoc);
}

static int xml_load_file (struct csv_xml_t *pXML)
{
	if (pXML->file == NULL) {
		log_info("ERROR : xml file NULL.");
		pXML->file = FILE_PATH_XML;
	}

	char cmd[256] = {0};
	memset(cmd, 0, 256);

	if (!csv_file_isExist(pXML->file)) {
		log_info("WARN : xml \"%s\" not exist.", pXML->file);
		if (!csv_file_isExist(FILE_PATH_XML_BK)) {
			log_info("WARN : backup xml \"%s\" not exist.", FILE_PATH_XML_BK);
		} else {
			log_info("WARN : use backup xml \"%s\".", FILE_PATH_XML_BK);
			sprintf(cmd, "cp -f %s %s", FILE_PATH_XML_BK, FILE_PATH_XML);
			system_redef(cmd);
			sync();
		}
	}

	//xmlLineNumbersDefault(1);
	xmlKeepBlanksDefault(0);
	xmlIndentTreeOutput = 1;

	pXML->pDoc = xmlReadFile(pXML->file, "UTF-8", XML_PARSE_NOBLANKS);
	if (pXML->pDoc == NULL) {
		log_info("WARN : xmlReadFile \"%s\", make a new one.", pXML->file);

		sprintf(cmd, "echo \\<?xml version=\\\"1.0\\\" " \
			"encoding=\\\"UTF-8\\\"?\\>\\<CSVxmlRoot\\>\\</CSVxmlRoot\\> > %s", 
			FILE_PATH_XML);
		system_redef(cmd);
		sync();

		pXML->pDoc = xmlReadFile(pXML->file, "UTF-8", XML_PARSE_NOBLANKS);
		if (pXML->pDoc == NULL) {
			return -1;
		}
	}

	pXML->pRoot = xmlDocGetRootElement(pXML->pDoc);
	if (pXML->pRoot == NULL) {
		log_info("ERROR : xmlDocGetRootElement NULL");

		goto mk_root;
	}

	if (xmlStrcmp(pXML->pRoot->name, BAD_CAST "CSVxmlRoot")) {
		log_info("ERROR : RootNode not found. \"CSVxmlRoot\" vs \"%s\"", pXML->pRoot->name);
		goto mk_root;
	} else {
		return 0;
	}

  mk_root:

	pXML->pRoot = xmlNewNode(NULL, BAD_CAST "CSVxmlRoot");
	if (pXML->pRoot == NULL) {
		return -1;
	}
	xmlDocSetRootElement(pXML->pDoc, pXML->pRoot);
	pXML->SaveFile = true;

	return 0;
}

static int xml_get_num_nodes (xmlNodePtr pNodePtr, const char *nodeName)
{
	uint32_t num_nodes = 0;
	xmlNodePtr curPtr = NULL;

	if ((pNodePtr == NULL) || (nodeName == NULL)) {
		return 0;
	}

	if (!xmlStrcmp(pNodePtr->name, BAD_CAST nodeName)) {
		num_nodes++;
	}

	for (curPtr = xmlFirstElementChild(pNodePtr); 
		curPtr != NULL; 
		curPtr = xmlNextElementSibling(curPtr)) {
		num_nodes += xml_get_num_nodes(curPtr, nodeName);
	}

	return num_nodes;
}

static xmlNodePtr xml_get_node (xmlNodePtr pNodePtr, const char *nodeName, uint32_t node_index)
{
	uint32_t node_count = 0;
	xmlNodePtr curPtr = NULL;

	if ((pNodePtr == NULL) || (nodeName == NULL)) {
		return NULL;
	}

	if (!xmlStrcmp(pNodePtr->name, BAD_CAST nodeName)) {
		if (node_index == 0) {
			return pNodePtr;
		} else {
			node_index--;
		}
	}

	for (curPtr = xmlFirstElementChild(pNodePtr);
		curPtr != NULL;
		curPtr = xmlNextElementSibling(curPtr)) {
		node_count = xml_get_num_nodes(curPtr, nodeName);
		if (node_count > node_index) {
			return xml_get_node(curPtr, nodeName, node_index);
		} else {
			node_index -= node_count;
		}
	}

	// mk_node if not exist
	log_info("WARN : no tag \"%s\".", nodeName);
	gCSV->xml.SaveFile = true;
	return xmlNewChild(pNodePtr, NULL, BAD_CAST nodeName, NULL);
	//return NULL;
}

int xml_get_node_data (xmlDocPtr pDoc, xmlNodePtr pNodePtr, const char *parentName,
	struct key_value_pair_t *key_pair, uint16_t nums, uint8_t index)
{
	uint16_t i = 0;
	char *str = NULL;
	xmlNodePtr curPtr = NULL;

	if ((pDoc == NULL)||(pNodePtr == NULL)||(parentName == NULL)||(key_pair == NULL)) {
		log_info("ERROR : NULL");
		return 1;
	}

	if (!nums || (nums > MAX_KEY_VALUE_PAIRS)) {
		log_info("ERROR : nums %d", nums);
		return 1;
	}

	pNodePtr = xml_get_node(pNodePtr, parentName, index);
	if (pNodePtr == NULL) {
		return 1;
	}

	for (i = 0; i < nums; i++) {
		if (key_pair[i].value == NULL) {
			log_info("ERROR : NULL");
			return 1;
		}

		if (key_pair[i].nodeType == XML_ELEMENT_NODE) {
			for (curPtr = xmlFirstElementChild(pNodePtr);
  			  curPtr && xmlStrcmp(curPtr->name, BAD_CAST key_pair[i].key);
			  curPtr = xmlNextElementSibling(curPtr)) {
				;
			}

			if (curPtr == NULL) {
				log_info("WARN : no tag \"%s\".", key_pair[i].key);

				xmlNodePtr node = NULL;
				node = xmlNewChild(pNodePtr, NULL, BAD_CAST key_pair[i].key, NULL);
				char str1[512] = { 0 };
				memset(str1, 0, 512);
				switch (key_pair[i].value_type) {
				case XML_VALUE_STRING:
					xml_strlcpy(str1, key_pair[i].value, xmlStrlen(BAD_CAST key_pair[i].value)+1);
				break;
				case XML_VALUE_INT8:
					sprintf(str1, "%d", *((int8_t *) key_pair[i].value));
				break;
				case XML_VALUE_UINT8:
					sprintf(str1, "%u", *((uint8_t *) key_pair[i].value));
				break;
				case XML_VALUE_INT16:
					sprintf(str1, "%d", *((int16_t *) key_pair[i].value));
				break;
				case XML_VALUE_UINT16:
					sprintf(str1, "%u", *((uint16_t *) key_pair[i].value));
				break;
				case XML_VALUE_INT32:
					sprintf(str1, "%d", *((int *) key_pair[i].value));
				break;
				case XML_VALUE_UINT32:
					sprintf(str1, "%u", *((uint32_t *) key_pair[i].value));
				break;
				case XML_VALUE_UINT64:
					sprintf(str1, "%ld", *((uint64_t *) key_pair[i].value));
				break;
				case XML_VALUE_FLOAT:
					sprintf(str1, "%f", *((float *)key_pair[i].value));
				break;
				default:
					log_info("ERROR : Invalid value_type[%d]", key_pair[i].value_type);
				return -1;
				}

				if (str1[0] != 0x00) {
					xmlNodeSetContent(node, BAD_CAST str1);
				} else {
					xmlNodeSetContent(node, BAD_CAST "0");
				}
				gCSV->xml.SaveFile = true;
				continue;
			}

			str = (char *)xmlNodeGetContent(curPtr->children);
		} else if (key_pair[i].nodeType == XML_ATTRIBUTE_NODE) {
			str = (char *)xmlGetProp(pNodePtr, BAD_CAST key_pair[i].key);
		} else {
			log_info("ERROR : Invalid nodeType[%d]", key_pair[i].nodeType);
			return -1;
		}

		if (str == NULL) {
			log_info("WARN : Empty node \"%s\".", key_pair[i].key);
			continue;
		}

		switch (key_pair[i].value_type) {

		  case XML_VALUE_STRING:
			  xml_strlcpy(key_pair[i].value, str, xmlStrlen(BAD_CAST str) + 1);
			  break;
		  case XML_VALUE_INT8:
			  *((int8_t *) key_pair[i].value) = (int8_t) strtoll(str, NULL, 0);
			  break;
		  case XML_VALUE_UINT8:
			  *((uint8_t *) key_pair[i].value) = (uint8_t) strtoll(str, NULL, 0);
			  break;
		  case XML_VALUE_INT16:
			  *((int16_t *) key_pair[i].value) = (int16_t) strtoll(str, NULL, 0);
			  break;
		  case XML_VALUE_UINT16:
			  *((uint16_t *) key_pair[i].value) = (uint16_t) strtoll(str, NULL, 0);
			  break;
		  case XML_VALUE_UINT64:
			  *((uint64_t *) key_pair[i].value) = (uint64_t) strtoll(str, NULL, 0);
			  break;
		  case XML_VALUE_INT32:
			  *((int *) key_pair[i].value) = (int) strtoll(str, NULL, 0);
			  break;
		  case XML_VALUE_UINT32:
			  *((uint32_t *) key_pair[i].value) = (uint32_t) strtoll(str, NULL, 0);
			  break;
		  case XML_VALUE_FLOAT:
			  *((float *)key_pair[i].value) = atof(str);
			  break;
		  default:
			  log_info("ERROR : Invalid value_type[%d]", key_pair[i].value_type);
			  return -1;
		}

		xmlFree(str);
	}

	return 0;
}

int xml_set_node_data (xmlDocPtr pDoc, xmlNodePtr pNodePtr, const char *parentName,
	struct key_value_pair_t *key_pair, uint16_t nums, uint8_t index)
{
	uint16_t i = 0;
	char str[512] = { 0 };
	xmlNodePtr curPtr = NULL;

	if ((pDoc == NULL)||(pNodePtr == NULL)||(parentName == NULL)||(key_pair == NULL)) {
		log_info("ERROR : NULL");
		return 1;
	}

	if (!nums || (nums > MAX_KEY_VALUE_PAIRS)) {
		log_info("ERROR : nums %d", nums);
		return 1;
	}

	pNodePtr = xml_get_node(pNodePtr, parentName, index);
	if (pNodePtr == NULL) {
		return 1;
	}

	for (i = 0; i < nums; i++) {
		if (key_pair[i].value == NULL) {
			log_info("ERROR : NULL");
			return 1;
		}

		memset(str, 0, 512);

		switch (key_pair[i].value_type) {
		case XML_VALUE_STRING:
			xml_strlcpy(str, key_pair[i].value, xmlStrlen(BAD_CAST key_pair[i].value)+1);
		break;
		case XML_VALUE_INT8:
			sprintf(str, "%d", *((int8_t *) key_pair[i].value));
		break;
		case XML_VALUE_UINT8:
			sprintf(str, "%u", *((uint8_t *) key_pair[i].value));
		break;
		case XML_VALUE_INT16:
			sprintf(str, "%d", *((int16_t *) key_pair[i].value));
		break;
		case XML_VALUE_UINT16:
			sprintf(str, "%u", *((uint16_t *) key_pair[i].value));
		break;
		case XML_VALUE_INT32:
			sprintf(str, "%d", *((int *) key_pair[i].value));
		break;
		case XML_VALUE_UINT32:
			sprintf(str, "%u", *((uint32_t *) key_pair[i].value));
		break;
		case XML_VALUE_UINT64:
			sprintf(str, "%ld", *((uint64_t *) key_pair[i].value));
		break;
		case XML_VALUE_FLOAT:
			sprintf(str, "%f", *((float *)key_pair[i].value));
		break;
		default:
			log_info("ERROR : Invalid value_type[%d]", key_pair[i].value_type);
		return -1;
		}

		if (key_pair[i].nodeType == XML_ELEMENT_NODE) {
			for (curPtr = xmlFirstElementChild(pNodePtr);
			  curPtr && xmlStrcmp(curPtr->name, BAD_CAST key_pair[i].key);
			  curPtr = xmlNextElementSibling(curPtr)) {
				;
			}

			if (curPtr == NULL) {
				log_info("ERROR : no tag \"%s\"", key_pair[i].key);
				xmlNodePtr node = NULL;
				node = xmlNewChild(pNodePtr, NULL, BAD_CAST key_pair[i].key, NULL);
				xmlNodeSetContent(node, BAD_CAST str);
				gCSV->xml.SaveFile = true;
				continue;
			}

			if (str[0] != 0x00) {
				xmlNodeSetContent(curPtr->children, BAD_CAST str);
			}
		} else if (key_pair[i].nodeType == XML_ATTRIBUTE_NODE) {
			xmlSetProp(pNodePtr, BAD_CAST key_pair[i].key, BAD_CAST str);
		} else {
			log_info("ERROR : Invalid nodeType[%d]", key_pair[i].nodeType);
			return -1;
		}
	}

	return 0;
}

static int csv_xml_DeviceParameters (
	struct csv_xml_t *pXML, uint8_t mode)
{
	int ret = 0;
	uint32_t nums = 0;
	struct key_value_pair_t key_pair[10];

	xml_strlcpy(key_pair[nums].key, "device_type", MAX_KEY_SIZE);
	key_pair[nums].value = &gCSV->cfg.device_param.device_type;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "camera_leftright_type", MAX_KEY_SIZE);
	key_pair[nums].value = &gCSV->cfg.device_param.camera_leftright_type;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "camera_img_type", MAX_KEY_SIZE);
	key_pair[nums].value = &gCSV->cfg.device_param.camera_img_type;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "exposure_time", MAX_KEY_SIZE);
	key_pair[nums].value = &gCSV->cfg.device_param.exposure_time;
	key_pair[nums].value_type = XML_VALUE_FLOAT;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "exposure_time_for_rgb", MAX_KEY_SIZE);
	key_pair[nums].value = &gCSV->cfg.device_param.exposure_time_for_rgb;
	key_pair[nums].value_type = XML_VALUE_FLOAT;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "dlp_rate", MAX_KEY_SIZE);
	key_pair[nums].value = &gCSV->cfg.device_param.dlp_rate;
	key_pair[nums].value_type = XML_VALUE_FLOAT;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "dlp_brightness", MAX_KEY_SIZE);
	key_pair[nums].value = &gCSV->cfg.device_param.dlp_brightness;
	key_pair[nums].value_type = XML_VALUE_FLOAT;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "img_type", MAX_KEY_SIZE);
	key_pair[nums].value = &gCSV->cfg.device_param.img_type;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	if (mode == XML_GET) {
		ret = xml_get_node_data(pXML->pDoc, pXML->pNode,
			"DeviceParameters", key_pair, nums, 0);
	} else {
		ret = xml_set_node_data(pXML->pDoc, pXML->pNode,
			"DeviceParameters", key_pair, nums, 0);
	}

	if (ret != 0) {
		log_info("ERROR : DeviceParameters");
		return -1;
	}

	if (mode == XML_SET) {
		pXML->SaveFile = true;
	}

	return 0;
}

static int csv_xml_DepthImgParameters (
	struct csv_xml_t *pXML, uint8_t mode)
{
	int ret = 0;
	uint32_t nums = 0;
	struct key_value_pair_t key_pair[4];
	struct depthimg_param_t *pDEP = &gCSV->cfg.depthimg_param;

	xml_strlcpy(key_pair[nums].key, "numDisparities", MAX_KEY_SIZE);
	key_pair[nums].value = &pDEP->numDisparities;
	key_pair[nums].value_type = XML_VALUE_UINT32;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "blockSize", MAX_KEY_SIZE);
	key_pair[nums].value = &pDEP->blockSize;
	key_pair[nums].value_type = XML_VALUE_UINT32;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "uniqRatio", MAX_KEY_SIZE);
	key_pair[nums].value = &pDEP->uniqRatio;
	key_pair[nums].value_type = XML_VALUE_UINT32;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	if (mode == XML_GET) {
		ret = xml_get_node_data(pXML->pDoc, pXML->pNode,
			"DepthImgParameters", key_pair, nums, 0);
	} else {
		ret = xml_set_node_data(pXML->pDoc, pXML->pNode,
			"DepthImgParameters", key_pair, nums, 0);
	}

	if (ret != 0) {
		log_info("ERROR : DepthImgParameters");
		return -1;
	}

	if (mode == XML_SET) {
		pXML->SaveFile = true;
	}

	return 0;
}

static int csv_xml_PointCloudParameters (
	struct csv_xml_t *pXML, uint8_t mode)
{
	int ret = 0;
	uint32_t nums = 0;
	struct key_value_pair_t key_pair[6];
	struct pointcloud_param_t *pPC = &gCSV->cfg.pointcloud_param;

	xml_strlcpy(key_pair[nums].key, "imgRoot", MAX_KEY_SIZE);
	key_pair[nums].value = &pPC->imgRoot;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "imgPrefixNameL", MAX_KEY_SIZE);
	key_pair[nums].value = &pPC->imgPrefixNameL;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "imgPrefixNameR", MAX_KEY_SIZE);
	key_pair[nums].value = &pPC->imgPrefixNameR;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "outFileXYZ", MAX_KEY_SIZE);
	key_pair[nums].value = &pPC->outFileXYZ;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	if (mode == XML_GET) {
		ret = xml_get_node_data(pXML->pDoc, pXML->pNode,
			"PointCloudParameters", key_pair, nums, 0);
	} else {
		ret = xml_set_node_data(pXML->pDoc, pXML->pNode,
			"PointCloudParameters", key_pair, nums, 0);
	}

	if (ret != 0) {
		log_info("ERROR : PointCloudParameters");
		return -1;
	}

	if (mode == XML_SET) {
		pXML->SaveFile = true;
	}

	return 0;
}

static int csv_xml_CalibParameters (
	struct csv_xml_t *pXML, uint8_t mode)
{
	int ret = 0;
	uint32_t nums = 0;
	struct key_value_pair_t key_pair[4];
	struct calib_param_t *pCALIB = &gCSV->cfg.calib_param;

	xml_strlcpy(key_pair[nums].key, "path", MAX_KEY_SIZE);
	key_pair[nums].value = &pCALIB->path;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "calibFile", MAX_KEY_SIZE);
	key_pair[nums].value = &pCALIB->calibFile;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "groupDemarcate", MAX_KEY_SIZE);
	key_pair[nums].value = &pCALIB->groupDemarcate;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	if (mode == XML_GET) {
		ret = xml_get_node_data(pXML->pDoc, pXML->pNode,
			"CalibParameters", key_pair, nums, 0);
	} else {
		ret = xml_set_node_data(pXML->pDoc, pXML->pNode,
			"CalibParameters", key_pair, nums, 0);
	}

	if (ret != 0) {
		log_info("ERROR : CalibParameters");
		return -1;
	}

	if (mode == XML_SET) {
		pXML->SaveFile = true;
	}

	return 0;
}

// 一次全部读取
// 分模块写入

int csv_xml_read_ALL (struct csv_xml_t *pXML)
{
	int ret = 0;

	ret = xml_load_file(pXML);
	if (ret < 0) {
		log_info("ERROR : xml file load.");
		return -1;
	}

	/* node_index is 0, pNodePtr is pRootPtr */
	pXML->pNode = xml_get_node(pXML->pRoot, "CSVDeviceParameters", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_DeviceParameters(pXML, XML_GET);
		if (ret < 0) {
			log_info("ERROR : csv_xml_DeviceParameters GET");
		}
	}

	pXML->pNode = xml_get_node(pXML->pRoot, "CSVDepthImgParameters", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_DepthImgParameters(pXML, XML_GET);
		if (ret < 0) {
			log_info("ERROR : csv_xml_DepthImgParameters GET");
		}
	}

	pXML->pNode = xml_get_node(pXML->pRoot, "CSVPointCloudParameters", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_PointCloudParameters(pXML, XML_GET);
		if (ret < 0) {
			log_info("ERROR : csv_xml_PointCloudParameters GET");
		}
	}

	pXML->pNode = xml_get_node(pXML->pRoot, "CSVCalibParameters", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_CalibParameters(pXML, XML_GET);
		if (ret < 0) {
			log_info("ERROR : csv_xml_CalibParameters GET");
		}
	}

	xml_unload_file(pXML->pDoc);

	log_info("OK : read xml.");

	return ret;
}

int csv_xml_write_DeviceParameters (void)
{
	int ret = 0;
	struct csv_xml_t *pXML = &gCSV->xml;

	ret = xml_load_file(pXML);
	if (ret < 0) {
		log_info("ERROR : xml file load.");
		return -1;
	}

	/* node_index is 0, pNodePtr is pRootPtr */
	pXML->pNode = xml_get_node(pXML->pRoot, "CSVDeviceParameters", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_DeviceParameters(pXML, XML_SET);
		if (ret < 0) {
			log_info("ERROR : csv_xml_DeviceParameters SET");
			goto xml_exit;
		}
	}

  xml_exit:
	xml_unload_file(pXML->pDoc);

	return ret;
}

int csv_xml_write_DepthImgParameters (void)
{
	int ret = 0;
	struct csv_xml_t *pXML = &gCSV->xml;

	ret = xml_load_file(pXML);
	if (ret < 0) {
		log_info("ERROR : xml file load.");
		return -1;
	}

	/* node_index is 0, pNodePtr is pRootPtr */
	pXML->pNode = xml_get_node(pXML->pRoot, "CSVDepthImgParameters", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_DepthImgParameters(pXML, XML_SET);
		if (ret < 0) {
			log_info("ERROR : csv_xml_DepthImgParameters SET");
			goto xml_exit;
		}
	}

  xml_exit:
	xml_unload_file(pXML->pDoc);

	return ret;
}

int csv_xml_write_PointCloudParameters (void)
{
	int ret = 0;
	struct csv_xml_t *pXML = &gCSV->xml;

	ret = xml_load_file(pXML);
	if (ret < 0) {
		log_info("ERROR : xml file load.");
		return -1;
	}

	/* node_index is 0, pNodePtr is pRootPtr */
	pXML->pNode = xml_get_node(pXML->pRoot, "CSVPointCloudParameters", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_PointCloudParameters(pXML, XML_SET);
		if (ret < 0) {
			log_info("ERROR : csv_xml_PointCloudParameters SET");
			goto xml_exit;
		}
	}

  xml_exit:
	xml_unload_file(pXML->pDoc);

	return ret;
}

int csv_xml_write_CalibParameters (void)
{
	int ret = 0;
	struct csv_xml_t *pXML = &gCSV->xml;

	ret = xml_load_file(pXML);
	if (ret < 0) {
		log_info("ERROR : xml file load.");
		return -1;
	}

	/* node_index is 0, pNodePtr is pRootPtr */
	pXML->pNode = xml_get_node(pXML->pRoot, "CSVCalibParameters", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_CalibParameters(pXML, XML_SET);
		if (ret < 0) {
			log_info("ERROR : csv_xml_CalibParameters SET");
			goto xml_exit;
		}
	}

  xml_exit:
	xml_unload_file(pXML->pDoc);

	return ret;
}



static int csv_xml_directories_prepare (void)
{
	int ret = 0;
	struct csv_cfg_t *pCFG = &gCSV->cfg;
	char str_cmd[512] = {0};

	memset(str_cmd, 0, 512);

	if ((NULL != pCFG->pointcloud_param.imgRoot)
	  &&(!csv_file_isExist(pCFG->pointcloud_param.imgRoot))) {
		snprintf(str_cmd, 512, "mkdir -p %s", pCFG->pointcloud_param.imgRoot);
		ret = system(str_cmd);
	}

	if ((NULL != pCFG->calib_param.path)
	  &&(!csv_file_isExist(pCFG->calib_param.path))) {
		snprintf(str_cmd, 512, "mkdir -p %s", pCFG->calib_param.path);
		ret = system(str_cmd);
	}

	return ret;
}

int csv_xml_init (void)
{
	int ret = 0;
	struct csv_xml_t *pXML = &gCSV->xml;

	pXML->file = FILE_PATH_XML;
	pXML->SaveFile = false;
	pXML->pDoc = NULL;
	pXML->pRoot = NULL;
	pXML->pNode = NULL;
	pXML->pNode2 = NULL;
	pXML->pNode3 = NULL;
	pXML->pNode4 = NULL;
	pXML->pNode5 = NULL;
	pXML->pNode6 = NULL;

	ret = csv_xml_read_ALL(pXML);

	csv_xml_directories_prepare();

	return ret;
}


int csv_xml_deinit (void)
{


	return 0;
}


#ifdef __cplusplus
}
#endif

