#include <stdio.h>

#include <json/document.h>
#include <json/errcode.h>

#define REPORT printf("%d %s\n", ret, json_errcode_lookup[ret]);

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
    String buf, out;
    DArrayChar_initialize(&buf, 1001);
    DArrayChar_initialize(&out, 1001);
    read_file("data/a.json", &buf);
    JSONDocument document;
    int ret = JSONDocument_parse(&document, buf.data);
    REPORT
    DArrayChar_finalize(&buf);
    ret = JSONDocument_serialize(&document, &out, false);
    REPORT
    puts(out.data);
    JSONDocument_finalize(&document);
    return 0;
}
