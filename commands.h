#define CMD_MSG 0x10  // Messager Commands
#define CMD_MSG_SEND 0x11  // Message Sent
//            uint64 CONTACT_ID, uint16 MSG_Len, .........
#define CMD_REPLY_MSG 0x12  // Message Sent - Reply
//            uint64 CONTACT_ID, uint64 MSG_ID, uint16 MSG_Len,  ..........
#define CMD_MSG_RECV 0x13  // Message Received
//            uint64 CONTACT_ID, uint64 MSG_ID, uint16 MSG_Len, ...........  
#define CMD_ACCESS 0x20  // User Access
#define CMD_LOGIN 0x21  // Login
// Login structure: 1st byte username size, 2nd byte password size, N bytes username, M bytes password.
#define CMD_LOGOUT 0x22  // Logout
#define CMD_REGISTER 0x23 // Register

#define CMD_SEE_USERS 0x31 
#define CMD_SEE_ON_USERS 0x32
#define CMD_SEE_OFF_USERS 0x33
#define CMD_SEE_USERNAME 0x34

#define CMD_SEE_CONV 0x44
#define CMD_SEE_HISTORY 0x45
#define CMD_EXECUTED_OK 0xAA
#define CMD_ERROR 0xEE
// Error strcture: 1st byte message size N, message size N bytes
#define CMD_QUIT 0x02

#define DEBUG

struct COMMAND 
{
  uint8_t command_id;
  uint8_t size;
  uint8_t *data;
  uint8_t success;
};

typedef struct COMMAND COMMAND;

void printCommand(COMMAND *command)
{
    #ifdef DEBUG
        printf("BEGIN PRINT MSG\n");
        printf("DATA LEN %d \n", command->size);
        switch(command->command_id)
        {
            case CMD_LOGIN:
                printf("CMD_LOGIN: ");
                uint8_t uSize, pSize;
                char userName[256];
                char passWord[256];
                uSize = command->data[0];
                pSize = command->data[1];

                memcpy(userName, command->data+2, uSize);
                userName[uSize] = '\0';
                memcpy(passWord, command->data+2+uSize, pSize);
                passWord[pSize] = '\0';

                printf("UserName %s\n", userName);
                printf("PassWord %s\n", passWord);
                break;

            case CMD_LOGIN + 0x40:
                printf("Login Success!\n");
                break;

            case CMD_REGISTER:
                printf("CMD_REGISTER: ");
                uint8_t uSize2, pSize2;
                char userName2[256];
                char passWord2[256];
                uSize2 = command->data[0];
                pSize2 = command->data[1];

                memcpy(userName2, command->data+2, uSize2);
                userName2[uSize2] = '\0';
                memcpy(passWord2, command->data+2+uSize2, pSize2);
                passWord2[pSize2] = '\0';

                printf("UserName %s\n", userName2);
                printf("PassWord %s\n", passWord2);
                break;
                
            case CMD_REGISTER + 0x40:
                printf("Register Success!\n");
                break;

            case CMD_SEE_USERS:
                printf("View users\n");
                break;

            case CMD_SEE_USERS + 0x40:
                printf("View users response\n");
                break;

            case CMD_SEE_ON_USERS:
                printf("View online users\n");
                break;

            case CMD_SEE_ON_USERS + 0x40:
                printf("View online users response\n");
                break;

            case CMD_SEE_OFF_USERS:
                printf("View offline users\n");
                break;

            case CMD_SEE_OFF_USERS + 0x40:
                printf("View offline users response\n");
                break;

            case CMD_MSG_SEND:
                printf("Send message\n");
                break;

            case CMD_MSG_SEND + 0x40:
                printf("Send message response\n");
                break;

            case CMD_REPLY_MSG:
                printf("Reply message\n");
                break;

            case CMD_REPLY_MSG + 0x40:
                printf("Reply message response\n");
                break;

            case CMD_MSG_RECV:
                printf("Message received\n");
                break;

            case CMD_MSG_RECV + 0x40:
                printf("Message received response\n");
                break;
            
            case CMD_SEE_CONV:
                printf("See conversation\n");
                break;

            case CMD_SEE_CONV + 0x40:
                printf("See conversation response\n");
                break;
            
            case CMD_SEE_HISTORY:
                printf("See history\n");
                break;

            case CMD_SEE_HISTORY + 0x40:
                printf("See history response\n");
                break;
            
            case CMD_LOGOUT:
                printf("Logout\n");
                break;

            case CMD_LOGOUT + 0x40:
                printf("Logout response\n");
                break;

            default:
                printf("UNKNOWN COMMAND %02x\n", command->command_id);
        }
        for(int i = 0; i< command->size; i++){
            printf("%02x", (unsigned char)command->data[i]);
        }
        printf("\nEND PRINT MSG\n");
    #endif
}



void print_hex(const unsigned char *buffer, size_t length) 
{
    if (!buffer || length == 0) 
    {
        printf("Buffer is empty or invalid.\n");
        return;
    }

    for (size_t i = 0; i < length; i++) 
    {
        // Print the hexadecimal representation
        printf("%02X ", buffer[i]);

        // Print a newline every 16 bytes for better readability
        if ((i + 1) % 16 == 0) 
        {
            printf("\n");
        }
    }

    // Final newline if the last line is incomplete
    if (length % 16 != 0) 
    {
        printf("\n");
    }
}