#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <conio.h>      // Для _kbhit() и _getch()
#include <windows.h>    // Для Sleep()

using namespace std;

// ============= МУЗЫКА (Windows) =============
static int musicPID = -1;

void stopMusic() {
    if (musicPID != -1) {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "taskkill /PID %d /F >nul 2>&1", musicPID);
        system(cmd);
        musicPID = -1;
    }
}

void playMusic(const char* filename) {
    stopMusic();
    char cmd[512];
    // Используем PowerShell для фонового воспроизведения
    snprintf(cmd, sizeof(cmd), 
             "powershell -Command \"$p = Start-Process -FilePath '%s' -PassThru; $p.Id\" > temp_pid.txt", 
             filename);
    system(cmd);
    
    // Читаем PID из временного файла
    FILE* pidFile = fopen("temp_pid.txt", "r");
    if (pidFile) {
        fscanf(pidFile, "%d", &musicPID);
        fclose(pidFile);
        remove("temp_pid.txt");
    }
}

// ============= КОНСТАНТЫ =============
const int W = 32;
const int H = 16;
const int MAX_LEVEL = 15;
const int RIDDLES_COUNT = 30;

// ============= СТРУКТУРЫ =============
struct Player {
    int x, y;
    char symbol = '@';
    int hp, maxHp;
    int damage;
    int level = 1;
    int xp = 0;
    int xpToNext = 20;
    int potions = 3;
    int coins = 0;
    int critChance = 10;
    int armor = 0;
    int dodgeChance = 5;
};

struct Boss {
    int x, y;
    int hp, maxHp;
    int alive;
    int damage;
    int attackCooldown;
    int attackTimer;
    string name;
    int attackChance;
    int critChance;
    int armor;
};

struct Chest {
    int x, y;
    int opened;
    string puzzle;
    string answer;
    int solved;
};

struct Game {
    vector<string> map;
    Player player;
    Boss boss;
    Chest chest;
    int running = 1;
    int level = 1;
    int gameState = 1;
    int turnCounter = 0;
    int musicEnabled = 1;
};

void levelUp(Game &g);
void nextLevel(Game &g);
void shop(Game &g);
void bossAI(Game &g);

// ============= ВВОД (Windows) =============
char getKey() {
    if (_kbhit()) {
        return _getch();
    }
    return 0;
}

int kbhit() {
    return _kbhit();
}

void waitKey() {
    while (!_kbhit()) {
        Sleep(10);
    }
    _getch(); // Очищаем буфер
}

// ============= ГЕНЕРАЦИЯ КАРТЫ =============
void generateMap(Game &g) {
    g.map.clear();
    
    for (int y = 0; y < H; y++) {
        string row;
        for (int x = 0; x < W; x++) {
            if (x == 0 || y == 0 || x == W-1 || y == H-1)
                row += '#';
            else
                row += '.';
        }
        g.map.push_back(row);
    }
    
    g.player.x = 2;
    g.player.y = 2;
    
    g.chest.x = 5 + rand() % (W - 10);
    g.chest.y = 5 + rand() % (H - 10);
    g.chest.opened = 0;
    g.chest.solved = 0;
    
    const char* riddles[RIDDLES_COUNT] = {
        "Что можно увидеть с закрытыми глазами?",
        "Что идёт, но не движется?",
        "Что имеет ключ, но не открывает замок?",
        "Что становится больше, когда его берут?",
        "Что можно сломать, не прикасаясь?",
        "Что всегда перед тобой, но ты не можешь его увидеть?",
        "Что можно держать в руке, но нельзя потрогать?",
        "Что бывает раз в минуту, два раза в момент, но никогда в час?",
        "Что не имеет веса, но может поднять человека?",
        "Что можно найти в конце дороги?",
        "Что имеет зубы, но не может кусать?",
        "Что можно слышать, но никогда не увидеть?",
        "Что растёт вверх ногами?",
        "Что можно разбить, не касаясь его?",
        "Что имеет голову и хвост, но не имеет тела?",
        "Что можно увидеть в середине марта?",
        "Что всегда идёт, но никогда не приходит?",
        "Что имеет много иголок, но не шьёт?",
        "Что может быть мокрым даже в самый сухой день?",
        "Что не имеет рта, но может говорить?",
        "Что имеет шесть ног, но ходит только на четырёх?",
        "Что можно купить, но нельзя носить?",
        "Что имеет крылья, но не может летать?",
        "Что можно найти в самом центре земли?",
        "Что может быть сладким, но не является едой?",
        "Что имеет дно, но не является ямой?",
        "Что можно потерять, но никогда не найти?",
        "Что имеет корни, но не растёт?",
        "Что может плакать, но не имеет глаз?",
        "Что можно согнуть, но нельзя сломать?"
    };
    
    const char* answers[RIDDLES_COUNT] = {
        "сон", "время", "компьютер", "яма", "слово",
        "будущее", "мысль", "буква м", "любовь", "тень",
        "расчёска", "звук", "сосулька", "сердце", "монета",
        "буква р", "время", "ёж", "язык", "эхо",
        "лошадь", "имя", "страус", "буква м", "сон",
        "стакан", "время", "зуб", "облако", "палец"
    };
    
    int idx = rand() % RIDDLES_COUNT;
    g.chest.puzzle = riddles[idx];
    g.chest.answer = answers[idx];
}

