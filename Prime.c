#include <fcntl.h> /*Open and close system calls*/
#include <unistd.h> /*write and read system calls*/
#include <stdlib.h> /*For malloc*/
#include <stdio.h> /*For perror*/
#include <sys/wait.h> /*wait system calls*/
#include <math.h> /*for square root*/
#include <time.h> /*nano sleep*/

/*Converting string to integer*/
int *StrToInt(char *Str, int *Size)
{
    int i=0;
    int ArraySize=0;

    /*Convert the first characters in the array before the first \n */
    /*The first integer represents the array size.*/
    while(Str[i]!='\n')
    {
        ArraySize=ArraySize*10 + (Str[i] - '0');  /*converting char to digit*/
        i++;
    }
    *Size=ArraySize; /*pass the value of size by reference*/
    i++; /*Bypass \n in the string*/

    /*Alocate dynamic memory*/
    int *Num=(int *)malloc(ArraySize*sizeof(int));

    /*Error checking for allocated memory*/
    if(Num==NULL)
    {
        perror("malloc failed");
    }

    int result=0;
    int j=0; /*Index to for the Num array*/
    while(Str[i]!='\0')
    {
        if(Str[i]!='\n')   /*Convert str to int before \n */
        {
            result=result*10 + (Str[i] - '0');  /*Result is the converted string before \n to int*/
        }

        else  /*When \n is encountered increment j by one*/
        {
            Num[j]=result;  /*store the result to the array*/
            j++; /*Increment the index everytime I store */
            result=0; /*Reset for the next number*/
        }
        i++; /*Increment i to move on to the next character in the string*/
    }
    return Num; /*Return the array*/
}

/*Check the num for primality*/
/*If prime, return 1*/
/*If not prime, return 0*/
int isPrime(int Num)
{
    int Prime=0;
    if(Num<=1)
    {
        Prime=0;
    }

    else if(Num==2)
    {
        Prime=1;
    }

    else  /*Trial division algorithm*/
    {
        int Sqrt=sqrt(Num); /*Calculate the square root of the num*/
        Prime=1; /*Set the prime to true*/
        for(int i=2; i<=Sqrt && Prime!=0; i++)  /*The process immediately terminates when it can be divided by one num*/
        {
            struct timespec t;
            t.tv_sec=0; /*Initialize second*/
            t.tv_nsec=1000000L; /*Initialize nanosecond*/
            nanosleep(&t, NULL); /*sleep for 1ms*/
            if(Num % i==0)
            {
                Prime=0; /*Prime becomes false when the num is divisble by other number*/
            }
        }
    }
    return Prime; /*Return the result, 1 for prime number, 0 for non-prime num*/
}

/*Count the length of the string*/
int count_str(char *Str)
{
    int len=0; /*Initialize a counter len*/
    while(Str[len]!='\0')
    {
        len++; /*Keep incrementing the counter until null character*/
    }
    return len;
}

/*Convert int to str so that the number can be displayed on console. */
void convert_int_to_str(int Num, char *Buf)
{
    int i=0;
    int j;
    char temp[20]; /*Create temp char arrays to store the inverse of the number*/

    if(Num==0) /*Handles exception case like Num=0*/
    {
        Buf[0]='0'; /*if the num is 0, we can manually assign the string to the array*/
        Buf[1]='\0'; /*Include the string terminator*/
    }

    else
    {
        /*Num % 10 gives us the last digit of the number.*/
        /*This approach will store the integers as char arrays in reverse order*/
        while(Num>0)
        {
            temp[i]=(Num%10) + '0'; 
            i++;
            Num=Num/10; /*Removes the last digit*/
        }

        /*This approach convert the reverse int back to normal*/
        /*i in case will be the length of the integer*/
        /*i-j-1 gives the index of the last digit in temp array*/
        for(j=0; j<i; j++)
        {
            Buf[j]=temp[i-j-1];
        }
        Buf[j]='\0'; /*Include string terminator in the array*/
    }
}

