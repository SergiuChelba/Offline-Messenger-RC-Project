/*---------------------------------------------------------------------*/

/* DECLARARI HEADERE*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sqlite3.h>

#include "commands.h"

/*---------------------------------------------------------------------*/

/* DECLARARI GLOBALE*/

int currentMenuId = 0;

char keyboard_input[256];
char keyboard_input_2[256];

COMMAND command, command_resp;
COMMAND respUserName;


/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server */
int port;

/*---------------------------------------------------------------------*/

/* FUNCTII DE CONSTRUIRE A COMENZILOR*/

/* I. COMANDA LOGIN */

void buildLoginCommand(COMMAND *command)
{
    uint16_t dataSize = 0, userNameSize = 0, passWordSize = 0; 
    command->command_id = CMD_LOGIN;

    /* Citesc username-ul de la tastatura */
    printf("Username: \n");
    scanf("%255s", keyboard_input);
    userNameSize = strlen(keyboard_input);

    /* Citesc parola de la tastatura */
    printf("Password: \n");
    scanf("%255s", keyboard_input_2);
    passWordSize = strlen(keyboard_input_2);

    dataSize = 2 + userNameSize + passWordSize;
    command->size = dataSize;
    command->data = (uint8_t*)malloc(dataSize);
    command->data[0] = (uint8_t)userNameSize;
    command->data[1] = (uint8_t)passWordSize;
    memcpy(command->data + 2, keyboard_input, userNameSize);
    memcpy(command->data + 2 + userNameSize, keyboard_input_2, passWordSize);
}

/* II. COMANDA REGISTER */

void buildRegisterCommand(COMMAND *command)
{
    uint16_t dataSize = 0, userNameSize = 0, passWordSize = 0; 
    command->command_id = CMD_REGISTER;

    /* Citesc username-ul de la tastatura */
    printf("Username: \n");
    scanf("%255s", keyboard_input);
    userNameSize = strlen(keyboard_input);

    /* Citesc parola de la tastatura */
    printf("Password: \n");
    scanf("%255s", keyboard_input_2);
    passWordSize = strlen(keyboard_input_2);

    dataSize = 2 + userNameSize + passWordSize;
    command->size = dataSize;
    command->data = (uint8_t*)malloc(dataSize);
    command->data[0] = (uint8_t)userNameSize;
    command->data[1] = (uint8_t)passWordSize;
    memcpy(command->data + 2, keyboard_input, userNameSize);
    memcpy(command->data + 2 + userNameSize, keyboard_input_2, passWordSize);
}

/* III. COMANDA QUIT */

void build_quit_command(COMMAND *command) 
{
    command->command_id = CMD_QUIT;
    command->size = 0;
    command->data = NULL;
}

void buildSeeUserNameCommand(COMMAND *command, unsigned int UID){
    command->command_id = CMD_SEE_USERNAME;
    command->size = 1;
    command->data = (char*)malloc(1);
    command->data[0] = (char)UID;
}

/* 1. COMANDA SEE USERS */

void buildSeeUsersCommand(COMMAND* command)
{
    command->command_id = CMD_SEE_USERS;
    command->size = 0;
}

/* 2. COMANDA SEE ONLINE USERS */

void buildSeeOnlineUsersCommand(COMMAND* command)
{
    command->command_id = CMD_SEE_ON_USERS;
    command->size = 0;
}

/* 3. COMANDA SEE OFFLINE USERS */

void buildSeeOfflineUsersCommand(COMMAND* command)
{
    command->command_id = CMD_SEE_OFF_USERS;
    command->size = 0;
}

void buildSendMessage(COMMAND* command) 
{
    char recipient[100];
    char message[256];
    uint16_t recipientLen, messageLen;

    // Citire destinatar
    printf("Introduceți destinatarul: ");
    scanf("%99s", recipient);
    recipientLen = strlen(recipient);

    // Citire mesaj
    printf("Introduceți mesajul: ");
    getchar(); // Consumăm newline
    fgets(message, 256, stdin);
    messageLen = strlen(message);
    if (message[messageLen - 1] == '\n') message[--messageLen] = '\0'; // Eliminăm newline

    // Construirea comenzii
    command->command_id = CMD_MSG_SEND;
    command->size = 1 + recipientLen + messageLen;

    command->data = malloc(command->size);
    command->data[0] = (uint8_t)recipientLen; // Dimensiunea destinatarului
    memcpy(command->data + 1, recipient, recipientLen); // Destinatarul
    memcpy(command->data + 1 + recipientLen, message, messageLen); // Mesajul
}


