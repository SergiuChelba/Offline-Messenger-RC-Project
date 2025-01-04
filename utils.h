#include<string.h>
#include <stdio.h>
#include<errno.h>



typedef struct UserData 
{
    char username[100];
    char password[100];
} UserData;

typedef struct MessageData 
{
    int messageId;
    char recipient[100]; 
    char message[256]; 
} MessageData;

typedef struct MessageHistory
{
    unsigned int messageCount;   // Numărul de mesaje
    char **messages;             // Array de mesaje
} MessageHistory;

typedef struct ConversationData 
{
    char username[100];    // Numele utilizatorului
    unsigned int numMessages; // Numărul de mesaje
} ConversationData;


// Utility to print errno number
void print_error(int errno)
{
    printf("recv: %s (%d)\n", strerror(errno), errno);
}