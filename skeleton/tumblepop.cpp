
#include <iostream>
#include <fstream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <ctime>
#include <cstdlib>

using namespace sf;
using namespace std;

int screen_x = 1136;
int screen_y = 896;

// Enemy types
const int ENEMY_GHOST = 0;
const int ENEMY_SKELETON = 1;
const int ENEMY_INVISIBLE = 2;
const int ENEMY_CHELNOV = 3;

void display_level(RenderWindow& window, char** lvl, Texture& bgTex, Sprite& bgSprite, 
                   Texture& blockTexture, Sprite& blockSprite, const int height, 
                   const int width, const int cell_size)
{
    window.draw(bgSprite);
    
    for (int i = 0; i < height; i += 1)
    {
        for (int j = 0; j < width; j += 1)
        {
            if (lvl[i][j] == '#')
            {
                blockSprite.setPosition(j * cell_size, i * cell_size);
                window.draw(blockSprite);
            }
        }
    }
}

void handle_horizontal_collision(char** lvl, float& player_x, float velocityX, 
                                 float player_y, int Pheight, int Pwidth, 
                                 int cell_size) 
{
    if (velocityX == 0) return;
    
    float offset_x = player_x + velocityX;
    int playerRow = (int)(player_y + Pheight / 2) / cell_size;
    
    if (velocityX > 0) {
        int checkCol = (int)(offset_x + Pwidth) / cell_size;
        char rightCell = lvl[playerRow][checkCol];
        
        if (rightCell != '#') {
            player_x = offset_x;
        }
    }
    else if (velocityX < 0) {
        int checkCol = (int)(offset_x) / cell_size;
        char leftCell = lvl[playerRow][checkCol];
        
        if (leftCell != '#') {
            player_x = offset_x;
        }
    }
}

void player_gravity(char** lvl, float& offset_y, float& velocityY, bool& onGround, 
                   const float& gravity, float& terminal_Velocity, float& player_x, 
                   float& player_y, const int cell_size, int& Pheight, int& Pwidth)
{
    offset_y = player_y;
    offset_y += velocityY;
    
    char bottom_left_down = lvl[(int)(offset_y + Pheight) / cell_size][(int)(player_x) / cell_size];
    char bottom_right_down = lvl[(int)(offset_y + Pheight) / cell_size][(int)(player_x + Pwidth) / cell_size];
    char bottom_mid_down = lvl[(int)(offset_y + Pheight) / cell_size][(int)(player_x + Pwidth / 2) / cell_size];
    
    if (bottom_left_down == '#' || bottom_mid_down == '#' || bottom_right_down == '#')
    {
        onGround = true;
        velocityY = 0;
        
        int brickRow = (int)(offset_y + Pheight) / cell_size;
        player_y = (brickRow * cell_size) - Pheight;
    }
    else
    {
        onGround = false;
        player_y = offset_y;
        
        velocityY += gravity;
        if (velocityY >= terminal_Velocity) 
            velocityY = terminal_Velocity;
    }
}

void check_ceiling_collision(char** lvl, float& player_y, float& velocityY, float player_x, 
                             int Pheight, int Pwidth, int cell_size, int height, int width)
{
    if (velocityY >= 0) return;
    
    float offset_y = player_y + velocityY;
    
    char top_left = lvl[(int)(offset_y) / cell_size][(int)(player_x) / cell_size];
    char top_mid = lvl[(int)(offset_y) / cell_size][(int)(player_x + Pwidth / 2) / cell_size];
    char top_right = lvl[(int)(offset_y) / cell_size][(int)(player_x + Pwidth) / cell_size];
    
    if (top_left == '#' || top_mid == '#' || top_right == '#')
    {
        int brickRow = (int)(offset_y) / cell_size;
        player_y = (brickRow + 1) * cell_size;
        velocityY = 0;
    }
}