// ============= ИНИЦИАЛИЗАЦИЯ БОССА =============
void initBoss(Game &g) {
    g.boss.x = W - 3;
    g.boss.y = H - 3;
    g.boss.maxHp = 20 + g.level * 15;
    g.boss.hp = g.boss.maxHp;
    g.boss.alive = 1;
    g.boss.damage = 3 + g.level * 3;
    g.boss.attackCooldown = 0;
    g.boss.attackTimer = 0;
    g.boss.attackChance = 60 + g.level * 2;
    g.boss.critChance = 10 + g.level;
    g.boss.armor = g.level * 1;
    
    const char* bossNames[] = {
        "Гоблин", "Волк", "Скелет", 
        "Огненный элементаль", "Каменный голем",
        "Теневой убийца", "Молодой дракон", "Ледяной великан",
        "Король вампиров", "Демон", "Механический голем",
        "Призрачный рыцарь", "Повелитель стихий", 
        "Кошмар из глубин", "Тёмный бог"
    };
    int idx = min(g.level - 1, 14);
    g.boss.name = bossNames[idx];
}

// ============= ОТРИСОВКА =============
void render(Game &g) {
    system("cls");
    
    cout << "========================================\n";
    cout << "🏰 УРОВЕНЬ: " << g.level 
         << " | ПЕРСОНАЖ: " << g.player.level
         << " | ОПЫТ: " << g.player.xp << "/" << g.player.xpToNext << "\n";
    
    cout << "❤️ HP: " << g.player.hp << "/" << g.player.maxHp
         << " | ⚔️ УРОН: " << g.player.damage
         << " | 🛡️ БРОНЯ: " << g.player.armor
         << " | 💰 МОНЕТ: " << g.player.coins << "\n";
    
    if (g.boss.alive) {
        cout << "👹 БОСС: " << g.boss.name 
             << " | HP: " << g.boss.hp << "/" << g.boss.maxHp
             << " | 🛡️ БРОНЯ: " << g.boss.armor << "\n";
    } else {
        cout << "👹 БОСС: ПОВЕРЖЕН! 🎉\n";
    }
    
    cout << "🧪 ЗЕЛЬЯ: " << g.player.potions 
         << " | ⚡ КРИТ: " << g.player.critChance << "%"
         << " | 💨 УВОРОТ: " << g.player.dodgeChance << "%\n";
    cout << "========================================\n";
    
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (x == g.player.x && y == g.player.y) {
                cout << "\033[33m@\033[0m";
            } else if (g.boss.alive && x == g.boss.x && y == g.boss.y) {
                cout << "\033[31mB\033[0m";
            } else if (!g.chest.opened && x == g.chest.x && y == g.chest.y) {
                cout << "\033[33mC\033[0m";
            } else if (g.map[y][x] == '#') {
                cout << "\033[34m#\033[0m";
            } else {
                cout << g.map[y][x];
            }
        }
        cout << "\n";
    }
    
    cout << "\n🎮 WASD - ДВИЖЕНИЕ | SPACE - АТАКА | E - СУНДУК | H - ЛЕЧЕНИЕ | Q - ВЫХОД\n";
    cout << "🎵 MUSIC: " << (g.musicEnabled ? "ON" : "OFF") << " | M - TOGGLE MUSIC\n";
}

