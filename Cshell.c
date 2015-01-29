#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
	
struct cmd {
    int redirect_in;     /* Any stdin redirection?         */
    int redirect_out;    /* Any stdout redirection?        */
    int redirect_append; /* Append stdout redirection?     */
    int background;      /* Put process in background?     */
    int piping;          /* Pipe prog1 into prog2?         */
    char *infile;        /* Name of stdin redirect file    */
    char *outfile;       /* Name of stdout redirect file   */
    char *argv1[10];     /* First program to execute       */
    char *argv2[10];     /* Second program in pipe         */
};

int cmdscan(char *cmdbuf, struct cmd *com);

int main() {
    struct cmd command;
    char buf[1024];
    int p, fd[2], fdin, fdout;
    while( fgets(buf, 1024, stdin) != NULL && strcmp(buf, "exit\n") != 0) {
        pipe(fd);
        if( cmdscan( buf, &command ) ) {
            printf("Illegal Format: \n");
            continue;
        }
        if( command.redirect_out == 1 ) {
            if( command.redirect_append == 1 )
                fdout=open(command.outfile, O_CREAT | O_WRONLY | O_APPEND, 0600);
            else
               	fdout=open(command.outfile, O_CREAT | O_WRONLY| O_TRUNC, 0600);
            if( fdout < 0 ) {
                perror("Outfile Open ERROR");
                exit(-1);
            }
      	}
        if ( command.redirect_in == 1 ) {
            if( (fdin=open(command.infile, O_RDONLY)) < 0 ) {
                perror("Infile Open ERROR");
                exit(-1);
            }
       	}
        switch ( fork() ) {
            case -1:
                perror("fork error");
                exit(-1);
            case 0:
            	if (command.background == 1) {
                    if(fork()>0) exit(0);
                    sleep(2);
               	}
               	if(command.redirect_in == 1)
                    dup2(fdin, STDIN_FILENO);
                if(command.piping == 1) {
                   close(fd[0]);
                   dup2(fd[1], STDOUT_FILENO);
                   close(fd[1]);
               	}
                else if( command.redirect_out == 1 )
                    dup2(fdout, STDOUT_FILENO);
                execvp(command.argv1[0], command.argv1);
                perror("exec error");
                exit(-1);
            default:
            	if(command.piping == 1) {
                    switch( fork() ) {
                        case -1:
                            perror("fork error");
                            exit(-1);
                       	case 0:
                            // create a orphan grandchild so init adopts it
                            if (command.background == 1) {
                                if(fork()>0) exit(0);
                                sleep(2);
                            }
                            close(fd[1]);
                            dup2(fd[0], STDIN_FILENO);
                            close(fd[0]);
                            if ( command.redirect_out == 1 )
                            	dup2(fdout, STDOUT_FILENO);
                           	execvp(command.argv2[0], command.argv2);
                            perror("exec error");
                            exit(-1);
                    }
                }
                close(fd[0]);
                close(fd[1]);
                wait(NULL);
        }
    }
    return 0;
}
