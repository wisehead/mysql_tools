#include <print_data.h>
#include <decimal.h>

static byte buffer[UNIV_PAGE_SIZE * 2];
extern char blob_dir[256];

/*******************************************************************/
void print_timestamp(time_t *t){
	char buf[32];
	struct tm result;

	gmtime_r(t, &result);
	strftime(buf, sizeof(buf), "%F %T", &result);
	printf("\"%s\"", buf);
}

inline void print_datetime(ulonglong ldate) {
	int year, month, day, hour, min, sec;
	
	ldate &= ~(1ULL << 63);
    
	sec = ldate % 100; ldate /= 100;
	min = ldate % 100; ldate /= 100;
	hour = ldate % 100; ldate /= 100;
	day = ldate % 100; ldate /= 100;
	month = ldate % 100; ldate /= 100;
	year = ldate % 10000;
	
	printf("\"%04u-%02u-%02u %02u:%02u:%02u\"", year, month, day, hour, min, sec);
}

void print_string_raw(char *value, ulint len) {
    ulint i;
    for(i = 0; i < len; i++) {
        if (value[i] == '"') printf("\\\"");
        else if (value[i] == '\n') printf("\\n");
        else if (value[i] == '\r') printf("\\r");
        else printf("%c", value[i]);
        }
}

void print_hex(char *value, ulint len) {
    ulint i;
    unsigned int k = 0;
    char map[256][3];
    char* blob = malloc(len*2 + 1);
    if(blob == NULL){
    	fprintf(stderr, "ERROR: Unable to allocate %lu bytes memory to print BLOB value\n", len*2 + 1);
	fprintf(stderr, "BLOB field dump:\n");
	for(i = 0; i < len; i++){
		if(isprint(value[i])){
			fprintf(stderr, "%c", value[i]);
			}
		else{
			fprintf(stderr, ".");
			}
		}
	fprintf(stderr, "\n END OF BLOB field dump\n");
	exit(EXIT_FAILURE);
    	}
    for(k = 0; k < 256; k++){
    	sprintf(map[k], "%02X", k);
    	}
    for(i = 0; i < len; i++) {
	strncpy((blob + i*2), map[(unsigned int)value[i] & 0x000000FF], 2);
        }
    blob[len*2+1] = '\0';
    fwrite(blob, len*2, 1, stdout);
    free(blob);
}

/*******************************************************************/
inline void print_date(ulong ldate) {
	int year, month, day;

	ldate &= ~(1UL << 23);
	
	day = ldate % 32; ldate /= 32;
	month = ldate % 16; ldate /= 16;
	year = ldate;
	
	printf("\"%04u-%02u-%02u\"", year, month, day);
}

/*******************************************************************/
inline void print_time(ulong ltime) {
	int hour, min, sec;

	ltime &= ~(1UL << 23);
	
	sec = ltime % 60; ltime /= 60;
	min = ltime % 60; ltime /= 60;
	hour = ltime % 24;
	
	printf("\"%02u:%02u:%02u\"", hour, min, sec);
}


/*******************************************************************/
inline void print_enum(int value, field_def_t *field) {
	printf("\"%s\"", field->limits.enum_values[value-1]);
}

/*******************************************************************/
inline unsigned long long int get_uint_value(field_def_t *field, byte *value) {
	switch (field->fixed_length) {
		case 1: return mach_read_from_1(value);
		case 2: return mach_read_from_2(value);
		case 3: return mach_read_from_3(value) & 0x3FFFFFUL;
		case 4: return mach_read_from_4(value);
		case 8: return make_ulonglong(mach_read_from_8(value));
	}
	return 0;
}

