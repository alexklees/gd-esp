#include "Shop.h"
#include "Globals.h"
#include <vector>
#include <random>
#include <algorithm>

void generateShopItems() {
    shopItems.clear();
    std::vector<int> all;
    for (int i = 0; i < PU_COUNT; i++) all.push_back(i);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(all.begin(), all.end(), g);
    for (int i = 0; i < 3; i++) shopItems.push_back((PowerUpType)all[i]);
    selectedShopItem = 0;
    shopInitialized = true;
}
