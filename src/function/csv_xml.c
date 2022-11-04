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

		pXML->TimeOfModify = csv_file_modify_time(pXML->file);
	}

	xmlFreeDoc(pDoc);
}

static int xml_load_file (struct csv_xml_t *pXML)
{
	if (pXML->file == NULL) {
		log_warn("ERROR : xml file NULL.");
		pXML->file = FILE_PATH_XML;
	}

	char cmd[256] = {0};
	memset(cmd, 0, 256);

	if (!csv_file_isExist(pXML->file)) {
		log_warn("WARN : xml \"%s\" not exist.", pXML->file);
		if (!csv_file_isExist(FILE_PATH_XML_BK)) {
			log_warn("WARN : backup xml \"%s\" not exist.", FILE_PATH_XML_BK);
		} else {
			log_warn("WARN : use backup xml \"%s\".", FILE_PATH_XML_BK);
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
		log_warn("WARN : xmlReadFile \"%s\", make a new one.", pXML->file);

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
		log_warn("ERROR : xmlDocGetRootElement NULL.");

		goto mk_root;
	}

	if (xmlStrcmp(pXML->pRoot->name, BAD_CAST "CSVxmlRoot")) {
		log_warn("ERROR : RootNode not found. \"CSVxmlRoot\" vs \"%s\".", pXML->pRoot->name);
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
	log_warn("WARN : no tag \"%s\".", nodeName);
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
		log_warn("ERROR : NULL.");
		return 1;
	}

	if (!nums || (nums > MAX_KEY_VALUE_PAIRS)) {
		log_warn("ERROR : nums %d.", nums);
		return 1;
	}

	pNodePtr = xml_get_node(pNodePtr, parentName, index);
	if (pNodePtr == NULL) {
		return 1;
	}

	for (i = 0; i < nums; i++) {
		if (key_pair[i].value == NULL) {
			log_warn("ERROR : NULL.");
			return 1;
		}

		if (key_pair[i].nodeType == XML_ELEMENT_NODE) {
			for (curPtr = xmlFirstElementChild(pNodePtr);
  			  curPtr && xmlStrcmp(curPtr->name, BAD_CAST key_pair[i].key);
			  curPtr = xmlNextElementSibling(curPtr)) {
				;
			}

			if (curPtr == NULL) {
				log_warn("WARN : no tag \"%s\".", key_pair[i].key);

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
					log_warn("ERROR : Invalid value_type[%d]", key_pair[i].value_type);
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
			log_warn("ERROR : Invalid nodeType[%d].", key_pair[i].nodeType);
			return -1;
		}

		if (str == NULL) {
			log_warn("WARN : Empty node \"%s\".", key_pair[i].key);
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
			  log_warn("ERROR : Invalid value_type[%d].", key_pair[i].value_type);
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
		log_warn("ERROR : NULL.");
		return 1;
	}

	if (!nums || (nums > MAX_KEY_VALUE_PAIRS)) {
		log_warn("ERROR : nums %d.", nums);
		return 1;
	}

	pNodePtr = xml_get_node(pNodePtr, parentName, index);
	if (pNodePtr == NULL) {
		return 1;
	}

	for (i = 0; i < nums; i++) {
		if (key_pair[i].value == NULL) {
			log_warn("ERROR : NULL.");
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
			log_warn("ERROR : Invalid value_type[%d].", key_pair[i].value_type);
		return -1;
		}

		if (key_pair[i].nodeType == XML_ELEMENT_NODE) {
			for (curPtr = xmlFirstElementChild(pNodePtr);
			  curPtr && xmlStrcmp(curPtr->name, BAD_CAST key_pair[i].key);
			  curPtr = xmlNextElementSibling(curPtr)) {
				;
			}

			if (curPtr == NULL) {
				log_warn("WARN : no tag \"%s\".", key_pair[i].key);
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
			log_warn("ERROR : Invalid nodeType[%d].", key_pair[i].nodeType);
			return -1;
		}
	}

	return 0;
}

static int csv_xml_GeneralInfomation (
	struct csv_xml_t *pXML, uint8_t mode)
{
	int ret = 0;
	uint32_t nums = 0;
	struct key_value_pair_t key_pair[4];

	if (XML_SET == mode) {
		xml_strlcpy(key_pair[nums].key, "Version", MAX_KEY_SIZE);
		key_pair[nums].value = &gPdct.app_info;
		key_pair[nums].value_type = XML_VALUE_STRING;
		key_pair[nums].nodeType = XML_ELEMENT_NODE;
		nums++;

		ret = xml_set_node_data(pXML->pDoc, pXML->pNode,
			"GeneralInfomation", key_pair, nums, 0);
	}

	if (ret != 0) {
		log_warn("ERROR : GeneralInfomation.");
		return -1;
	}

	if (mode == XML_SET) {
		pXML->SaveFile = true;
	}

	return 0;
}

static int csv_xml_DlpAttribute (
	struct csv_xml_t *pXML, int inode, uint8_t mode)
{
	uint32_t nums = 0;
	struct key_value_pair_t key_pair[6];
	if (inode >= TOTAL_DLP_CMDS) {
		return -1;
	}

	struct dlp_cfg_t *pDlpcfg = &gCSV->cfg.devicecfg.dlpcfg[inode];

	xml_strlcpy(key_pair[nums].key, "name", MAX_KEY_SIZE);
	key_pair[nums].value = &pDlpcfg->name;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "rate", MAX_KEY_SIZE);
	key_pair[nums].value = &pDlpcfg->rate;
	key_pair[nums].value_type = XML_VALUE_FLOAT;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "brightness", MAX_KEY_SIZE);
	key_pair[nums].value = &pDlpcfg->brightness;
	key_pair[nums].value_type = XML_VALUE_FLOAT;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "expoTime", MAX_KEY_SIZE);
	key_pair[nums].value = &pDlpcfg->expoTime;
	key_pair[nums].value_type = XML_VALUE_FLOAT;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	int ret = 0;
	if (mode == XML_GET) {
		ret = xml_get_node_data(pXML->pDoc, pXML->pNode3,
			"DlpAttribute", key_pair, nums, inode);
	} else {
		ret = xml_set_node_data(pXML->pDoc, pXML->pNode3,
			"DlpAttribute", key_pair, nums, inode);
	}

	if (ret != 0) {
		log_warn("ERROR : DlpAttribute.");
		return -1;
	}

	pXML->pNode4 = xml_get_node(pXML->pNode3, "DlpAttribute", inode);
	if (pXML->pNode4 == NULL) {
		return -1;
	}

	char attr[32] = {0};
	memset(attr, 0, 32);
	sprintf(attr, "%d", inode);
	xmlSetProp(pXML->pNode4, BAD_CAST "id", BAD_CAST attr);

	return 0;
}

static int csv_xml_DlpConfiguration (
	struct csv_xml_t *pXML, uint8_t mode)
{
	pXML->pNode3 = xml_get_node(pXML->pNode2, "DlpConfiguration", 0);
	if (pXML->pNode3 == NULL) {
		return -1;
	}

	int i = 0;
	for (i = 0; i < TOTAL_DLP_CMDS; i++) {
		csv_xml_DlpAttribute(pXML, i, mode);
	}

	return 0;
}

static int csv_xml_DeviceConfiguration (
	struct csv_xml_t *pXML, uint8_t mode)
{
	int ret = 0;
	uint32_t nums = 0;
	struct key_value_pair_t key_pair[16];
	struct device_cfg_t *pDevC = &gCSV->cfg.devicecfg;

	xml_strlcpy(key_pair[nums].key, "SwitchCams", MAX_KEY_SIZE);
	key_pair[nums].value = &pDevC->SwitchCams;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "SaveImageFile", MAX_KEY_SIZE);
	key_pair[nums].value = &pDevC->SaveImageFile;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "SaveImageFormat", MAX_KEY_SIZE);
	key_pair[nums].value = &pDevC->SaveImageFormat;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "ftpupload", MAX_KEY_SIZE);
	key_pair[nums].value = &pDevC->ftpupload;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "exportCamsCfg", MAX_KEY_SIZE);
	key_pair[nums].value = &pDevC->exportCamsCfg;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	if (mode == XML_GET) {
		ret = xml_get_node_data(pXML->pDoc, pXML->pNode,
			"DeviceConfiguration", key_pair, nums, 0);

		switch (pDevC->SaveImageFormat) {
		case SUFFIX_PNG:
			pDevC->strSuffix = ".png";
			break;
		case SUFFIX_JPEG:
			pDevC->strSuffix = ".jpeg";
			break;
		case SUFFIX_BMP:
		default:
			pDevC->strSuffix = ".bmp";
			pDevC->SaveImageFormat = SUFFIX_BMP;
			break;
		}
	} else {
		ret = xml_set_node_data(pXML->pDoc, pXML->pNode,
			"DeviceConfiguration", key_pair, nums, 0);
	}

	if (ret != 0) {
		log_warn("ERROR : DeviceConfiguration.");
		return -1;
	}

	pXML->pNode2 = xml_get_node(pXML->pNode, "DeviceConfiguration", 0);
	if (pXML->pNode2 == NULL) {
		return -1;
	}

	csv_xml_DlpConfiguration(pXML, mode);

	if (mode == XML_SET) {
		pXML->SaveFile = true;
	}

	return 0;
}

