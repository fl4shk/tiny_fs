#include "tiny_fs.h"
#include <stdio.h>

#define MY_TEST_BUF_LEN 1024ul

int main(int argc, char** argv) {
    void* f = tiny_fs_fopen("asdf.txt", "w");
    const char* to_write = "This is a test!";
    const size_t MY_TEMP_LEN = strlen(to_write);
    tiny_fs_fwrite(f, to_write, sizeof(char) * MY_TEMP_LEN);
    tiny_fs_fclose(f);

    char buf[MY_TEST_BUF_LEN];
    memset(buf, 0, sizeof(buf));
    f = tiny_fs_fopen("asdf.txt", "r");

    tiny_fs_fread(f, buf, sizeof(char) * MY_TEMP_LEN);
    buf[MY_TEMP_LEN] = '\0';
    tiny_fs_fclose(f);
    fprintf(
        stderr,
        "Here is the value I read: %s\n",
        buf
    );

    return 0;
}
