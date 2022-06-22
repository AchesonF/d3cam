#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


//Guard against infinite recursion
#define MAX_RECURSION_COUNT 50

void recursion_guard_increment (int *count)
{
	int tmp = *count;

	tmp++;
	*count = tmp;
}

int csv_json_parse (json_object * jobj, int *recursion_guard_count)
{
	recursion_guard_increment(recursion_guard_count);

	if (*recursion_guard_count > MAX_RECURSION_COUNT) {
		log_warn("JSON CFG MAX RECURSION LIMIT HIT");

		return -1;
    }

	enum json_type type;

	json_object_object_foreach(jobj, key, val) {
		type = json_object_get_type(val);

		switch (type) {
        case json_type_null: {
			json_object *tmp_null = NULL;
			json_object_object_get_ex(jobj, key, &tmp_null);
			printf("null key: \"%s\" : ", key);
			printf("%s\n", json_object_get_string(tmp_null));
			json_object_put(tmp_null);
			break;
		}

		case json_type_object: {
			json_object *tmp_object = NULL;
			printf("obj key: \"%s\"\n", key);
			json_object_object_get_ex(jobj, key, &tmp_object);
			csv_json_parse(tmp_object, recursion_guard_count);
			json_object_put(tmp_object);
			break;
		}

		case json_type_array: {
			json_object *tmp_array = NULL;
			json_object_object_get_ex(jobj, key, &tmp_array);
			printf("array key: \"%s\"\n", key);
			int arraylen = json_object_array_length(tmp_array);
			printf("Array Length: %d\n", arraylen);
			int i;
			json_object * jvalue;
			for (i = 0; i< arraylen; i++) {
				jvalue = json_object_array_get_idx(tmp_array, i);
				printf("[%d] : \"%s\"\n",i, json_object_get_string(jvalue));
				enum json_type type0 = json_object_get_type(jvalue);

				switch(type0) {
				case json_type_object:
				case json_type_array:
					csv_json_parse(jvalue, recursion_guard_count);
					break;
				default:
					break;
				}
			}

			json_object_put(jvalue);
			json_object_put(tmp_array);
			break;
		}

		case json_type_string: {
			json_object *tmp_string = NULL;
			json_object_object_get_ex(jobj, key, &tmp_string);
			printf("str key: \"%s\" : ",key);
			printf("\"%s\"\n",json_object_get_string(tmp_string));
			json_object_put(tmp_string);
			break;
		}

		case json_type_boolean: {
			json_object *tmp_boolean = NULL;
			json_object_object_get_ex(jobj, key, &tmp_boolean);
			printf("boolean key: \"%s\" : ", key);
			printf("%s\n", json_object_get_boolean(tmp_boolean)? "true" : "false");
			json_object_put(tmp_boolean);
			break;
		}

		case json_type_int: {
			json_object *tmp_int = NULL;
			json_object_object_get_ex(jobj, key, &tmp_int);
			printf("int key: \"%s\" : ",key);
			printf("%d\n",json_object_get_int(tmp_int));
			json_object_put(tmp_int);
			break;
		}

		case json_type_double: {
			json_object *tmp_double = NULL;
			json_object_object_get_ex(jobj, key, &tmp_double);
			printf("double key: \"%s\" : ",key);
			printf("%f\n",json_object_get_double(tmp_double));
			json_object_put(tmp_double);
			break;
		}

        default:
        	printf("type: unknown\n");
			break;
		}
	}

	return 0;
}

static int csv_json_cfg_get (char *buffer)
{
	int recursion_guard_count = 0;
	json_object *jobj_parse;

	jobj_parse = json_tokener_parse(buffer);

	csv_json_parse(jobj_parse, &recursion_guard_count);
	json_object_put(jobj_parse);

	return 0;
}


int csv_json_init (void)
{
	uint32_t len_cfg = 0;
	int ret = 0;
	struct csv_json_t *pCFG = &gCSV->cfg;

	pCFG->name = FILE_NAME_CJSON;

	if (csv_file_isExist(pCFG->name)) {
		ret = csv_file_get_size(pCFG->name, &len_cfg);
		if ((0 == ret)&&(0 < len_cfg)) {
			uint8_t *cfg_data = (uint8_t *)malloc(len_cfg);
			if (cfg_data == NULL) {
				log_err("ERROR : malloc");
				goto cfg_default;
			}
			memset(cfg_data, 0, len_cfg);
			ret = csv_file_read_data(pCFG->name, cfg_data, len_cfg);
			if (ret < 0) {
				goto cfg_default;
			}

			ret = csv_json_cfg_get((char *)cfg_data);
			if (ret < 0) {
				goto cfg_default;
			}

			free(cfg_data);

			return 0;
		}
	}


cfg_default:

	log_warn("WARN : using default config.");

	// todo

	// try to save cfg file

	return 0;
}


int csv_json_deinit (void)
{


	return 0;
}


#ifdef __cplusplus
}
#endif