static int csv_xml_PointCloudConfiguration (
	struct csv_xml_t *pXML, uint8_t mode)
{
	int ret = 0;
	uint32_t nums = 0;
	struct key_value_pair_t key_pair[16];
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;

	xml_strlcpy(key_pair[nums].key, "ModelRoot", MAX_KEY_SIZE);
	key_pair[nums].value = &pPC->ModelRoot;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "PCImageRoot", MAX_KEY_SIZE);
	key_pair[nums].value = &pPC->PCImageRoot;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "calibFile", MAX_KEY_SIZE);
	key_pair[nums].value = &pPC->calibFile;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "outFileXYZ", MAX_KEY_SIZE);
	key_pair[nums].value = &pPC->outFileXYZ;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "saveXYZ", MAX_KEY_SIZE);
	key_pair[nums].value = &pPC->saveXYZ;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "groupPointCloud", MAX_KEY_SIZE);
	key_pair[nums].value = &pPC->groupPointCloud;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "initialized", MAX_KEY_SIZE);
	key_pair[nums].value = &pPC->initialized;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	if (mode == XML_GET) {
		ret = xml_get_node_data(pXML->pDoc, pXML->pNode,
			"PointCloudConfiguration", key_pair, nums, 0);
	} else {
		ret = xml_set_node_data(pXML->pDoc, pXML->pNode,
			"PointCloudConfiguration", key_pair, nums, 0);
	}

	if (ret != 0) {
		log_warn("ERROR : PointCloudConfiguration.");
		return -1;
	}

	if (mode == XML_SET) {
		pXML->SaveFile = true;
	}

	return 0;
}

