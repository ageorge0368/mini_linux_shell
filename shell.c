#include  <stdio.h>
#include  <sys/types.h>

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>

// Doubly Linked-list //
struct node {
    char *word;
    int isCommand;              // if LL element is a command, then isCommand = 1, else isCommand = 0, else if LL element is a command symbol, then isCommand = 2

    int hasPipe;                // if shell input as a pipe command, then hasPipe = 1, else hasPipe = 0
    int hasOutReDir;            // if shell input as a ">" command, then hasOutReDir = 1, else hasOutReDir = 0
    int hasInReDir;             // if shell input as a "<" command, then hasInReDir = 1, else hasInReDir = 0
    int hasAnd;                 // if shell input as a "&" command, then hasAnd = 1, else hasAnd = 0
    struct node *next;
    struct node *prev;
};

void createLinkedList(struct node* head, char **argv) {
    // initialize a linkedList
        // head and tail point at first node

    head->hasPipe = 0;
    struct node *current = head;        // current pointer
    struct node *tail = head;
    int i = 0;                          // counter

    while (strcmp(argv[i], "\0") != 0) {
        struct node *newNode = (struct node*) malloc(sizeof(struct node));

        newNode->hasPipe = newNode->hasOutReDir = newNode->hasInReDir = newNode->hasAnd = 0;

        if (i == 0 || (current->isCommand == 2 && strcmp(current->word, "|") == 0)) {
            newNode->isCommand = 1;
        } else if (strcmp(argv[i], "|") == 0 || strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0 || strcmp(argv[i], "&") == 0) {
            newNode->isCommand = 2;

            if (strcmp(argv[i], "|") == 0) {
                head->hasPipe = 1;
            } else if (strcmp(argv[i], ">") == 0) {
                head->hasOutReDir = 1;
            } else if (strcmp(argv[i], "<") == 0) {
                head->hasInReDir = 1;
            } else {
                head->hasAnd = 1;
            }
        } else {
            newNode->isCommand = 0;
        }

        newNode->word = strdup(argv[i]);                       // newNode has word from argv
        current->next = newNode;
        newNode->prev = current;

        current = current->next;
        tail = current;

        i++;
    }
}

void printLinkedList(struct node* head) {
    // printing commands line
    fputs("Commands:", stdout);
    struct node* current = head;
    while(current->next != NULL) {
        if (current->isCommand == 1) {
            fputs(" ", stdout);
            fputs(current->word, stdout);
        }

        current = current->next;
    }

    fputs("\n", stdout);

    current = head;
    while(current->next != NULL) {
        if (current->isCommand == 1) {
            struct node* commandPtr = current;
            fputs(commandPtr->word, stdout);
            fputs(": ", stdout);

            commandPtr = commandPtr->next;
            while (commandPtr->isCommand == 0) {
                fputs(commandPtr->word, stdout);
                fputs(" ", stdout);
                if (commandPtr->next == NULL) {
                    break;
                }
                commandPtr = commandPtr->next;
            }

            fputs("\n", stdout);
        }

        current = current->next;
    }

    fputs("\n", stdout);
}

void parse(char *line, char **argv)
{
     while (*line != '\0') {       /* if not the end of line ....... */ 
          while (*line == ' ' || *line == '\t' || *line == '\n')
               *line++ = '\0';     /* replace white spaces with 0    */
          *argv++ = line;          /* save the argument position     */
          while (*line != '\0' && *line != ' ' && 
                 *line != '\t' && *line != '\n') 
               line++;             /* skip the argument until ...    */
     }
     *argv = '\0';                 /* mark the end of argument list  */
}

