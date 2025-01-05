/*---------------------------------------------------------------------*/

/* DECLARARI HEADERE*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sqlite3.h>
#include "commands.h"
#include "database.h"
#include "utils.h"

/*---------------------------------------------------------------------*/

/* DECLARARI GLOBALE*/

#define PORT 2908

extern int errno; 

/*---------------------------------------------------------------------*/

/* Structura in care voi retine datele clientilor*/

typedef struct ClientData 
{
    int idThread; // Contor pentru clienți
    int socketID; // Socket din accept
    int loggedIn; // Starea utilizatorului: 0 = delogat / 1 = logat
    unsigned int dataBaseID;
    COMMAND* com_received;
    COMMAND* com_response;
    struct UserData* userData; // Datele utilizatorului curent
    struct MessageData* messageData; // Mesajul curent
    struct ConversationData* conversationData; // Lista de conversații
    uint8_t numConversations; // Numărul de conversații
    uint8_t numUsers;
} ClientCommunication;

struct ClientData* connectedClients[100]; //hardcodat momentan

/*---------------------------------------------------------------------*/

/* FUNCTIA DE TRATARE A CLIENTILOR*/

static void* treat(void *arg)
{
    struct ClientData tdL = *((struct ClientData *)arg);
    printf("[thread %d] Asteptam comenzile clientului...\n", tdL.idThread);
    connectedClients[tdL.idThread] = &tdL;
    fflush(stdout);

    pthread_detach(pthread_self());
    clientLoop((struct ClientData *)arg);

    close(tdL.socketID);
    free(arg);
    return NULL;
}   

/*---------------------------------------------------------------------*/

/* FUNCTIA CARE DISCUTA CU CLIENTUL*/

void clientLoop(void *arg)
{
    struct ClientData client = *((struct ClientData *)arg);
    char dataBuffer[256];
    int bytes;

    while (1)
    {
        if ((bytes = recv(client.socketID, dataBuffer, sizeof(dataBuffer) - 1, 0)) < 0)
        {
            printf("[Thread %d] Client disconnect.\n", client.idThread);
            break;
        }

        if (bytes > 0) 
        {
            printf("[Debug] Date brute primite (bytes=%d): ", bytes);
            for (int i = 0; i < bytes; i++) {
                printf("%02X ", (unsigned char)dataBuffer[i]);
            }
            printf("\n");
        }

        // Mesaj primit de la client
        if (bytes > 0)
        {
            printf("Interpret Request\n");
            interpretRequest(bytes, dataBuffer, &client);
            printf("Handle Request and build response\n");
            handleRequest(&client);
            printf("Send Response\n");
            sendResponse(&client);
        }        
        
        continue;

        dataBuffer[bytes] = '\0';
        printf("[Thread %d] Comanda primita: %s\n", client.idThread, dataBuffer);

        char raspuns[200];
        send(client.socketID, raspuns, strlen(raspuns), 0);
    }
}


/*---------------------------------------------------------------------*/

/* Functie de manuire a comenzii QUIT*/

void handle_quit_command(ClientCommunication *tdL) 
{
    printf("[Thread %d] Clientul a cerut deconectarea.\n", tdL->idThread);
    connectedClients[tdL->idThread]->loggedIn = 0;
    close(tdL->socketID);  // Închidem conexiunea cu clientul
    pthread_exit(NULL);  // Ieșim din thread
}

/*---------------------------------------------------------------------*/

/* MAIN */

int main()
{

    /*---------------------------------------------------------------------*/

    /* CHESTII DE BAZA - SOCKET */

    struct sockaddr_in server, from;
    int sd;
    pthread_t th[100];
    int i = 0;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server] Eroare la socket().\n");
        print_error(errno);
        return errno;
    }

    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    // Bind IP/Port 
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server] Error at bind.\n");
        print_error(errno);
        return errno;
    }

    // Listen for connections
    if (listen(sd, 10) == -1)
    {
        perror("[server] Error at listen.\n");
        print_error(errno);
        return errno;
    }

    /*---------------------------------------------------------------------*/

    /* SERVERUL PRIMESTE CLIENTI */

    while (1) // Bucla principala
    {
        int client;
        ClientCommunication *td;
        int length = sizeof(from);

        printf("[server] Wait clients at %d...\n", PORT);
        fflush(stdout);

        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[server] Erorr at accept().\n");
            print_error(errno);
            continue;
        }

        td = (struct ClientData *)malloc(sizeof(struct ClientData));
        td->idThread = i++;
        td->socketID = client;
        td->loggedIn = 0;
        td->com_received = (COMMAND*)malloc(sizeof(COMMAND));
        td->com_response = (COMMAND*)malloc(sizeof(COMMAND));

        pthread_create(&th[i], NULL, &treat, td);
    }
}

/*---------------------------------------------------------------------*/

/* Functia de interpretare a cererii din partea clientului de catre server*/