/*******************************************************************/
inline long long int get_int_value(field_def_t *field, byte *value) {
	char v1;
	short int v2;
	int v3, v4;
	long int v8;
	switch (field->fixed_length) {
		case 1: v1 = (mach_read_from_1(value) & 0x7F) | ((~mach_read_from_1(value)) & 0x80);
			return v1;
		case 2: v2 = (mach_read_from_2(value) & 0x7FFF) | ((~mach_read_from_2(value)) & 0x8000);
			return v2;
		case 3: v3 = mach_read_from_3(value);
			if((v3 >> 23) == 1){
				// Positive
				v3 &= 0x007FFFFF;
			}else{
				// Negative
				v3 |= 0xFFFF8000;
			}
			return v3;
		case 4: v4 = (mach_read_from_4(value) & 0x7FFFFFFF) | ((~mach_read_from_4(value)) & 0x80000000);
			return v4;
		case 8: v8 = (make_ulonglong(mach_read_from_8(value)) & 0x7FFFFFFFFFFFFFFFUL) | ((~make_ulonglong(mach_read_from_8(value))) & 0x8000000000000000UL);
			return v8;
	}
	return 0;
}

/*******************************************************************/
inline void print_string(char *value, ulint len, field_def_t *field) {
    uint i, num_spaces = 0, out_pos = 0;
    static char out[32768];
    
    out[out_pos++] = '"';
    for(i = 0; i < len; i++) {
        if (value[i] != ' ' && num_spaces > 0) {
            while(num_spaces > 0) {
                out[out_pos++] = ' ';
                num_spaces--;
            }
        }
		if (value[i] == '\\') out[out_pos++] = '\\', out[out_pos++] = '\\';
		else if (value[i] == '"') out[out_pos++] = '\\', out[out_pos++] = '"';
		else if (value[i] == '\n') out[out_pos++] = '\\', out[out_pos++] = 'n';
		else if (value[i] == '\r') out[out_pos++] = '\\', out[out_pos++] = 'r';
		else if (value[i] == '\t') out[out_pos++] = '\\', out[out_pos++] = 't';
    	else {
            if (value[i] == ' ') {
                num_spaces++;
            } else {
                if ((int)value[i] < 32) {
                    //out_pos += sprintf(out + out_pos, "\\x%02x", (uchar)value[i]);
                    out[out_pos++] = value[i];
                } else {
                    out[out_pos++] = value[i];
                }
            }
        }
	}

    if (num_spaces > 0 && !field->char_rstrip_spaces) {
        while(num_spaces > 0) {
            out[out_pos++] = ' ';
            num_spaces--;
        }
    }
    out[out_pos++] = '"';
    out[out_pos++] = 0;
    fputs(out, stdout);
}

/*******************************************************************/
inline void print_decimal(byte *value, field_def_t *field) {
    char string_buf[256];
    decimal_digit_t dec_buf[256];
    int len = 255;
    
    decimal_t dec;
    dec.buf = dec_buf;
    dec.len = 256;
    
    bin2decimal((char*)value, &dec, field->decimal_precision, field->decimal_digits);
    decimal2string(&dec, string_buf, &len, field->decimal_precision, field->decimal_digits, ' ');
    print_string(string_buf, len, field);
}

/*******************************************************************/
inline void print_field_value(byte *value, ulint len, field_def_t *field) {
	time_t t;

	switch (field->type) {
		case FT_INTERNAL:
    		break;

		case FT_CHAR:
		case FT_TEXT:
			print_string((char*)value, len, field);
			break;

		case FT_BLOB:
            		print_hex((char*)value, len);
            		break;

		case FT_UINT:
            		printf("%llu", get_uint_value(field, value));
			break;

		case FT_INT:
            		printf("%lli", get_int_value(field, value));
			break;

		case FT_FLOAT:
			printf("%f", mach_float_read(value));
			break;

		case FT_DOUBLE:
			printf("%lf", mach_double_read(value));
			break;

		case FT_DATETIME:
			print_datetime(make_longlong(mach_read_from_8(value)));
			break;

		case FT_DATE:
			print_date(mach_read_from_3(value));
			break;

		case FT_TIME:
			print_time(mach_read_from_3(value));
			break;

		case FT_ENUM:
			print_enum(mach_read_from_1(value), field);
			break;

	        case FT_DECIMAL:
			print_decimal(value, field);
			break;
		case FT_TIMESTAMP:
			t = mach_read_from_4(value);
			print_timestamp(&t);
			break;

		default:
    			printf("undef(%d)", field->type);
		}
}


