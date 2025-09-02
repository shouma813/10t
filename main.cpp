#include <Novice.h>
#include <cmath>
#include <cstdlib>
#include <ctime>

const char kWindowTitle[] = "6403";

// 便利関数
struct Vec2 {
    float x, y;
};

static float Length(const Vec2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }
static Vec2   Normalize(const Vec2& v) {
    float l = Length(v);
    if (l == 0.0f) return { 0, 0 };
    return { v.x / l, v.y / l };
}
static Vec2   Sub(const Vec2& a, const Vec2& b) { return { a.x - b.x, a.y - b.y }; }
static Vec2   Add(const Vec2& a, const Vec2& b) { return { a.x + b.x, a.y + b.y }; }
static Vec2   Mul(const Vec2& a, float s) { return { a.x * s, a.y * s }; }
static bool   CircleHit(const Vec2& p1, float r1, const Vec2& p2, float r2) {
    float dx = p1.x - p2.x, dy = p1.y - p2.y;
    float rr = (r1 + r2) * (r1 + r2);
    return dx * dx + dy * dy <= rr;
}

// アリーナ（四角）の設定
struct Arena {
    int left, top, right, bottom;
};

// ラップ（境界を越えたら反対側へ）
static void WrapPosition(Vec2& p, float radius, const Arena& a) {
    if (p.x < a.left - radius)  p.x = (float)(a.right + radius);
    if (p.x > a.right + radius) p.x = (float)(a.left - radius);
    if (p.y < a.top - radius)   p.y = (float)(a.bottom + radius);
    if (p.y > a.bottom + radius)p.y = (float)(a.top - radius);
}

// プレイヤー・敵・弾
struct Player {
    Vec2 pos;
    float speed;
    float radius;
    int hp;
};

struct Enemy {
    Vec2 pos;
    float speed;
    float radius;
    int hp;
    int level; // 倒すたびに少しずつ強くなる
};

