#include <Novice.h>
#include <cmath>
#include <cstdlib>
#include <ctime>

struct Vector2 {
    float x, y;
};

struct Player {
    Vector2 pos;
    float radius;
    int hp;
};

struct Enemy {
    Vector2 pos;
    float radius;
    int hp;
    int level;
    float speed;
};

struct Bullet {
    Vector2 pos;
    Vector2 vel;
    float radius;
    int damage;
    bool alive;
};

struct Upgrade {
    const char* name;
    const char* desc;
};

enum class GameState {
    Playing,
    ChoosingUpgrade,
    GameOver
};

const int kWindowWidth = 1280;
const int kWindowHeight = 720;
const int kMaxBullets = 50;

Player player;
Enemy enemy;
Bullet bullets[kMaxBullets];

// 弾の基礎パラメータ（強化で増加）
float bulletBaseRadius = 10.0f;
int bulletBaseDamage = 1;
float bulletBaseSpeed = 10.0f;

// 強化候補
Upgrade upgrades[3];
int chosenUpgrade = -1;

GameState gameState = GameState::Playing;

struct Rect {
    int left, top, right, bottom;
} arena = { 100, 100, 1180, 620 };

// 距離計算
float Length(const Vector2& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

Vector2 Normalize(const Vector2& v) {
    float len = Length(v);
    if (len == 0) return { 0,0 };
    return { v.x / len, v.y / len };
}

// 敵の初期化
void SpawnEnemy() {
    enemy.pos = { (float)(rand() % (arena.right - arena.left) + arena.left),
                  (float)(rand() % (arena.bottom - arena.top) + arena.top) };
    enemy.radius = 30;
    enemy.hp = 5 + enemy.level * 2;
    enemy.level++;
    enemy.speed = 2.0f + enemy.level * 0.3f;
}

// 弾発射
void FireBullet(const Vector2& dir) {
    for (int i = 0; i < kMaxBullets; i++) {
        if (!bullets[i].alive) {
            bullets[i].alive = true;
            bullets[i].pos = player.pos;
            bullets[i].vel = { dir.x * bulletBaseSpeed, dir.y * bulletBaseSpeed };
            bullets[i].radius = bulletBaseRadius;
            bullets[i].damage = bulletBaseDamage;
            break;
        }
    }
}

// 強化候補生成
void GenerateUpgrades() {
    upgrades[0] = { "Size Up", "Bullet size +2" };
    upgrades[1] = { "Power Up", "Bullet damage +1" };
    upgrades[2] = { "Speed Up", "Bullet speed +2" };
}

// 強化適用
void ApplyUpgrade(int index) {
    if (index == 0) bulletBaseRadius += 2.0f;
    else if (index == 1) bulletBaseDamage += 1;
    else if (index == 2) bulletBaseSpeed += 2.0f;
}

const char kWindowTitle[] = "Bullet Upgrade Demo";

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, kWindowWidth, kWindowHeight);

    srand((unsigned int)time(nullptr));

    // 初期化
    player = { {640, 360}, 20.0f, 20 };
    enemy = { {400, 300}, 30.0f, 5, 1, 2.0f };

    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();

        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        // ===== 更新 =====
        if (gameState == GameState::Playing) {
            // 移動
            if (keys[DIK_W]) player.pos.y -= 5;
            if (keys[DIK_S]) player.pos.y += 5;
            if (keys[DIK_A]) player.pos.x -= 5;
            if (keys[DIK_D]) player.pos.x += 5;

            // ★ プレイヤーのワープ処理 ★
            if (player.pos.x < arena.left) player.pos.x = (float)arena.right;
            if (player.pos.x > arena.right) player.pos.x = (float)arena.left;
            if (player.pos.y < arena.top) player.pos.y = (float)arena.bottom;
            if (player.pos.y > arena.bottom) player.pos.y = (float)arena.top;

            // マウス位置
            int mx, my;
            Novice::GetMousePosition(&mx, &my);
            Vector2 aimDir = { mx - player.pos.x, my - player.pos.y };
            aimDir = Normalize(aimDir);

            // 射撃
            if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) {
                FireBullet(aimDir);
            }

            // 弾更新
            for (int i = 0; i < kMaxBullets; i++) {
                if (!bullets[i].alive) continue;
                bullets[i].pos.x += bullets[i].vel.x;
                bullets[i].pos.y += bullets[i].vel.y;
                if (bullets[i].pos.x < arena.left || bullets[i].pos.x > arena.right ||
                    bullets[i].pos.y < arena.top || bullets[i].pos.y > arena.bottom) {
                    bullets[i].alive = false;
                }
                // 当たり判定
                float dx = bullets[i].pos.x - enemy.pos.x;
                float dy = bullets[i].pos.y - enemy.pos.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < bullets[i].radius + enemy.radius) {
                    enemy.hp -= bullets[i].damage;
                    bullets[i].alive = false;
                    if (enemy.hp <= 0) {
                        gameState = GameState::ChoosingUpgrade;
                        GenerateUpgrades();
                    }
                }
            }

            // 敵移動
            Vector2 dir = { player.pos.x - enemy.pos.x, player.pos.y - enemy.pos.y };
            dir = Normalize(dir);
            enemy.pos.x += dir.x * enemy.speed;
            enemy.pos.y += dir.y * enemy.speed;

            // 敵攻撃
            float dx = player.pos.x - enemy.pos.x;
            float dy = player.pos.y - enemy.pos.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < player.radius + enemy.radius) {
                player.hp--;
                if (player.hp <= 0) gameState = GameState::GameOver;
            }
        }
        else if (gameState == GameState::ChoosingUpgrade) {
            if (keys[DIK_1] && !preKeys[DIK_1]) {
                ApplyUpgrade(0);
                SpawnEnemy();
                gameState = GameState::Playing;
            }
            if (keys[DIK_2] && !preKeys[DIK_2]) {
                ApplyUpgrade(1);
                SpawnEnemy();
                gameState = GameState::Playing;
            }
            if (keys[DIK_3] && !preKeys[DIK_3]) {
                ApplyUpgrade(2);
                SpawnEnemy();
                gameState = GameState::Playing;
            }
        }

        // ===== 描画 =====
        Novice::DrawBox(0, 0, kWindowWidth, kWindowHeight, 0.0f, 0x101018FF, kFillModeSolid);
        Novice::DrawBox(arena.left, arena.top, arena.right - arena.left, arena.bottom - arena.top, 0.0f, 0xFFFFFFFF, kFillModeWireFrame);

        if (gameState == GameState::Playing) {
            // プレイヤー
            Novice::DrawEllipse((int)player.pos.x, (int)player.pos.y, (int)player.radius, (int)player.radius, 0.0f, 0x44CCFFFF, kFillModeSolid);

            // 敵
            Novice::DrawEllipse((int)enemy.pos.x, (int)enemy.pos.y, (int)enemy.radius, (int)enemy.radius, 0.0f, 0xFF4444FF, kFillModeSolid);

            // 弾
            for (int i = 0; i < kMaxBullets; i++) {
                if (!bullets[i].alive) continue;
                Novice::DrawEllipse((int)bullets[i].pos.x, (int)bullets[i].pos.y, (int)bullets[i].radius, (int)bullets[i].radius, 0.0f, 0xFFFFFFFF, kFillModeSolid);
            }

            // UI
            Novice::ScreenPrintf(20, 20, "WASD: Move   Mouse: Aim   Space: Shoot");
            Novice::ScreenPrintf(20, 40, "Player HP: %d", player.hp);
            Novice::ScreenPrintf(20, 60, "Enemy HP: %d  LV: %d  SPD: %.1f", enemy.hp, enemy.level, enemy.speed);

            // 弾ステータス
            Novice::ScreenPrintf(20, 680, "Bullet Status:");
            Novice::ScreenPrintf(40, 700, "Size: %.1f", bulletBaseRadius);
            Novice::ScreenPrintf(160, 700, "Damage: %d", bulletBaseDamage);
            Novice::ScreenPrintf(320, 700, "Speed: %.1f", bulletBaseSpeed);
        }
        else if (gameState == GameState::ChoosingUpgrade) {
            Novice::DrawBox(200, 200, 880, 300, 0.0f, 0x000000C0, kFillModeSolid);
            Novice::ScreenPrintf(220, 220, "Choose Upgrade! (Press 1,2,3)");
            for (int i = 0; i < 3; i++) {
                Novice::ScreenPrintf(240, 260 + i * 40, "%d: %s - %s", i + 1, upgrades[i].name, upgrades[i].desc);
            }
        }
        else if (gameState == GameState::GameOver) {
            Novice::ScreenPrintf(600, 360, "GAME OVER");
        }

        Novice::EndFrame();
    }

    Novice::Finalize();
    return 0;
}