void update_enemies(float enemy_x[], float enemy_y[], float enemy_velocityX[], 
                   float enemy_velocityY[], bool enemy_onGround[],
                   int enemy_width[], int enemy_height[], bool enemy_movingRight[], 
                   int enemy_type[], bool enemy_visible[], float enemy_timer[],
                   int enemyCount, char** lvl, int cell_size, int width, Clock& gameClock)
{
    const float enemy_gravity = 0.8f;
    const float enemy_terminal_velocity = 15.0f;
    
    for (int i = 0; i < enemyCount; i++)
    {
        if (enemy_x[i] < -500) continue;
        
        // Apply gravity
        enemy_velocityY[i] += enemy_gravity;
        if (enemy_velocityY[i] > enemy_terminal_velocity) {
            enemy_velocityY[i] = enemy_terminal_velocity;
        }
        
        // Update vertical position
        float new_y = enemy_y[i] + enemy_velocityY[i];
        
        // Check ground collision - check bottom of enemy
        int cellX = (int)(enemy_x[i] + enemy_width[i] / 2) / cell_size;
        int cellY_bottom = (int)(new_y + enemy_height[i]) / cell_size;
        
        enemy_onGround[i] = false;
        
        if (cellY_bottom >= 0 && cellY_bottom < 14 && cellX >= 0 && cellX < width) {
            if (lvl[cellY_bottom][cellX] == '#') {
                // Hit ground - snap to platform
                enemy_onGround[i] = true;
                enemy_velocityY[i] = 0;
                enemy_y[i] = (cellY_bottom * cell_size) - enemy_height[i];
            } else {
                // In air
                enemy_y[i] = new_y;
            }
        } else {
            enemy_y[i] = new_y;
        }
        
        // Horizontal movement
        enemy_x[i] += enemy_velocityX[i];
        
        cellX = (int)(enemy_x[i] + enemy_width[i] / 2) / cell_size;
        int cellY = (int)(enemy_y[i] + enemy_height[i] / 2) / cell_size;
        
        if (cellX < 0 || cellX >= width || cellY < 0 || cellY >= 14) continue;
        
        // Invisible Man - random teleportation and invisibility
        if (enemy_type[i] == ENEMY_INVISIBLE) {
            enemy_timer[i] += 0.016f;
            
            if (enemy_timer[i] > 3.0f) {
                enemy_visible[i] = !enemy_visible[i];
                enemy_timer[i] = 0;
                
                if (!enemy_visible[i]) {
                    enemy_x[i] = 100 + rand() % 900;
                    enemy_y[i] = 100;
                    enemy_velocityY[i] = 0;
                }
            }
        }
        
        // Skeleton - can jump between platforms
        if (enemy_type[i] == ENEMY_SKELETON) {
            enemy_timer[i] += 0.016f;
            
            if (enemy_timer[i] > 2.0f && rand() % 100 < 3 && enemy_onGround[i]) {
                enemy_velocityY[i] = -12.0f; // Jump
                enemy_onGround[i] = false;
                enemy_timer[i] = 0;
            }
        }
        
        // Wall collision
        if (enemy_movingRight[i])
        {
            int rightCell = (int)(enemy_x[i] + enemy_width[i]) / cell_size;
            if (rightCell >= width - 1 || (rightCell >= 0 && rightCell < width && lvl[cellY][rightCell] == '#'))
            {
                enemy_movingRight[i] = false;
                enemy_velocityX[i] = -abs(enemy_velocityX[i]);
            }
        }
        else
        {
            int leftCell = (int)(enemy_x[i]) / cell_size;
            if (leftCell <= 0 || (leftCell >= 0 && leftCell < width && lvl[cellY][leftCell] == '#'))
            {
                enemy_movingRight[i] = true;
                enemy_velocityX[i] = abs(enemy_velocityX[i]);
            }
        }
        
        // Random direction change for Ghost
        if (enemy_type[i] == ENEMY_GHOST && rand() % 100 < 1)
        {
            enemy_movingRight[i] = !enemy_movingRight[i];
            enemy_velocityX[i] = -enemy_velocityX[i];
        }
    }
}

bool check_player_enemy_collision(float playerX, float playerY, int playerW, int playerH, 
                                  float enemy_x[], float enemy_y[], int enemy_width[], 
                                  int enemy_height[], bool enemy_visible[], int enemyCount)
{
    for (int i = 0; i < enemyCount; i++)
    {
        if (enemy_x[i] < -500 || enemy_x[i] > 2000 || !enemy_visible[i]) continue;
        
        if (playerX < enemy_x[i] + enemy_width[i] &&
            playerX + playerW > enemy_x[i] &&
            playerY < enemy_y[i] + enemy_height[i] &&
            playerY + playerH > enemy_y[i])
        {
            return true;
        }
    }
    return false;
}

void try_capture_enemy(float enemy_x[], float enemy_y[], int enemy_width[], 
                      int enemy_height[], int enemy_type[], bool enemy_visible[],
                      int enemyCount, float playerX, float playerY, 
                      int playerW, int playerH, int& capturedEnemyIndex, 
                      bool& capturedEnemyActive, float& capturedEnemyTime, 
                      float gunRange, bool facingRight, int& score, int& comboStreak,
                      int cell_size)
{
    if (capturedEnemyActive) return;
    
    // Calculate which row the player is in (based on center of player)
    int playerRow = (int)((playerY + playerH / 2) / cell_size);
    
    for (int i = 0; i < enemyCount; i++)
    {
        if (enemy_x[i] < -500 || !enemy_visible[i]) continue;
        
        // Calculate which row the enemy is in (based on center of enemy)
        int enemyRow = (int)((enemy_y[i] + enemy_height[i] / 2) / cell_size);
        
        // Check if enemy is in the SAME ROW as player
        if (enemyRow != playerRow) continue;
        
        // Check horizontal distance
        float distX = enemy_x[i] - playerX;
        
        // Check if enemy is in the direction player is facing
        bool inDirection = facingRight ? (distX > 0 && distX <= gunRange) 
                                        : (distX < 0 && distX >= -gunRange);
        
        if (inDirection)
        {
            // Check if enemy is within gun range horizontally
            float horizontalDist = abs(distX);
            
            if (horizontalDist <= gunRange)
            {
                // Add capture points
                if (enemy_type[i] == ENEMY_GHOST) score += 50;
                else if (enemy_type[i] == ENEMY_SKELETON) score += 75;
                else if (enemy_type[i] == ENEMY_INVISIBLE) score += 150;
                else if (enemy_type[i] == ENEMY_CHELNOV) score += 200;
                
                comboStreak++;
                
                capturedEnemyIndex = i;
                capturedEnemyActive = true;
                capturedEnemyTime = 0;
                
                enemy_x[i] = -1000;
                enemy_y[i] = -1000;
                break;
            }
        }
    }
}

