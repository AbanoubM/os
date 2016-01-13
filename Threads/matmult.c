#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/time.h>

double ** mata;
double ** matb;
double ** matc;
char *mata_file_name;
char *matb_file_name;
char *matc1_file_name;
char *matc2_file_name;
int x,y,z;

struct timeval stop, start;

struct thread_row_data
{
    int row;
};

struct thread_element_data
{
    int row;
    int col;
};
// check if a given string is a valid double
bool is_valid_double(char *str)
{
    if(str==NULL || strlen(str)==0)
        return false;
    int i;
    bool dot_found=false;
    for(i=0; str[i]!='\0'; i++)
    {
        if(!isdigit(str[i]))
        {
            if(str[i]!='.')
                return false;
            if(dot_found)
                return false;
            dot_found=true;
        }

    }
    return true;
}
/* write matrix c to output file */
void write_matout(int i)
{
    FILE *out;
    if(i==1)
        out = fopen(matc1_file_name, "w");
    else
        out = fopen(matc2_file_name, "w");
    if(out!=NULL)
    {
        int row, col;
        for(row=0; row<x; row++)
        {
            for(col=0; col<z; col++)
                fprintf(out, "%f\t", matc[row][col]);
            fprintf(out, "\n");
        }

        fclose(out);
    }
    else
    {
        printf("%s\n", " ERROR : cann't open output file");
    }
}

