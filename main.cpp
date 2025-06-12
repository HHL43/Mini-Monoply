#include <iostream>
#include <vector>
#include <string>
#include <iomanip>    // For std::setw and std::left
#include <limits>     // For std::numeric_limits
#include <cstdlib>    // For rand() and srand()
#include <ctime>      // For time()

#include "map.h"
#include "Player.h"

void clearScreen();
// Function Prototypes
void displayBoard(const WorldMap& map, const std::vector<Player*>& players);
void displayPlayerStatus(const std::vector<Player*>& players, int currentPlayerIndex);
int rollDice();
void handleBuyAction(Player* player, MapUnit* unit);
void handleUpgradeAction(Player* player, UpgradableUnit* unit);
void waitForEnter();

int main() {
    srand(time(0));

    // 1. Game Setup
    WorldMap worldMap;
    std::vector<Player*> players;
    int numPlayers = 0;
    std::vector<std::string> defaultNames = {"A-Tu", "Little-Mei", "King-Baby", "Mrs.Money"};

    // ==================== �B�z��r�μƦr��J���޿� ====================
    std::cout << "How many players?(Maximum:4)...>";
    std::cin >> numPlayers;

    // ���p 1: ��J���O�L�Ĥ��e (�Ҧp��r)
    if (std::cin.fail()) {
        numPlayers = 1;
        std::cin.clear(); // �M�����~�X��
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // �����w�İϤ����L�Ŀ�J

        // �����Ыعw�]�� 1 �쪱�a�A���ݦA�߰ݩm�W
        players.push_back(new Player(0, defaultNames[0]));
        worldMap.getUnit(0)->addPlayerHere(players.back());

    }
    // ���p 2: ��J���O�Ʀr
    else {
        // �N���a�H�ƭ���b 1 �� 4 ����
        if (numPlayers > 4) {
            numPlayers = 4;
        } else if (numPlayers < 1) {
            numPlayers = 1;
        }

        // �b�ϥ� getline ���e�A�M����J�w�İϤ��i��ݯd�������
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // �ھڪ��a�H�ơA�`���߰ݩm�W
        for (int i = 0; i < numPlayers; ++i) {
            std::string name;
            std::cout << "Please input player " << i + 1 << "'s name (Default: " << defaultNames[i] << ")...>";
            std::getline(std::cin, name);

            if (name.empty()) {
                name = defaultNames[i];
            }
            players.push_back(new Player(i, name));
            // �N�Ҧ����a�[��_�l�I (��m 0)
            worldMap.getUnit(0)->addPlayerHere(players.back());
        }
    }
    clearScreen();

    // --- Initial Game State Display ---
    displayBoard(worldMap, players);
    displayPlayerStatus(players, 0);

    // 2. Main Game Loop
    int currentPlayerIndex = 0;
    int activePlayers = numPlayers;

    while (activePlayers > 1) {
        Player* currentPlayer = players[currentPlayerIndex];

        if (currentPlayer->getStatus() == PlayerStatus::Bankrupt) {
            currentPlayerIndex = (currentPlayerIndex + 1) % numPlayers;
            continue;
        }

        std::cout << currentPlayer->getName() << ", your action? (1:Dice [default] / 2:Exit)...>";
        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "2") {
            break;
        }

        if (currentPlayer->getStatus() == PlayerStatus::InJail) {
            std::cout << currentPlayer->getName() << " is in jail and misses a turn." << std::endl;
            currentPlayer->releaseFromJail();
            waitForEnter();
            currentPlayerIndex = (currentPlayerIndex + 1) % numPlayers;
            displayBoard(worldMap, players);
            displayPlayerStatus(players, currentPlayerIndex);
            continue;
        }

        // 4. Roll Dice and Move
        int diceRoll = rollDice();

        int oldLocation = currentPlayer->getLocation();
        int newLocation = (oldLocation + diceRoll) % worldMap.getUnitCount();

        if (newLocation < oldLocation) {
            int reward = 2000;
            std::cout << currentPlayer->getName() << " passed GO and collects $" << reward << "!" << std::endl; // Passing start gives reward
            currentPlayer->receive(reward);
        }
        currentPlayer->moveTo(newLocation, &worldMap);

        MapUnit* currentUnit = worldMap.getUnit(newLocation);

        // ����ܲ��ʫ᪺�C���L��
        clearScreen();
        displayBoard(worldMap, players);
        displayPlayerStatus(players, currentPlayerIndex);

        std::cout << currentPlayer->getName() << " moved to " << currentUnit->getName() << std::endl;

        currentUnit->onVisit(currentPlayer);
        handleBuyAction(currentPlayer, currentUnit);

        // Check for bankruptcy
        if (currentPlayer->getMoney() < 0) {
            std::cout << currentPlayer->getName() << " is bankrupt!" << std::endl;
            currentPlayer->declareBankruptcy();
            currentPlayer->releaseAllUnits();
            activePlayers--;
        }

        waitForEnter();

        // Move to the next player
        currentPlayerIndex = (currentPlayerIndex + 1) % numPlayers;

        // Display board for the next turn
        clearScreen();
        displayBoard(worldMap, players);
        displayPlayerStatus(players, currentPlayerIndex);
    }
