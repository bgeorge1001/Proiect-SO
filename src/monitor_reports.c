#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/wait.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int signal){
    char msg[] = "\n[Monitor] S-a primit SIGINT. Se inchide programul... \n";
    write(STDOUT_FILENO,msg,strlen(msg));
    keep_running = 0;
}

void handle_sigusr1(int signal){
    char msg[] = "[Monitor] Un nou raport a fost adaugat de city_manager!\n";
    write(STDOUT_FILENO,msg,strlen(msg));
}

int main(void){
    struct sigaction sa_int, sa_usr1;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);
    pid_t my_pid = getpid();
    int f_pid = open(".monitor_pid",O_WRONLY | O_CREAT | O_TRUNC ,0644);
    if (f_pid == -1){
        perror("Eroare la creearea fisierului .monitor_pid");
        exit(1);
    }
    char pid_str[32];
    int len = snprintf(pid_str,sizeof(pid_str), "%d\n", my_pid);
    write(f_pid,pid_str,len);
    close(f_pid);
    printf("Monitorul a pornit cu PID-ul : %d\n", my_pid);
    while(keep_running){
        pause();
    }
    if (unlink(".monitor_pid") == 0){
        printf(" [Monitor] Fisierul .monitor_pid a fost sters.\n");
    }
    else{
        perror("[Monitor] Avertisment : Nu s-a putut sterge .monitor_pid");
    }
    printf("Monitorul s-a oprit cu succes!\n");
    return 0;
}