// ============= МАГАЗИН =============
void shop(Game &g) {
    bool inShop = true;
    
    while (inShop) {
        system("cls");
        cout << "\033[36m";
        cout << "  ╔═══════════════════════════════════════════╗\n";
        cout << "  ║              🏪 МАГАЗИН                   ║\n";
        cout << "  ╠═══════════════════════════════════════════╣\n";
        cout << "  ║                                           ║\n";
        cout << "  ║  💰 МОНЕТ: " << g.player.coins;
        for (int i = 0; i < 25 - to_string(g.player.coins).length(); i++) cout << " ";
        cout << "║\n";
        cout << "  ║                                           ║\n";
        cout << "  ║  [1] +10 HP            = 10 монет        ║\n";
        cout << "  ║  [2] +2 УРОНА          = 15 монет        ║\n";
        cout << "  ║  [3] +1 ЗЕЛЬЕ          = 8 монет         ║\n";
        cout << "  ║  [4] +5% КРИТА         = 20 монет        ║\n";
        cout << "  ║  [5] +2 БРОНИ          = 18 монет        ║\n";
        cout << "  ║  [6] +5% УВОРОТА       = 25 монет        ║\n";
        cout << "  ║  [7] ВЫЙТИ ИЗ МАГАЗИНА                   ║\n";
        cout << "  ║                                           ║\n";
        cout << "  ╚═══════════════════════════════════════════╝\n";
        cout << "\033[0m\n";
        cout << "  Выберите действие: ";
        fflush(stdout);
        
        char choice = 0;
        while (choice == 0) {
            choice = getKey();
            Sleep(10);
        }
        
        switch(choice) {
            case '1': {
                if (g.player.coins >= 10) {
                    g.player.coins -= 10;
                    g.player.maxHp += 10;
                    g.player.hp += 10;
                    cout << "\n✅ +10 HP! (Макс: " << g.player.maxHp << ")\n";
                    waitKey();
                } else {
                    cout << "\n❌ Недостаточно монет! Нужно: 10\n";
                    waitKey();
                }
                break;
            }
            case '2': {
                if (g.player.coins >= 15) {
                    g.player.coins -= 15;
                    g.player.damage += 2;
                    cout << "\n✅ +2 урона! (Теперь: " << g.player.damage << ")\n";
                    waitKey();
                } else {
                    cout << "\n❌ Недостаточно монет! Нужно: 15\n";
                    waitKey();
                }
                break;
            }
            case '3': {
                if (g.player.coins >= 8) {
                    g.player.coins -= 8;
                    g.player.potions++;
                    cout << "\n✅ +1 зелье! (Теперь: " << g.player.potions << ")\n";
                    waitKey();
                } else {
                    cout << "\n❌ Недостаточно монет! Нужно: 8\n";
                    waitKey();
                }
                break;
            }
            case '4': {
                if (g.player.coins >= 20) {
                    g.player.coins -= 20;
                    g.player.critChance += 5;
                    cout << "\n✅ +5% крита! (Теперь: " << g.player.critChance << "%)\n";
                    waitKey();
                } else {
                    cout << "\n❌ Недостаточно монет! Нужно: 20\n";
                    waitKey();
                }
                break;
            }
            case '5': {
                if (g.player.coins >= 18) {
                    g.player.coins -= 18;
                    g.player.armor += 2;
                    cout << "\n✅ +2 брони! (Теперь: " << g.player.armor << ")\n";
                    waitKey();
                } else {
                    cout << "\n❌ Недостаточно монет! Нужно: 18\n";
                    waitKey();
                }
                break;
            }
            case '6': {
                if (g.player.coins >= 25) {
                    g.player.coins -= 25;
                    g.player.dodgeChance += 5;
                    cout << "\n✅ +5% уворота! (Теперь: " << g.player.dodgeChance << "%)\n";
                    waitKey();
                } else {
                    cout << "\n❌ Недостаточно монет! Нужно: 25\n";
                    waitKey();
                }
                break;
            }
            case '7': {
                inShop = false;
                cout << "\n🚪 Выход из магазина...\n";
                waitKey();
                break;
            }
            default: {
                cout << "\n❌ Неизвестная команда!\n";
                waitKey();
                break;
            }
        }
    }
}