/* 5. COMANDA REPLY MESSAGE*/

void buildReplyMessage(COMMAND* command)
{
    command->command_id = CMD_REPLY_MSG;
    command->size = 0;
}

/* 6. COMANDA RECEIVED MESSAGE */

void buildReceivedMessage(COMMAND* command)
{
    command->command_id = CMD_MSG_RECV;
    command->size = 0;
}

/* 7. COMANDA SEE CONVERSATIONS */

void buildSeeConversations(COMMAND* command)
{
    command->command_id = CMD_SEE_CONV;
    command->size = 0;
    command->data = NULL;
}

/* 8. COMANDA SEE HISTORY CONVERSATION */

void buildSeeHistory(COMMAND* command) 
{
    uint16_t userLen;
    char UserConv[100];

    // Citire utilizator (în cazul în care dorim să vedem istoricul unui anumit utilizator)
    printf("Introduceți numele de utilizator pentru a vedea istoricul: ");
    scanf("%99s", UserConv);
    userLen = strlen(UserConv);

    // Construirea comenzii
    command->command_id = CMD_SEE_HISTORY;
    command->size = 1 + userLen;  // 1 pentru dimensiunea numelui de utilizator și restul pentru numele în sine

    // Alocăm memorie pentru datele comenzii
    command->data = malloc(command->size);

    // Adăugăm lungimea numelui de utilizator și numele acestuia
    command->data[0] = (uint8_t)userLen;  // Dimensiunea numelui de utilizator
    memcpy(command->data + 1, UserConv, userLen);  // Numele utilizatorului
}


/* 9. LOGOUT */

void buildLogout(COMMAND* command)
{
    command->command_id = CMD_LOGOUT;
    command->size = 0;
}

/*---------------------------------------------------------------------*/


/* FUNCTIILE DE TRIMITERE COMANDA SI PRIMIRE COMANDA - CLIENT*/

int sendRequest(int sd, COMMAND* command_sent)
{
    // Two step     
    // Send command id
    //char raspuns[256];
    if (send(sd, &command_sent->command_id, 1, MSG_MORE) < 0)
    {
                perror("Eroare la write() spre server.\n");
                return errno;
    }
    // Send command data
    print_hex(command_sent->data, command_sent->size); /*pt debug*/
    if (send(sd, command_sent->data, command_sent->size, 0) < 0)
    {
            perror("Eroare la send() spre server.\n");
            return errno;
    }
    return 1;
}

int receiveRequest(int sd, COMMAND* command_received)
{
    char raspuns[256];
    memset(raspuns, 0, 256);
    unsigned int bytes;
    bytes = recv(sd, raspuns, sizeof(raspuns), 0);
    printf("Bytes count recv %d\n", bytes);
    // printf("Data %2x\n", raspuns[0]);
    if (bytes == -1)
    {
        perror("Eroare la read() de la server.\n");
        // print_error(errno);
    }
    if(bytes)
    {
        command_received->command_id = raspuns[0];
        if(command_received->size < bytes - 1 )
        {
            if(command_received->data == NULL)
            {
                command_received->data = malloc(bytes - 1);
            }
            else
            {
                realloc(command_received->data, bytes - 1);
            }             
        }
        memset(command_received->data, 0, bytes - 1);
        memcpy(command_received->data, raspuns + 1, bytes - 1);
        command_received->size = bytes - 1;
    }
}

/*---------------------------------------------------------------------*/

/*FUNCTIA DE INTERPRETARE A RASPUNSULUI PRIMIT DE LA SERVER*/