bool loadLevelFromFile(const string& filename, char** lvl, int height, int width)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "ERROR: Cannot open level file: " << filename << endl;
        return false;
    }
    
    string line;
    int row = 0;
    
    while (getline(file, line) && row < height)
    {
        if (line.length() < width)
        {
            cout << "ERROR: Line " << row << " too short!" << endl;
            return false;
        }
        
        for (int col = 0; col < width; col++)
        {
            lvl[row][col] = line[col];
        }
        
        row++;
    }
    
    if (row < height)
    {
        cout << "ERROR: Not enough rows in map file!" << endl;
        return false;
    }
    
    file.close();
    return true;
}


void update_projectiles(float proj_x[], float proj_y[], float proj_velocityX[], 
                       float proj_velocityY[], bool proj_active[], int proj_width[], 
                       int proj_height[], int maxProjectiles, float enemy_x[], 
                       float enemy_y[], int enemy_width[], int enemy_height[], 
                       int enemy_type[], bool enemy_visible[], int enemyCount, 
                       char** lvl, int cell_size, int& score, int& comboStreak)
{
    for (int i = 0; i < maxProjectiles; i++)
    {
        if (!proj_active[i]) continue;
        
        proj_x[i] += proj_velocityX[i];
        proj_y[i] += proj_velocityY[i];
        
        int cellX = (int)(proj_x[i]) / cell_size;
        int cellY = (int)(proj_y[i]) / cell_size;
        
        if (cellX < 0 || cellX >= 18 || cellY < 0 || cellY >= 14) {
            proj_active[i] = false;
            continue;
        }
        
        if (lvl[cellY][cellX] == '#') {
            proj_active[i] = false;
            continue;
        }
        
        for (int j = 0; j < enemyCount; j++)
        {
            if (enemy_x[j] < -500 || !enemy_visible[j]) continue;
            
            if (proj_x[i] < enemy_x[j] + enemy_width[j] &&
                proj_x[i] + proj_width[i] > enemy_x[j] &&
                proj_y[i] < enemy_y[j] + enemy_height[j] &&
                proj_y[i] + proj_height[i] > enemy_y[j])
            {
                // Defeat by projectile - 2X points
                int basePoints = 0;
                if (enemy_type[j] == ENEMY_GHOST) basePoints = 50;
                else if (enemy_type[j] == ENEMY_SKELETON) basePoints = 75;
                else if (enemy_type[j] == ENEMY_INVISIBLE) basePoints = 150;
                else if (enemy_type[j] == ENEMY_CHELNOV) basePoints = 200;
                
                score += basePoints * 2; // 2X for projectile defeat
                comboStreak++;
                
                enemy_x[j] = -1000;
                enemy_y[j] = -1000;
                proj_active[i] = false;
                break;
            }
        }
        
        if (proj_x[i] < 0 || proj_x[i] > 1200 || 
            proj_y[i] < 0 || proj_y[i] > 900) {
            proj_active[i] = false;
        }
    }
}

void shoot_projectile(float proj_x[], float proj_y[], float proj_velocityX[], 
                     float proj_velocityY[], bool proj_active[], int proj_width[], 
                     int proj_height[], int maxProjectiles, float playerX, 
                     float playerY, int playerW, int playerH, bool facingRight, 
                     float speed)
{
    for (int i = 0; i < maxProjectiles; i++)
    {
        if (!proj_active[i])
        {
            proj_x[i] = playerX + (facingRight ? playerW : -40);
            proj_y[i] = playerY + playerH / 2;
            proj_velocityX[i] = facingRight ? speed : -speed;
            proj_velocityY[i] = 0;
            proj_active[i] = true;
            break;
        }
    }
}

