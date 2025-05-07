#include<stdio.h>
#include<time.h>
#include<windows.h>
#include<stdlib.h>
#include<string.h>

// 蛇身结构体
typedef struct SNAKE {
    int x;
    int y;
    struct SNAKE* next;
} snake;

#define U 1
#define D 2
#define L 3
#define R 4       // 移动方向

// 用户系统结构体
typedef struct {
    char username[20];
    char password[20];
} User;

// 游戏日志结构体
typedef struct GameLog {
    int log_id;
    char username[20];
    time_t start_time;
    time_t end_time;
    int score;
    char game_over_reason[50];
} GameLog;

// 新增游戏状态保存结构体
typedef struct GameState {
    int snake_nodes[1000][2];  // 保存蛇身坐标（x,y），足够大的数组
    int snake_length;         // 蛇身长度
    int food_x, food_y;       // 食物坐标
    int score;                // 当前得分
} GameState;

// 全局变量
int score = 0, add = 10;
int status, sleeptime = 200;
char current_username[20] = "";
snake *head = NULL, *food = NULL;
int endgamestatus = 0;
static int log_counter = 1;
GameState saved_state;      // 保存的游戏状态

// 函数声明
void Pos(int x, int y);
void creatMap();
void initsnake();
void freesnake();
int biteself();
void createfood();
int snake_contains(snake* head, int x, int y);
void cantcrosswall();
void snakemove();
void pause();
void gamecircle();
void endgame();
void gamestart();
void registerUser();
int loginUser();
void recordGameLog(time_t start_time);
void displayGameLogs();
void save_game_state();       // 新增保存状态函数
void restore_game_state();    // 新增恢复状态函数

