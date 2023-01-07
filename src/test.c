#include <stdio.h>

#include <json/document.h>
#include <json/errcode.h>

#define TEST(fn)\
String buf_##fn, out_##fn;\
DArrayChar_initialize(&buf_##fn, 1001);\
DArrayChar_initialize(&out_##fn, 1001);\
puts("data/"#fn".json");\
read_file("data/"#fn".json", &buf_##fn);\
JSONDocument document_##fn;\
ret = JSONDocument_parse(&document_##fn, buf_##fn.data);\
printf("%d %s\n", ret, json_errcode_lookup[ret]);\
DArrayChar_finalize(&buf_##fn);\
if (!ret) {\
    ret = JSONDocument_serialize(&document_##fn, &out_##fn, false);\
    printf("%d %s\n", ret, json_errcode_lookup[ret]);\
    if (!ret) {\
        puts(out_##fn.data);\
    }\
}\
DArrayChar_finalize(&out_##fn);\
JSONDocument_finalize(&document_##fn);

void read_file(char *fn, String *buf) {
    char in_buf[1000];
    FILE *fin = fopen(fn, "rb");
    size_t sz = 0;
    for (; (sz = fread(in_buf, 1, 1000, fin)) == 1000;) {
        DArrayChar_push_back_batch(buf, in_buf, sz);
    }
    DArrayChar_push_back_batch(buf, in_buf, sz);
    char zero = 0;
    DArrayChar_push_back(buf, &zero);
    fclose(fin);
}

int main(void) {
    int ret = JSON_ERR_OK;
    TEST(null)
    TEST(str1)
    TEST(str2)
    TEST(str3)
    TEST(number1)
    TEST(number2)
    TEST(number3)
    TEST(number4)
    TEST(number5)
    TEST(number6)
    return 0;
}