int main()
{
    srand(time(0));
    
    RenderWindow window(VideoMode(screen_x, screen_y), "Tumble-POP", Style::Default);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);
    
    Font font;
    if (!font.loadFromFile("Data/ARIAL.TTF"))
    {
        cout << "ERROR: Cannot load font!" << endl;
    }
    
    // CHARACTER SELECTION MENU
    Text charOptions[2];
    string charNames[2] = {"Yellow TumblePopper - Fast Speed (1.5x)", 
                          "Green TumblePopper - Strong Vacuum (1.2x)"};
    
    for (int i = 0; i < 2; i++) {
        charOptions[i].setFont(font);
        charOptions[i].setString(charNames[i]);
        charOptions[i].setCharacterSize(35);
        charOptions[i].setPosition(200, 300 + i * 80);
    }
    
    int selectedCharIndex = 0;
    int gameState = 0; // 0 = CHAR_SELECT, 1 = PLAYING
    int selectedCharacter = -1;
    
    // Level specifics
    const int cell_size = 64;
    const int height = 14;
    const int width = 18;
    char** lvl;
    
    Texture bgTex;
    Sprite bgSprite;
    Texture blockTexture;
    Sprite blockSprite;
    
    bgTex.loadFromFile("Data/bg.png");
    bgSprite.setTexture(bgTex);
    bgSprite.setPosition(0, 0);
    
    blockTexture.loadFromFile("Data/block1.png");
    blockSprite.setTexture(blockTexture);
    
    Music lvlMusic;
    lvlMusic.openFromFile("Data/mus.ogg");
    lvlMusic.setVolume(20);
    lvlMusic.play();
    lvlMusic.setLoop(true);
    
    // Player data
    float player_x = 80;
    float player_y = 80;
    float speed = 5;
    const float jumpStrength = -20;
    const float gravity = 1;
    bool onGround = false;
    float offset_y = 0;
    float velocityY = 0;
    float terminal_Velocity = 20;
    int PlayerHeight = 64;
    int PlayerWidth = 60;
    bool facingRight = true;
    
    // Character-specific attributes (updated per PDF)
    float char1_speed = 7.5f; // 1.5x faster (5 * 1.5)
    float char1_gunRange = 150.0f;
    float char2_speed = 5.0f; // Standard
    float char2_gunRange = 180.0f; // 1.2x stronger (150 * 1.2)
    
    Texture PlayerTexture;
    Sprite PlayerSprite;
    PlayerTexture.loadFromFile("Data/player_green.png");
    PlayerSprite.setTexture(PlayerTexture);
    PlayerSprite.setScale(2, 2);
    PlayerSprite.setPosition(player_x, player_y);
    
    Texture Player2Texture;
    Sprite Player2Sprite;
    Player2Texture.loadFromFile("Data/player_yellow.png");
    Player2Sprite.setTexture(Player2Texture);
    Player2Sprite.setScale(2, 2);
    
    // Enemy data - Level 1: 8 ghosts, 4 skeletons
    const int MAX_ENEMIES = 15;
    float enemy_x[MAX_ENEMIES];
    float enemy_y[MAX_ENEMIES];
    float enemy_velocityX[MAX_ENEMIES];
    int enemy_width[MAX_ENEMIES];
    int enemy_height[MAX_ENEMIES];
    bool enemy_movingRight[MAX_ENEMIES];
    int enemy_type[MAX_ENEMIES];
    bool enemy_visible[MAX_ENEMIES];
    float enemy_timer[MAX_ENEMIES];
    int enemyCount = 12; // 8 ghosts + 4 skeletons

    float enemy_velocityY[MAX_ENEMIES];
bool enemy_onGround[MAX_ENEMIES];


    // Enemy spawn tracking
int enemiesSpawned = 0;
float spawnDelay = 4.0f; // 4 seconds between each spawn
float initialDelay = 10.0f; // 10 seconds before first delayed spawn
Clock spawnClock;
bool spawnTimerStarted = false;


// Safe spawn positions - DECLARED HERE so they're available everywhere
int ghostSpawnX[8] = {512, 832, 704, 896, 192, 448, 320, 384};
int ghostSpawnY[8] = {128, 128, 384, 384, 512, 640, 640, 768};
int skeletonSpawnX[4] = {448, 576, 384, 768};
int skeletonSpawnY[4] = {320, 320, 576, 576};

