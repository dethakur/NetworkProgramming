#include <stdio.h>
int main(void)
{
    printf("Enters main\n");
    int pfd[2];
    int pid;
    if (pipe(pfd) == -1)
    {
     perror("pipe failed");
     exit(1);
 }
 if ((pid = fork()) < 0)
 {
     perror("fork failed");
     exit(2);
 }
 
 if (pid == 0)
 {
    printf("WC waits\n");
    close(pfd[1]);
    dup2(pfd[0], 0);
    close(pfd[0]);
    
    execlp("wc", "wc",
     (char *) 0);
    perror("wc failed");
    exit(3);
} else {
    printf("ls is called now");
    close(pfd[0]);
    dup2(pfd[1], 1);
    close(pfd[1]);
    
    execlp("ls", "ls",
     (char *) 0);
    perror("ls failed");
    exit(4);
}
exit(0);
}