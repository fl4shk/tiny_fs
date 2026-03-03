#include "tiny_fs.h"
#include <stdio.h>

#define MY_TEST_BUF_LEN 1024ul

int main(int argc, char** argv) {
    void* f = tiny_fs_fopen("aa", "w");

    #define TO_WRITE_LEN 2

    const char* to_write[TO_WRITE_LEN] = {
        "This is a test!",
        "Time for some Quirky Questions!",
    };

    const size_t MY_TEMP_LEN[TO_WRITE_LEN] = {
        strlen(to_write[0]),
        strlen(to_write[1]),
    };

    tiny_fs_fwrite(f, to_write[0], sizeof(char) * MY_TEMP_LEN[0]);
    tiny_fs_fclose(f);

    char buf[TO_WRITE_LEN][MY_TEST_BUF_LEN];
    memset(buf[0], 0, sizeof(buf[0]));
    f = tiny_fs_fopen("aa", "r");

    tiny_fs_fread(f, buf[0], sizeof(char) * MY_TEMP_LEN[0]);
    buf[0][MY_TEMP_LEN[0]] = '\0';

    tiny_fs_fclose(f);
    fprintf(
        stderr,
        "contents of \"aa\": %s\n",
        buf[0]
    );

    memset(buf[1], 0, sizeof(buf[1]));

    f = tiny_fs_fopen("bb", "w");
    tiny_fs_fclose(f);

    return 0;
}