// 设置光标位置
void Pos(int x, int y) {
    COORD pos = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

// 创建游戏地图（游戏框范围：x=0~56，y=0~26）
void creatMap() {
    for (int i = 0; i < 58; i += 2) {
        Pos(i, 0); printf("■");   // 顶部边框
        Pos(i, 26); printf("■");  // 底部边框
    }
    for (int i = 1; i < 26; i++) {
        Pos(0, i); printf("■");     // 左侧边框
        Pos(56, i); printf("■");    // 右侧边框
    }
}

// 初始化蛇（初始长度5节，横向排列）
void initsnake() {
    snake *tail = (snake*)malloc(sizeof(snake));
    tail->x = 24; tail->y = 5; tail->next = NULL;

    for (int i = 1; i <= 4; i++) {
        snake *new_head = (snake*)malloc(sizeof(snake));
        new_head->next = tail;
        new_head->x = 24 + 2 * i;  // 每节占2个字符宽度
        new_head->y = 5;
        tail = new_head;
    }
    head = tail;

    // 绘制初始蛇身
    snake *p = head;
    while (p) {
        Pos(p->x, p->y);
        printf("■");
        p = p->next;
    }
}

// 释放蛇身内存
void freesnake() {
    snake *current = head;
    while (current) {
        snake *next = current->next;
        free(current);
        current = next;
    }
    head = NULL;
}

// 判断是否咬到自己
int biteself() {
    snake *self = head->next;
    while (self) {
        if (self->x == head->x && self->y == head->y) return 1;
        self = self->next;
    }
    return 0;
}

// 创建食物（确保在有效区域且不在蛇身上）
void createfood() {
    if (food) {
        free(food);
        food = NULL;
    }

    snake *food_1 = (snake*)malloc(sizeof(snake));
    do {
        food_1->x = 2 + (rand() % 27) * 2;  // x: 2-54（偶数，避开边框）
        food_1->y = rand() % 25 + 1;         // y: 1-25（避开边框）
    } while (snake_contains(head, food_1->x, food_1->y));

    Pos(food_1->x, food_1->y);
    printf("■");
    food = food_1;
}

// 检测坐标是否在蛇身
int snake_contains(snake *head, int x, int y) {
    snake *p = head;
    while (p) {
        if (p->x == x && p->y == y) return 1;
        p = p->next;
    }
    return 0;
}

// 穿墙检测
void cantcrosswall() {
    if (head->x < 2 || head->x > 54 || head->y < 1 || head->y > 25) {
        endgamestatus = 1;
        endgame();
    }
}

// 蛇移动逻辑
void snakemove() {
    snake *new_head = (snake*)malloc(sizeof(snake));
    switch (status) {
        case U: new_head->x = head->x; new_head->y = head->y - 1; break;
        case D: new_head->x = head->x; new_head->y = head->y + 1; break;
        case L: new_head->x = head->x - 2; new_head->y = head->y; break;
        case R: new_head->x = head->x + 2; new_head->y = head->y; break;
    }

    Pos(new_head->x, new_head->y);
    printf("■");  // 绘制新头部

    if (new_head->x == food->x && new_head->y == food->y) {
        // 吃食物：不删除尾部，增加长度
        new_head->next = head;
        head = new_head;
        score += add;
        createfood();  // 生成新食物
    } else {
        // 普通移动：删除尾部
        new_head->next = head;
        head = new_head;

        snake *tail = head;
        while (tail->next->next) tail = tail->next;
        Pos(tail->next->x, tail->next->y);
        printf("  ");  // 清除旧尾部
        free(tail->next);
        tail->next = NULL;
    }

    if (biteself()) endgamestatus = 2;  // 检测咬到自己
}

// 暂停功能（提示在右侧信息区）
void pause() {
    Pos(64, 12);  // 右侧信息区，y=12（介于得分和操作说明之间）
    printf("游戏暂停，按空格继续...");
    while (!GetAsyncKeyState(VK_SPACE)) Sleep(100);
    Pos(64, 12);
    printf("                          ");  // 清除提示
}

// 新增：保存游戏状态
void save_game_state() {
    snake *p = head;
    saved_state.snake_length = 0;
    while (p && saved_state.snake_length < 1000) {
        saved_state.snake_nodes[saved_state.snake_length][0] = p->x;
        saved_state.snake_nodes[saved_state.snake_length][1] = p->y;
        saved_state.snake_length++;
        p = p->next;
    }
    if (food) {
        saved_state.food_x = food->x;
        saved_state.food_y = food->y;
    }
    saved_state.score = score;
}

// 新增：恢复游戏状态
void restore_game_state() {
    // 清理当前游戏对象
    freesnake();
    if (food) free(food);

    // 重建蛇身
    snake *new_tail = NULL, *new_node;
    for (int i = saved_state.snake_length - 1; i >= 0; i--) {
        new_node = (snake*)malloc(sizeof(snake));
        new_node->x = saved_state.snake_nodes[i][0];
        new_node->y = saved_state.snake_nodes[i][1];
        new_node->next = new_tail;
        new_tail = new_node;
    }
    head = new_tail;

    // 重建食物
    food = (snake*)malloc(sizeof(snake));
    food->x = saved_state.food_x;
    food->y = saved_state.food_y;

    // 恢复得分
    score = saved_state.score;

    // 重新绘制游戏界面
    system("cls");
    creatMap();

    // 绘制蛇身
    snake *p = head;
    while (p) {
        Pos(p->x, p->y);
        printf("■");
        p = p->next;
    }

    // 绘制食物
    Pos(food->x, food->y);
    printf("■");

    // 恢复右侧信息
    Pos(64, 8);   printf("玩家: %s", current_username);
    Pos(64, 10);  printf("得分：%d", score);
    Pos(64, 11);  printf("每食物得分：%d分", add);
    Pos(64, 15);  printf("操作：↑↓←→移动|空格暂停|F1加速|F2减速|F5日志|ESC退出");
}

// 游戏主循环
void gamecircle() {
    status = R;  // 初始向右移动
    time_t start_time = time(NULL);

    // 右侧信息区布局（x=64开始，与游戏框完全分离）
    Pos(64, 8);   printf("玩家: %s", current_username);
    Pos(64, 10);  printf("得分：%d", score);
    Pos(64, 11);  printf("每食物得分：%d分", add);
    Pos(64, 15);  printf("操作：↑↓←→移动|空格暂停|F1加速|F2减速|F5日志|ESC退出");

    while (!endgamestatus) {
        // 退出检测
        if (GetAsyncKeyState(VK_ESCAPE)) endgamestatus = 3;
        // 日志显示
        if (GetAsyncKeyState(VK_F5)) displayGameLogs();
        // 方向键处理（防止反向移动）
        if (GetAsyncKeyState(VK_UP) && status != D) status = U;
        else if (GetAsyncKeyState(VK_DOWN) && status != U) status = D;
        else if (GetAsyncKeyState(VK_LEFT) && status != R) status = L;
        else if (GetAsyncKeyState(VK_RIGHT) && status != L) status = R;
        // 速度调节
        if (GetAsyncKeyState(VK_F1) && sleeptime > 50) { sleeptime -= 30; add += 2; }
        if (GetAsyncKeyState(VK_F2) && sleeptime < 350) { sleeptime += 30; add = add > 1 ? add - 2 : 1; }
        // 暂停处理
        if (GetAsyncKeyState(VK_SPACE)) pause();

        Sleep(sleeptime);
        snakemove();
        cantcrosswall();
        Pos(64, 10);  printf("得分：%d", score);  // 更新得分显示
    }

    recordGameLog(start_time);
    endgame();
}

// 优化后的主菜单
void showMainMenu() {
    system("cls");
    printf("╔══════════════════════════════╗\n");
    printf("║         贪吃蛇游戏           ║\n");
    printf("╠══════════════════════════════╣\n");
    printf("║  1. 注册账号                 ║\n");
    printf("║  2. 用户登录                 ║\n");
    printf("║  3. 退出游戏                 ║\n");
    printf("╚══════════════════════════════╝\n");
    printf("请输入您的选择 (1-3)：");
}

// 优化后的注册界面（使用txt文件）
void registerUser() {
    User new_user;
    system("cls");
    printf("╔════════════════════════╗\n");
    printf("║       用户注册         ║\n");
    printf("╚════════════════════════╝\n");

    printf("用户名（1-20字符）: ");
    scanf("%19s", new_user.username);

    FILE *fp = fopen("users.txt", "r");
    if (fp) {
        User tmp;
        while (fscanf(fp, "%s %s", tmp.username, tmp.password) != EOF) {
            if (!strcmp(tmp.username, new_user.username)) {
                printf("用户名已存在！\n");
                fclose(fp);
                Sleep(1000);
                return;
            }
        }
        fclose(fp);
    }

    printf("密码（1-20字符）: ");
    scanf("%19s", new_user.password);

    fp = fopen("users.txt", "a");
    fprintf(fp, "%s %s\n", new_user.username, new_user.password);
    fclose(fp);
    printf("注册成功！\n");
    Sleep(1000);
}

// 优化后的登录提示（使用txt文件）
int loginUser() {
    User input;
    system("cls");
    printf("╔════════════════════════╗\n");
    printf("║       用户登录         ║\n");
    printf("╚════════════════════════╝\n");
    printf("用户名: ");
    scanf("%19s", input.username);
    printf("密码: ");
    scanf("%19s", input.password);

    FILE *fp = fopen("users.txt", "r");
    if (!fp) return 0;

    User stored;
    while (fscanf(fp, "%s %s", stored.username, stored.password) != EOF) {
        if (!strcmp(input.username, stored.username) &&
            !strcmp(input.password, stored.password)) {
            strcpy(current_username, input.username);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// 优化后的游戏结束界面
void endgame() {
    system("cls");
    Pos(20, 10);
    printf("╔════════════════════════════╗");
    Pos(20, 11);
    switch (endgamestatus) {
        case 1: printf("║     游戏结束：撞到墙壁！   ║"); break;
        case 2: printf("║     游戏结束：咬到自己！   ║"); break;
        case 3: printf("║     您已主动退出游戏。     ║"); break;
    }
    Pos(20, 12);
    printf("║     最终得分：%-4d         ║", score);
    Pos(20, 13);
    printf("╚════════════════════════════╝");
    Pos(20, 15);
    printf("按任意键返回菜单...");
    getchar(); getchar();
}

// 优化后的日志显示界面（解析txt文件）
void displayGameLogs() {
    save_game_state();
    system("cls");
    Pos(0, 0); printf("╔══════════════════════════════════════════════════════════════════════════════╗");
    Pos(0, 1); printf("║ 日志ID | 玩家   | 开始时间         | 结束时间         | 得分 | 结束原因     ║");
    Pos(0, 2); printf("╠══════════════════════════════════════════════════════════════════════════════╣");

    FILE *fp = fopen("game_logs.txt", "r");
    int y = 3;
    if (fp) {
        GameLog log;
        long start_sec, end_sec;
        while (fscanf(fp, "%d %s %ld %ld %d %s",
                      &log.log_id, log.username, &start_sec, &end_sec,
                      &log.score, log.game_over_reason) != EOF) {
            log.start_time = (time_t)start_sec;
            log.end_time = (time_t)end_sec;
            char start[20], end[20];
            strftime(start, 20, "%Y-%m-%d %H:%M", localtime(&log.start_time));
            strftime(end, 20, "%Y-%m-%d %H:%M", localtime(&log.end_time));
            Pos(0, y++);
            printf("║ %-6d | %-6s | %-16s | %-16s | %-4d | %-12s ║",
                   log.log_id, log.username, start, end, log.score, log.game_over_reason);
        }
        fclose(fp);
    } else {
        Pos(0, y++);
        printf("║                          暂无游戏日志记录                                 ║");
    }

    Pos(0, y++); printf("╚══════════════════════════════════════════════════════════════════════════════╝");
    Pos(0, y++); printf("按任意键返回游戏...");
    getchar(); getchar();
    restore_game_state();
}


// 记录游戏日志（使用txt文件）
void recordGameLog(time_t start_time) {
    GameLog log = {
        .log_id = log_counter++,
        .start_time = start_time,
        .end_time = time(NULL),
        .score = score
    };
    strcpy(log.username, current_username);

    switch (endgamestatus) {
        case 1: strcpy(log.game_over_reason, "撞到墙壁"); break;
        case 2: strcpy(log.game_over_reason, "咬到自己"); break;
        case 3: strcpy(log.game_over_reason, "主动退出"); break;
    }

    FILE *fp = fopen("game_logs.txt", "a");
    if (fp) {
        fprintf(fp, "%d %s %ld %ld %d %s\n",
                log.log_id, log.username,
                (long)log.start_time, (long)log.end_time,
                log.score, log.game_over_reason);
        fclose(fp);
    }
}



// 游戏初始化（清屏并重置数据）
void gamestart() {
    system("cls");
    freesnake();
    if (food) { free(food); food = NULL; }
    score = 0; add = 10; sleeptime = 200; endgamestatus = 0;
    system("mode con cols=100 lines=30");  // 设置足够宽的控制台窗口
    creatMap();
    initsnake();
    createfood();
}
// 主程序主循环（整合主菜单）
int main() {
    srand(time(NULL));
    while (1) {
        showMainMenu();
        int choice;
        scanf("%d", &choice);
        while (getchar() != '\n'); // 清除换行符
        switch (choice) {
            case 1:
                registerUser(); break;
            case 2:
                if (loginUser()) {
                    gamestart();
                    gamecircle();
                } else {
                    printf("登录失败！\n");
                    Sleep(1000);
                }
                break;
            case 3:
                exit(0);
            default:
                printf("无效选项，请重试！\n");
                Sleep(1000);
        }
    }
    return 0;
}
