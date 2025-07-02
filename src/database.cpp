#include "database.h"
#include "Arduino.h"
#include <vector>
#include "LittleFS.h"
#include "ArduinoJson.h"

#define DP_PATH "/database.json"

std::vector<Item> database;

void initDatabase()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("Failed to mount LittleFS");
        return;
    }

    if (LittleFS.exists(DP_PATH))
    {
        loadDatabase();
    }
    else
    {
        Serial.println("Database file does not exist, creating a new one");
        saveDatabase();
    }
}

void saveDatabase()
{
    File file = LittleFS.open(DP_PATH, "w");
    if (!file)
    {
        Serial.println("Failed to open database file for writing");
        return;
    }

    DynamicJsonDocument doc(1024);
    JsonArray items = doc.to<JsonArray>();

    for (const auto &item : database)
    {
        JsonObject obj = items.createNestedObject();
        obj["name"] = item.name;
        obj["amount"] = item.amount;
    }

    if (serializeJson(doc, file) == 0)
    {
        Serial.println("Failed to write database to file");
    }

    file.close();
}

void loadDatabase()
{
    File file = LittleFS.open(DP_PATH, "r");
    if (!file)
    {
        Serial.println("Failed to open database file for reading");
        return;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        Serial.println("Failed to parse database file");
        file.close();
        return;
    }

    database.clear();
    JsonArray items = doc.as<JsonArray>();
    for (const auto &item : items)
    {
        Item newItem;
        newItem.name = strdup(item["name"].as<const char *>());
        newItem.amount = item["amount"].as<int>();
        database.push_back(newItem);
    }

    file.close();
}

void addItemToDatabase(const char *name)
{
    for (auto &item : database)
    {
        if (strcmp(item.name, name) == 0)
        {
            item.amount++;
            saveDatabase();
            return;
        }
    }

    Item newItem;
    newItem.name = strdup(name);
    newItem.amount = 1;
    database.push_back(newItem);
    saveDatabase();
}

void removeItemFromDatabase(const char *name)
{
    for (auto it = database.begin(); it != database.end(); ++it)
    {
        if (strcmp(it->name, name) == 0)
        {
            if (--it->amount <= 0)
            {
                free(it->name);
                database.erase(it);
            }
            saveDatabase();
            return;
        }
    }
}

void printDatabase()
{
    Serial.println("Database contents:");
    for (const auto &item : database)
    {
        Serial.printf("Item: %s, Amount: %d\n", item.name, item.amount);
    }
}

String getDatabaseContentForPrompt()
{
    String result = "Aktuelle Datenbankinhalte:\n";
    for (const auto &item : database)
    {
        result += String(item.name) + " (Anzahl: " + String(item.amount) + ")\n";
    }
    return result;
}