/*a thread function that computes each row in the output C matrix*/
void * mul_row_method(void* threadarg)
{
    struct thread_row_data *my_data= (struct thread_row_data *) threadarg;
    int row = my_data->row;
    int i,j;
    double sum;
    for(j=0; j<z; j++)
    {
        sum=0;
        for(i=0; i<y; i++)
            sum+=mata[row][i]*matb[i][j];
        matc[row][j]=sum;
    }
    pthread_exit(NULL);
}
/*a thread function that computes each element in the output C matrix*/
void * mul_element_method(void* threadarg)
{
    struct thread_element_data *my_data= (struct thread_element_data *) threadarg;
    int row = my_data->row;
    int col = my_data->col;
    int i,sum=0;
    for(i=0; i<y; i++)
        sum+=mata[row][i]*matb[i][col];
    matc[row][col]=sum;
    pthread_exit(NULL);
}
/* creates the threads where each thread computes each row in the output C matrix*/
void operate_row_method()
{
    pthread_t threads[x];
    int rc;
    int row;
    for(row=0; row<x; row++)
    {
        struct thread_row_data *data = malloc(sizeof(struct thread_row_data));
        data->row=row;
        rc = pthread_create(&threads[row], NULL, mul_row_method, (void *)data);
        if (rc)
        {
            printf(" ERROR : return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
    for(row=0; row<x; row++)
        pthread_join(threads[row], NULL);
}
/* creates the threads where each thread computes an element in the output C matrix*/
void operate_element_method()
{
    pthread_t threads[x*z];
    int rc;
    int row;
    int col;
    int counter=0;
    for(row=0; row<x; row++)
    {
        for(col=0; col<z; col++)
        {
            struct thread_element_data *data = malloc(sizeof(struct thread_element_data));
            data->row=row;
            data->col=col;
            rc = pthread_create(&threads[counter++], NULL, mul_element_method, (void *)data);
            if (rc)
            {
                printf(" ERROR : could not create thread!\n");
                exit(-1);
            }
        }
    }
    counter=0;
    for(row=0; row<x; row++)
        for(col=0; col<z; col++)
            pthread_join(threads[counter++], NULL);



}
/* initialize the file names associated with matrices A,B and C
argc : number of arguments sent to program call
argv[] : input arguments sent to program call
*/
void initialize_options(int argc, const char * argv[])
{
    /*
    by user's preferences
    */
    if(argc==4)
    {
        if( access(argv[1], F_OK ) == -1)
        {
            printf(" ERROR : first matrix file \"%s\" does not exist or can not be opened!\n",argv[1]);
            exit(0);
        }
        mata_file_name=(char *)malloc(strlen(argv[1])+1);
        strcpy(mata_file_name,argv[1]);

        if( access( argv[2], F_OK ) == -1)
        {
            printf(" ERROR : second matrix file \"%s\" does not exist or can not be opened!\n",argv[2]);
            exit(0);
        }
        matb_file_name=(char *)malloc(strlen(argv[2])+1);
        strcpy(matb_file_name,argv[2]);

        matc1_file_name=(char *)malloc(strlen(argv[3])+3);
        if (matc1_file_name==NULL)
        {
            printf(" ERROR : could not allocate memory for output file name\n");
            exit(-1);
        }
        matc2_file_name=(char *)malloc(strlen(argv[3])+3);
        if (matc2_file_name==NULL)
        {
            printf(" ERROR : could not allocate memory for output file name\n");
            exit(-1);
        }
        strcpy(matc1_file_name,argv[3]);
        strcpy(matc2_file_name,argv[3]);
        strcat(matc1_file_name,"_1");
        strcat(matc2_file_name,"_2");
    }
    /*
    by defaults
    Matrix A : a.txt
    Matrix B : b.txt
    Matrix C : c.out
    */
    else if(argc==1)
    {
        if( access( "a.txt", F_OK ) == -1)
        {
            printf(" ERROR : first matrix file \"%s\" does not exist or can not be opened!\n","a.txt");
            exit(0);
        }
        mata_file_name=(char *)malloc(6);
        strcpy(mata_file_name,"a.txt");

        if( access( "b.txt", F_OK ) == -1)
        {
            printf(" ERROR : second matrix file \"%s\" does not exist or can not be opened!\n","b.txt");
            exit(0);
        }
        matb_file_name=(char *)malloc(6);
        strcpy(matb_file_name,"b.txt");

        matc1_file_name=(char *)malloc(8);
        strcpy(matc1_file_name,"c_1.out");

        matc2_file_name=(char *)malloc(8);
        strcpy(matc2_file_name,"c_2.out");
    }
    else
    {
        printf("%s\n"," ERROR : invalid number of parameters");
        exit(0);
    }
}
/* extracts the matrix dimenions from a string line
line : input string line
dim1 : output first dimention
dim2 : output second dimention
*/
void extract_dim(char * line,int* dim1,int* dim2)
{
    int i;
    int flag=0;
    bool num_found=false;
    int start_index;
    for(i=0; line[i]!='\0'; i++)
    {
        if(line[i]=='=')
        {
            flag++;
            num_found=true;
            start_index=i+1;
        }
        else if(num_found&&!isdigit(line[i]))
        {
            char * temp =(char *)malloc(i-start_index+1);
            int j,k;
            for(k=0, j=start_index; j<i; j++,k++)
                temp[k]=line[j];
            temp[k]='\0';
            if(flag==1)
            {
                *dim1=atoi(temp);
                num_found=false;
            }
            else
            {
                *dim2=atoi(temp);
                break;
            }

        }
    }
    if(flag!=2 || *dim1 <=0 || *dim2 <=0)
    {
        printf(" ERROR : given invalid matrix dimentions in line\n%s",line);
        exit(0);
    }
}
/* initialize the matrices A, B and C using dimenions x,y and z
allocates memeory for each matrix
*/
void initialize_matrices()
{
    char line [512];
    FILE* file = fopen(mata_file_name, "r");
    if (file == NULL)
    {
        printf(" ERROR : first matrix file \"%s\" does not exist or can not be opened!\n",mata_file_name);
        exit(0);
    }
    else
    {
        if(fgets(line, 512, file))
            extract_dim(line,&x,&y);
        fclose(file);
    }
    file = fopen(matb_file_name, "r");
    if (file == NULL)
    {
        printf(" ERROR : second matrix file \"%s\" does not exist or can not be opened!\n",matb_file_name);
        exit(0);
    }
    else
    {
        int temp_y=0;
        if(fgets(line, 512, file))
            extract_dim(line,&temp_y,&z);
        fclose(file);
        if (temp_y != y)
        {
            printf(" ERROR : matrices can not be multiplied, incompatible dimentions! %d x %d and %d x %d\n",x,y,temp_y,z);
            exit(0);
        }
    }


    // x * y
    mata=(double **)malloc(x*sizeof(double *));
    if (mata==NULL)
    {
        printf(" ERROR : could not allocate memory for matrix A\n");
        exit(-1);
    }
    // y * z
    matb=(double **)malloc(y*sizeof(double *));
    if (matb==NULL)
    {
        printf(" ERROR : could not allocate memory for matrix B\n");
        exit(-1);
    }
    // x * z
    matc=(double **)malloc(x*sizeof(double *));
    if (matc==NULL)
    {
        printf(" ERROR : could not allocate memory for matrix C\n");
        exit(-1);
    }
    int i;
    for(i=0; i<x; i++)
        matc[i]=(double *)malloc(z*sizeof(double));
}
/* populate the matrices A and B using pre-defined input files */
void populate_matrices()
{
    char line [100000];
    FILE* file = fopen(mata_file_name, "r");
    if (file == NULL)
    {
        printf("%s\n", "ERROR : matrix A file does not exist or cannot be opened");
        exit(0);
    }
    else
    {
        int row_cursor=0;
        int col_cursor=0;
        fgets(line, 100000, file);
        while (fgets(line, 100000, file)&& row_cursor<x)
        {
            line[strlen(line)-1]='\0';
            mata[row_cursor]=( double *)malloc(y*sizeof(double));
            if (mata[row_cursor]==NULL)
            {
                printf(" ERROR : could not allocate memory for row number %d in matrix A\n",row_cursor+1);
                exit(-1);
            }
            char *token;
            token = strtok(line, "\t");
            col_cursor=0;
            while( is_valid_double(token) && col_cursor<y)
            {
                mata[row_cursor][col_cursor]=atof(token);
                col_cursor++;
                token = strtok(NULL, "\t");
            }
            if (col_cursor != y)
            {
                printf("ERROR : number of valid columns (%d) does not equal col value (%d) given in matrix A file\n",col_cursor,y);
                exit(0);
            }
            row_cursor++;
        }
        fclose(file);
        if (row_cursor != x)
        {
            printf("ERROR : number of lines (%d) does not equal row value (%d) given in matrix A file\n",row_cursor,x);
            exit(0);
        }
    }
    file = fopen(matb_file_name, "r");
    if (file == NULL)
    {
        printf("%s\n", "ERROR : matrix B file does not exist or cannot be opened");
        exit(0);
    }
    else
    {
        int row_cursor=0;
        int col_cursor=0;
        fgets(line, 100000, file);
        while (fgets(line, 100000, file) && row_cursor<y)
        {
            line[strlen(line)-1]='\0';
            matb[row_cursor]=( double *)malloc(z*sizeof(double));
            if (matb[row_cursor]==NULL)
            {
                printf(" ERROR : could not allocate memory for row number %d in matrix B\n",row_cursor+1);
                exit(-1);
            }
            char *token;
            token = strtok(line, "\t");
            col_cursor=0;
            while( is_valid_double(token) && col_cursor<z)
            {
                matb[row_cursor][col_cursor]=atof(token);
                col_cursor++;
                token = strtok(NULL, "\t");
            }
            if (col_cursor != z)
            {
                printf("ERROR : number of valid columns (%d) does not equal col value (%d) given in matrix B file\n",col_cursor,z);
                exit(0);
            }
            row_cursor++;
        }
        fclose(file);
        if (row_cursor != y)
        {
            printf("ERROR : number of lines (%d) does not equal row value (%d) given in matrix B file\n",row_cursor,y);
            exit(0);
        }
    }
}

/* calls the procdure functions */
void operate()
{
    initialize_matrices();
    populate_matrices();
    printf(" MatA  ( %d x %d ) - MatB ( %d x %d ) - MatC ( %d x %d )\n",x,y,y,z,x,z);

    printf("\n");

    gettimeofday(&start, NULL);
    operate_row_method();
    gettimeofday(&stop, NULL);
    printf(" Row method required %d threads\n", x);
    printf(" Row method took %lu in seconds, %lu in milliseconds, %lu in microseconds\n", stop.tv_sec - start.tv_sec,(stop.tv_usec - start.tv_usec)/1000,stop.tv_usec - start.tv_usec);

    printf("\n");

    write_matout(1);

    gettimeofday(&start, NULL);
    operate_element_method();
    gettimeofday(&stop, NULL);
    printf(" Element method required %d threads\n", x*z);
    printf(" Element method took %lu in seconds, %lu in milliseconds, %lu in microseconds\n", stop.tv_sec - start.tv_sec,(stop.tv_usec - start.tv_usec)/1000,stop.tv_usec - start.tv_usec);

    printf("\n");

    write_matout(2);


    free(mata);
    free(matb);
    free(matc);
    free(mata_file_name);
    free(matb_file_name);
    free(matc1_file_name);
    free(matc2_file_name);
}
int main(int argc, const char * argv[])
{
    initialize_options(argc,argv);
    operate();
    return 0;
}

