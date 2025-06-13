#ifndef DATABASE_H
#define DATABASE_H

struct Item {
    char* name;
    int amount;
};

void saveDatabase();

void loadDatabase();

void addItemToDatabase(const char* name);

void removeItemFromDatabase(const char* name);

void printDatabase();

#endif DATABASE_H