static int csv_xml_CalibConfiguration (
	struct csv_xml_t *pXML, uint8_t mode)
{
	int ret = 0;
	uint32_t nums = 0;
	struct key_value_pair_t key_pair[4];
	struct calib_conf_t *pCALIB = &gCSV->cfg.calibcfg;

	xml_strlcpy(key_pair[nums].key, "CalibImageRoot", MAX_KEY_SIZE);
	key_pair[nums].value = &pCALIB->CalibImageRoot;
	key_pair[nums].value_type = XML_VALUE_STRING;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	xml_strlcpy(key_pair[nums].key, "groupCalibrate", MAX_KEY_SIZE);
	key_pair[nums].value = &pCALIB->groupCalibrate;
	key_pair[nums].value_type = XML_VALUE_UINT8;
	key_pair[nums].nodeType = XML_ELEMENT_NODE;
	nums++;

	if (mode == XML_GET) {
		ret = xml_get_node_data(pXML->pDoc, pXML->pNode,
			"CalibConfiguration", key_pair, nums, 0);
	} else {
		ret = xml_set_node_data(pXML->pDoc, pXML->pNode,
			"CalibConfiguration", key_pair, nums, 0);
	}

	if (ret != 0) {
		log_warn("ERROR : CalibConfiguration.");
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
		log_warn("ERROR : xml file load.");
		return -1;
	}