// ============= ПРОВЕРКА СТОЛКНОВЕНИЙ =============
bool canMove(Game &g, int nx, int ny) {
    if (nx < 0 || ny < 0 || nx >= W || ny >= H) return false;
    if (g.map[ny][nx] == '#') return false;
    if (g.boss.alive && nx == g.boss.x && ny == g.boss.y) return false;
    return true;
}

// ============= ЗАГАДКА ДЛЯ СУНДУКА =============
bool solvePuzzle(Game &g) {
    if (g.chest.solved) return true;
    
    cout << "\n=== 🧩 ЗАГАДКА СУНДУКА ===\n";
    cout << g.chest.puzzle << "\n";
    cout << "Ваш ответ: ";
    fflush(stdout);
    
    string input;
    cin >> input;  // В Windows используем обычный ввод
    
    transform(input.begin(), input.end(), input.begin(), ::tolower);
    
    if (input == g.chest.answer) {
        cout << "✅ Правильно! Сундук открыт!\n";
        g.chest.solved = 1;
        waitKey();
        return true;
    } else {
        cout << "❌ Неправильно! Сундук заперт.\n";
        waitKey();
        return false;
    }
}

// ============= ОТКРЫТИЕ СУНДУКА =============
void openChest(Game &g) {
    if (g.chest.opened) {
        cout << "❌ Сундук уже открыт!\n";
        waitKey();
        return;
    }
    
    if (abs(g.player.x - g.chest.x) > 1 || abs(g.player.y - g.chest.y) > 1) {
        cout << "❌ Вы слишком далеко от сундука!\n";
        waitKey();
        return;
    }
    
    cout << "🎁 Вы нашли сундук!\n";
    
    if (solvePuzzle(g)) {
        g.chest.opened = 1;
        
        int reward = rand() % 5;
        switch(reward) {
            case 0:
                int heal;
                heal = 10 + rand() % 15;
                g.player.hp = min(g.player.maxHp, g.player.hp + heal);
                cout << "💚 +" << heal << " HP!\n";
                break;
            case 1:
                g.player.potions++;
                cout << "🧪 +1 Зелье здоровья!\n";
                break;
            case 2:
                int coins;
                coins = 15 + rand() % 25;
                g.player.coins += coins;
                cout << "💰 +" << coins << " монет!\n";
                break;
            case 3:
                g.player.damage += 2;
                cout << "⚔️ Урон увеличен на 2! (Теперь: " << g.player.damage << ")\n";
                break;
            case 4:
                g.player.dodgeChance += 3;
                cout << "💨 Шанс уворота увеличен на 3%! (Теперь: " << g.player.dodgeChance << "%)\n";
                break;
        }
        waitKey();
    }
}