/*
    // 8. Announce Winner
    for(Player* p : players) {
        if(p->getStatus() != PlayerStatus::Bankrupt) {
            std::cout << "\nGame Over! The winner is " << p->getName() << "!" << std::endl;
            break;
        }
    }

    // Cleanup
    for (Player* p : players) {
        delete p;
    }
*/
    return 0;
}

/* �M���D���x�ù� */
void clearScreen() {
    system("cls");
}

int rollDice() {
    return rand() % 6 + 1; // A random number in [1, 6]
}

void waitForEnter() {
    std::cout << "\nPress Enter to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
}


void handleBuyAction(Player* player, MapUnit* unit) {
    if (unit->getHost() == nullptr && unit->getPrice() > 0) { // Unowned and buyable
        int price = unit->getPrice();
        if (player->getMoney() >= price) {
            std::cout << player->getName() << ", do you want to buy " << unit->getName() << "? (1: Yes [default] / 2: No) ...>";
            std::string buy_choice;
            std::getline(std::cin, buy_choice);
            if (buy_choice != "2") {
                player->pay(price);
                player->addUnit(unit);
                std::cout << "You pay $" << price << " to buy " << unit->getName() << std::endl;
            }
        }
    }
    // Dynamic cast to check if the unit is upgradable
    else if (unit->getHost() == player) {
        if (UpgradableUnit* u_unit = dynamic_cast<UpgradableUnit*>(unit)) {
             handleUpgradeAction(player, u_unit);
        }
    }
}

void handleUpgradeAction(Player* player, UpgradableUnit* unit) {
    if (unit->getLevel() < 5) { // Maximum level is 5
        int upgrade_price = unit->getUpgradePrice();
        if (player->getMoney() >= upgrade_price) {
             std::cout << "You own " << unit->getName() << " (Lv." << unit->getLevel() << "). Upgrade to Lv." << (unit->getLevel() + 1) << " costs $" << upgrade_price << std::endl;
             std::cout << "Do you want to upgrade? (1:Yes [default] / 2:No)...>";
             std::string upgrade_choice;
             std::getline(std::cin, upgrade_choice);
             if(upgrade_choice != "2") {
                player->pay(upgrade_price);
                unit->upgrade();
                std::cout << player->getName() << " upgraded " << unit->getName() << " to Lv." << unit->getLevel() << std::endl;
             }
        }
    }
}

std::string getUnitDetailsString(MapUnit* unit) {
    std::stringstream ss;
    Player* owner = unit->getHost();

    // �p�G�a���Q�֦�
    if (owner) {
        ss << "{" << owner->getId() << "} ";
        // �ˬd�O�_���i�ɯų��
        if (UpgradableUnit* u_unit = dynamic_cast<UpgradableUnit*>(unit)) {
            ss << "U$ " << std::left << std::setw(5) << u_unit->getFine()
               << "L" << u_unit->getLevel();
        } else { // ��L�����]�p C, R�^�A�~������ʶR��
            ss << "B$ " << std::left << std::setw(8) << unit->getPrice();
        }
    }
    // �p�G�a�����Q�֦�
    else {
        ss << "B$ " << std::left << std::setw(8) << unit->getPrice();
    }
    return ss.str();
}


