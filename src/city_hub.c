#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(void){
    char input[256];
    char *args[10];
    while (1) {
        printf("city_hub> ");
        if (fgets(input,sizeof(input),stdin) == NULL){
            printf("\nEXITING\n");
            break;
        }
        input[strcspn(input,"\n")] = 0;

        if (strlen(input) == 0){
            continue;
        }
        int arg_count = 0;
        char *token = strtok(input, " ");
        while (token != NULL && arg_count < 9){
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;
        char *command = args[0];

        if (strcmp(command, "exit") == 0){
            printf("Shutting down City Hub ... \n");
            break;
        }
        else if (strcmp(command,"start_monitor") == 0){
            pid_t hub_mon_pid = fork();
            if (hub_mon_pid < 0){
                perror("Fork for hub_mon failed");
            }
            else if(hub_mon_pid == 0){
                int pipefd[2];
                if (pipe(pipefd) == -1){
                    perror("Pipe failed !");
                    exit(1);
                }
                execlp("./monitor_reports","monitor_reports",NULL);
            }
        }
        else if(strcmp(command, "calculate_scores") == 0){
            if (arg_count < 2){
                printf("Error : Please provide at least one district!\n");
                printf("Usage : calculate_scores <district1> <district2>\n");
            } else{
                int num_districts = arg_count - 1;
                int pipes[num_districts][2];
                pid_t pids[num_districts];
                for (int i = 0; i < num_districts; i++){
                    if (pipe(pipes[i] == -1)){
                        perror("Pipe creation failed");
                        continue;
                    }
                    pids[i] = fork();
                    if (pids[i] == 0){
                        close(pipes[i][0]);
                        dup2(pipes[i][1], STDOUT_FILENO );
                        close(pipes[i][1]);
                        execlp("./scorer","scorer",args[i+1],NULL);
                        perror("Exec scorer failed");
                        exit(1);
                    }
                    close(pipes[i][1]);
                }
                printf("Districts required :");
                for (int i = 1; i < arg_count; i++){
                    printf("%s ", args[i]);
                }
                printf("\n");
            }
        }
        else{
            printf("Unknown command ... Use the following commands : start_monitor, calculate_scores or exit\n");
        }
    }
    return 0;
}