// ============= АТАКА =============
void attack(Game &g) {
    if (!g.boss.alive) {
        cout << "❌ Босс уже повержен!\n";
        waitKey();
        return;
    }
    
    int dist = abs(g.player.x - g.boss.x) + abs(g.player.y - g.boss.y);
    if (dist > 1) {
        cout << "❌ Вы слишком далеко от босса!\n";
        waitKey();
        return;
    }
    
    int damage = g.player.damage;
    int armorReduction = g.boss.armor / 2;
    
    if (rand() % 100 < g.player.critChance) {
        damage *= 2;
        cout << "💥 КРИТИЧЕСКИЙ УДАР!\n";
    }
    
    damage = max(1, damage - armorReduction);
    
    g.boss.hp -= damage;
    cout << "⚔️ Вы нанесли " << damage << " урона боссу!\n";
    g.turnCounter++;
    waitKey();
    
    if (g.boss.hp <= 0) {
        g.boss.alive = 0;
        g.boss.hp = 0;
        
        int xpGain = 15 + g.level * 5;
        int coinGain = 20 + g.level * 5;
        g.player.xp += xpGain;
        g.player.coins += coinGain;
        
        cout << "\n🎉 БОСС ПОВЕРЖЕН!\n";
        cout << "✨ +" << xpGain << " опыта\n";
        cout << "💰 +" << coinGain << " монет\n";
        
        levelUp(g);
        
        cout << "\n🛒 Открывается магазин...\n";
        waitKey();
        
        shop(g);
        
        if (g.running) {
            nextLevel(g);
        }
        return;
    }
    
    if (rand() % 100 < g.boss.attackChance) {
        bossAI(g);
    } else {
        cout << "👹 " << g.boss.name << " промахнулся!\n";
        waitKey();
    }
}

// ============= ЛЕЧЕНИЕ =============
void heal(Game &g) {
    if (g.player.potions <= 0) {
        cout << "❌ Нет зелий!\n";
        waitKey();
        return;
    }
    
    g.player.potions--;
    int healAmount = 15 + rand() % 20;
    g.player.hp = min(g.player.maxHp, g.player.hp + healAmount);
    cout << "💚 Вы выпили зелье! +" << healAmount << " HP (Текущее: " << g.player.hp << ")\n";
    waitKey();
}

// ============= ИИ БОССА =============
void bossAI(Game &g) {
    if (!g.boss.alive) return;
    if (g.boss.attackCooldown > 0) {
        g.boss.attackCooldown--;
        return;
    }
    
    int dist = abs(g.boss.x - g.player.x) + abs(g.boss.y - g.player.y);
    
    if (dist <= 1) {
        int damage = g.boss.damage;
        
        if (rand() % 100 < g.boss.critChance) {
            damage *= 2;
            cout << "💀 БОСС НАНЁС КРИТИЧЕСКИЙ УДАР!\n";
        }
        
        if (rand() % 100 < g.player.dodgeChance) {
            cout << "💨 Вы увернулись от атаки!\n";
            g.boss.attackCooldown = 1;
            waitKey();
            return;
        }
        
        damage = max(1, damage - g.player.armor);
        
        g.player.hp -= damage;
        cout << "💥 " << g.boss.name << " атакует! -" << damage << " HP (Осталось: " << g.player.hp << ")\n";
        g.boss.attackCooldown = 2;
        waitKey();
        return;
    }
    
    if (rand() % 100 < 70) {
        int dx = 0, dy = 0;
        if (g.boss.x < g.player.x) dx = 1;
        else if (g.boss.x > g.player.x) dx = -1;
        
        if (g.boss.y < g.player.y) dy = 1;
        else if (g.boss.y > g.player.y) dy = -1;
        
        if (rand() % 100 < 30) {
            if (rand() % 2 == 0) dx = 0;
            else dy = 0;
        }
        
        int nx = g.boss.x + dx;
        int ny = g.boss.y + dy;
        
        if (canMove(g, nx, ny) && !(nx == g.player.x && ny == g.player.y)) {
            g.boss.x = nx;
            g.boss.y = ny;
        }
    }
}