struct Bullet {
    Vec2 pos, vel;
    float radius;
    bool alive;
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, 1280, 720);
    std::srand((unsigned)std::time(nullptr));

    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    // アリーナはウィンドウ中央に大きめの四角
    Arena arena{ 140, 60, 1140, 660 };

    // プレイヤー初期化
    Player player{};
    player.pos = { (float)((arena.left + arena.right) / 2), (float)((arena.top + arena.bottom) / 2) };
    player.speed = 6.0f;
    player.radius = 14.0f;
    player.hp = 5;

    // 敵初期化
    Enemy enemy{};
    enemy.radius = 18.0f;
    enemy.speed = 3.0f;
    enemy.level = 0;
    enemy.hp = 5;
    enemy.pos = { (float)arena.left + 80.0f, (float)arena.top + 80.0f };

    // 弾
    const int kMaxBullets = 64;
    Bullet bullets[kMaxBullets]{};
    int shootCooldown = 0;

    auto RespawnEnemy = [&](int bonusLevel) {
        enemy.level += bonusLevel;
        enemy.hp = 5 + enemy.level;                 // 少し硬く
        enemy.speed = 3.0f + 0.2f * enemy.level;    // 少し速く
        // 端のどこかに出す
        int edge = std::rand() % 4;
        switch (edge) {
        case 0: enemy.pos = { (float)arena.left + 10.0f, (float)(arena.top + std::rand() % (arena.bottom - arena.top)) }; break;
        case 1: enemy.pos = { (float)arena.right - 10.0f, (float)(arena.top + std::rand() % (arena.bottom - arena.top)) }; break;
        case 2: enemy.pos = { (float)(arena.left + std::rand() % (arena.right - arena.left)), (float)arena.top + 10.0f }; break;
        case 3: enemy.pos = { (float)(arena.left + std::rand() % (arena.right - arena.left)), (float)arena.bottom - 10.0f }; break;
        }
        };

    auto ResetGame = [&]() {
        player.pos = { (float)((arena.left + arena.right) / 2), (float)((arena.top + arena.bottom) / 2) };
        player.hp = 5;
        enemy.level = 0;
        enemy.speed = 3.0f;
        enemy.hp = 5;
        enemy.pos = { (float)arena.left + 80.0f, (float)arena.top + 80.0f };
        for (auto& b : bullets) b.alive = false;
        };

    // ループ
    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        // ===== 更新 =====
        // 入力（移動）
        Vec2 move{ 0,0 };
        if (keys[DIK_W] || keys[DIK_UP])    move.y -= 1.0f;
        if (keys[DIK_S] || keys[DIK_DOWN])  move.y += 1.0f;
        if (keys[DIK_A] || keys[DIK_LEFT])  move.x -= 1.0f;
        if (keys[DIK_D] || keys[DIK_RIGHT]) move.x += 1.0f;
        move = Normalize(move);
        player.pos = Add(player.pos, Mul(move, player.speed));

        // シュート（スペース）
        if (shootCooldown > 0) shootCooldown--;
        if (preKeys[DIK_SPACE] == 0 && keys[DIK_SPACE] != 0 && shootCooldown == 0) {
            // マウス方向に撃つ
            int mx, my;
            Novice::GetMousePosition(&mx, &my);
            Vec2 dir = Normalize(Sub(Vec2{ (float)mx, (float)my }, player.pos));
            if (dir.x != 0.0f || dir.y != 0.0f) {
                for (int i = 0; i < kMaxBullets; ++i) {
                    if (!bullets[i].alive) {
                        bullets[i].alive = true;
                        bullets[i].pos = Add(player.pos, Mul(dir, player.radius + 4.0f));
                        bullets[i].vel = Mul(dir, 12.0f);
                        bullets[i].radius = 6.0f;
                        shootCooldown = 6; // 連射間隔
                        break;
                    }
                }
            }
        }

        // 敵AI：プレイヤーに向かう
        Vec2 toPlayer = Sub(player.pos, enemy.pos);
        Vec2 dirTo = Normalize(toPlayer);
        enemy.pos = Add(enemy.pos, Mul(dirTo, enemy.speed));

        // ラップ（境界超え→反対側）
        WrapPosition(player.pos, player.radius, arena);
        WrapPosition(enemy.pos, enemy.radius, arena);
        for (int i = 0; i < kMaxBullets; ++i) {
            if (!bullets[i].alive) continue;
            bullets[i].pos = Add(bullets[i].pos, bullets[i].vel);
            WrapPosition(bullets[i].pos, bullets[i].radius, arena);
        }

        // 弾と敵の当たり判定
        for (int i = 0; i < kMaxBullets; ++i) {
            if (!bullets[i].alive) continue;
            if (CircleHit(bullets[i].pos, bullets[i].radius, enemy.pos, enemy.radius)) {
                bullets[i].alive = false;
                enemy.hp--;
                if (enemy.hp <= 0) {
                    RespawnEnemy(1); // 倒すと強くなって再出現
                }
            }
        }

        // 敵とプレイヤーの当たり判定（触れるとダメージ＆ノックバック）
        if (CircleHit(player.pos, player.radius, enemy.pos, enemy.radius)) {
            player.hp--;
            Vec2 away = Normalize(Sub(player.pos, enemy.pos));
            player.pos = Add(player.pos, Mul(away, 24.0f));
            if (player.hp <= 0) {
                // リセット
                ResetGame();
            }
        }

        // ===== 描画 =====
        // 背景
        Novice::DrawBox(0, 0, 1280, 720, 0.0f, 0x101018FF, kFillModeSolid);

        // アリーナ（四角い境界）
        Novice::DrawBox(arena.left, arena.top, arena.right - arena.left, arena.bottom - arena.top, 0.0f, 0xFFFFFFFF, kFillModeWireFrame);

        // プレイヤー
        Novice::DrawEllipse((int)player.pos.x, (int)player.pos.y, (int)player.radius, (int)player.radius, 0.0f, 0x44CCFFFF, kFillModeSolid);

        // 敵
        Novice::DrawEllipse((int)enemy.pos.x, (int)enemy.pos.y, (int)enemy.radius, (int)enemy.radius, 0.0f, 0xFF4444FF, kFillModeSolid);

        // 弾
        for (int i = 0; i < kMaxBullets; ++i) {
            if (!bullets[i].alive) continue;
            Novice::DrawEllipse((int)bullets[i].pos.x, (int)bullets[i].pos.y, (int)bullets[i].radius, (int)bullets[i].radius, 0.0f, 0xFFFFFFFF, kFillModeSolid);
        }

        // UI
        Novice::ScreenPrintf(20, 20, "WASD/Arrow: Move");
        Novice::ScreenPrintf(20, 40, "Mouse: Aim   Space: Shoot");
        Novice::ScreenPrintf(20, 60, "Wrap: Leave the box -> appear from the opposite side");
        Novice::ScreenPrintf(20, 90, "Player HP: %d", player.hp);
        Novice::ScreenPrintf(20, 110, "Enemy HP: %d  LV: %d  SPD: %.1f", enemy.hp, enemy.level, enemy.speed);

        Novice::EndFrame();

        // ESC で終了
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) break;
    }

    Novice::Finalize();
    return 0;
}