// Initialize all enemies to off-screen initially
for (int i = 0; i < MAX_ENEMIES; i++) {
    enemy_x[i] = -1000;
    enemy_y[i] = -1000;
    enemy_visible[i] = false;
    enemy_velocityX[i] = 0;
    enemy_width[i] = 50;
    enemy_height[i] = 50;
    enemy_movingRight[i] = false;
    enemy_type[i] = ENEMY_GHOST;
    enemy_timer[i] = 0;

    enemy_velocityY[i] = 0;
    enemy_onGround[i] = false;
}
    
    // Enemy textures
    Texture ghostTexture, skeletonTexture, invisibleTexture, chelnovTexture;
    Sprite ghostSprite, skeletonSprite, invisibleSprite, chelnovSprite;
    
    ghostTexture.loadFromFile("Data/ghost.png");
    skeletonTexture.loadFromFile("Data/skeleton.png"); 
    invisibleTexture.loadFromFile("Data/ghost.png");
    chelnovTexture.loadFromFile("Data/ghost.png");
    
    ghostSprite.setTexture(ghostTexture);
    skeletonSprite.setTexture(skeletonTexture);
    invisibleSprite.setTexture(invisibleTexture);
    chelnovSprite.setTexture(chelnovTexture);
    
    ghostSprite.setScale(2, 2);
    skeletonSprite.setScale(2, 2);
    invisibleSprite.setScale(2, 2);
    chelnovSprite.setScale(2, 2);
    
    // Game stats
    int score = 0;
    int health = 3;
    int comboStreak = 0;
    Clock comboTimer;
    Clock levelTimer;
    bool levelComplete = false;
    
    // Gun system
    bool gunActive = false;
    float gunRange = 150.0f;
    int capturedEnemyIndex = -1;
    float capturedEnemyTime = 0;
    bool capturedEnemyActive = false;
    const float MAX_CAPTURE_TIME = 5.0f;
    Clock captureClock;
    Clock gameClock;
    
    // Projectiles
    const int MAX_PROJECTILES = 10;
    float proj_x[MAX_PROJECTILES];
    float proj_y[MAX_PROJECTILES];
    float proj_velocityX[MAX_PROJECTILES];
    float proj_velocityY[MAX_PROJECTILES];
    bool proj_active[MAX_PROJECTILES];
    int proj_width[MAX_PROJECTILES];
    int proj_height[MAX_PROJECTILES];
    float projectileSpeed = 10.0f;
    
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        proj_x[i] = 0;
        proj_y[i] = 0;
        proj_velocityX[i] = 0;
        proj_velocityY[i] = 0;
        proj_active[i] = false;
        proj_width[i] = 40;
        proj_height[i] = 40;
    }
    
    Texture projectileTexture;
    Sprite projectileSprite;
    projectileTexture.loadFromFile("Data/projectile1.png");
    projectileSprite.setTexture(projectileTexture);
    projectileSprite.setScale(1.5f, 1.5f);

    // Beam texture
Texture beamTexture;
Sprite beamSprite;
beamTexture.loadFromFile("Data/beam.png");
beamSprite.setTexture(beamTexture);
    
    // Create level array
    lvl = new char*[height];
    for (int i = 0; i < height; i += 1)
    {
        lvl[i] = new char[width];
    }
    
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            lvl[i][j] = '.';
        }
    }
    
    loadLevelFromFile("Data/level1.txt", lvl, height, width);

// Initialize enemies for Level 1
// 8 Ghosts - First 4 spawn immediately, rest spawn after delays
for (int i = 0; i < 8; i++) {
    if (i < 4) {
        // First 4 ghosts - spawn immediately in safe positions
        enemy_x[i] = ghostSpawnX[i]; 
        enemy_y[i] = ghostSpawnY[i];
        enemy_visible[i] = true;
    } else {
        // Rest spawn later
        enemy_x[i] = -1000;
        enemy_y[i] = -1000;
        enemy_visible[i] = false;
    }
    enemy_velocityX[i] = 1.5f + (rand() % 10) / 10.0f;
    enemy_width[i] = 50;
    enemy_height[i] = 50;
    enemy_movingRight[i] = (i % 2 == 0);
    enemy_type[i] = ENEMY_GHOST;
    enemy_timer[i] = 0;
}

// 4 Skeletons - First 2 spawn immediately, rest spawn after delays
for (int i = 8; i < 12; i++) {
    int idx = i - 8;
    if (i < 10) {
        // First 2 skeletons - spawn immediately in safe positions
        enemy_x[i] = skeletonSpawnX[idx];
        enemy_y[i] = skeletonSpawnY[idx];
        enemy_visible[i] = true;
    } else {
        // Rest spawn later
        enemy_x[i] = -1000;
        enemy_y[i] = -1000;
        enemy_visible[i] = false;
    }
    enemy_velocityX[i] = 2.0f;
    enemy_width[i] = 50;
    enemy_height[i] = 50;
    enemy_movingRight[i] = (i % 2 == 0);
    enemy_type[i] = ENEMY_SKELETON;
    enemy_timer[i] = 0;
}