void displayBoard(const WorldMap& map, const std::vector<Player*>& players) {
    if (map.getUnitCount() == 0) return;

    const int n_players = players.size();
    int half_size = (map.getUnitCount() + 1) / 2;

    for (int i = 0; i < half_size; ++i) {
        int left_id = i;
        int right_id = map.getUnitCount() - 1 - i;

        MapUnit* left_unit = map.getUnit(left_id);
        MapUnit* right_unit = (left_id != right_id) ? map.getUnit(right_id) : nullptr;

        // --- 1. �ǳƪ��a��m�y�D�r�� ---
        std::string left_track(n_players, ' ');
        for (const auto& p : players) {
            if (p->getStatus() != PlayerStatus::Bankrupt && p->getLocation() == left_id) {
                left_track[p->getId()] = std::to_string(p->getId())[0];
            }
        }

        std::string right_track(n_players, ' ');
        if (right_unit) {
            for (const auto& p : players) {
                if (p->getStatus() != PlayerStatus::Bankrupt && p->getLocation() == right_id) {
                    right_track[p->getId()] = std::to_string(p->getId())[0];
                }
            }
        }

        // --- 2. �ǳƥ����a�����U�Ӹ�T�϶� ---
        std::stringstream left_owner_ss;
        if (left_unit->getHost() != nullptr) {
            left_owner_ss << "{" << left_unit->getHost()->getId() << "}";
        }

        std::stringstream left_info_ss;
        if (left_unit->getHost() == nullptr) {
            left_info_ss << "B$ " << left_unit->getPrice();
        } else {
            if (UpgradableUnit* u_unit = dynamic_cast<UpgradableUnit*>(left_unit)) {
                left_info_ss << "U$ " << u_unit->getUpgradePrice() << " L" << u_unit->getLevel();
            } else { // �B�z��L�����p C, R ���a��
                left_info_ss << "Owned";
            }
        }

        // --- 3. �ǳƥk���a�����U�Ӹ�T�϶� ---
        std::stringstream right_owner_ss, right_info_ss;
        if (right_unit) {
            if (right_unit->getHost() != nullptr) {
                right_owner_ss << "{" << right_unit->getHost()->getId() << "}";
            }
            if (right_unit->getHost() == nullptr) {
                right_info_ss << "B$ " << right_unit->getPrice();
            } else {
                if (UpgradableUnit* u_unit = dynamic_cast<UpgradableUnit*>(right_unit)) {
                    right_info_ss << "U$ " << u_unit->getUpgradePrice() << " L" << u_unit->getLevel();
                } else {
                    right_info_ss << "Owned";
                }
            }
        }

        // --- 4. �̲׿�X�ƪ� ---
        // ���b��
        std::cout << "=" << std::setw(n_players) << std::left << left_track << "=  "
                  << "[" << left_id << "] "
                  << std::setw(10) << std::right << left_unit->getName() // �W�٦� 10 ��
                  << " " << std::setw(4) << std::left << left_owner_ss.str()   // �֦��̦� 5 ��
                  << std::setw(14) << std::left << left_info_ss.str();  // �ԲӸ�T�� 12 ��

        // �k�b��
        if (right_unit) {
            std::cout << "="
                      << std::setw(n_players) << std::left << right_track << "=  "
                      << "[" << right_id << "] "
                      << std::setw(10) << std::right << right_unit->getName()
                      << " " << std::setw(4) << std::left << right_owner_ss.str()
                      << std::setw(12) << std::left << right_info_ss.str();
        }

        std::cout << std::endl;
    }
}



void displayPlayerStatus(const std::vector<Player*>& players, int currentPlayerIndex) {
    std::cout << std::endl;
    for (int i = 0; i < players.size(); ++i) {
        const auto& p = players[i];
        if (p->getStatus() == PlayerStatus::Bankrupt) {
            std::cout << "   [" << p->getId() << "] " << std::setw(12) << std::left << p->getName() << "is BANKRUPT" << std::endl;
            continue;
        }

        if (i == currentPlayerIndex) std::cout << "=>";
        else std::cout << "  ";

        std::cout << "[" << p->getId() << "] " << std::setw(12) << std::left << p->getName()
                  << "$" << std::setw(8) << std::left << p->getMoney()
                  << "with " << p->getUnitCount() << " units" << std::endl;
    }
    std::cout << std::endl;
}
