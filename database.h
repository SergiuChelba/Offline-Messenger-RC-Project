#include <sqlite3.h>
#include <stdio.h>
#include <ctype.h>

#define DB_NAME "Messenger.sqlite"


typedef struct UsersList 
{
    unsigned int userCount;
    unsigned int* usersList; 
} UsersList;

/*
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT, -- Unique ID for each user
    username TEXT NOT NULL UNIQUE,        -- Username, must be unique
    password TEXT NOT NULL                -- Password
);

CREATE TABLE chats (
    user1 INTEGER NOT NULL, 
    user2 INTEGER NOT NULL, 
    messages_table_name TEXT NOT NULL,
    PRIMARY KEY (user1, user2),
    CHECK (user1 < user2)
);

CREATE TABLE chat_1_2 (
    id INTEGER PRIMARY KEY AUTOINCREMENT, -- Unique ID for each message
    date DATETIME DEFAULT CURRENT_TIMESTAMP,
    sender INTEGER NOT NULL,
    reply_to INTEGER
);


*/

int registerUserDB(char *userName, char* passWord)
{
    sqlite3* dataBase;
    sqlite3_stmt* queryStmt;
    int result;
    int dbResult = sqlite3_open(DB_NAME, &dataBase);

    if (dbResult != SQLITE_OK){
        printf("Error to open %s db error %s \n", DB_NAME, sqlite3_errmsg(dataBase));
        return 0;
    }

    const char* registerQuery = "INSERT INTO users (username, password) VALUES (?, ?)";

    if(sqlite3_prepare_v2(dataBase, registerQuery, -1, &queryStmt, NULL) != SQLITE_OK){
        printf("Failed to prepare statement: %s\n", sqlite3_errmsg(dataBase));
        return 0;
    }

    sqlite3_bind_text(queryStmt, 1, userName, -1, SQLITE_STATIC);
    sqlite3_bind_text(queryStmt, 2, passWord, -1, SQLITE_STATIC);
    
    result = sqlite3_step(queryStmt);
    if( result == SQLITE_DONE)
    {
        return 1;
    }
    else if (result == SQLITE_CONSTRAINT){
        printf("User already exists %d \n", result);
        return 0;
    }
    else {
        printf("Error to register user %d \n", result);
        return 0;
    }
}