void interpretRequest(int bytesCount, char *buffer, ClientCommunication *tdL) 
{
    
    //Function which receive bytesCount, buffer to data and command_received data
    // 1st byte is command_id
    COMMAND* command = tdL->com_received;
    command->command_id = buffer[0]; 

    // Total bytes -1 shall be command size
    printf("1\n");
    command->size = bytesCount - 1;
    if (command->size > 0) 
    {
        command->data = malloc(command->size);
        printf("2\n");
        memcpy(command->data, buffer + 1, command->size);
        printf("3\n");
    } 
    else 
    {
        command->data = NULL;
    }


    printf("4\n");
    if (command->command_id == CMD_LOGIN) 
    {
        printf("[Server] Login.\n");

        // Login structure: 1st byte username size, 2nd byte password size, N bytes username, M bytes password.
        if (command->size > 0) 
        {
            char username[256];
            char password[256];
            snprintf(username, command->size + 1, "%s", command->data);
            printf("[Server]Login message\n");
            if(tdL->userData == NULL){
                tdL->userData = (struct UserData*)malloc(sizeof(struct UserData));
            }
            memcpy(tdL->userData->username, command->data + 2, command->data[0]);
            memcpy(tdL->userData->password, command->data + 2 + command->data[0], command->data[1]);
        } 
        else 
        {
            printf("[Server] Comandă Login primită fără date. Eroare!\n");
        }
    }

    if (command->command_id == CMD_REGISTER) 
    {
        printf("[Server] Register.\n");

        // Login structure: 1st byte username size, 2nd byte password size, N bytes username, M bytes password.
        if (command->size > 0) 
        {
            char username[256];
            char password[256];
            snprintf(username, command->size + 1, "%s", command->data);
            printf("[Server]Register message\n");
            if(tdL->userData == NULL)
            {
                tdL->userData = (struct UserData*)malloc(sizeof(struct UserData));
            }
            memcpy(tdL->userData->username, command->data + 2, command->data[0]);
            memcpy(tdL->userData->password, command->data + 2 + command->data[0], command->data[1]);
            // if (registerUserDB(tdL->userData->username, tdL->userData->password)){

            // } else {

            // }
        } 
        else 
        {
            printf("[Server] Comandă Register primită fără date. Eroare!\n");
        }
    }  

    else if (command->command_id == CMD_QUIT) 
    {
        printf("[Server] Procesăm comanda Quit.\n");
        //handle_quit_command(tdL); // Gestionăm ieșirea clientului
    } 

    else if (command->command_id == CMD_MSG_SEND) 
    {
        printf("[Server] Interpretăm comanda Send Message.\n");

        if (command->size > 0) 
        {
            uint8_t recipientSize = command->data[0];
            char recipient[100];
            char message[256];

            // Verificăm dacă datele sunt valide
            if (command->size < recipientSize + 1) 
            {
                printf("[Server] Eroare: Dimensiunea datelor este insuficientă pentru CMD_MSG_SEND.\n");
                return;
            }

            // Extragem destinatarul și mesajul
            memcpy(recipient, command->data + 1, recipientSize);
            recipient[recipientSize] = '\0'; // Adăugăm terminatorul de șir

            uint16_t messageSize = command->size - 1 - recipientSize;
            memcpy(message, command->data + 1 + recipientSize, messageSize);
            message[messageSize] = '\0'; // Adăugăm terminatorul de șir

            printf("[Server] Destinatar: %s, Mesaj: %s\n", recipient, message);

            tdL->messageData = malloc(sizeof(struct MessageData));
            if (tdL->messageData == NULL) {
                perror("[Server] Eroare la alocarea memoriei pentru MessageData");
                return;
            }


            strncpy(tdL->messageData->recipient, recipient, sizeof(tdL->messageData->recipient) - 1);
            tdL->messageData->recipient[sizeof(tdL->messageData->recipient) - 1] = '\0';

            strncpy(tdL->messageData->message, message, sizeof(tdL->messageData->message) - 1);
            tdL->messageData->message[sizeof(tdL->messageData->message) - 1] = '\0';

            printf("[Debug] În interpretRequest, recipient=%s, message=%s\n", 
                tdL->messageData->recipient, tdL->messageData->message);

        } 
        else 
        {
            printf("[Server] CMD_MSG_SEND fără date valide.\n");
        }
    }


    else if (command->command_id == CMD_SEE_HISTORY) 
    {
        printf("[Server] Interpretăm comanda See History.\n");

        if (command->size > 0) 
        {
            uint8_t userLen = command->data[0];
            char recipient[100];

            // Verificăm dacă datele sunt valide
            if (command->size < userLen + 1) 
            {
                printf("[Server] Eroare: Dimensiunea datelor este insuficientă pentru CMD_SEE_HISTORY.\n");
                return;
            }

            // Extragem destinatarul
            memcpy(recipient, command->data + 1, userLen);
            recipient[userLen] = '\0'; // Adăugăm terminatorul de șir

            printf("[Server] Istoricul conversatiei cu: %s\n", recipient);

            if (tdL->messageData == NULL) 
            {
                tdL->messageData = malloc(sizeof(MessageData));
            }

            strncpy(tdL->messageData->recipient, recipient, sizeof(tdL->messageData->recipient) - 1);
            tdL->messageData->recipient[sizeof(tdL->messageData->recipient) - 1] = '\0';

            printf("[Debug] În interpretRequest, recipient=%s\n", 
                tdL->messageData->recipient);

        } 
        else 
        {
            printf("[Server] CMD_SEE_HISTORY fără date valide.\n");
        }
    }

    else if (command->command_id == CMD_SEE_CONV) 
    {
        printf("[Server] Interpretăm comanda See Conversations.\n");

        if (command->size > 0) 
        {
            char conversations[256][100]; // Matrice pentru până la 256 conversații de maximum 100 de caractere fiecare
            uint8_t numConversations = command->data[0]; // Primul byte indică numărul de conversații

            // Verificăm dacă datele sunt valide
            if (command->size < 1 + numConversations * 100) 
            {
                printf("[Server] Eroare: Dimensiunea datelor este insuficientă pentru CMD_SEE_CONV.\n");
                return;
            }

            // Extragem conversațiile
            for (uint8_t i = 0; i < numConversations; i++) 
            {
                memcpy(conversations[i], command->data + 1 + i * 100, 100);
                conversations[i][99] = '\0'; // Asigurăm terminatorul de șir
            }

            printf("[Server] Lista conversațiilor:\n");
            for (uint8_t i = 0; i < numConversations; i++) 
            {
                printf("- %s\n", conversations[i]);
            }

            // Stocăm datele în `tdL` (dacă este cazul)
            if (tdL->conversationData == NULL) 
            {
                tdL->conversationData = malloc(sizeof(ConversationData) * numConversations);
            }

            for (uint8_t i = 0; i < numConversations; i++) 
            {
                strncpy(tdL->conversationData[i].username, conversations[i], sizeof(tdL->conversationData[i].username) - 1);
                tdL->conversationData[i].username[sizeof(tdL->conversationData[i].username) - 1] = '\0';
            }

            tdL->numConversations = numConversations;

            printf("[Debug] În interpretRequest, numConversations=%d\n", tdL->numConversations);
        } 
        else 
        {
            printf("[Server] CMD_SEE_CONV fără date valide.\n");
        }
    }

    else if (command->command_id == CMD_REPLY_MSG) 
    {
        printf("[Server] Interpretăm comanda Reply Message.\n");

        uint32_t networkMessageNumber;
        memcpy(&networkMessageNumber, command->data, sizeof(uint32_t));
        int messageId = ntohl(networkMessageNumber);

        // Preluăm mesajul de răspuns (restul datelor)
        char* replyMessage = (char*)(command->data + sizeof(uint32_t));
        printf("[Debug Server] Mesaj ID primit: %d, Reply: %s\n", messageId, replyMessage);

        // Verificăm dacă datele sunt valide
        if (command->size > sizeof(uint32_t)) 
        {
            printf("[Server] Răspundem mesajului %d cu textul: %s\n", messageId, replyMessage);

            // Actualizăm structura `tdL->messageData`
            if (tdL->messageData == NULL) 
            {
                tdL->messageData = malloc(sizeof(MessageData));
                if (tdL->messageData == NULL) 
                {
                    perror("[Server] Eroare la alocarea memoriei pentru MessageData");
                    return;
                }
            }

            tdL->messageData->messageId = messageId;
            strncpy(tdL->messageData->message, replyMessage, sizeof(tdL->messageData->message) - 1);
            tdL->messageData->message[sizeof(tdL->messageData->message) - 1] = '\0';

            printf("[Server] Datele pentru răspuns au fost stocate corect.\n");
        } 
        else 
        {
            printf("[Server] Comanda Reply Message nu are date valide.\n");
        }
    }

    else if (command->command_id == CMD_SEE_USERS) 
    {
        printf("[Server] Interpretăm comanda See Users.\n");

        if (command->size > 0) 
        {
            char users[256][100]; // Matrice pentru până la 256 utilizatori de maximum 100 de caractere fiecare
            uint8_t numUsers = command->data[0]; // Primul byte indică numărul de utilizatori

            // Verificăm dacă datele sunt valide
            if (command->size < 1 + numUsers * 100) 
            {
                printf("[Server] Eroare: Dimensiunea datelor este insuficientă pentru CMD_SEE_USERS.\n");
                return;
            }

            // Extragem utilizatorii
            for (uint8_t i = 0; i < numUsers; i++) 
            {
                memcpy(users[i], command->data + 1 + i * 100, 100);
                users[i][99] = '\0'; // Asigurăm terminatorul de șir
            }

            printf("[Server] Lista utilizatorilor:\n");
            for (uint8_t i = 0; i < numUsers; i++) 
            {
                printf("- %s\n", users[i]);
            }

            // Stocăm datele în tdL (dacă este cazul)
            if (tdL->userData == NULL) 
            {
                tdL->userData = malloc(sizeof(UserData) * numUsers);
            }

            for (uint8_t i = 0; i < numUsers; i++) 
            {
                strncpy(tdL->userData[i].username, users[i], sizeof(tdL->userData[i].username) - 1);
                tdL->userData[i].username[sizeof(tdL->userData[i].username) - 1] = '\0';
            }

            tdL->numUsers = numUsers;

            printf("[Debug] În interpretRequest, numUsers=%d\n", tdL->numUsers);
        } 
        else 
        {
            printf("[Server] CMD_SEE_USERS fără date valide.\n");
        }
    }

    else if (command->command_id == CMD_SEE_ON_USERS) 
    {
        printf("[Server] Interpretăm comanda See Online Users.\n");

        if (command->size > 0) 
        {
            char users[256][100]; // Matrice pentru până la 256 utilizatori de maximum 100 de caractere fiecare
            uint8_t numUsers = command->data[0]; // Primul byte indică numărul de utilizatori

            // Verificăm dacă datele sunt valide
            if (command->size < 1 + numUsers * 100) 
            {
                printf("[Server] Eroare: Dimensiunea datelor este insuficientă pentru CMD_SEE_ON_USERS.\n");
                return;
            }

            // Extragem utilizatorii
            for (uint8_t i = 0; i < numUsers; i++) 
            {
                memcpy(users[i], command->data + 1 + i * 100, 100);
                users[i][99] = '\0'; // Asigurăm terminatorul de șir
            }

            printf("[Server] Lista utilizatorilor online:\n");
            for (uint8_t i = 0; i < numUsers; i++) 
            {
                printf("- %s\n", users[i]);
            }

            // Stocăm datele în tdL (dacă este cazul)
            if (tdL->userData == NULL) 
            {
                tdL->userData = malloc(sizeof(UserData) * numUsers);
            }

            for (uint8_t i = 0; i < numUsers; i++) 
            {
                strncpy(tdL->userData[i].username, users[i], sizeof(tdL->userData[i].username) - 1);
                tdL->userData[i].username[sizeof(tdL->userData[i].username) - 1] = '\0';
            }

            tdL->numUsers = numUsers;

            printf("[Debug] În interpretRequest, numUsers=%d\n", tdL->numUsers);
        } 
        else 
        {
            printf("[Server] CMD_SEE_ON_USERS fără date valide.\n");
        }
    }

    else if (command->command_id == CMD_SEE_OFF_USERS) 
    {
        printf("[Server] Interpretăm comanda See Offline Users.\n");

        if (command->size > 0) 
        {
            char users[256][100]; // Matrice pentru până la 256 utilizatori de maximum 100 de caractere fiecare
            uint8_t numUsers = command->data[0]; // Primul byte indică numărul de utilizatori

            // Verificăm dacă datele sunt valide
            if (command->size < 1 + numUsers * 100) 
            {
                printf("[Server] Eroare: Dimensiunea datelor este insuficientă pentru CMD_SEE_OFF_USERS.\n");
                return;
            }

            // Extragem utilizatorii
            for (uint8_t i = 0; i < numUsers; i++) 
            {
                memcpy(users[i], command->data + 1 + i * 100, 100);
                users[i][99] = '\0'; // Asigurăm terminatorul de șir
            }

            printf("[Server] Lista utilizatorilor offline:\n");
            for (uint8_t i = 0; i < numUsers; i++) 
            {
                printf("- %s\n", users[i]);
            }

            // Stocăm datele în tdL (dacă este cazul)
            if (tdL->userData == NULL) 
            {
                tdL->userData = malloc(sizeof(UserData) * numUsers);
            }

            for (uint8_t i = 0; i < numUsers; i++) 
            {
                strncpy(tdL->userData[i].username, users[i], sizeof(tdL->userData[i].username) - 1);
                tdL->userData[i].username[sizeof(tdL->userData[i].username) - 1] = '\0';
            }

            tdL->numUsers = numUsers;

            printf("[Debug] În interpretRequest, numUsers=%d\n", tdL->numUsers);
        } 
        else 
        {
            printf("[Server] CMD_SEE_OFF_USERS fără date valide.\n");
        }
    }
    
    else if (command->command_id == CMD_QUIT)
    {
        printf("[Server] Interpretăm comanda Quit.\n");
        tdL->com_received->command_id = command->command_id;
    }

    else if (command->command_id == CMD_LOGOUT)
    {
        printf("[Server] Interpretăm comanda Logout.\n");
        tdL->com_received->command_id = command->command_id;
    }

    else if (command->command_id == CMD_MSG_RECV) 
    {
        printf("[Server] Interpretăm comanda Receive Messages.\n");

        if (command->size > 0) 
        {
            printf("[Server] Comanda CMD_MSG_RECV nu ar trebui să conțină date suplimentare la recepție.\n");
        }
    }


    else 
    {
        printf("[Server] Unknown command ID=%X.\n", command->command_id);

        // Trimitem un răspuns de eroare
        // command_raspuns.command_id = 0xFF; // Cod de eroare
        // command_raspuns.size = 0;
        // command_raspuns.data = NULL;

        //  send_command(tdL->socketID, &command_raspuns);
    }

    //Eliberăm memoria dacă datele comenzii au fost alocate
    if (command->data != NULL) 
    {
        free(command->data);
        command->data = NULL;
    }
}

