#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


//Guard against infinite recursion
#define MAX_RECURSION_COUNT 50		// 递归深度

void csv_json_recursion_increment (int *count)
{
	int tmp = *count;

	tmp++;
	*count = tmp;
}

int csv_json_parse (json_object *jobj, int *cnt_recursion)
{
	csv_json_recursion_increment(cnt_recursion);

	if (*cnt_recursion > MAX_RECURSION_COUNT) {
		log_warn("JSON CFG MAX RECURSION LIMIT HIT %d", cnt_recursion);

		return -1;
    }

	int ret = 0;
	enum json_type type;
	json_bool j_ret = FALSE;
	json_object *j_object = NULL, *j_array = NULL, *j_value = NULL;

	json_object_object_foreach(jobj, key, val) {
		type = json_object_get_type(val);

		switch (type) {
        case json_type_null: {
			j_ret = json_object_object_get_ex(jobj, key, &j_object);
			if (j_ret && (NULL != j_object)) {
				log_debug("null ['%s' : '%s']", key, json_object_get_string(j_object));
				// todo 
				json_object_put(j_object);
			}
			break;
		}

		case json_type_boolean: {
			json_object_object_get_ex(jobj, key, &j_object);
			if (NULL != j_object) {
				log_debug("boolean ['%s' : '%s']", key, 
					json_object_get_boolean(j_object)? "true" : "false");
				// todo 
				json_object_put(j_object);
			}
			break;
		}

		case json_type_double: {
			json_object_object_get_ex(jobj, key, &j_object);
			if (NULL != j_object) {
				log_debug("double ['%s' : '%f']", key, json_object_get_double(j_object));
				// todo 

				if (!strcmp(key, "exposure_time")) {
					gCSV->cfg.device_param.exposure_time = json_object_get_double(j_object);
					log_debug("ExposureTime %f", gCSV->cfg.device_param.exposure_time);
				}

				json_object_put(j_object);
			}
			break;
		}

		case json_type_int: {
			json_object_object_get_ex(jobj, key, &j_object);
			if (NULL != j_object) {
				log_debug("int ['%s' : '%d']", key, json_object_get_int(j_object));
				// todo 
				json_object_put(j_object);
			}
			break;
		}

		case json_type_object: {
			json_object_object_get_ex(jobj, key, &j_object);
			if (NULL != j_object) {
				log_debug("object ['%s']", key);
				ret |= csv_json_parse(j_object, cnt_recursion);
				json_object_put(j_object);
			}
			break;
		}

		case json_type_array: {
			json_object_object_get_ex(jobj, key, &j_array);
			if (NULL != j_array) {
				int arraylen = json_object_array_length(j_array);
				log_debug("array '%s[%d]'", key, arraylen);

				int i = 0;
				for (i = 0; i < arraylen; i++) {
					j_value = json_object_array_get_idx(j_array, i);
					if (NULL != j_value) {
						log_debug("[%d] : %s", i, json_object_get_string(j_value));

						enum json_type a_type = json_object_get_type(j_value);
						switch (a_type) {
						case json_type_object:
						case json_type_array:
							ret |= csv_json_parse(j_value, cnt_recursion);
							break;
						default:
							break;
						}
					}
				}

				json_object_put(j_value);
				json_object_put(j_array);
			}
			break;
		}

		case json_type_string: {
			json_object_object_get_ex(jobj, key, &j_object);
			if (NULL != j_object) {
				log_debug("string ['%s' : '%s']", key, json_object_get_string(j_object));
				// todo 
				json_object_put(j_object);
			}
			break;
		}

        default:
        	log_info("WARN : unknown type");
			break;
		}
	}

	return ret;
}


int csv_json_init (void)
{
	int ret = -1;
	struct csv_json_t *pCFG = &gCSV->cfg;

	pCFG->name = FILE_NAME_CJSON;

	pCFG->device_param.device_type = 0;
	pCFG->device_param.camera_leftright_type = false;
	pCFG->device_param.camera_img_type = true;
	pCFG->device_param.exposure_time = 10000.0f;
	pCFG->device_param.exposure_time_for_rgb = 10000.0f;

	pCFG->depthimg_param.numDisparities = 816;
	pCFG->depthimg_param.blockSize = 21;
	pCFG->depthimg_param.uniqRatio = 9;


	json_object *j_cfg = NULL;
	int cnt = 0;

	j_cfg = json_object_from_file(pCFG->name);
	if (NULL != j_cfg) {
		ret = csv_json_parse(j_cfg, &cnt);
		json_object_put(j_cfg);
		log_info("OK : parse cfg");
		return 0;
	} else {
		//log_info("ERROR : %s", json_util_get_last_err());
	}

	log_warn("WARN : using default config.");

	// todo

	// try to save cfg file

	return ret;
}


int csv_json_deinit (void)
{


	return 0;
}


#ifdef __cplusplus
}
#endif