void interpretResponse(int sd)
{
    char errorMsg[256];
    unsigned int msgSize = 0;
    
    if (command_resp.command_id == CMD_EXECUTED_OK)
    {
        printf("COMMAND_EXECUTED_OK \n");
        command_resp.success = 1;
    }

    if (command_resp.command_id == CMD_ERROR)
    {
        if(command_resp.size > 0)
        {
            msgSize = command_resp.data[0];
            memcpy(errorMsg, command_resp.data + 1, msgSize);
            printf("ERROR MSG %s \n", errorMsg);
        }
        command_resp.success = 0;
    }

    if(command_resp.command_id == CMD_LOGIN + 0x40)
    {
        printf("Login Success!\n");
        command_resp.success = 1;
    }

    if(command_resp.command_id == CMD_REGISTER + 0x40)
    {
        printf("Register Success!\n");
        command_resp.success = 1;
    }

    if(command_resp.command_id == CMD_QUIT + 0x40)
    {
        printf("Quit Success!\n");
        command_resp.success = 1;
    }

    if (command_resp.command_id == CMD_SEE_USERS + 0x40)
    {
        printf("Users Success!: \n");
        unsigned int usersSize = command_resp.data[0];
        for(unsigned int i = 1; i< command_resp.size; i++)
        {
            printf("UID %d \n", command_resp.data[i]);
            buildSeeUserNameCommand(&command, command_resp.data[i]);
            sendRequest(sd, &command);
            receiveRequest(sd, &respUserName);
            if(respUserName.command_id == CMD_SEE_USERNAME + 0x40){
                printf("See UserName Success!\n");
                printf("See user name %s \n", respUserName.data);
                
            }
        }
        command_resp.success = 1;
    }

    if (command_resp.command_id == CMD_SEE_ON_USERS + 0x40)
    {
        printf("Online users Success!: \n");
        unsigned int onusersSize = command_resp.data[0];
        command_resp.success = 1;
    }

    if (command_resp.command_id == CMD_SEE_OFF_USERS + 0x40)
    {
        printf("Offline users Success!: \n");
        unsigned int offusersSize = command_resp.data[0];
        command_resp.success = 1;
    }
    
    if (command_resp.command_id == CMD_MSG_SEND + 0x40)
    {
        printf("Send message Success!: \n");       
        command_resp.success = 1;
    }

    if(command_resp.command_id == CMD_REPLY_MSG + 0x40)
    {
        printf("Reply message Success!\n");
        command_resp.success = 1;
    }

    if(command_resp.command_id == CMD_MSG_RECV + 0x40)
    {
        printf("Message received Success!\n");
        command_resp.success = 1;
    }

    /*if(command_resp.command_id == CMD_SEE_CONV + 0x40)
    {
        printf("See conversation Success!\n");
        command_resp.success = 1;
    }*/
   if (command_resp.command_id == CMD_SEE_CONV + 0x40) 
    {
        printf("[Client] Date primite (dimensiune: %d):\n", command_resp.size);
        for (unsigned int i = 0; i < command_resp.size; i++) {
            printf("%02X ", command_resp.data[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n");

        printf("Conversațiile disponibile:\n");

        // Parcurgem conversațiile din bufferul primit
        unsigned int offset = 0;
        while (offset < command_resp.size) {
            char* currentConv = (char*)(command_resp.data + offset);
            printf("[Debug] Offset: %u, Conversație: %s\n", offset, currentConv);
            printf("%s\n", currentConv);
            offset += strlen(currentConv) + 1; // Trecem la următoarea conversație
        }

        command_resp.success = 1;
    }


    if (command_resp.command_id == CMD_SEE_HISTORY + 0x40) 
    {

        //printf("[Client] Date primite (dimensiune: %d):\n", command_resp.size);
        //for (unsigned int i = 0; i < command_resp.size; i++) {
        //    printf("%02X ", command_resp.data[i]);
        //    if ((i + 1) % 16 == 0) printf("\n");
        //}
        //printf("\n");


        printf("Istoricul conversației:\n");

        // Parcurgem toate mesajele din bufferul primit
        unsigned int offset = 0;
        while (offset < command_resp.size) {
            char* currentMessage = (char*)(command_resp.data + offset);
            //printf("[Debug] Offset: %u, Mesaj: %s\n", offset, currentMessage);
            printf("%s\n", currentMessage);
            offset += strlen(currentMessage) + 1; // Trecem la următorul mesaj
        }

        command_resp.success = 1;
    }

    if(command_resp.command_id == CMD_LOGOUT + 0x40)
    {
        printf("Logout Success!\n");
        command_resp.success = 1;
    }

}

/*---------------------------------------------------------------------*/

/*AFISAREA MENIULUI IN FUNCTIE DE OPTIUNILE ALESE ANTERIOR*/


void show_menu(int menuId)
{
    if (menuId == 0)
    {
        printf("Alegeți o opțiune: Register / Login / Quit\n");
    }
    else if(menuId == 1)
    {
        printf("\nAlegeți una dintre opțiunile:\n");
        printf("1. See users\n");
        printf("2. See online users\n");
        printf("3. See offline users\n");
        printf("4. Send message\n");
        printf("5. Reply\n");
        printf("6. Received message\n");
        printf("7. See conversations\n");
        printf("8. See history conversation\n");
        printf("9. Logout\n");
        printf("10. Quit\n");        
    }
    else if(menuId == 2)
    {
            printf("Alegeți o opțiune: Login / Quit\n");
    }
    fflush(stdout);
    currentMenuId = menuId;
}

/*---------------------------------------------------------------------*/

/* MAIN */


int main(int argc, char *argv[])
{

    /*---------------------------------------------------------------------*/

    /* CHESTII DE BAZA - SOCKET */

    int sd;                    // descriptorul de socket
    struct sockaddr_in server; // structura pentru conexiune
    char buf[100];             // buffer pentru input
    char optiune[10];          // opțiunea utilizatorului

    /* verificăm argumentele */
    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    /* stabilim portul */
    port = atoi(argv[2]);

     /* Conectare la server */
     
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket().\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client] Eroare la connect().\n");
        return errno;
    }

    /*---------------------------------------------------------------------*/

    /*CLIENTUL INTRA IN APLICATIE*/

    show_menu(0); //afisez meniul principal
    /* Optiuni: Register / Login / Quit */

    while (1) // Bucla principala
    {   
        read(0, optiune, sizeof(optiune)); //clientul citeste optiunea
        optiune[strcspn(optiune, "\n")] = '\0'; // eliminăm '\n'

        if (strcmp(optiune, "Login") == 0  && (currentMenuId == 0 || currentMenuId == 2))  // LOGIN
        {
            buildLoginCommand(&command);
            printCommand(&command);
            sendRequest(sd, &command);
            receiveRequest(sd, &command_resp);
            interpretResponse(sd);
            if(command_resp.success == 1)
            {
                show_menu(1);
            } 
            else //daca am introdus date incorecte la Login 
            {
                if(currentMenuId == 2)
                    show_menu(2);
                else if(currentMenuId == 0)
                    show_menu(0);
            }
        }

        else if(strcmp(optiune, "Register") == 0  && (currentMenuId == 0 || currentMenuId == 2)) // REGISTER
        {
            buildRegisterCommand(&command);
            printCommand(&command);
            sendRequest(sd, &command);
            receiveRequest(sd, &command_resp);
            interpretResponse(sd);
            if(command_resp.success == 1)
            {
                show_menu(2);
            } 
            else 
            {
                show_menu(0);
            }
        }

        else if (strcmp(optiune, "Quit") == 0  && (currentMenuId == 0 || currentMenuId == 2)) // QUIT
        {
            build_quit_command(&command);
            printCommand(&command); 
            sendRequest(sd, &command);
            printf("Clientul se va închide.\n");
            close(sd);                              
            exit(0);                                 
        }

        else if((strcmp(optiune, "1") == 0) && (currentMenuId == 1)) // See users
        {
            buildSeeUsersCommand(&command);
            printCommand(&command);
            sendRequest(sd, &command);
            receiveRequest(sd, &command_resp);
            printCommand(&command_resp);
            interpretResponse(sd);
            show_menu(1);
        }

        else if(strcmp(optiune, "2") == 0 && (currentMenuId == 1)) // See online users
        {
            buildSeeOnlineUsersCommand(&command);
            printCommand(&command);
            sendRequest(sd, &command);
            receiveRequest(sd, &command_resp);
            printCommand(&command_resp);
            interpretResponse(sd);
            show_menu(1);
        }
        else if(strcmp(optiune, "3") == 0 && (currentMenuId == 1)) // See offline users
        {
            buildSeeOfflineUsersCommand(&command);
            printCommand(&command);
            sendRequest(sd, &command);
            receiveRequest(sd, &command_resp);
            printCommand(&command_resp);
            interpretResponse(sd);
            show_menu(1);
        }

        else if(strcmp(optiune, "4") == 0 && (currentMenuId == 1)) // Send message
        {
            buildSendMessage(&command);
            printCommand(&command);
            sendRequest(sd, &command);
            receiveRequest(sd, &command_resp);
            printCommand(&command_resp);
            interpretResponse(sd);
            show_menu(1);
        }

        else if(strcmp(optiune, "5") == 0 && (currentMenuId == 1)) // Reply message
        {
            buildReplyMessage(&command);
            printCommand(&command);
            sendRequest(sd, &command);
            receiveRequest(sd, &command_resp);
            printCommand(&command_resp);
            interpretResponse(sd);
            show_menu(1);
        }

        else if(strcmp(optiune, "6") == 0 && (currentMenuId == 1)) // Received message
        {
            buildReceivedMessage(&command);
            printCommand(&command);
            sendRequest(sd, &command);
            receiveRequest(sd, &command_resp);
            printCommand(&command_resp);
            interpretResponse(sd);
            show_menu(1);
        }

        else if(strcmp(optiune, "7") == 0 && (currentMenuId == 1)) // See conversations
        {
            buildSeeConversations(&command);
            printCommand(&command);
            sendRequest(sd, &command);
            receiveRequest(sd, &command_resp);
            printCommand(&command_resp);
            interpretResponse(sd);
            show_menu(1);
        }

        else if(strcmp(optiune, "8") == 0 && (currentMenuId == 1)) // See history conversation
        {
            buildSeeHistory(&command);
            printCommand(&command);
            sendRequest(sd, &command);
            receiveRequest(sd, &command_resp);
            printCommand(&command_resp);
            interpretResponse(sd);
            show_menu(1);
        }

        else if (strcmp(optiune, "9") == 0 && (currentMenuId == 1)) // Logout
        {
            buildLogout(&command);
            printCommand(&command);
            sendRequest(sd, &command);
            receiveRequest(sd, &command_resp);
            printCommand(&command_resp);
            interpretResponse(sd);
            show_menu(0);                             
        }

        else if (strcmp(optiune, "10") == 0 && (currentMenuId == 1)) // QUIT
        {
            build_quit_command(&command);
            printCommand(&command); 
            sendRequest(sd, &command);
            printf("Clientul se va închide.\n");
            close(sd);                              
            exit(0);                                 
        }
        
        else
        {
            printf("Opțiune invalidă. Încercați din nou.\n");
        }

        /*else if (strcmp(optiune, "Register") == 0)
        {
            /* Înregistrare 
            printf("Introduceti un nume cu care sa va inregistrati: ");
            fflush(stdout);
            read(0, buf, sizeof(buf));
            buf[strcspn(buf, "\n")] = '\0'; // eliminăm '\n'

            if (send(sd, buf, strlen(buf), 0) <= 0)
            {
                perror("Eroare la write() spre server.\n");
                return errno;
            }

            char raspuns[64];
            if (recv(sd, raspuns, sizeof(raspuns), 0) < 0)
            {
                perror("Eroare la read() de la server.\n");
                return errno;
            }

            printf("Mesajul primit este: %s\n", raspuns);
            printf("V-ati inregistrat cu succes!\n");
            while (1) // Submeniul Register
            {
                printf("Ce optiune doriti: [Login] sau [Quit]?\n");
                fflush(stdout);
                read(0, optiune, sizeof(optiune));
                optiune[strcspn(optiune, "\n")] = '\0';

                if (strcmp(optiune, "Login") == 0)
                {
                    meniu_login(sd); // Apelăm meniul Login
                }
                else if (strcmp(optiune, "Quit") == 0)
                {
                    close(sd);
                    printf("Ați ieșit din aplicație. La revedere!\n");
                    return 0; // Ieșire completă
                }
                else
                {
                    printf("Opțiune invalidă. Încercați din nou.\n");
                }
            }
        }*/
    
    }

    return 0;
}

/* Meniul Login 
void meniu_login(int sd)
{
    char optiune[10];
    while (1)
    {
        printf("\nAlegeți o opțiune:\n");
        printf("1. Vezi utilizatorii online\n");
        printf("2. Vezi utilizatorii offline\n");
        printf("3. Vezi toti utilizatorii\n");
        printf("4. Logout\n");
        printf("5. Quit\n");
        fflush(stdout);

        read(0, optiune, sizeof(optiune));
        optiune[strcspn(optiune, "\n")] = '\0';

        if (strcmp(optiune, "1") == 0)
        {
            printf("Utilizatorii online:\n");
            // Trimitere cerere către server și afișarea răspunsului
        }
        else if (strcmp(optiune, "2") == 0)
        {
            printf("Utilizatorii offline:\n");
            // Trimitere cerere către server și afișarea răspunsului
        }
        else if (strcmp(optiune, "3") == 0)
        {
            printf("Toți utilizatorii:\n");
            // Trimitere cerere către server și afișarea răspunsului
        }
        else if (strcmp(optiune, "4") == 0)
        {
            printf("V-ați deconectat. Ce opțiune doriți: [Quit] sau [Login]?\n");
            return; // Revenim în meniul Register
        }
        else if (strcmp(optiune, "5") == 0)
        {
            printf("Ați ieșit din aplicație. La revedere!\n");
            close(sd);
            exit(0);
        }
        else
        {
            printf("Opțiune invalidă. Încercați din nou.\n");
        }
    }
}*/