// ============= ПОВЫШЕНИЕ УРОВНЯ =============
void levelUp(Game &g) {
    while (g.player.xp >= g.player.xpToNext) {
        g.player.xp -= g.player.xpToNext;
        g.player.level++;
        g.player.xpToNext += 15;
        
        cout << "\n✨ УРОВЕНЬ ПОВЫШЕН! Теперь вы " << g.player.level << " уровень!\n";
        cout << "Выберите улучшение:\n";
        cout << "1) +10 HP\n";
        cout << "2) +2 УРОНА\n";
        cout << "3) +1 ЗЕЛЬЕ\n";
        cout << "4) +5% КРИТА\n";
        cout << "5) +3% УВОРОТА\n";
        cout << "Ваш выбор: ";
        fflush(stdout);
        
        char choice = getKey();
        while (choice < '1' || choice > '5') {
            Sleep(100);
            choice = getKey();
        }
        
        switch(choice) {
            case '1':
                g.player.maxHp += 10;
                g.player.hp += 10;
                cout << "❤️ +10 HP! (Макс: " << g.player.maxHp << ")\n";
                break;
            case '2':
                g.player.damage += 2;
                cout << "⚔️ +2 урона! (Теперь: " << g.player.damage << ")\n";
                break;
            case '3':
                g.player.potions++;
                cout << "🧪 +1 зелье!\n";
                break;
            case '4':
                g.player.critChance += 5;
                cout << "⚡ +5% крита! (Теперь: " << g.player.critChance << "%)\n";
                break;
            case '5':
                g.player.dodgeChance += 3;
                cout << "💨 +3% уворота! (Теперь: " << g.player.dodgeChance << "%)\n";
                break;
        }
        waitKey();
    }
}

// ============= СЛЕДУЮЩИЙ УРОВЕНЬ =============
void nextLevel(Game &g) {
    cout << "\n🎉 УРОВЕНЬ " << g.level << " ПРОЙДЕН!\n";
    waitKey();
    
    g.level++;
    if (g.level > MAX_LEVEL) {
        cout << "\n🏆 ПОЗДРАВЛЯЮ! ВЫ ПРОШЛИ ВСЮ ИГРУ! 🏆\n";
        cout << "Финальная статистика:\n";
        cout << "❤️ HP: " << g.player.hp << "/" << g.player.maxHp << "\n";
        cout << "⚔️ УРОН: " << g.player.damage << "\n";
        cout << "🛡️ БРОНЯ: " << g.player.armor << "\n";
        cout << "💰 МОНЕТ: " << g.player.coins << "\n";
        cout << "💨 УВОРОТ: " << g.player.dodgeChance << "%\n";
        cout << "⚡ КРИТ: " << g.player.critChance << "%\n";
        waitKey();
        g.running = 0;
        return;
    }
    
    g.player.hp = min(g.player.hp + 30, g.player.maxHp);
    generateMap(g);
    initBoss(g);
    
    // Воспроизводим музыку для нового уровня
    if (g.musicEnabled) {
        char filename[256];
        snprintf(filename, sizeof(filename), "music/level%d.mp3", g.level);
        playMusic(filename);
    }
    
    cout << "🚀 ПЕРЕХОД НА " << g.level << " УРОВЕНЬ...\n";
    cout << "👹 Новый босс: " << g.boss.name << "!\n";
    waitKey();
}

// ============= ДВИЖЕНИЕ =============
void movePlayer(Game &g, int dx, int dy) {
    int nx = g.player.x + dx;
    int ny = g.player.y + dy;
    
    if (canMove(g, nx, ny)) {
        g.player.x = nx;
        g.player.y = ny;
        g.turnCounter++;
    }
}