	pXML->pNode = xml_get_node(pXML->pRoot, "CSVGeneralInfomation", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_GeneralInfomation(pXML, XML_GET);
		if (ret < 0) {
			log_warn("ERROR : GeneralInfomation GET.");
		}
	}

	pXML->pNode = xml_get_node(pXML->pRoot, "CSVDeviceConfiguration", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_DeviceConfiguration(pXML, XML_GET);
		if (ret < 0) {
			log_warn("ERROR : DeviceConfiguration GET.");
		}
	}

	pXML->pNode = xml_get_node(pXML->pRoot, "CSVPointCloudConfiguration", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_PointCloudConfiguration(pXML, XML_GET);
		if (ret < 0) {
			log_warn("ERROR : PointCloudConfiguration GET.");
		}
	}

	pXML->pNode = xml_get_node(pXML->pRoot, "CSVCalibConfiguration", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_CalibConfiguration(pXML, XML_GET);
		if (ret < 0) {
			log_warn("ERROR : CalibConfiguration GET.");
		}
	}

	xml_unload_file(pXML->pDoc);

	log_info("OK : read xml.");

	return ret;
}

int csv_xml_write_GeneralInfomation (void)
{
	int ret = 0;
	struct csv_xml_t *pXML = &gCSV->xml;

	ret = xml_load_file(pXML);
	if (ret < 0) {
		log_warn("ERROR : xml file load.");
		return -1;
	}

	/* node_index is 0, pNodePtr is pRootPtr */
	pXML->pNode = xml_get_node(pXML->pRoot, "CSVGeneralInfomation", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_GeneralInfomation(pXML, XML_SET);
		if (ret < 0) {
			log_warn("ERROR : GeneralInfomation SET.");
			goto xml_exit;
		}
	}

  xml_exit:
	xml_unload_file(pXML->pDoc);

	return ret;
}

int csv_xml_write_DeviceParameters (void)
{
	int ret = 0;
	struct csv_xml_t *pXML = &gCSV->xml;

	ret = xml_load_file(pXML);
	if (ret < 0) {
		log_warn("ERROR : xml file load.");
		return -1;
	}

	/* node_index is 0, pNodePtr is pRootPtr */
	pXML->pNode = xml_get_node(pXML->pRoot, "CSVDeviceConfiguration", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_DeviceConfiguration(pXML, XML_SET);
		if (ret < 0) {
			log_warn("ERROR : DeviceConfiguration SET.");
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
		log_warn("ERROR : xml file load.");
		return -1;
	}

	/* node_index is 0, pNodePtr is pRootPtr */
	pXML->pNode = xml_get_node(pXML->pRoot, "CSVPointCloudConfiguration", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_PointCloudConfiguration(pXML, XML_SET);
		if (ret < 0) {
			log_warn("ERROR : PointCloudConfiguration SET.");
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
		log_warn("ERROR : xml file load.");
		return -1;
	}

	/* node_index is 0, pNodePtr is pRootPtr */
	pXML->pNode = xml_get_node(pXML->pRoot, "CSVCalibConfiguration", 0);
	if (pXML->pNode != NULL) {
		ret = csv_xml_CalibConfiguration(pXML, XML_SET);
		if (ret < 0) {
			log_warn("ERROR : CalibConfiguration SET.");
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

	ret = csv_file_mkdir(pCFG->pointcloudcfg.ModelRoot);
	ret |= csv_file_mkdir(pCFG->pointcloudcfg.PCImageRoot);
	ret |= csv_file_mkdir(pCFG->calibcfg.CalibImageRoot);

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

	csv_xml_write_GeneralInfomation();

	csv_xml_directories_prepare();

	pXML->TimeOfModify = csv_file_modify_time(pXML->file);

	return ret;
}


int csv_xml_check (void)
{
	int ret = 0;
	uint32_t new_tom = 0;
	struct csv_xml_t *pXML = &gCSV->xml;

	new_tom = csv_file_modify_time(pXML->file);
	if (new_tom != pXML->TimeOfModify) {
		ret = csv_xml_read_ALL(pXML);
		csv_xml_directories_prepare();
		pXML->TimeOfModify = new_tom;
	}

	return ret;
}


#ifdef __cplusplus
}
#endif