enemiesSpawned = 6; // 4 ghosts + 2 skeletons initially spawned


    Event ev;
    
    // Main loop
    while (window.isOpen())
    {
        while (window.pollEvent(ev))
        {
            if (ev.type == Event::Closed) 
            {
                window.close();
            }
            
            if (gameState == 0) // Character selection
            {
                if (ev.type == Event::KeyPressed)
                {
                    if (ev.key.code == Keyboard::Up) {
                        selectedCharIndex = 0;
                    }
                    else if (ev.key.code == Keyboard::Down) {
                        selectedCharIndex = 1;
                    }
                    else if (ev.key.code == Keyboard::Enter) {
                        selectedCharacter = selectedCharIndex;
                        gameState = 1;
                        
                        // Set character attributes
                        if (selectedCharacter == 0) {
                            speed = char1_speed;
                            gunRange = char1_gunRange;
                        } else {
                            speed = char2_speed;
                            gunRange = char2_gunRange;
                        }
                        
                        // Reset game
                        score = 0;
                        health = 3;
                        comboStreak = 0;
                        levelTimer.restart();
                        player_x = 80;
                        player_y = 80;
                        
// Reset spawn system
enemiesSpawned = 6;
spawnClock.restart();
spawnTimerStarted = false;

// Reinitialize all enemies to off-screen
for (int i = 0; i < MAX_ENEMIES; i++) {
    enemy_x[i] = -1000;
    enemy_y[i] = -1000;
    enemy_visible[i] = false;
}

// Respawn initial enemies (4 ghosts + 2 skeletons)
for (int i = 0; i < 8; i++) {
    if (i < 4) {
        enemy_x[i] = ghostSpawnX[i]; 
        enemy_y[i] = ghostSpawnY[i];
        enemy_visible[i] = true;
        enemy_velocityX[i] = 1.5f + (rand() % 10) / 10.0f;
        enemy_movingRight[i] = (i % 2 == 0);
        enemy_type[i] = ENEMY_GHOST;
    }
}

for (int i = 8; i < 12; i++) {
    int skelIdx = i - 8;
    if (i < 10) {
        enemy_x[i] = skeletonSpawnX[skelIdx];
        enemy_y[i] = skeletonSpawnY[skelIdx];
        enemy_visible[i] = true;
        enemy_velocityX[i] = 2.0f;
        enemy_movingRight[i] = (i % 2 == 0);
        enemy_type[i] = ENEMY_SKELETON;
    }
}

                    }
                }
            }
        }
        
        if (Keyboard::isKeyPressed(Keyboard::Escape))
        {
            window.close();
        }
        
        float velocityX = 0;
        
       if (gameState == 1) // Playing
{
    // Start spawn timer when game starts
    if (!spawnTimerStarted) {
        spawnClock.restart();
        spawnTimerStarted = true;
    }
    
    // Spawn remaining enemies after delays
    if (enemiesSpawned < enemyCount) {
        float elapsedTime = spawnClock.getElapsedTime().asSeconds();
        
        // Check if initial 10 second delay has passed
        if (elapsedTime >= initialDelay) {
            // Calculate which enemy should spawn based on time
            float timeSinceFirstSpawn = elapsedTime - initialDelay;
            int enemiesToSpawn = (int)(timeSinceFirstSpawn / spawnDelay) + 1;
            
           // Spawn enemies one by one
while (enemiesSpawned < enemyCount && enemiesSpawned < 6 + enemiesToSpawn) {
    if (enemiesSpawned < 8) {
        // Spawn ghost
        int idx = enemiesSpawned;
        enemy_x[idx] = ghostSpawnX[idx];
        enemy_y[idx] = ghostSpawnY[idx];
        enemy_visible[idx] = true;
        enemy_velocityY[idx] = 0;  // ADD THIS LINE
        enemy_onGround[idx] = false;  // ADD THIS LINE
    } else {
        // Spawn skeleton
        int idx = enemiesSpawned;
        int skelIdx = idx - 8;
        enemy_x[idx] = skeletonSpawnX[skelIdx];
        enemy_y[idx] = skeletonSpawnY[skelIdx];
        enemy_visible[idx] = true;
        enemy_velocityY[idx] = 0;  // ADD THIS LINE
        enemy_onGround[idx] = false;  // ADD THIS LINE
    }
    enemiesSpawned++;
}
        }
    }
    
            // Check if level is complete
            int aliveEnemies = 0;
            for (int i = 0; i < enemyCount; i++) {
                if (enemy_x[i] > -500) aliveEnemies++;
            }
            
            if (aliveEnemies == 0 && !levelComplete) {
                levelComplete = true;
                score += 1000; // Level clear bonus
                
                // Time bonuses
                float levelTime = levelTimer.getElapsedTime().asSeconds();
                if (levelTime < 30) score += 2000;
                else if (levelTime < 45) score += 1000;
                else if (levelTime < 60) score += 500;
                
                // No damage bonus
                if (health == 3) score += 1500;
                
                // Character bonus
                score += 500;
            }
            
            // Combo multiplier (resets after 2 seconds)
            if (comboTimer.getElapsedTime().asSeconds() > 2.0f) {
                comboStreak = 0;
            }
            
            // Input handling
            if (Keyboard::isKeyPressed(Keyboard::D))
            {
                velocityX += speed;
                if (!facingRight)
                {
                    if (selectedCharacter == 0) {
                        PlayerSprite.setScale(-2.0f, 2.0f);
                    } else {
                        Player2Sprite.setScale(-2.0f, 2.0f);
                    }
                    facingRight = true;
                }
            }
            if (Keyboard::isKeyPressed(Keyboard::A))
            {
                velocityX -= speed;
                if (facingRight)
                {
                    if (selectedCharacter == 0) {
                        PlayerSprite.setScale(2.0f, 2.0f);
                    } else {
                        Player2Sprite.setScale(2.0f, 2.0f);
                    }
                    facingRight = false;
                }
            }
            
            if (Keyboard::isKeyPressed(Keyboard::W))
            {
                if (onGround)
                {
                    velocityY = jumpStrength;
                    onGround = false;
                }
            }
            
            // Gun system
            if (Keyboard::isKeyPressed(Keyboard::Space))
            {
                if (!gunActive)
                {
                    gunActive = true;
                }
                
                if (!capturedEnemyActive)
                {
                    comboTimer.restart();
                    try_capture_enemy(enemy_x, enemy_y, enemy_width, enemy_height, 
                enemy_type, enemy_visible, enemyCount, 
                player_x, player_y, PlayerWidth, PlayerHeight, 
                capturedEnemyIndex, capturedEnemyActive, 
                capturedEnemyTime, gunRange, facingRight, 
                score, comboStreak, cell_size); 
                    
                    if (capturedEnemyActive)
                    {
                        captureClock.restart();
                    }
                }
                
                if (capturedEnemyActive)
                {
                    capturedEnemyTime = captureClock.getElapsedTime().asSeconds();
                    
                    if (capturedEnemyTime >= MAX_CAPTURE_TIME)
                    {
                        health--;
                        score -= 50;
                        
                        if (health <= 0) {
                            score -= 200;
                            gameState = 0;
                            health = 3;
                            levelComplete = false;
                        }
                        
                        capturedEnemyActive = false;
                        gunActive = false;
                    }
                }
            }
            else
            {
                if (gunActive && capturedEnemyActive)
                {
                    shoot_projectile(proj_x, proj_y, proj_velocityX, proj_velocityY, 
                                   proj_active, proj_width, proj_height, MAX_PROJECTILES, 
                                   player_x, player_y, PlayerWidth, PlayerHeight, 
                                   facingRight, projectileSpeed);
                    capturedEnemyActive = false;
                }
                gunActive = false;
            }
            
            // Update game logic
            check_ceiling_collision(lvl, player_y, velocityY, player_x, PlayerHeight, 
                                   PlayerWidth, cell_size, height, width);
            handle_horizontal_collision(lvl, player_x, velocityX, player_y, PlayerHeight, 
                                       PlayerWidth, cell_size);
            player_gravity(lvl, offset_y, velocityY, onGround, gravity, terminal_Velocity, 
                          player_x, player_y, cell_size, PlayerHeight, PlayerWidth);
            
           update_enemies(enemy_x, enemy_y, enemy_velocityX, enemy_velocityY, enemy_onGround,
              enemy_width, enemy_height, enemy_movingRight, enemy_type, 
              enemy_visible, enemy_timer, enemyCount, lvl, cell_size, width, gameClock);
              
            update_projectiles(proj_x, proj_y, proj_velocityX, proj_velocityY, proj_active, 
                             proj_width, proj_height, MAX_PROJECTILES, enemy_x, enemy_y, 
                             enemy_width, enemy_height, enemy_type, enemy_visible,
                             enemyCount, lvl, cell_size, score, comboStreak);
            
            // Check collision with enemies
            if (check_player_enemy_collision(player_x, player_y, PlayerWidth, PlayerHeight, 
                                            enemy_x, enemy_y, enemy_width, enemy_height, 
                                            enemy_visible, enemyCount))
            {
                health--;
                score -= 50;
                
                if (health <= 0) {
                    score -= 200;
                    gameState = 0;
                    health = 3;
                    levelComplete = false;
                }
                else {
                    // Respawn player
                    player_x = 80;
                    player_y = 80;
                    velocityY = 0;
                }
                
                capturedEnemyActive = false;
                gunActive = false;
            }
        }
        
        window.clear();
        
        if (gameState == 0) // Character selection menu
        {
            Text title;
            title.setFont(font);
            title.setString("SELECT YOUR CHARACTER");
            title.setCharacterSize(50);
            title.setPosition(250, 150);
            title.setFillColor(Color::White);
            window.draw(title);
            
            for (int i = 0; i < 2; i++) {
                if (i == selectedCharIndex)
                    charOptions[i].setFillColor(Color::Yellow);
                else
                    charOptions[i].setFillColor(Color::White);
                
                window.draw(charOptions[i]);
            }
            
            RectangleShape char1Border(Vector2f(100, 100));
            char1Border.setPosition(200, 300);
            char1Border.setFillColor(Color::Transparent);
            char1Border.setOutlineThickness(5);
            char1Border.setOutlineColor(Color::Yellow);
            
            RectangleShape char2Border(Vector2f(100, 100));
            char2Border.setPosition(200, 380);
            char2Border.setFillColor(Color::Transparent);
            char2Border.setOutlineThickness(5);
            char2Border.setOutlineColor(Color::Green);
            
            window.draw(char1Border);
            window.draw(char2Border);
        }
        else if (gameState == 1) // Playing
        {
            display_level(window, lvl, bgTex, bgSprite, blockTexture, blockSprite, 
                         height, width, cell_size);
            
            // Draw enemies based on type
            for (int i = 0; i < enemyCount; i++)
            {
                if (enemy_x[i] > -500 && enemy_visible[i])
                {
                    if (enemy_type[i] == ENEMY_GHOST) {
                        ghostSprite.setPosition(enemy_x[i], enemy_y[i]);
                        window.draw(ghostSprite);
                    }
                    else if (enemy_type[i] == ENEMY_SKELETON) {
                        skeletonSprite.setPosition(enemy_x[i], enemy_y[i]);
                        window.draw(skeletonSprite);
                    }
                    else if (enemy_type[i] == ENEMY_INVISIBLE) {
                        invisibleSprite.setPosition(enemy_x[i], enemy_y[i]);
                        window.draw(invisibleSprite);
                    }
                    else if (enemy_type[i] == ENEMY_CHELNOV) {
                        chelnovSprite.setPosition(enemy_x[i], enemy_y[i]);
                        window.draw(chelnovSprite);
                    }
                }
            }
            
            // Draw projectiles
            for (int i = 0; i < MAX_PROJECTILES; i++)
            {
                if (proj_active[i])
                {
                    projectileSprite.setPosition(proj_x[i], proj_y[i]);
                    window.draw(projectileSprite);
                }
            }
            
// Draw gun beam when active
if (gunActive && Keyboard::isKeyPressed(Keyboard::Space))
{
    if (facingRight) {
        // Player facing RIGHT - beam goes to the right
        beamSprite.setPosition(player_x + PlayerWidth, player_y + PlayerHeight / 2 - 10);
        beamSprite.setScale(gunRange / beamTexture.getSize().x, 1.0f);
        beamSprite.setRotation(0);
        beamSprite.setOrigin(0, 0);
    } else {
        // Player facing LEFT (default) - beam goes to the left
        beamSprite.setPosition(player_x, player_y + PlayerHeight / 2 - 10);
        beamSprite.setScale(-gunRange / beamTexture.getSize().x, 1.0f);
        beamSprite.setRotation(0);
        beamSprite.setOrigin(0, 0);
    }
    window.draw(beamSprite);
}
            // Draw gun timer
            if (gunActive && capturedEnemyActive)
            {
                RectangleShape timerBar;
                timerBar.setSize(Vector2f(100 * (capturedEnemyTime / MAX_CAPTURE_TIME), 10));
                timerBar.setPosition(player_x, player_y - 20);
                timerBar.setFillColor(Color::Red);
                window.draw(timerBar);
            }
            
            // Draw player
            if (selectedCharacter == 0) {
                PlayerSprite.setPosition(player_x, player_y);
                window.draw(PlayerSprite);
            } else {
                Player2Sprite.setPosition(player_x, player_y);
                window.draw(Player2Sprite);
            }
            
            // Draw HUD - Score and Health
            Text scoreText;
            scoreText.setFont(font);
            scoreText.setString("SCORE: " + to_string(score));
            scoreText.setCharacterSize(30);
            scoreText.setPosition(20, 20);
            scoreText.setFillColor(Color::White);
            window.draw(scoreText);
            
            Text healthText;
            healthText.setFont(font);
            healthText.setString("HEALTH: " + to_string(health));
            healthText.setCharacterSize(30);
            healthText.setPosition(20, 60);
            healthText.setFillColor(Color::Red);
            window.draw(healthText);
            
            // Combo streak display
            if (comboStreak > 2) {
                Text comboText;
                comboText.setFont(font);
                comboText.setString("COMBO x" + to_string(comboStreak));
                comboText.setCharacterSize(25);
                comboText.setPosition(screen_x - 200, 20);
                comboText.setFillColor(Color::Yellow);
                window.draw(comboText);
            }
            
            // Level complete message
            if (levelComplete) {
                Text completeText;
                completeText.setFont(font);
                completeText.setString("LEVEL COMPLETE!");
                completeText.setCharacterSize(60);
                completeText.setPosition(300, 400);
                completeText.setFillColor(Color::Green);
                window.draw(completeText);
                
                Text pressText;
                pressText.setFont(font);
                pressText.setString("Press ESC to return to menu");
                pressText.setCharacterSize(30);
                pressText.setPosition(320, 480);
                pressText.setFillColor(Color::White);
                window.draw(pressText);
            }
        }
        
        window.display();
    }
    
    // Cleanup
    lvlMusic.stop();
    for (int i = 0; i < height; i++)
    {
        delete[] lvl[i];
    }
    delete[] lvl;
    
    return 0;
}