void executePipe(struct node* head, char **argv) {
    // fork again
    // open a pipe file
    // store output/result into input of pipe
        // execvp(first command, arguement)
    // store output/result into output of pipe (i.e. print result)
        // execvp(second command, argument, input from pipe)
    // close both ends of parent pipe
    // waitpid for both file descriptors for pipePIDS array

    // run pipe in parent process   (DONE)
    pid_t pipePIDs[2];

    if (pipe(pipePIDs) == -1) {
        perror("Error with making pipe!");
        exit(1);
    }

    // create argv1 and argv2
    char **argv1 = (char**)malloc(10 * sizeof(char *));                 // arbitrary size of argv1 and argv2
    char **argv2 = (char**)malloc(10 * sizeof(char *));
    struct node* current = head;                    // going to first string in command line (argv)
    current = current->next;
    int c1_index = 0;
    int c2_index = 0;

    while (strcmp(current->word, "|") != 0 && current->next != NULL) {
        argv1[c1_index] = current->word;
        c1_index++;
        current = current->next;
    }

    current = current->next;                        // skipping past | in command line

    while (current != NULL) {
        argv2[c2_index] = current->word;
        c2_index++;
        current = current->next;
    }

    // run fork for first child process                 (create P and C1)
    //              P
    //             / \
    //            P   C1

    pid_t child_1 = fork();
    if (child_1 == -1) {
        perror("Error with forking for first child!");
        exit(1);
    }

    // C1 => does left hand side of command line
        // run left hand side of command line
    // C2 => does right hand side of command line
        // run right hand side of command line
    // P => close both ports of parent pipe and do waitpid on both C1 and C2

    if (child_1 == 0) {                                 // child 1 process
        close(pipePIDs[0]);                         // closing read end to pipe, since we are writing
        dup2(pipePIDs[1], STDOUT_FILENO);
        execvp(argv1[0], argv1);
    } else {
        pid_t child_2 = fork();                         // run fork for second process          (create P and C2)
        //              P
        //             / \
        //            P   C1
        //           /      \
        //          P       C2

        if (child_2 == 0) {                             // child 2 process
            close(pipePIDs[1]);                     // closing writing end to pipe, since we are reading
            dup2(pipePIDs[0], STDIN_FILENO);
            execvp(argv2[0], argv2);
        } else {                                        // parent process
            close(pipePIDs[0]);                     // closing both sides of parent pipe
            close(pipePIDs[1]);
            waitpid(child_1, NULL, 0);
            waitpid(child_2, NULL, 0);
        }
    }
}

void execute(struct node* head, char **argv) //write your code here
{

//HINT you need to introduce the following functions: fork, execvp, waitpid
//Advanced HINT: pipe, dup2, close, open

    pid_t  pid;
    int    status;

    int andFlag = 0;
    if (head->hasAnd == 1) {
        andFlag = 1;
    }

    if (head->hasPipe == 1) {                   // create and do pipe only if pipe is found in the command line
        executePipe(head, argv);
    } else {
        pid = fork();                             // forking process first

        if (pid == 0) {                                 // child process
            if (head->hasOutReDir == 1) {                       // ">" command
                struct node* current = head;
                int j = 0;
                current = current->next;

                while (strcmp(current->word, ">") != 0 && current->next != NULL) {
                    current = current->next;
                    j++;
                }

                argv[j] = NULL;                                             // making ">" symbol be NULL

                // Create new PID for output
                pid_t outputPID;

                // open output file
                outputPID = open(current->next->word, O_RDWR | O_CREAT, 0644);                // read/write permission and file should be created if DNE

                if (outputPID == -1) {
                    perror("Error opening file. ('>' command)");
                    exit(1);
                }

                // use dup2 to redirect all traffic from stdout to outputPID
                dup2(outputPID, STDOUT_FILENO);

                // close outputPID
                close(outputPID);

            } else if (head->hasInReDir == 1) {                 // "<" command
                struct node* current = head;
                int j = 0;
                current = current->next;

                while (strcmp(current->word, "<") != 0 && current->next != NULL) {
                    current = current->next;
                    j++;
                }

                argv[j] = NULL;                                             // making "<" symbol be NULL

                // Create new PID for input
                pid_t inputPID;

                // open input file
                inputPID = open(current->next->word, O_RDWR);                // read/write permission and file should be created if DNE

                if (inputPID == -1) {
                    perror("Error opening file. ('<' command)");
                    exit(1);
                }

                // use dup2 to redirect all traffic from stdout to outputPID
                dup2(inputPID, STDIN_FILENO);

                // close inputPID
                close(inputPID);

            } else if (head->hasAnd == 1) {                     // "&" command
                struct node* current = head;
                int j = 0;
                current = current->next;

                while (strcmp(current->word, "<") != 0 && current->next != NULL) {
                    current = current->next;
                    j++;
                }

                argv[j] = NULL;                                // making "&" symbol be NULL
            }

            // Always execute code in any case, >, <, &, or command with no special characters
            execvp(argv[0], argv);
            
        } else {                                                // parent process
            if (andFlag == 0) {                                 // if no background execution, then waitpid, if there is background execution, then no waitpid
                waitpid(pid, NULL, 0);
            }
        }
    }
}

void main(void)
{
     char  line[1024];             /* the input line                 */
     char  *argv[64];              /* the command line argument      */

     while (1) {                   /* repeat until done ....         */
          fputs("Shell -> ",stdout);     /*   display a prompt             */
          fgets(line, 1024, stdin);              /*   read in the command line     */
          fputs("\n", stdout);
          parse(line, argv);       /*   parse the line               */

          struct node *head = (struct node*) malloc(sizeof(struct node));

          createLinkedList(head, argv);                 // creates a linked list from elements in argv

          if (strcmp(argv[0], "exit") == 0)  /* is it an "exit"?     */                         // does exit command when needed
             exit(0);            /*   exit if it is                */

          printLinkedList(head);                        // prints the linked list in necessary format

          execute(head, argv);           /* otherwise, execute the command */
     }
}