/*---------------------------------------------------------------------*/

/*pt logout*/
void resetUserData(struct UserData* userData) 
{
    if (userData) 
    {
        memset(userData->username, 0, sizeof(userData->username));
        memset(userData->password, 0, sizeof(userData->password));
    }
}

/* Functia de manuire a cererii din partea clientului de catre server*/

void handleRequest(ClientCommunication *tdL)
{
    COMMAND *command = tdL->com_received;
    COMMAND *commandResponse = tdL->com_response;

    if (command->command_id == CMD_LOGIN)
    {
        // SUCCES!
        printf("[Server]User tries to login !\n");
        printf("[Server] %s! \n", tdL->userData->username);
        printf("[Server] %s! \n", tdL->userData->password);
        
        // Dummy login check
        if (loginUserDB( tdL->userData->username,  tdL->userData->password, &tdL->dataBaseID) == 1) //CEVA GRESIT
        {
            commandResponse->command_id = CMD_LOGIN + 0x40;
            commandResponse->size = 0;
            printf("\nLogin OK!\n");
            tdL->loggedIn = 1;
        } 
        else 
        {
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
            printf("\nLogin Error!\n");
            tdL->loggedIn = 0;
        }
    }

    

    else if (command->command_id == CMD_REGISTER)
    {
        // SUCCES!
        printf("[Server]User tries to register !\n");
        printf("[Server] %s! \n", tdL->userData->username);
        printf("[Server] %s! \n", tdL->userData->password);
        // Dummy register check
        if (registerUserDB(tdL->userData->username, tdL->userData->password) == 1){
            commandResponse->command_id = CMD_REGISTER + 0x40;
            commandResponse->size = 0;
            printf("\nRegister OK!\n");
            tdL->loggedIn = 0;
        }
    }

    else if (command->command_id == CMD_LOGOUT)
    {
        printf("[Server] Logout!\n");

        if (tdL->loggedIn == 0) {
            printf("[Server] Utilizatorul nu este logat.\n");
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
            return;
        }

        // Apelăm funcția logoutUserDB din database.h
        if (logoutUserDB(tdL->userData->username)) {
            printf("[Server] Utilizatorul %s a fost delogat cu succes.\n", tdL->userData->username);

            // Resetează datele utilizatorului
            resetUserData(tdL->userData);

            // Marchează utilizatorul ca delogat
            tdL->loggedIn = 0;

            // Trimitem un răspuns de succes
            commandResponse->command_id = CMD_EXECUTED_OK;
            commandResponse->size = 0;
        } else {
            printf("[Server] Eroare la delogare pentru utilizatorul %s.\n", tdL->userData->username);
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
        }
    }
    else if (command->command_id == CMD_QUIT)
    {
        printf("[Server] Quit!\n");

        if (tdL->loggedIn == 0) {
            printf("[Server] Utilizatorul nu este logat.\n");
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
            return;
        }

        // Apelăm funcția quitUserDB din database.h
        if (quitUserDB(tdL->userData->username)) {
            printf("[Server] Utilizatorul %s a fost delogat prin quit.\n", tdL->userData->username);

            // Resetează datele utilizatorului
            resetUserData(tdL->userData);

            // Marchează utilizatorul ca delogat
            tdL->loggedIn = 0;

            // Trimitem un răspuns de succes
            commandResponse->command_id = CMD_EXECUTED_OK;
            commandResponse->size = 0;
        } else {
            printf("[Server] Eroare la quit pentru utilizatorul %s.\n", tdL->userData->username);
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
        }
    }



    else if (command->command_id == CMD_SEE_USERS) 
    {
        printf("[Server] See users!\n");

        // Eliberează memoria pentru datele anterioare
        if (commandResponse->data) {
            free(commandResponse->data);
            commandResponse->data = NULL;
        }
        commandResponse->size = 0;

        if (tdL->loggedIn == 0) {
            // Utilizatorul nu este logat
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
        } else {
            commandResponse->command_id = CMD_SEE_USERS + 0x40;
            char** users = NULL;
            int userCount = 0;

            // Obține lista utilizatorilor
            if (getUsers(&users, &userCount)) 
            {
                printf("[Server] Număr de utilizatori găsiți: %d\n", userCount);

                if (userCount > 0) {
                    // Concatenează utilizatorii într-un buffer unic
                    size_t totalSize = 0;
                    for (int i = 0; i < userCount; i++) 
                    {
                        totalSize += strlen(users[i]) + 1; // +1 pentru '\0'
                    }

                    commandResponse->data = malloc(totalSize);
                    if (commandResponse->data == NULL) {
                        printf("[Error] Eroare la alocarea memoriei pentru lista utilizatorilor.\n");
                        for (int i = 0; i < userCount; i++) 
                        {
                            free(users[i]);
                        }
                        free(users);
                        return;
                    }

                    char* currentPos = (char*)commandResponse->data;
                    for (int i = 0; i < userCount; i++) {
                        strcpy(currentPos, users[i]);
                        currentPos += strlen(users[i]) + 1;
                        free(users[i]); // Eliberăm memoria pentru fiecare utilizator
                    }
                    free(users);

                    commandResponse->size = totalSize;
                    printf("[Server] Lista de utilizatori trimisă cu succes.\n");
                } 
                else 
                {
                    printf("[Server] Nu s-au găsit utilizatori.\n");
                    commandResponse->command_id = CMD_ERROR;
                    commandResponse->size = 0;
                }
            } 
            else 
            {
                printf("[Server] Nu s-a putut obține lista de utilizatori.\n");
                commandResponse->command_id = CMD_ERROR;
                commandResponse->size = 0;
            }
        }
    }


    else if (command->command_id == CMD_SEE_ON_USERS) 
    {
        printf("[Server] See online users!\n");

        // Eliberează memoria pentru datele anterioare
        if (commandResponse->data) {
            free(commandResponse->data);
            commandResponse->data = NULL;
        }
        commandResponse->size = 0;

        if (tdL->loggedIn == 0) {
            // Utilizatorul nu este logat
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
        } else {
            commandResponse->command_id = CMD_SEE_ON_USERS + 0x40;
            char** users = NULL;
            int userCount = 0;

            // Obține lista utilizatorilor logați (online)
            if (getUsersOnline(&users, &userCount)) 
            {
                printf("[Server] Număr de utilizatori online găsiți: %d\n", userCount);

                if (userCount > 0) {
                    // Concatenează utilizatorii într-un buffer unic
                    size_t totalSize = 0;
                    for (int i = 0; i < userCount; i++) 
                    {
                        totalSize += strlen(users[i]) + 1; // +1 pentru '\0'
                    }

                    commandResponse->data = malloc(totalSize);
                    if (commandResponse->data == NULL) {
                        printf("[Error] Eroare la alocarea memoriei pentru lista utilizatorilor online.\n");
                        for (int i = 0; i < userCount; i++) 
                        {
                            free(users[i]);
                        }
                        free(users);
                        return;
                    }

                    char* currentPos = (char*)commandResponse->data;
                    for (int i = 0; i < userCount; i++) {
                        strcpy(currentPos, users[i]);
                        currentPos += strlen(users[i]) + 1;
                        free(users[i]); // Eliberăm memoria pentru fiecare utilizator
                    }
                    free(users);

                    commandResponse->size = totalSize;
                    printf("[Server] Lista de utilizatori online trimisă cu succes.\n");
                } 
                else 
                {
                    printf("[Server] Nu s-au găsit utilizatori online.\n");
                    commandResponse->command_id = CMD_ERROR;
                    commandResponse->size = 0;
                }
            } 
            else 
            {
                printf("[Server] Nu s-a putut obține lista de utilizatori online.\n");
                commandResponse->command_id = CMD_ERROR;
                commandResponse->size = 0;
            }
        }
    }


   else if (command->command_id == CMD_SEE_OFF_USERS) 
    {
        printf("[Server] See offline users!\n");

        // Eliberează memoria pentru datele anterioare
        if (commandResponse->data) {
            free(commandResponse->data);
            commandResponse->data = NULL;
        }
        commandResponse->size = 0;

        if (tdL->loggedIn == 0) {
            // Utilizatorul nu este logat
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
        } else {
            commandResponse->command_id = CMD_SEE_OFF_USERS + 0x40;
            char** users = NULL;
            int userCount = 0;

            // Obține lista utilizatorilor offline (neautentificați)
            if (getUsersOffline(&users, &userCount)) 
            {
                printf("[Server] Număr de utilizatori offline găsiți: %d\n", userCount);

                if (userCount > 0) {
                    // Concatenează utilizatorii într-un buffer unic
                    size_t totalSize = 0;
                    for (int i = 0; i < userCount; i++) 
                    {
                        totalSize += strlen(users[i]) + 1; // +1 pentru '\0'
                    }

                    commandResponse->data = malloc(totalSize);
                    if (commandResponse->data == NULL) {
                        printf("[Error] Eroare la alocarea memoriei pentru lista utilizatorilor offline.\n");
                        for (int i = 0; i < userCount; i++) 
                        {
                            free(users[i]);
                        }
                        free(users);
                        return;
                    }

                    char* currentPos = (char*)commandResponse->data;
                    for (int i = 0; i < userCount; i++) {
                        strcpy(currentPos, users[i]);
                        currentPos += strlen(users[i]) + 1;
                        free(users[i]); // Eliberăm memoria pentru fiecare utilizator
                    }
                    free(users);

                    commandResponse->size = totalSize;
                    printf("[Server] Lista de utilizatori offline trimisă cu succes.\n");
                } 
                else 
                {
                    printf("[Server] Nu s-au găsit utilizatori offline.\n");
                    commandResponse->command_id = CMD_ERROR;
                    commandResponse->size = 0;
                }
            } 
            else 
            {
                printf("[Server] Nu s-a putut obține lista de utilizatori offline.\n");
                commandResponse->command_id = CMD_ERROR;
                commandResponse->size = 0;
            }
        }
    }


    else if(command->command_id == CMD_MSG_SEND)
    {
        printf("[Server]Send message!\n");
        if (tdL->loggedIn == 0) 
        {
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
        }
        else
        {
            printf("[Server]Send Message Good!\n");
            commandResponse->command_id = CMD_MSG_SEND + 0x40;
            
            printf("[Debug] În handleRequest, recipient=%s, message=%s\n", tdL->messageData->recipient, tdL->messageData->message);

            if (!sendMessageDB(tdL->userData->username, tdL->messageData->recipient, tdL->messageData->message)) 
            {
                printf("[Server] Eroare la trimiterea mesajului.\n");
                commandResponse->command_id = CMD_ERROR;
            } else {
                printf("[Server] Mesaj trimis cu succes.\n");
                commandResponse->command_id = CMD_MSG_SEND + 0x40;
            }
        }
    }

    else if (command->command_id == CMD_REPLY_MSG) 
    {
        printf("[Server] Gestionăm comanda Reply Message.\n");

        if (tdL->loggedIn == 0) 
        {
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
            return;
        }

        // Apelăm funcția replyMessage cu parametrii necesari
        if (replyMessage(
                tdL->userData->username,               // Numele utilizatorului care trimite răspunsul
                tdL->messageData->recipient,          // Destinatarul mesajului
                tdL->messageData->messageId,          // ID-ul mesajului la care se răspunde
                tdL->messageData->message))           // Textul mesajului de răspuns
        {
            commandResponse->command_id = CMD_REPLY_MSG + 0x40;
            commandResponse->size = 0;
            printf("[Server] Reply Message trimis cu succes.\n");
        } else {
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
            printf("[Server] Eroare la trimiterea Reply Message.\n");
        }
    }


     else if (command->command_id == CMD_MSG_RECV)
    {
        printf("[Server] Gestionăm comanda Receive Messages.\n");

        if (tdL->loggedIn == 0) 
        {
            printf("[Server] Utilizatorul nu este autentificat.\n");
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
            return;
        }

        char** messages = NULL;
        int messageCount = 0;

        // Funcție care obține mesajele necitite din baza de date
        if (!getUnreadMessages(tdL->userData->username, &messages, &messageCount)) 
        {
            printf("[Error] Nu s-a putut obține lista de mesaje necitite.\n");
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
            return;
        }

        if (messages == NULL || messageCount <= 0) 
        {
            printf("[Info] Nu există mesaje necitite pentru acest utilizator.\n");
            commandResponse->command_id = CMD_MSG_RECV + 0x40;
            commandResponse->data = NULL;
            commandResponse->size = 0;
            return;
        }

        // Calculăm dimensiunea totală necesară pentru bufferul concatenat
        size_t totalSize = 0;
        for (int i = 0; i < messageCount; i++) 
        {
            if (messages[i] != NULL) 
            {
                totalSize += strlen(messages[i]) + 1; // +1 pentru '\0'
            }
        }

        commandResponse->data = malloc(totalSize);
        if (commandResponse->data == NULL) 
        {
            printf("[Error] Eroare la alocarea memoriei pentru bufferul concatenat.\n");
            for (int i = 0; i < messageCount; i++) 
            {
                free(messages[i]);
            }
            free(messages);
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
            return;
        }

        // Copiem mesajele în bufferul concatenat
        size_t offset = 0;
        for (int i = 0; i < messageCount; i++) 
        {
            if (messages[i] != NULL) 
            {
                strcpy((char*)commandResponse->data + offset, messages[i]);
                offset += strlen(messages[i]) + 1; // Trecem după '\0'
                free(messages[i]); // Eliberăm memoria pentru mesajul individual
            }
        }
        free(messages);

        commandResponse->command_id = CMD_MSG_RECV + 0x40;
        commandResponse->size = totalSize;

        printf("[Server] Mesajele necitite trimise cu succes.\n");
    }


   else if (command->command_id == CMD_SEE_CONV) 
   {
        printf("[Server] See conversations!\n");

        // Eliberează memoria pentru datele anterioare
        if (commandResponse->data) {
            free(commandResponse->data);
            commandResponse->data = NULL;
        }
        commandResponse->size = 0;

        if (tdL->loggedIn == 0) {
            // Utilizatorul nu este logat
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
        } else {
            commandResponse->command_id = CMD_SEE_CONV + 0x40;
            char** conversations = NULL;
            int conversationCount = 0;

            // Obține lista conversațiilor
            if (getConversations(tdL->userData->username, &conversations, &conversationCount)) 
            {
                printf("[Server] Număr de conversații găsite: %d\n", conversationCount);

                if (conversationCount > 0) {
                    // Concatenează conversațiile într-un buffer unic
                    size_t totalSize = 0;
                    for (int i = 0; i < conversationCount; i++) 
                    {
                        totalSize += strlen(conversations[i]) + 1; // +1 pentru '\0'
                    }

                    commandResponse->data = malloc(totalSize);
                    if (commandResponse->data == NULL) {
                        printf("[Error] Eroare la alocarea memoriei pentru lista conversațiilor.\n");
                        for (int i = 0; i < conversationCount; i++) 
                        {
                            free(conversations[i]);
                        }
                        free(conversations);
                        return;
                    }

                    char* currentPos = (char*)commandResponse->data;
                    for (int i = 0; i < conversationCount; i++) {
                        strcpy(currentPos, conversations[i]);
                        currentPos += strlen(conversations[i]) + 1;
                        free(conversations[i]); // Eliberăm memoria pentru fiecare conversație
                    }
                    free(conversations);

                    commandResponse->size = totalSize;
                    printf("[Server] Lista de conversații trimisă cu succes.\n");
                } 
                else 
                {
                    printf("[Server] Nu s-au găsit conversații pentru utilizatorul curent.\n");
                    commandResponse->command_id = CMD_ERROR;
                    commandResponse->size = 0;
                }
            } 
            else 
            {
                printf("[Server] Nu s-a putut obține lista de conversații.\n");
                commandResponse->command_id = CMD_ERROR;
                commandResponse->size = 0;
            }
        }
    }

    else if (command->command_id == CMD_SEE_HISTORY) 
    {
        printf("[Server] See history!\n");

        if (tdL->loggedIn == 0) {
            printf("[Error] Utilizatorul nu este autentificat.\n");
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
            return;
        }

        char** messages = NULL;
        int messageCount = 0;

        // Obținem istoricul conversației
        if (!getConversationHistory(tdL->userData->username, tdL->messageData->recipient, &messages, &messageCount)) {
            printf("[Error] Nu s-a putut obține istoricul conversației.\n");
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
            return;
        }

        // Verificăm dacă există mesaje valide
        if (messages == NULL || messageCount <= 0) {
            printf("[Info] Nu există mesaje pentru această conversație.\n");
            commandResponse->command_id = CMD_SEE_HISTORY + 0x40;
            commandResponse->data = NULL;
            commandResponse->size = 0;
            return;
        }

        // Calculăm dimensiunea totală necesară pentru bufferul concatenat
        uint32_t totalSize = 0;
        for (int i = 0; i < messageCount; i++) {
            if (messages[i] != NULL) {
                totalSize += strlen(messages[i]) + 1; // +1 pentru '\0'
            }
        }

        printf("TotalSize: %d\n", totalSize);

        // Alocăm bufferul pentru toate mesajele
        commandResponse->data = malloc(totalSize);
        if (commandResponse->data == NULL) {
            printf("[Error] Eroare la alocarea memoriei pentru bufferul concatenat.\n");
            for (int i = 0; i < messageCount; i++) 
            {
                free(messages[i]);
            }
            free(messages);
            commandResponse->command_id = CMD_ERROR;
            commandResponse->size = 0;
            return;
        }

        // Copiem mesajele în bufferul concatenat
        uint32_t offset = 0;
        for (int i = 0; i < messageCount; i++) {
            if (messages[i] != NULL) {
                strcpy((char*)commandResponse->data + offset, messages[i]);
                offset += strlen(messages[i]) + 1; // Trecem după '\0'
                free(messages[i]); // Eliberăm memoria pentru mesajul individual
            }
        }
        free(messages); // Eliberăm lista de mesaje

        // Setăm dimensiunea totală a datelor și trimitem răspunsul
        commandResponse->command_id = CMD_SEE_HISTORY + 0x40;
        commandResponse->size = totalSize;

        printf("[Server] Istoric conversație concatenat trimis cu succes.\n");
    }

    
    else 
    {
        printf("asdfasdfasd\n");
        commandResponse->command_id = CMD_ERROR;
        commandResponse->size = 0;
    }
    printf("[Server]handleResponse response cmd_id {%2x} !\n",  commandResponse->command_id );
}

/*---------------------------------------------------------------------*/

/* Functia de trimitere a unui raspuns catre client in urma intelegerii comenzii primite*/

int sendResponse(ClientCommunication *tdL)
{
    int sd = tdL->socketID;
    int flags = 0;
    // Two steps     
    // Send command id
    COMMAND *commandResponse = tdL->com_response;
    printf("Send COMMAND_ID %X\n", commandResponse->command_id);

    if(commandResponse->size > 0)
    {
        flags = MSG_MORE;
    }

    if (send(sd, &commandResponse->command_id, 1, flags) <= 0)
    {
            perror("[Server]Eroare la send() command_id.\n");
            return errno;
    }
    // Send command data    
    if (commandResponse->size > 0)
    {
        print_hex(commandResponse->data, commandResponse->size);
        if (send(sd, commandResponse->data, commandResponse->size, 0) <= 0){
            perror("[Server]Eroare la send() command_data.\n");
            return errno;
        }
    } 
    else{
        printf("[Server]NO COMMAND DATA\n");
    }
    printf("[Server]Send message back\n");
    return 1;
}