// ============= ОБНОВЛЕНИЕ =============
void update(Game &g, char input) {
    if (input == 'q' || input == 'Q') {
        g.running = 0;
        return;
    }
    
    if (input == 'm' || input == 'M') {
        g.musicEnabled = !g.musicEnabled;
        if (!g.musicEnabled) {
            stopMusic();
            cout << "🔇 Музыка выключена\n";
        } else {
            char filename[256];
            snprintf(filename, sizeof(filename), "music/level%d.mp3", g.level);
            playMusic(filename);
            cout << "🔊 Музыка включена\n";
        }
        waitKey();
        return;
    }
    
    if (input == ' ') {
        attack(g);
        return;
    }
    
    if (input == 'h' || input == 'H') {
        heal(g);
        return;
    }
    
    if (input == 'e' || input == 'E') {
        openChest(g);
        return;
    }
    
    int dx = 0, dy = 0;
    if (input == 'w' || input == 'W') dy = -1;
    if (input == 's' || input == 'S') dy = 1;
    if (input == 'a' || input == 'A') dx = -1;
    if (input == 'd' || input == 'D') dx = 1;
    
    if (dx != 0 || dy != 0) {
        movePlayer(g, dx, dy);
        if (g.boss.alive && rand() % 100 < 30) {
            bossAI(g);
        }
    }
}

// ============= ГЛАВНОЕ МЕНЮ =============
void showMenu(Game &g) {
    system("cls");
    cout << "\033[36m";
    cout << "  ╔═══════════════════════════════════════════╗\n";
    cout << "  ║                                           ║\n";
    cout << "  ║   ██████╗  ██████╗ ██████╗  ██████╗      ║\n";
    cout << "  ║   ██╔══██╗██╔═══██╗██╔══██╗██╔═══██╗     ║\n";
    cout << "  ║   ██████╔╝██║   ██║██████╔╝██║   ██║     ║\n";
    cout << "  ║   ██╔═══╝ ██║   ██║██╔══██╗██║   ██║     ║\n";
    cout << "  ║   ██║     ╚██████╔╝██║  ██║╚██████╔╝     ║\n";
    cout << "  ║   ╚═╝      ╚═════╝ ╚═╝  ╚═╝ ╚═════╝      ║\n";
    cout << "  ║                                           ║\n";
    cout << "  ╚═══════════════════════════════════════════╝\n";
    cout << "\033[0m\n";
    cout << "\033[33m";
    cout << "  ╔═══════════════════════════════════════════╗\n";
    cout << "  ║            ГЛАВНОЕ МЕНЮ                  ║\n";
    cout << "  ╠═══════════════════════════════════════════╣\n";
    cout << "  ║                                           ║\n";
    cout << "  ║    [1] НАЧАТЬ ИГРУ                        ║\n";
    cout << "  ║    [2] ПОМОЩЬ                             ║\n";
    cout << "  ║    [3] МУЗЫКА: " << (g.musicEnabled ? "ВКЛ" : "ВЫКЛ") << "                  ║\n";
    cout << "  ║    [4] ВЫХОД                              ║\n";
    cout << "  ║                                           ║\n";
    cout << "  ╚═══════════════════════════════════════════╝\n";
    cout << "\033[0m\n";
    cout << "  Выберите опцию: ";
    fflush(stdout);
}