/*This function handles pipes creation. */
/*Parent writes integer to child via parent_to_child_pipe*/
/*Parent reads result of primality check from child via child_to_parent_pipe*/
/*Child reads integer from parent*/
/*Child sends the int to isPrime func*/
/*Child write the result to the parent*/
/*Parent wait for any child to terminate before printing to console*/
void Two_Way_Pipe(int ArraySize, int *N)
{
    /*Create two pipes to establish two way communication between child and parent*/
    int parent_to_child_pipe[ArraySize][2]; /*Create an array of 12 parent to child pipes*/
    int child_to_parent_pipe[ArraySize][2]; /*Create an array of 12 child to parent pipes*/

    /*Create an array of child_id to compared with the terminated id*/
    pid_t *child_id=malloc(sizeof(pid_t) * ArraySize); 

    for(int i=0; i<ArraySize; i++)
    {
        /*Error checking for each pipe created*/
        if(pipe(parent_to_child_pipe[i])==-1 || pipe(child_to_parent_pipe[i])==-1)
        {
            perror("Error in creating pipe");
        }

        else
        {
            /*Create a child process*/
            pid_t pid=fork();

            if(pid < 0)
            {
                perror("Error in forking");
            }

            else if(pid==0) /*Child process*/
            {
                /*Child reading message from parent*/
                close(parent_to_child_pipe[i][1]); /*Close write end parent's pipe*/
                int ReceivedNum;
                read(parent_to_child_pipe[i][0], &ReceivedNum, sizeof(ReceivedNum));
                close(parent_to_child_pipe[i][0]); /*Close read end parent's pipe*/

                /*Child writing message to parent*/
                close(child_to_parent_pipe[i][0]); /*Close read end of child pipe*/
                int result=isPrime(ReceivedNum);
                write(child_to_parent_pipe[i][1], &result, sizeof(result));
                close(child_to_parent_pipe[i][1]);
                
                /*Since each child will get a copy of the parent's memory, we need to free the resources of the child before exiting*/
                free(N);
                free(child_id);

                exit(0); /*Child will exit after execution*/
            }

            else /*parent process*/
            {
                /*Store the child id*/
                child_id[i]=pid;

                /*Parent writing message to child*/
                close(parent_to_child_pipe[i][0]); /*Close read end*/
                write(parent_to_child_pipe[i][1], &N[i], sizeof(int));  /*Parent writes to child*/
                close(parent_to_child_pipe[i][1]); /*Close write end*/
            }
        }
    }

    for(int j=0; j<ArraySize; j++)
    {
        /*Parent waits for any terminated child*/
        /*Wait system calls return the pid of the terminated child which is used to read the correct pipe*/
        pid_t terminated_child_id=wait(NULL);

        /*Find the child that has terminated*/
        for(int i=0; i<ArraySize; i++)
        {
            if(child_id[i]==terminated_child_id)
            {
                int result_Num;
                close(child_to_parent_pipe[i][1]); /*Close the unused write end pipe*/
                read(child_to_parent_pipe[i][0], &result_Num, sizeof(int)); /*read the result from child*/
                close(child_to_parent_pipe[i][0]); /*Close read end of child to parent*/
                if(result_Num ==1)
                {
                    char buff[20]; /*Create a array for pass by reference to convert_int_str func*/
                    int len=count_str("Child "); /*Pass the string to count_str for length before printing to console*/
                    write(1,"Child ", len); /*Write to the console after knowing the length*/
                    convert_int_to_str(i,buff); /*Convert the array index to str and this str will be stored in the array buff*/
                    len=count_str(buff);
                    write(1,buff,len);
                    len=count_str(" completed the task: "); 
                    write(1, " completed the task: ",len);
                    convert_int_to_str(N[i], buff);
                    len=count_str(buff);
                    write(1,buff,len);
                    len=count_str(" is a prime number.\n");
                    write(1," is a prime number.\n",len);
                }

                else
                {
                    char buff[20]; /*Create a array for pass by reference to convert_int_str func*/
                    int len=count_str("Child "); /*Pass the string to count_str for length before printing to console*/
                    write(1,"Child ", len); /*Write to the console after knowing the length*/
                    convert_int_to_str(i,buff); /*Convert the array index to str and this str will be stored in the array buff*/
                    len=count_str(buff);
                    write(1,buff,len);
                    len=count_str(" completed the task: "); 
                    write(1, " completed the task: ",len);
                    convert_int_to_str(N[i], buff);
                    len=count_str(buff);
                    write(1,buff,len);
                    len=count_str(" is not a prime number.\n");
                    write(1," is not a prime number.\n",len);
                }
            }
        }
    }
    free(child_id); /*Free the memory allocated for child_id*/
}

/*Performing file reading and store char of numbers in array*/
int main(int argc, char *argv[])
{
    /*Perform argument check first*/
    if(argc < 2)
    {
        perror("Not enough argument");
    }

    else
    {
        int fd=open(argv[1], O_RDONLY); /*Open the file in read only mode*/
        if(fd<0)
        {
            perror("Error in opening the file");
        }

        else
        {
            int byteread; /*Variable to store the byte read each time*/
            char ch[4];   /*read a chunk of 4 bytes*/
            char Num[100]; /*Store all int from the file*/
            int i=0;
            while((byteread=read(fd,&ch,sizeof(ch)))>0) /*Keep reading until the end of the file. read will read up to 4 bytes at a time.*/
            {
                for(int j=0; j<byteread && i<99; j++) /*i<99 is to prevent overflow*/
                {
                    Num[i]=ch[j];
                    i++;
                }
            }
            Num[i]='\n'; /*add \n to the end as the file does not contain \n at the last line*/
            Num[i+1]='\0'; /*String terminator*/

            /*Pass the num array to StrToInt to convert each str to char*/
            int Size=0; /*This variable is used for pass by reference to the func*/
            int *N=StrToInt(Num, &Size); /*This function returns dynamic memory allocated array*/ 
            Two_Way_Pipe(Size, N); /*This func deals with pipes creation*/
            free(N); /*free the memory allocated for N*/
        }
        close(fd);
    }
    return 0;
}