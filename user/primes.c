#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_PRIMES 280

void
primes(int input_fd) {
    int prime;
    int n;

    // read the first prime
    if (read(input_fd, &prime, sizeof(prime)) == 0) {
        close(input_fd);
        exit(0);
    }

    printf("prime %d\n", prime);

    int p[2];
    pipe(p);

    if (fork() == 0) {
        // child process
        close(p[1]);
        close(input_fd);
        primes(p[0]);
    } else {
        // parent process
        close(p[0]);
        while (read(input_fd, &n, sizeof(n)) > 0) {
            if (n % prime != 0) {
                write(p[1], &n, sizeof(n));
            }
        }
        close(input_fd);
        close(p[1]);
        wait(0);  // wait for child process to finish
    }
}

int 
main(int argc, char *argv[]) {
    int p[2];
    pipe(p);

    if (fork() == 0) {
        // child process
        close(p[1]);  // close write end
        primes(p[0]);
    } else {
        // parent process
        close(p[0]);  // close read end

        for (int i = 2; i <= MAX_PRIMES; i++) {
            write(p[1], &i, sizeof(i));
        }
        close(p[1]);  // close write end
        wait(0);  // wait for child process to finish
    }

    exit(0);
}