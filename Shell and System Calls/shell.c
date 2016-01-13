#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <termios.h>

char *history_file_name;
char *log_file_name;
char ** variables_array;
char ** values_array;
char **parsed;
char **file_names;
int cursor;
int variables_counter;

const char * ENVN_VAR[]=
{
    "PATH"
    ,"HOME"
    ,"TERM"
    ,"PS1"
    ,"MAIL"
    ,"TEMP"
    ,"JAVA_HOME"
    ,"ORACLE_HOME"
    ,"TZ"
    ,"PWD"
    ,"HISTFILE"
    ,"HISTFILESIZE"
    ,"HOSTNAME"
    ,"LD_LIBRARY_PATH"
    ,"USER"
    ,"DISPLAY"
    ,"SHEL"
    ,"TERMCAP"
    ,"OSTYPE"
    ,"MACHTYPE"
    ,"EDITOR"
    ,"PAGER"
    ,"MANPATH"
};

/*
get variable value if found
return index of val if found else -1
*/
int get_shell_variable(char * variable)
{

    int i;
    for(i=0; i<variables_counter; i++)
    {
        if(strcmp(variables_array[i],variable)==0)
            return i;
    }
    return -1;
}
/*
check if given variable is an environmental variable or not
*/
bool is_env_variable(char * variable)
{
    int length=sizeof(ENVN_VAR) / sizeof(ENVN_VAR[0]);
    int i;
    for(i=0; i<length; i++)
    {
        if(strcmp(ENVN_VAR[i],variable)==0)
            return true;
    }
    return false;
}
/* set variable value if found
else add it
*/
void add_shell_variable(char * variable,char * value)
{
    int i;
    for(i=0; i<variables_counter; i++)
    {
        if(strcmp(variables_array[i],variable)==0)
        {
            values_array[i]=(char *)malloc(strlen(value)+1);
            strcpy(values_array[i],value);
            return;
        }
    }
    variables_array[variables_counter]=(char *)malloc(strlen(variable)+1);
    strcpy(variables_array[variables_counter],variable);
    values_array[variables_counter]=(char *)malloc(strlen(value)+1);
    strcpy(values_array[variables_counter],value);
    variables_counter++;
}
/* display commands history */
void display_history()
{
    char command [514];
    FILE* in = fopen(history_file_name, "r");
    if (in == NULL)
        printf("%s\n", "ERROR : cann't open history file");
    else
    {
        while (fgets(command, 514, in))
            printf("%s", command);
        fclose(in);
    }
}
/* add command to the end of hisotry file */
void append_history(char *command)
{
    FILE *out = fopen(history_file_name, "a");
    if(out!=NULL)
    {
        fprintf(out, "%s", command);
        fclose(out);
    }
    else
    {
        printf("%s\n", " ERROR : cann't open history file");
    }
}
/* add log to the end of log file */
void append_log()
{
    FILE *log_out = fopen(log_file_name, "a");
    if(log_out!=NULL)
    {
        fprintf(log_out, "%s", "child process was terminated\n");
        fclose(log_out);
    }
    else
    {
        printf("%s\n", " ERROR : cann't open log file");
    }
}
/* check if string is blank or empty */
bool is_blank(char str[])
{
    int i;
    for (i = 0; str[i] != '\n' && str[i] != '\0'; i++)
        if(str[i]!=' ')
            return false;
    return true;
}
/* parse a line into command and parameters "if found"
parsed : (output) array of cmd and parameters
cursor : (output) number of parameters including the cmd
line : (input) the command line
return 1 if command valid 0 otherwise
*/
int parse_command(char * line)
{
    cursor=0;
    parsed = (char **)malloc(513 * sizeof (char *));
    bool quot_found=false;
    int start_ptr=-1,end_ptr=-1;
    int i;

    for (i = 0; line[i] != '\0' && line[i] != '\n'; i++)
    {
        if(!quot_found)
        {
            if(line[i]=='"')
            {
                start_ptr=i+1;
                quot_found=true;
            }
            else if(line[i]==' ')
            {
                if(start_ptr!=-1)
                {
                    end_ptr=i-1;
                    parsed[cursor]=(char *)malloc((end_ptr-start_ptr+2) *sizeof(char));
                    int j,index;
                    for(index=0, j=start_ptr; j<=end_ptr; j++,index++)
                        parsed[cursor][index]=line[j];
                    parsed[cursor][index]='\0';
                    cursor++;
                    start_ptr=-1;
                }
            }
            else
            {
                if(start_ptr==-1)
                    start_ptr=i;
            }
        }
        else if(line[i]=='"')
        {
            quot_found=false;
            end_ptr=i-1;

            if(end_ptr>=start_ptr)
            {
                parsed[cursor]=(char *)malloc((end_ptr-start_ptr+2) *sizeof(char));
                int j,index;
                for(index=0, j=start_ptr; j<=end_ptr; j++,index++)
                    parsed[cursor][index]=line[j];
                parsed[cursor][index]='\0';
                ++cursor;
            }
            start_ptr=-1;
        }
    }
    if(start_ptr!=-1)
    {

        end_ptr=i-1;
        parsed[cursor]=(char *)malloc((end_ptr-start_ptr+2) *sizeof(char));
        int j,index;
        for(index=0, j=start_ptr; j<=end_ptr; j++,index++)
            parsed[cursor][index]=line[j];
        parsed[cursor][index]='\0';
        cursor++;
    }
    parsed[cursor]=NULL;
    if(quot_found)
        return 0;
    return 1;
}
/*
execute the change directory command with its parameters
*/
void handle_cd()
{
    if(cursor>2)
    {
        printf("%s\n", "ERROR : too many arguments");
        return;
    }
    int chdir_id;
    if(cursor==1 || strcmp(parsed[1],"~")==0)
        chdir_id=chdir(getenv("HOME"));
    else if(parsed[1][0]=='~')
    {
        char * home_temp=getenv("HOME");
        char * cd_temp = (char *)malloc(strlen(parsed[1])+strlen(home_temp));
        int j,k;
        for(j=0; j<strlen(home_temp); j++)
            cd_temp[j]=home_temp[j];

        for(k=1; k<strlen(parsed[1]); j++,k++)
            cd_temp[j]=parsed[1][k];
        cd_temp[j]='\0';
        chdir_id=chdir(cd_temp);
        free(cd_temp);

    }
    else
        chdir_id=chdir(parsed[1]);

    if(chdir_id!=0)
        printf("%s\n", "ERROR : can't change directory");
}
/*
execute the expression assignment commands
*/
void handle_expression(char* sub)
{
    int index=strlen(parsed[0])-strlen(sub);

    if(cursor!=1 || index==0|| strlen(sub)==1 ||strlen(parsed[0])<3 )
        printf("%s\n", "ERROR : command not found");
    else
    {
        char * cmd_var=( char *)malloc(index+2);
        int j;
        for(j=0; j<index; j++)
            cmd_var[j]=parsed[0][j];
        cmd_var[j]='\0';

        char * cmd_val;
        int k;
        if(parsed[0][index+1]=='$')
        {
            cmd_val=(char *)malloc(strlen(sub)-1);
            for(j=index+2,k=0; j<strlen(parsed[0]); j++,k++)
                cmd_val[k]=parsed[0][j];
            cmd_val[k]='\0';

            int check =get_shell_variable(cmd_val);
            if(check==-1)
            {
                if(is_env_variable(cmd_val))
                    add_shell_variable(cmd_var,getenv(cmd_val));
                else
                    add_shell_variable(cmd_var,"");
            }
            else
            {
                add_shell_variable(cmd_var,values_array[check]);
            }
        }
        else
        {
            cmd_val=( char *)malloc(strlen(sub));
            for(j=index+1,k=0; j<strlen(parsed[0]); j++,k++)
                cmd_val[k]=parsed[0][j];
            cmd_val[k]='\0';
            add_shell_variable(cmd_var,cmd_val);

        }
        free(cmd_val);
        free(cmd_var);
    }
}
/*
replaces the $ variables (shell or environmental) by their equivalent values
*/
void handle_variables()
{
    int i;
    for(i=1; i<cursor; i++)
    {
        if(parsed[i][0]=='$')
        {
            char * var_temp=( char *)malloc(strlen(parsed[i]));
            int k;
            int j;
            for(j=0,k=1; parsed[i][k]!='\0'; k++,j++)
                var_temp[j]=parsed[i][k];
            var_temp[j]='\0';

            int check =get_shell_variable(var_temp);
            if(check==-1)
            {
                if(is_env_variable(var_temp))
                {
                    char * env_str=getenv(var_temp);
                    parsed[i]=(char *)malloc(strlen(env_str)+1);
                    strcpy(parsed[i],env_str);

                }
                else
                {
                    parsed[i]=(char *)malloc(1);
                    strcpy(parsed[i],"");
                }
            }
            else
            {
                parsed[i]=(char *)malloc(strlen(values_array[check])+1);
                strcpy(parsed[i],values_array[check]);
            }
            free(var_temp);
        }
    }

}
/*
execute the command by making system calls
*/
void exec_command()
{
    bool is_bg_task=false;
    if(strcmp(parsed[cursor-1],"&")==0)
    {
        is_bg_task=true;
        parsed[cursor-1]=NULL;
    }
    int status;
    int id=fork();

    if(id>0)
    {
        if(!is_bg_task)
            waitpid(id, &status, 0);
    }
    else if(id==0)
    {
        if(access( parsed[0], F_OK ) != -1)
        {
            if(execv(parsed[0],parsed) ==-1)
            {
                perror("ERROR ");
                exit(0);
            }
        }
        else
        {

            int i;
            for(i=0; file_names[i]!=NULL; i++)
            {
                char * exe_path =(char *)malloc(strlen(file_names[i])+strlen(parsed[0])+2);
                strcpy(exe_path,file_names[i]);
                strcat(exe_path,"/");
                strcat(exe_path,parsed[0]);

                if( access( exe_path, F_OK ) != -1 )
                {
                    parsed[0]=(char *)malloc(strlen(exe_path)+1);
                    strcpy(parsed[0],exe_path);
                    if(execv(parsed[0],parsed) ==-1)
                    {
                        perror("ERROR ");
                        exit(0);
                    }
                    return;
                }

            }
            // command didn't match any, exit the child process
            printf("%s\n", "ERROR : command not found");
            exit(0);
        }
    }
    else
        printf("%s\n", "ERROR : fork failed");
}
/*
determine the command type
*/
void handle_command()
{
    if(parsed[0][0]=='#')
        return;
    if(strcmp(parsed[0],"exit")==0)
        exit(0);
    if(strcmp(parsed[0],"history")==0)
    {
        if(cursor!=1)
            printf("%s\n", "ERROR : invalid command");
        else
            display_history();
        return;

    }
    handle_variables();

    if(strcmp(parsed[0],"cd")==0)
        handle_cd();
    else
    {
        char* sub=strchr(parsed[0],'=');
        if(sub!=NULL)
        {
            handle_expression(sub);
        }
        else
        {
            exec_command();

        }
    }


}
/* start the batch mode procedures
takes user's input, calls parser and executer functions
*/
void start_interactive_mode()
{
    char command [514];
    printf("shell >> ");
    while (fgets (command, 514, stdin))
    {
        if(!is_blank(command))
        {
            if(strlen(command)>512)
                printf("%s\n", "ERROR : very long command line (over 512 characters)");
            else
            {
                append_history(command);
                if(parse_command(command)==0)
                    printf("%s\n", "ERROR : invalid command");
                else
                    handle_command();
            }
        }
        printf("shell >> ");
    }
}
/* start the batch mode procedures
takes batch file, calls parser and executer functions
*/
void start_batch_mode(const char *file_name)
{
    char command [514];
    FILE* file = fopen(file_name, "r");
    if (file == NULL)
        printf("%s\n", "ERROR : batch file does not exist or cannot be opened");
    else
    {
        int line_num=1;
        while (fgets(command, 514, file))
        {
            printf("\n cmd line %d , length= %d : %s\n", line_num,(int)strlen(command),command);
            line_num++;
            if(!is_blank(command))
            {
                if(strlen(command)>512)
                    printf("%s\n", "ERROR : very long command line (over 512 characters)");
                else
                {
                    append_history(command);
                    if(parse_command(command)==0)
                        printf("%s\n", "ERROR : invalid command");
                    else
                        handle_command();
                }
            }
        }
        fclose(file);

    }
}
/*
initialize variables and values array, counter and
handle the history and log files directroy
*/
void initialize()
{
    variables_array=(char **)malloc(512 * sizeof (char *));
    values_array=(char **)malloc(512 * sizeof (char *));
    variables_counter=0;

    char history []="/shell_history";
    char log []="/shell_log";
    char * temp=getenv("HOME");
    history_file_name=( char *)malloc(strlen(temp)+15);
    strcpy(history_file_name, temp);
    strcat(history_file_name, history);

    log_file_name=( char *)malloc(strlen(temp)+11);
    strcpy(log_file_name, temp);
    strcat(log_file_name, log);
}
/*
handle file names of exe
*/
void handle_files_names()
{
    int index=0;
    file_names=(char **)malloc(20 * sizeof (char *));
    char * arr =getenv("PATH");
    char *token;
    token = strtok(arr, ":");
    while( token != NULL )
    {
        file_names[index]=( char *)malloc(strlen(token)+1);
        strcpy(file_names[index], token);
        index++;

        token = strtok(NULL, ":");
    }
    file_names[index]=NULL;
}
/*
handle Ctrl+D termiantion
*/
void ctrld_action(){
exit(0);
}
int main(int argc, const char * argv[])
{

    struct termios t;
    tcgetattr(0,&t);
    t.c_cc[VEOF]  = 3;
    t.c_cc[VINTR] = 4;
    tcsetattr(0,TCSANOW,&t);

    signal(SIGINT, ctrld_action);

    signal(SIGCHLD,append_log);

    initialize();
    handle_files_names();

    if(argc==1)
        start_interactive_mode();
    else if(argc==2)
        start_batch_mode(argv[1]);
    else
        printf("%s\n","invalid or undefined command");
    free(variables_array);
    free(values_array);
    free(log_file_name);
    free(history_file_name);
    free(parsed);
    free(file_names);
    return 0;
}

