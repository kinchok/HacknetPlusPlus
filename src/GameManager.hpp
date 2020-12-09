#pragma once
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "lib/json.hpp"
#include "Computer.hpp"
#include "FileSystem/FileSystemImport.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

class GameManager {
    private:
        static GameManager* instance;

        GameManager() { }

        GameManager(Computer* player) {
            playerComp = player;
            currentComp = player;
            currentFolder = player->getFileSystem();

            std::string ip = player->getIP();
            computerNetwork[ip] = player;
            showConnected();
        }

        void setPlayer(Computer* player) {
            playerComp = player;
            currentComp = player;
            currentFolder = player->getFileSystem();
        }

        Computer* playerComp;
        Computer* currentComp;

        Folder* currentFolder;

        std::map<std::string, Computer*> computerIDs;
        std::map<std::string, Computer*> computerNetwork;

    public:
        static GameManager* getInstance() {
            if (instance == nullptr) {
                instance = new GameManager(new Computer("Tarche's battlestation", "tarche", "123.123.123.123", 4));
            }
            return instance;
        }

        static void loadExtension(std::string path);

        ~GameManager() {
            for (auto c : computerNetwork) {
                delete c.second;
            }
        }

        void addComputer(Computer* comp) {
            std::string ip = comp->getIP();
            computerNetwork[ip] = comp;
            currentComp->addLink(comp);
        }

        void addLink(Computer* from, Computer* to) {
            from->addLink(to);
        }

        void connect(std::string ip) {
            if (computerNetwork[ip] == nullptr) {
                std::cout << "Connection refused." << '\n';
            }
            else {
                currentComp = computerNetwork[ip];
                currentFolder = currentComp->getFileSystem();
                showConnected();
            }
        }

        void setDirectory(FileSystemElement* dir) {
            currentFolder = (Folder*)dir;
        }

        Computer* getCurrent() {
            return currentComp;
        }

        Computer* getPlayer() {
            return playerComp;
        }

        Folder* getDirectory() {
            return currentFolder;
        }

        Folder* getPlayerDir() {
            return playerComp->getFileSystem();
        }

        void showConnected() {
            std::cout << "Connected to " << currentComp->toString() << '\n';
        }
};

GameManager* GameManager::instance = nullptr;

void GameManager::loadExtension(std::string path) {
    json extensionInfo;
    std::ifstream infoFile(path + "/extension_info.json");
    
    if (!infoFile.is_open()) {
        std::cerr << "Error opening extension_info.json" << '\n';
        return;
    }

    infoFile >> extensionInfo;

    if (extensionInfo["game"] != "hacknet++") {
        std::cerr << "This extension if not for Hacknet++" << '\n';
        return;
    }

    std::string playerID = extensionInfo["playerComp"];
    std::string nodeFolder = path + "/" + (std::string)extensionInfo["nodeFolder"];

    std::map<std::string, std::vector<std::string>> dlinks;

    GameManager *g = new GameManager();
    instance = g;

    for (const auto &nodeFile : fs::directory_iterator(nodeFolder)) {
        json curr;
        std::ifstream currFile(nodeFile.path());
        currFile >> curr;

        std::string id = curr["id"];

        Computer* currComp = new Computer(
            curr["name"],
            id,
            curr["ip"],
            curr["security"]["required"]
        );

        g->computerIDs[id] = currComp;
        g->computerNetwork[curr["ip"]] = currComp;

        if (id == playerID) {
            g->setPlayer(currComp);
        }

        for (auto link : curr["dlinks"]) {
            dlinks[id].push_back(link);
        }
    }

    for (auto i : dlinks) {
        for (auto j : i.second) {
            g->addLink(g->computerIDs[i.first], g->computerIDs[j]);
        }
    }
}
