#include <signal.h>

int dummy_main(int argc, char **argv);
int main(int argc, char **argv) {
    // Immediately stop itself - scheduler will later resume it
    raise(SIGSTOP);
    
    int ret = dummy_main(argc, argv);
    return ret;
}

#define main dummy_main