/*******************************************************************/
void print_field_value_with_external(byte *value, ulint len, field_def_t *field) {
   ulint   space_id, page_no, offset, extern_len, len_sum;
   char tmp[256];
   int fn;
   page_t *page = ut_align(buffer, UNIV_PAGE_SIZE);

   switch (field->type) {
       case FT_TEXT:
       case FT_BLOB:
           space_id = mach_read_from_4(value + len - BTR_EXTERN_FIELD_REF_SIZE + BTR_EXTERN_SPACE_ID);
           page_no = mach_read_from_4(value + len - BTR_EXTERN_FIELD_REF_SIZE + BTR_EXTERN_PAGE_NO);
           offset = mach_read_from_4(value + len - BTR_EXTERN_FIELD_REF_SIZE + BTR_EXTERN_OFFSET);
           extern_len = mach_read_from_4(value + len - BTR_EXTERN_FIELD_REF_SIZE + BTR_EXTERN_LEN + 4);
           len_sum = 0;

           if (field->type == FT_TEXT) {
               printf("\"");
               print_string_raw((char*)value, len - BTR_EXTERN_FIELD_REF_SIZE);
           } else {
               print_hex((char*)value, len - BTR_EXTERN_FIELD_REF_SIZE);
           }

           for (;;) {
               byte*   blob_header;
               ulint   part_len;

               sprintf(tmp, "%s/page-%08lu.page", blob_dir, page_no);

               fn = open(tmp, O_RDONLY);
               if (fn != -1) {
                   read(fn, page, UNIV_PAGE_SIZE);

                   if (offset > UNIV_PAGE_SIZE)
                       break;

                   blob_header = page + offset;
                   part_len = mach_read_from_4(blob_header + 0 /*BTR_BLOB_HDR_PART_LEN*/);
                   if (part_len > UNIV_PAGE_SIZE)
                       break;
                   len_sum += part_len;
                   page_no = mach_read_from_4(blob_header + 4 /*BTR_BLOB_HDR_NEXT_PAGE_NO*/);
                   offset = FIL_PAGE_DATA;

                   if (field->type == FT_TEXT) {
                       print_string_raw((char*)blob_header + 8 /*BTR_BLOB_HDR_SIZE*/, part_len);
                   } else {
                       print_hex((char*)blob_header + 8 /*BTR_BLOB_HDR_SIZE*/, part_len);
                   }

                   close(fn);

                   if (page_no == FIL_NULL)
                       break;

                   if (len_sum >= extern_len)
                       break;
               } else {
                   fprintf(stderr, "#####CannotOpen_%s", tmp);
                   break;
               }
           }
           if (field->type == FT_TEXT) {
               printf("\"");
           }

           break;

       default:
           printf("error(%d)", field->type);
   }
}

/*******************************************************************/
void
rec_print_new(
/*==========*/
	FILE*		file,	/* in: file where to print */
	rec_t*		rec,	/* in: physical record */
	const ulint*	offsets)/* in: array returned by rec_get_offsets() */
{
	const byte*	data;
	ulint		len;
	ulint		i;

	ut_ad(rec_offs_validate(rec, NULL, offsets));

	ut_ad(rec);

	fprintf(file, "PHYSICAL RECORD: n_fields %lu;"
		" compact format; info bits %lu\n",
		(ulong) rec_offs_n_fields(offsets),
		(ulong) rec_get_info_bits(rec, TRUE));
	
	for (i = 0; i < rec_offs_n_fields(offsets); i++) {

		data = rec_get_nth_field(rec, offsets, i, &len);

		fprintf(file, " %lu:", (ulong) i);
	
		if (len != UNIV_SQL_NULL) {
			if (len <= 30) {

				ut_print_buf(file, data, len);
			} else {
				ut_print_buf(file, data, 30);

				fputs("...(truncated)", file);
			}
		} else {
			fputs(" SQL NULL", file);
		}
		putc(';', file);
	}

	putc('\n', file);
}