int loginUserDB(char* userName, char* passWord, unsigned int* userID)
{
    sqlite3* dataBase;
    sqlite3_stmt* queryStmt;
    sqlite3_stmt* updateStmt;
    int result = 0;
    int dbResult = sqlite3_open(DB_NAME, &dataBase);

    if (dbResult != SQLITE_OK) {
        printf("Error to open %s db error %s \n", DB_NAME, sqlite3_errmsg(dataBase));
        return 0;
    }

    const char* loginQuery = "SELECT * FROM users WHERE username = ? AND password = ?";

    // Pregătim interogarea pentru verificarea utilizatorului
    if (sqlite3_prepare_v2(dataBase, loginQuery, -1, &queryStmt, NULL) != SQLITE_OK) {
        printf("Failed to prepare statement: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    sqlite3_bind_text(queryStmt, 1, userName, -1, SQLITE_STATIC);
    sqlite3_bind_text(queryStmt, 2, passWord, -1, SQLITE_STATIC);

    if (sqlite3_step(queryStmt) == SQLITE_ROW) {
        *userID = sqlite3_column_int(queryStmt, 0);  // Obținem ID-ul utilizatorului
        if (*userID > 0) {
            // Utilizatorul a fost găsit, acum îl logăm
            const char* updateQuery = "UPDATE users SET Logat = 1 WHERE username = ?";

            // Pregătim interogarea de actualizare a câmpului Logat
            if (sqlite3_prepare_v2(dataBase, updateQuery, -1, &updateStmt, NULL) != SQLITE_OK) {
                printf("Failed to prepare update statement: %s\n", sqlite3_errmsg(dataBase));
                sqlite3_finalize(queryStmt);
                sqlite3_close(dataBase);
                return 0;
            }

            sqlite3_bind_text(updateStmt, 1, userName, -1, SQLITE_STATIC);

            // Executăm interogarea de actualizare
            if (sqlite3_step(updateStmt) != SQLITE_DONE) {
                printf("Failed to update Logat status: %s\n", sqlite3_errmsg(dataBase));
                sqlite3_finalize(updateStmt);
                sqlite3_finalize(queryStmt);
                sqlite3_close(dataBase);
                return 0;
            }

            // Finalizăm interogările
            sqlite3_finalize(updateStmt);
            result = 1;  // Logarea a fost efectuată cu succes
        }
    }

    sqlite3_finalize(queryStmt);
    sqlite3_close(dataBase);

    return result;  // Dacă 1, logarea a fost reușită, altfel 0
}

// Function to log out a user (set Logat = 0 in the database)
int logoutUserDB(char* userName)
{
    sqlite3* dataBase;
    sqlite3_stmt* updateStmt;
    int result = 0;
    int dbResult = sqlite3_open(DB_NAME, &dataBase);

    if (dbResult != SQLITE_OK) {
        printf("Error to open %s db error %s \n", DB_NAME, sqlite3_errmsg(dataBase));
        return 0;
    }

    const char* updateQuery = "UPDATE users SET Logat = 0 WHERE username = ?";

    // Prepare the update query to log out the user
    if (sqlite3_prepare_v2(dataBase, updateQuery, -1, &updateStmt, NULL) != SQLITE_OK) {
        printf("Failed to prepare update statement: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    sqlite3_bind_text(updateStmt, 1, userName, -1, SQLITE_STATIC);

    // Execute the update query
    if (sqlite3_step(updateStmt) != SQLITE_DONE) {
        printf("Failed to update Logat status: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_finalize(updateStmt);
        sqlite3_close(dataBase);
        return 0;
    }

    sqlite3_finalize(updateStmt);
    sqlite3_close(dataBase);

    return 1;  // Success
}


void normalizeString(char* str) 
{
    for (int i = 0; str[i]; i++) 
    {
        str[i] = tolower((unsigned char)str[i]);
    }
}

int sendMessageDB(const char* currentUser, const char* recipient, const char* messageText) 
{
    sqlite3* dataBase;
    sqlite3_stmt* stmt;
    char tableName1[256];
    char tableName2[256];
    char query[1024];
    int result;

    char normalizedCurrentUser[256];
    char normalizedRecipient[256];
    strncpy(normalizedCurrentUser, currentUser, sizeof(normalizedCurrentUser) - 1);
    normalizedCurrentUser[sizeof(normalizedCurrentUser) - 1] = '\0';
    strncpy(normalizedRecipient, recipient, sizeof(normalizedRecipient) - 1);
    normalizedRecipient[sizeof(normalizedRecipient) - 1] = '\0';

    // Normalizează numele
    normalizeString(normalizedCurrentUser);
    normalizeString(normalizedRecipient);

    // Deschiderea bazei de date
    if (sqlite3_open(DB_NAME, &dataBase) != SQLITE_OK) {
        printf("Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(dataBase));
        return 0;
    }

    // Construirea numelor posibile ale tabelei
    snprintf(tableName1, sizeof(tableName1), "conv_%s_%s", normalizedCurrentUser, normalizedRecipient);
    snprintf(tableName2, sizeof(tableName2), "conv_%s_%s", normalizedRecipient, normalizedCurrentUser);

    // Verificăm dacă tabela există deja
    snprintf(query, sizeof(query),
        "SELECT name FROM sqlite_master WHERE type='table' AND (name='%s' OR name='%s');",
        tableName1, tableName2);

    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Eroare la verificarea existenței tabelei: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    char selectedTableName[256] = {0}; // Copie sigură a numelui tabelei
    result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) 
    {
        // Tabela există deja, utilizăm numele existent
        const char* tableNameFromDB = (const char*)sqlite3_column_text(stmt, 0);
        if (tableNameFromDB != NULL) {
            strncpy(selectedTableName, tableNameFromDB, sizeof(selectedTableName) - 1);
            selectedTableName[sizeof(selectedTableName) - 1] = '\0'; // Asigurăm terminatorul de șir
        }
        printf("Tabela existentă detectată: %s\n", selectedTableName);
    } 
    else 
    {
        // Tabela nu există, folosim numele implicit (currentUser_recipient)
        strncpy(selectedTableName, tableName1, sizeof(selectedTableName) - 1);
        selectedTableName[sizeof(selectedTableName) - 1] = '\0'; // Asigurăm terminatorul de șir
        printf("Tabela nu există, va fi creată: %s\n", selectedTableName);

        // Creăm tabela conversației
        snprintf(query, sizeof(query), 
            "CREATE TABLE [%s] ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "date DATETIME DEFAULT CURRENT_TIMESTAMP, "
            "sender TEXT NOT NULL, "
            "message TEXT, "
            "reply_to INTEGER, "
            "read1 INTEGER DEFAULT 0, "
            "read2 INTEGER DEFAULT 0);",
            selectedTableName);

        if (sqlite3_exec(dataBase, query, NULL, NULL, NULL) != SQLITE_OK) {
            printf("Eroare la crearea tabelei: %s\n", sqlite3_errmsg(dataBase));
            sqlite3_finalize(stmt);
            sqlite3_close(dataBase);
            return 0;
        }
    }
    sqlite3_finalize(stmt);

    printf("Tabela selectată: %s\n", selectedTableName);


    // Determinăm dacă destinatarul este logat
    int recipientLoggedIn = 0;
    snprintf(query, sizeof(query), "SELECT Logat FROM users WHERE username = ?;");
    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, recipient, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            recipientLoggedIn = sqlite3_column_int(stmt, 0); // 1 dacă este logat, altfel 0
        }
    }
    sqlite3_finalize(stmt);


    // Determinăm valorile pentru read1 și read2
    //int read1 = strcmp(normalizedCurrentUser, currentUser) == 0 ? 1 : recipientLoggedIn ? 1 : 0;
    //int read2 = strcmp(normalizedRecipient, recipient) == 0 ? (recipientLoggedIn ? 1 : 0) : 1;


    char user1[256], user2[256];
    int read1, read2;
    sscanf(selectedTableName, "conv_%255[^_]%255s", user1, user2);

    if(strcmp(normalizedCurrentUser, user1) == 0)
    {
        read1 = 1;
        if(recipientLoggedIn)
            read2 = 1;
        else read2 = 0;
    }
    else if(strcmp(normalizedRecipient, user1) == 0)
    {
        if(recipientLoggedIn)
            read1 = 1;
        else read1 = 0;
        read2 = 1;
    }

    // Inserăm mesajul în tabelă
    snprintf(query, sizeof(query),
        "INSERT INTO [%s] (sender, message, read1, read2) VALUES (?, ?, ?, ?);",
        selectedTableName);
    printf("Interogarea: %s\n", query);

    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Eroare la pregătirea interogării: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, currentUser, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, messageText, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, read1);
    sqlite3_bind_int(stmt, 4, read2);

    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        printf("Eroare la inserarea mesajului: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_finalize(stmt);
        sqlite3_close(dataBase);
        return 0;
    }

    printf("Mesaj inserat cu succes în tabelul %s (read1=%d, read2=%d)\n", 
           selectedTableName, read1, read2);

    sqlite3_finalize(stmt);
    sqlite3_close(dataBase);

    return 1;
}


int getConversationHistory(const char* currentUser, const char* recipient, char*** messages, int* messageCount) 
{
    sqlite3* dataBase;
    sqlite3_stmt* stmt;
    char tableName1[256], tableName2[256], query[1024];
    char selectedTableName[256] = {0}; // Buffer sigur pentru numele tabelei
    int result;

    char normalizedCurrentUser[256], normalizedRecipient[256];
    strncpy(normalizedCurrentUser, currentUser, sizeof(normalizedCurrentUser) - 1);
    strncpy(normalizedRecipient, recipient, sizeof(normalizedRecipient) - 1);
    normalizeString(normalizedCurrentUser);
    normalizeString(normalizedRecipient);

    *messages = NULL;  // Inițializează pointerul de mesaje
    *messageCount = 0; // Inițializează numărul de mesaje

    printf("[Debug] Utilizator curent normalizat: %s\n", normalizedCurrentUser);
    printf("[Debug] Destinatar normalizat: %s\n", normalizedRecipient);

    // Deschiderea bazei de date
    if (sqlite3_open(DB_NAME, &dataBase) != SQLITE_OK) {
        printf("[Error] Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(dataBase));
        return 0;
    }

    // Construirea numelor posibile ale tabelei
    snprintf(tableName1, sizeof(tableName1), "conv_%s_%s", normalizedCurrentUser, normalizedRecipient);
    snprintf(tableName2, sizeof(tableName2), "conv_%s_%s", normalizedRecipient, normalizedCurrentUser);

    // Verifică dacă tabela există
    snprintf(query, sizeof(query), 
        "SELECT name FROM sqlite_master WHERE type='table' AND (name='%s' OR name='%s');", 
        tableName1, tableName2);

    printf("[Debug] Interogare verificare existență tabelă: %s\n", query);

    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[Error] Eroare la verificarea existenței tabelei: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        const char* tempTableName = (const char*)sqlite3_column_text(stmt, 0);
        if (tempTableName != NULL) {
            strncpy(selectedTableName, tempTableName, sizeof(selectedTableName) - 1);
            selectedTableName[sizeof(selectedTableName) - 1] = '\0';
            printf("[Debug] Tabela selectată: %s\n", selectedTableName);
        } else {
            printf("[Error] Nu am putut obține numele tabelei.\n");
            sqlite3_finalize(stmt);
            sqlite3_close(dataBase);
            return 0;
        }
    } else {
        printf("[Info] Nu există mesaje pentru această conversație.\n");
        sqlite3_finalize(stmt);
        sqlite3_close(dataBase);
        return 0;
    }
    sqlite3_finalize(stmt);

    // Interoghează mesajele din tabelă
    snprintf(query, sizeof(query), "SELECT date, sender, message FROM \"%s\" ORDER BY date;", selectedTableName);

    printf("[Debug] Interogare pentru selectarea mesajelor: %s\n", query);

    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[Error] Eroare la pregătirea interogării: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    // Alocă și salvează mesaje
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* date = (const char*)sqlite3_column_text(stmt, 0);
        const char* sender = (const char*)sqlite3_column_text(stmt, 1);
        const char* message = (const char*)sqlite3_column_text(stmt, 2);

        if (!date || !sender || !message) {
            printf("[Warning] Un câmp al unui mesaj este NULL, ignorăm acest mesaj.\n");
            continue;
        }

        char formattedMessage[1024];
        snprintf(formattedMessage, sizeof(formattedMessage), "[%s] %s: %s", date, sender, message);
        printf("[Debug] Mesaj formatat: %s\n", formattedMessage);

        char** temp = realloc(*messages, (*messageCount + 1) * sizeof(char*));
        if (temp == NULL) {
            printf("[Error] Eroare la alocarea memoriei pentru mesaje.\n");
            sqlite3_finalize(stmt);
            sqlite3_close(dataBase);
            return 0;
        }
        *messages = temp;
        (*messages)[*messageCount] = strdup(formattedMessage); // Copiem mesajul
        if ((*messages)[*messageCount] == NULL) {
            printf("[Error] Eroare la duplicarea mesajului formatat.\n");
            continue;
        }
        (*messageCount)++;
    }

    if (result != SQLITE_DONE) {
        printf("[Error] Eroare la parcurgerea rezultatelor: %s\n", sqlite3_errmsg(dataBase));
    } else {
        printf("[Debug] Toate mesajele au fost încărcate cu succes.\n");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(dataBase);

    printf("[Info] Total mesaje încărcate: %d\n", *messageCount);
    return 1;
}
 
 int getConversations(const char* username, char*** conversations, int* conversationCount) 
 {
    sqlite3* dataBase;
    sqlite3_stmt* stmt;
    char query[1024];
    char normalizedUsername[256];
    int result;

    strncpy(normalizedUsername, username, sizeof(normalizedUsername));
    normalizeString(normalizedUsername);

    *conversations = NULL;
    *conversationCount = 0;

    // Deschiderea bazei de date
    if (sqlite3_open(DB_NAME, &dataBase) != SQLITE_OK) {
        printf("Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(dataBase));
        return 0;
    }

    // Interogare pentru tabele care conțin numele utilizatorului
    snprintf(query, sizeof(query), 
        "SELECT name FROM sqlite_master "
        "WHERE type='table' AND name LIKE 'conv_%%' "
        "AND (name LIKE 'conv_%s_%%' OR name LIKE 'conv_%%_%s');",
        normalizedUsername, normalizedUsername);

    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Eroare la pregătirea interogării: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    // Procesarea rezultatelor
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* tableName = (const char*)sqlite3_column_text(stmt, 0);

        *conversations = realloc(*conversations, (*conversationCount + 1) * sizeof(char*));
        (*conversations)[*conversationCount] = strdup(tableName);
        (*conversationCount)++;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(dataBase);

    return 1;
}

int replyMessage(const char* username, const char* recipient, int replyToMessageId, const char* messageText) 
{
    sqlite3* dataBase;
    sqlite3_stmt* stmt;
    char tableName1[256], tableName2[256];
    char query[1024];
    int result;
    char* existingTable = NULL;

    // Normalizează numele utilizatorilor
    char normalizedUsername[256];
    char normalizedRecipient[256];
    strncpy(normalizedUsername, username, sizeof(normalizedUsername));
    strncpy(normalizedRecipient, recipient, sizeof(normalizedRecipient));
    normalizeString(normalizedUsername);
    normalizeString(normalizedRecipient);

    // Construim cele două variante pentru numele tabelei
    snprintf(tableName1, sizeof(tableName1), "conv_%s_%s", normalizedUsername, normalizedRecipient);
    snprintf(tableName2, sizeof(tableName2), "conv_%s_%s", normalizedRecipient, normalizedUsername);

    // Deschide baza de date
    if (sqlite3_open(DB_NAME, &dataBase) != SQLITE_OK) {
        printf("Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(dataBase));
        return 0;
    }

    // Verificăm dacă prima tabelă există
    snprintf(query, sizeof(query), "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", tableName1);
    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Eroare la verificarea existenței tabelei %s: %s\n", tableName1, sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }
    result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        existingTable = tableName1;
    }
    sqlite3_finalize(stmt);

    // Dacă prima tabelă nu există, verificăm a doua
    if (existingTable == NULL) {
        snprintf(query, sizeof(query), "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", tableName2);
        if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) {
            printf("Eroare la verificarea existenței tabelei %s: %s\n", tableName2, sqlite3_errmsg(dataBase));
            sqlite3_close(dataBase);
            return 0;
        }
        result = sqlite3_step(stmt);
        if (result == SQLITE_ROW) {
            existingTable = tableName2;
        }
        sqlite3_finalize(stmt);
    }

    // Dacă niciuna dintre tabele nu există, returnăm eroare
    if (existingTable == NULL) 
    {
        printf("Nicio tabelă de conversație nu există pentru utilizatorii %s și %s.\n", username, recipient);
        sqlite3_close(dataBase);
        return 0;
    }

    // Verificăm dacă mesajul original la care se răspunde există
    snprintf(query, sizeof(query), "SELECT id FROM %s WHERE id=?;", existingTable);
    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) 
    {
        printf("Eroare la pregătirea interogării pentru verificarea mesajului original: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, replyToMessageId);

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) 
    {
        printf("Mesajul original cu ID=%d nu există în tabela %s.\n", replyToMessageId, existingTable);

        // Debugging: afișează toate mesajele din tabelă
        snprintf(query, sizeof(query), "SELECT id, sender, message FROM %s;", existingTable);
        if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) == SQLITE_OK) {
            printf("Mesaje existente în tabela %s:\n", existingTable);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                const char* sender = (const char*)sqlite3_column_text(stmt, 1);
                const char* message = (const char*)sqlite3_column_text(stmt, 2);
                printf("ID: %d, Sender: %s, Message: %s\n", id, sender, message);
            }
        }
        sqlite3_finalize(stmt);
        sqlite3_close(dataBase);
        return 0;
    }
    sqlite3_finalize(stmt);

    // Inserează mesajul de răspuns
    snprintf(query, sizeof(query), "INSERT INTO %s (sender, message, reply_to) VALUES (?, ?, ?);", existingTable);
    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) 
    {
        printf("Eroare la pregătirea interogării pentru inserare: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, messageText, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, replyToMessageId);

    result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) 
    {
        printf("Eroare la inserarea mesajului de răspuns: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_finalize(stmt);
        sqlite3_close(dataBase);
        return 0;
    }

    printf("Mesaj de răspuns inserat cu succes în tabela %s.\n", existingTable);
    sqlite3_finalize(stmt);
    sqlite3_close(dataBase);

    return 1;
}

int getUsers(char*** users, int* userCount) 
{
    sqlite3* dataBase;
    sqlite3_stmt* stmt;
    char query[1024];
    int result;

    *users = NULL;
    *userCount = 0;

    // Deschiderea bazei de date
    if (sqlite3_open(DB_NAME, &dataBase) != SQLITE_OK) {
        printf("Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(dataBase));
        return 0;
    }

    // Interogare pentru a obține utilizatorii din tabelul "users"
    snprintf(query, sizeof(query), "SELECT username FROM users;");

    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Eroare la pregătirea interogării: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    // Procesarea rezultatelor
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* username = (const char*)sqlite3_column_text(stmt, 0);

        *users = realloc(*users, (*userCount + 1) * sizeof(char*));
        (*users)[*userCount] = strdup(username);
        (*userCount)++;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(dataBase);

    return 1;
}

int getUsersOnline(char*** users, int* userCount) 
{
    sqlite3* dataBase;
    sqlite3_stmt* stmt;
    char query[1024];
    int result;

    *users = NULL;
    *userCount = 0;

    // Deschiderea bazei de date
    if (sqlite3_open(DB_NAME, &dataBase) != SQLITE_OK) {
        printf("Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(dataBase));
        return 0;
    }

    // Modificăm interogarea pentru a obține doar utilizatorii logați
    snprintf(query, sizeof(query), "SELECT username FROM users WHERE Logat = 1;");

    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Eroare la pregătirea interogării: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    // Procesarea rezultatelor
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* username = (const char*)sqlite3_column_text(stmt, 0);

        *users = realloc(*users, (*userCount + 1) * sizeof(char*));
        (*users)[*userCount] = strdup(username);
        (*userCount)++;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(dataBase);

    return 1;
}

int getUsersOffline(char*** users, int* userCount) 
{
    sqlite3* dataBase;
    sqlite3_stmt* stmt;
    char query[1024];
    int result;

    *users = NULL;
    *userCount = 0;

    // Deschiderea bazei de date
    if (sqlite3_open(DB_NAME, &dataBase) != SQLITE_OK) {
        printf("Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(dataBase));
        return 0;
    }

    // Modificăm interogarea pentru a obține doar utilizatorii logați
    snprintf(query, sizeof(query), "SELECT username FROM users WHERE Logat = 0;");

    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Eroare la pregătirea interogării: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    // Procesarea rezultatelor
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* username = (const char*)sqlite3_column_text(stmt, 0);

        *users = realloc(*users, (*userCount + 1) * sizeof(char*));
        (*users)[*userCount] = strdup(username);
        (*userCount)++;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(dataBase);

    return 1;
}

int quitUserDB(char* userName) 
{
    sqlite3* dataBase;
    sqlite3_stmt* updateStmt;
    int result = 0;
    int dbResult = sqlite3_open(DB_NAME, &dataBase);

    if (dbResult != SQLITE_OK) {
        printf("Error to open %s db error %s \n", DB_NAME, sqlite3_errmsg(dataBase));
        return 0;
    }

    const char* updateQuery = "UPDATE users SET Logat = 0 WHERE username = ?";

    // Prepare the update query to log out the user
    if (sqlite3_prepare_v2(dataBase, updateQuery, -1, &updateStmt, NULL) != SQLITE_OK) {
        printf("Failed to prepare update statement: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    sqlite3_bind_text(updateStmt, 1, userName, -1, SQLITE_STATIC);

    // Execute the update query
    if (sqlite3_step(updateStmt) != SQLITE_DONE) {
        printf("Failed to update Logat status: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_finalize(updateStmt);
        sqlite3_close(dataBase);
        return 0;
    }

    sqlite3_finalize(updateStmt);
    sqlite3_close(dataBase);

    return 1;  // Success
}

int getUnreadMessages(const char* username, char*** messages, int* messageCount) 
{
    sqlite3* dataBase;
    sqlite3_stmt* stmt;
    char query[1024];
    char** resultMessages = NULL;
    int resultCount = 0;

    // Deschiderea bazei de date
    if (sqlite3_open(DB_NAME, &dataBase) != SQLITE_OK) 
    {
        printf("[Database] Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(dataBase));
        return 0;
    }

    // Construim interogarea pentru a obține toate tabelele conversațiilor
    snprintf(query, sizeof(query),
        "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE 'conv_%%';");

    if (sqlite3_prepare_v2(dataBase, query, -1, &stmt, NULL) != SQLITE_OK) 
    {
        printf("[Database] Eroare la pregătirea interogării: %s\n", sqlite3_errmsg(dataBase));
        sqlite3_close(dataBase);
        return 0;
    }

    // Iterăm prin toate tabelele conversațiilor
    while (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        const char* tableName = (const char*)sqlite3_column_text(stmt, 0);

        // Determinăm poziția utilizatorului în tabel
        if (strncmp(tableName + 5, username, strlen(username)) == 0) 
        {
            // username este primul nume
            snprintf(query, sizeof(query),
                "SELECT id, sender, message FROM %s WHERE read1 = 0;", tableName);
        } 
        else if (strstr(tableName, username) != NULL) 
        {
            // username este al doilea nume
            snprintf(query, sizeof(query),
                "SELECT id, sender, message FROM %s WHERE read2 = 0;", tableName);
        } 
        else 
        {
            continue; // Tabelul nu conține utilizatorul
        }

        // Pregătim și executăm interogarea pentru mesajele necitite
        sqlite3_stmt* messageStmt;
        if (sqlite3_prepare_v2(dataBase, query, -1, &messageStmt, NULL) != SQLITE_OK) 
        {
            printf("[Database] Eroare la pregătirea interogării pentru %s: %s\n",
                tableName, sqlite3_errmsg(dataBase));
            continue;
        }

        // Iterăm prin mesajele necitite și le adăugăm la lista de rezultate
        while (sqlite3_step(messageStmt) == SQLITE_ROW) 
        {
            int messageId = sqlite3_column_int(messageStmt, 0);
            const char* sender = (const char*)sqlite3_column_text(messageStmt, 1);
            const char* message = (const char*)sqlite3_column_text(messageStmt, 2);

            char formattedMessage[1024];
            snprintf(formattedMessage, sizeof(formattedMessage), 
                "[De la: %s] %s", sender ? sender : "Necunoscut", message ? message : "");

            // Adăugăm mesajul formatat la lista de rezultate
            resultMessages = realloc(resultMessages, (resultCount + 1) * sizeof(char*));
            resultMessages[resultCount] = strdup(formattedMessage);
            resultCount++;

            // Marcare ca citit
            if (strncmp(tableName + 5, username, strlen(username)) == 0) 
            {
                // username este primul nume
                snprintf(query, sizeof(query), "UPDATE %s SET read1 = 1 WHERE id = ?;", tableName);
            } 
            else 
            {
                // username este al doilea nume
                snprintf(query, sizeof(query), "UPDATE %s SET read2 = 1 WHERE id = ?;", tableName);
            }

            sqlite3_stmt* updateStmt;
            if (sqlite3_prepare_v2(dataBase, query, -1, &updateStmt, NULL) != SQLITE_OK) 
            {
                printf("[Database] Eroare la pregătirea interogării de update pentru %s: %s\n",
                    tableName, sqlite3_errmsg(dataBase));
                continue;
            }

            sqlite3_bind_int(updateStmt, 1, messageId);
            if (sqlite3_step(updateStmt) != SQLITE_DONE) 
            {
                printf("[Database] Eroare la actualizarea mesajului %d în %s: %s\n",
                    messageId, tableName, sqlite3_errmsg(dataBase));
            }
            sqlite3_finalize(updateStmt);
        }

        sqlite3_finalize(messageStmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(dataBase);

    // Setăm rezultatele
    *messages = resultMessages;
    *messageCount = resultCount;

    printf("[Database] Mesaje necitite găsite și marcate ca citite: %d\n", resultCount);
    return 1; // Succes
}