void showHelp() {
    system("cls");
    cout << "\033[36m";
    cout << "  ╔═══════════════════════════════════════════╗\n";
    cout << "  ║              ПОМОЩЬ                       ║\n";
    cout << "  ╠═══════════════════════════════════════════╣\n";
    cout << "  ║                                           ║\n";
    cout << "  ║  🎮 УПРАВЛЕНИЕ:                          ║\n";
    cout << "  ║  W/A/S/D - Движение                       ║\n";
    cout << "  ║  SPACE   - Атака босса                   ║\n";
    cout << "  ║  E       - Открыть сундук                ║\n";
    cout << "  ║  H       - Использовать зелье            ║\n";
    cout << "  ║  M       - Включить/выключить музыку     ║\n";
    cout << "  ║  Q       - Выход                          ║\n";
    cout << "  ║                                           ║\n";
    cout << "  ║  ⚔️ СИСТЕМА БОЯ:                         ║\n";
    cout << "  ║  - Босс атакует с вероятностью 60-80%    ║\n";
    cout << "  ║  - У вас есть шанс увернуться            ║\n";
    cout << "  ║  - Броня снижает получаемый урон         ║\n";
    cout << "  ║  - Критические удары удваивают урон      ║\n";
    cout << "  ║                                           ║\n";
    cout << "  ║  🏪 МАГАЗИН:                              ║\n";
    cout << "  ║  После победы над боссом открывается     ║\n";
    cout << "  ║  магазин, где можно купить улучшения     ║\n";
    cout << "  ║  за монеты!                              ║\n";
    cout << "  ║                                           ║\n";
    cout << "  ║  🎯 ЦЕЛЬ:                                 ║\n";
    cout << "  ║  Пройти " << MAX_LEVEL << " уровней и победить всех боссов!  ║\n";
    cout << "  ║                                           ║\n";
    cout << "  ║  💡 СОВЕТЫ:                               ║\n";
    cout << "  ║  - Копите монеты для магазина             ║\n";
    cout << "  ║  - Улучшайте уворот, чтобы реже получать урон ║\n";
    cout << "  ║  - Используйте зелья в критический момент ║\n";
    cout << "  ║                                           ║\n";
    cout << "  ╚═══════════════════════════════════════════╝\n";
    cout << "\033[0m\n";
    cout << "  Нажмите любую клавишу для возврата...";
    fflush(stdout);
    waitKey();
}

// ============= ГЛАВНАЯ ИГРА =============
void startGame(Game &game) {
    game.running = 1;
    game.level = 1;
    game.turnCounter = 0;
    game.musicEnabled = 1;
    
    game.player.x = 2;
    game.player.y = 2;
    game.player.hp = 40;
    game.player.maxHp = 40;
    game.player.damage = 5;
    game.player.potions = 3;
    game.player.coins = 0;
    game.player.xp = 0;
    game.player.xpToNext = 20;
    game.player.level = 1;
    game.player.critChance = 10;
    game.player.armor = 0;
    game.player.dodgeChance = 5;
    
    generateMap(game);
    initBoss(game);
    
    // Запускаем музыку для первого уровня
    if (game.musicEnabled) {
        char filename[256];
        snprintf(filename, sizeof(filename), "music/level%d.mp3", game.level);
        playMusic(filename);
    }
    
    const int FPS = 30;
    const int frameDelay = 1000 / FPS;
    
    while (game.running) {
        char input = getKey();
        update(game, input);
        
        if (game.player.hp <= 0) {
            cout << "\n💀 ВЫ УМЕРЛИ! Игра окончена.\n";
            cout << "Пройдено уровней: " << game.level - 1 << "\n";
            cout << "Всего ходов: " << game.turnCounter << "\n";
            cout << "Нажмите любую клавишу для возврата...";
            waitKey();
            game.running = 0;
            stopMusic();
            break;
        }
        
        render(game);
        Sleep(frameDelay);
    }
}

// ============= MAIN =============
int main() {
    srand(time(NULL));
    
    Game game;
    game.musicEnabled = 1;
    
    // Воспроизводим музыку меню
    if (game.musicEnabled) {
        playMusic("music/menu.mp3");
    }
    
    while (true) {
        showMenu(game);
        char choice = getKey();
        
        switch(choice) {
            case '1': {
                stopMusic();
                startGame(game);
                if (game.musicEnabled) {
                    playMusic("music/menu.mp3");
                }
                break;
            }
            case '2':
                showHelp();
                break;
            case '3': {
                game.musicEnabled = !game.musicEnabled;
                if (!game.musicEnabled) {
                    stopMusic();
                } else {
                    playMusic("music/menu.mp3");
                }
                break;
            }
            case '4': {
                cout << "\n👋 До свидания!\n";
                stopMusic();
                return 0;
            }
        }
    }
    
    return 0;
}