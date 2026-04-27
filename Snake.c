// 这个是给非UTF-8编码看的↓
// һ��ע��ʹ�õ���UTF-8���룬���ʱ�������������ģʽΪUTF-8
/*
    版本迭代： 2025.8.25  Base    0.1-项目起项,构思出基本逻辑框架
              2025.8.28  Alpha   0.5-初步完成贪吃蛇的基本功能
              2025.8.29  Alpha   0.7-修复边界显示bug,适当调整蛇初始移动速度 speed 250 -> 200
              2025.8.30  Beta    0.8-增加机制"得分加速":当玩家得分到达10时速度加倍，经过测试员(同学)测试暂无bug
              2025.11.13 Release 1.0-修复Score得分栏错位bug
                                     新增space暂停键,shift加速键
              2025.11.14 Release 1.1-修复终端非UTF-8编码运行乱码问题(头疼...),修改蛇头的碰撞逻辑,优化显示排版
                                     新增两种特殊食物 1.发霉的食物
                                                     2.蜕皮(严格讲这不叫食物)
              2025.11.15 Release 1.2-修改"得分加速"机制(更名为"蛇身加速"):当玩家得分到达10时速度加倍 -> 当玩家蛇身到达10时速度加倍
                                     增加开始介绍菜单,颜色区分显示(优化游玩体验)
              2025.11.17 Release 1.3-添加背景音乐，音效，添加蛇身长度显示
              2025.11.18 Release 1.4-修复异常减速bug,修复length显示错误bug
                                     适当调整蛇初始移动速度 speed 200 -> 170
*/

/*
    基本游戏键介绍，WASD 上下左右
                   sapce 暂停/继续游戏
                   shift 加速
    食物类型介绍,  G基本食物-吃下后分数加一
                  W发霉的食物-吃下后随机生成10个墙方块
                  T蜕皮-吃下后蛇身一分为二，后一半变为墙脱离蛇身，蛇头速度变慢(所谓蜕皮)))
    Enjoy!!! 争取得到最高分吧!!!
*/

// 特别感谢论坛大佬提供的音乐导入技术!!!(音乐应该没侵权吧....)

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>

// 结构体定义
typedef struct snake
{
    int snake_x;
    int snake_y;
    struct snake *next_node;
} snake;

typedef struct wall
{
    int wall_x;
    int wall_y;
    struct wall *next;
} wall;

// 函数指针
typedef DWORD(WINAPI *MCI_SEND_STRING)(LPCSTR, LPSTR, UINT, HANDLE);

// 定义
int width = 50;
int height = 20;
int snake_x = 25;
int snake_y = 10;
int mode = 1; // 1上 2下 3左 4右
int speed = 170;
int normal_speed = 170;
int fast_speed = 100;
int rate = 2;
int max_length = 500;
int length = 1;
snake *head_node = NULL;
int point_x = 0;
int point_y = 0;
int score = 0;
int game_paused = 0;         // 0游戏运行中, 1游戏暂停
int is_speeding = 0;         // 0正常速度, 1加速状态
int special_food_active = 0; // 0没有特殊食物, 1有特殊食物
int special_food_x = 0;
int special_food_y = 0;
int moldy_food_active = 0; // 0没有发霉食物, 1有发霉食物
int moldy_food_x = 0;
int moldy_food_y = 0;
wall *wall_head = NULL; // 墙的链表头
int current_bgm = 0;    // bgm随机数

// 颜色常量
#define COLOR_RED 12
#define COLOR_GREEN 10
#define COLOR_YELLOW 14
#define COLOR_BLUE 9
#define COLOR_PURPLE 13
#define COLOR_CYAN 11
#define COLOR_WHITE 15
#define COLOR_GRAY 8

// 全局变量
static HMODULE hWinMM = NULL;
static MCI_SEND_STRING mciSendStringFunc = NULL;

// 背景音乐文件列表(不支持自定义，请勿改变列表的三首歌)-吃过不少亏qaq
const char *bgMusicFiles[] =
    {
        "slow01.WAV",
        "slow02.WAV",
        "slow03.WAV",
};

// 函数声明
void draw();
void setCursorPosition(int x, int y);
void setColor(int color);
void setpoint();
void move_snake();
void key_scanf();
void gameover();
void add_bodynode(int x, int y);
void clear_node();
void show_body();
void clearafter(snake *temp);
void update_snake_body();
void start_screen();
void pause_game();
void check_speed_key();
void add_wall(int x, int y);
void clear_walls();
void show_walls();
void generate_special_food();
void generate_moldy_food();
void shed_skin();
void create_random_walls();
int check_wall_collision(int x, int y);
int is_position_valid(int x, int y);
void playSoundEffect(const char *filename);
void playBackgroundMusic(const char *filename);
void restartBackgroundMusicIfEnded(const char *filename);
int initAudio();
void cleanupAudio();
void update_score_display();
void update_length_display();
void cleanup_all();
void redraw_all_foods();

// 主函数
int main()
{
    // 初始化音频
    initAudio();

    // 强制设置控制台窗口大小和缓冲区，避免显示错位
    HWND console = GetConsoleWindow();
    RECT rect;
    GetWindowRect(console, &rect);
    MoveWindow(console, rect.left, rect.top, 800, 600, TRUE);
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), (COORD){80, 30});

    // 用来解决编码问题
    SetConsoleOutputCP(65001);

    // 禁用快速编辑模式（ENABLE_QUICK_EDIT_MODE）和插入模式（ENABLE_INSERT_MODE）
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hConsole, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    mode &= ~ENABLE_INSERT_MODE;
    SetConsoleMode(hConsole, mode);

    // 隐藏光标
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);

    // 显示开始界面
    start_screen();

    // 初始绘制
    draw();
    setpoint();

    // 开始播放音乐
    srand((unsigned int)time(NULL));
    current_bgm = rand() % 3;
    playBackgroundMusic(bgMusicFiles[current_bgm]);

    // 初始化蛇身
    for (int i = 0; i < length; i++)
    {
        add_bodynode(snake_x, snake_y - i);
    }

    // 显示初始蛇头
    setCursorPosition(snake_x + 1, snake_y + 2);
    setColor(COLOR_GREEN);
    printf("↑");
    setColor(COLOR_WHITE);

    // BGM计数器
    int bgm_count = 0;
    while (1)
    {
        // 检查是否切换bgm
        bgm_count++;
        if (bgm_count > 102)
        {
            bgm_count = 0;

            srand((unsigned int)time(NULL));
            current_bgm = rand() % 3;
            restartBackgroundMusicIfEnded(bgMusicFiles[current_bgm]);
        }

        key_scanf();
        check_speed_key(); // 检查加速按键状态

        if (!game_paused)
        {
            move_snake();
            update_snake_body();
            show_body();
            show_walls();

            // 检查是否吃到普通食物
            if (snake_x == point_x && snake_y == point_y)
            {
                playSoundEffect("coin.WAV");

                score++;
                if (length < max_length)
                {
                    length++;
                }

                // 当蛇身长度达到10时，速度加倍
                if (length == 10)
                {
                    // 更新基础速度
                    normal_speed /= rate;
                    fast_speed /= rate;

                    // 在加速状态，应用新速度
                    if (is_speeding)
                    {
                        speed = fast_speed;
                    }
                    else
                    {
                        speed = normal_speed;
                    }
                }

                // 更新分数和长度显示
                update_score_display();
                update_length_display();

                // 当得分达到5后，有几率生成特殊食物T
                if (score >= 5 && !special_food_active && (rand() % 5 == 0))
                {
                    generate_special_food();
                }

                // 当得分大于3后，有几率生成发霉食物W
                if (score > 3 && !moldy_food_active && (rand() % 7 == 0))
                {
                    generate_moldy_food();
                }

                setpoint();
            }

            // 检查是否吃到特殊食物T
            if (special_food_active && snake_x == special_food_x && snake_y == special_food_y)
            {
                shed_skin();
                special_food_active = 0;
                // 清除特殊食物显示
                setCursorPosition(special_food_x + 1, special_food_y + 2);
                printf(" ");
                fflush(stdout);
            }

            // 检查是否吃到发霉食物W
            if (moldy_food_active && snake_x == moldy_food_x && snake_y == moldy_food_y)
            {
                create_random_walls();
                moldy_food_active = 0;
                // 清除发霉食物显示
                setCursorPosition(moldy_food_x + 1, moldy_food_y + 2);
                printf(" ");
                fflush(stdout);
            }

            // 检查是否撞到身体
            snake *temp = head_node->next_node;
            while (temp != NULL)
            {
                if (temp->snake_x == snake_x && temp->snake_y == snake_y)
                {
                    gameover();
                    goto fail;
                }
                temp = temp->next_node;
            }

            // 检查是否撞到墙
            if (check_wall_collision(snake_x, snake_y))
            {
                gameover();
                goto fail;
            }

            // 边界检查
            if (snake_x < 0 || snake_x >= width || snake_y < 0 || snake_y >= height)
            {
                gameover();
                break;
            }
        }

        Sleep(speed);
    }

fail:
    cleanup_all();
    // 恢复光标显示和默认颜色
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    setColor(COLOR_WHITE);

    return 0;
}

// 函数区---------------------------------------------------------------
//  更新分数显示
void update_score_display()
{
    setCursorPosition((width / 2) + 2, 0);
    setColor(COLOR_WHITE);
    printf("Score: %-5d", score);
    fflush(stdout);
}

//  更新长度显示
void update_length_display()
{
    setCursorPosition((width / 2) - 10, 0);
    setColor(COLOR_WHITE);
    printf("Length: %-3d", length);
    fflush(stdout);
}

//  统一清理资源
void cleanup_all()
{
    clear_node();
    clear_walls();
    cleanupAudio();
}

void redraw_all_foods()
{
    // 重新绘制普通食物G
    setCursorPosition(point_x + 1, point_y + 2);
    setColor(COLOR_RED);
    printf("G");
    setColor(COLOR_WHITE);
    fflush(stdout);

    // 重新绘制特殊食物T（如果存在）
    if (special_food_active)
    {
        setCursorPosition(special_food_x + 1, special_food_y + 2);
        setColor(COLOR_RED);
        printf("T");
        setColor(COLOR_WHITE);
        fflush(stdout);
    }

    // 重新绘制发霉食物W（如果存在）
    if (moldy_food_active)
    {
        setCursorPosition(moldy_food_x + 1, moldy_food_y + 2);
        setColor(COLOR_RED);
        printf("W");
        setColor(COLOR_WHITE);
        fflush(stdout);
    }
}

//  开始界面
void start_screen()
{
    system("cls");

    // 显示游戏标题
    setCursorPosition(width / 2 - 5, height / 2 - 5);
    setColor(COLOR_CYAN);
    printf("贪吃蛇游戏");
    setColor(COLOR_WHITE);

    // 显示操作说明
    setCursorPosition(width / 2 - 10, height / 2 - 3);
    printf("操作说明:");
    setCursorPosition(width / 2 - 10, height / 2 - 2);
    printf("W - 向上移动");
    setCursorPosition(width / 2 - 10, height / 2 - 1);
    printf("S - 向下移动");
    setCursorPosition(width / 2 - 10, height / 2);
    printf("A - 向左移动");
    setCursorPosition(width / 2 - 10, height / 2 + 1);
    printf("D - 向右移动");
    setCursorPosition(width / 2 - 10, height / 2 + 2);
    printf("空格键 - 暂停/继续游戏");
    setCursorPosition(width / 2 - 10, height / 2 + 3);
    printf("Shift键 - 按住加速");

    // 显示游戏规则
    setCursorPosition(width / 2 - 10, height / 2 + 5);
    setColor(COLOR_YELLOW);
    printf("游戏规则:");
    setCursorPosition(width / 2 - 10, height / 2 + 6);
    printf("蛇身长度达到10时速度会加倍");
    setCursorPosition(width / 2 - 10, height / 2 + 8);
    printf("得分≥5后可能出现特殊食物T");
    setCursorPosition(width / 2 - 10, height / 2 + 9);
    printf("吃T后蛇会蜕皮，后半段变墙");
    setCursorPosition(width / 2 - 10, height / 2 + 10);
    printf("蛇的速度减半,前半段仍跟随蛇");
    setCursorPosition(width / 2 - 10, height / 2 + 12);
    printf("得分>3后可能出现发霉食物W");
    setCursorPosition(width / 2 - 10, height / 2 + 13);
    printf("吃W后会生成10个随机墙方块");
    setColor(COLOR_WHITE);

    // 提示按任意键开始
    setCursorPosition(width / 2 - 10, height / 2 + 15);
    setColor(COLOR_GREEN);
    printf("按任意键开始游戏...");
    setColor(COLOR_WHITE);

    // 等待按键
    _getch();
}

// 暂停游戏（核心修改：暂停提示移到游戏区域外，恢复时重绘食物）
void pause_game()
{
    playSoundEffect("system.WAV");

    // 切换暂停状态
    game_paused = !game_paused;

    // 暂停提示移到游戏区域外（右上角，不与食物/蛇身重叠）
    int pause_x = width + 5; // 游戏区域宽度50，移到55列
    int pause_y = 0;         // 第0行（分数栏同一行，不干扰）

    if (game_paused)
    {
        // 显示暂停信息（游戏区域外）
        setCursorPosition(pause_x, pause_y);
        setColor(COLOR_YELLOW);
        printf("【游戏暂停】");
        setCursorPosition(pause_x, pause_y + 1);
        printf("按空格继续");
        setColor(COLOR_WHITE);
        fflush(stdout);
    }
    else
    {
        // 清除暂停信息（仅清除游戏区域外的提示）
        setCursorPosition(pause_x, pause_y);
        printf("            "); // 覆盖"【游戏暂停】"
        setCursorPosition(pause_x, pause_y + 1);
        printf("          "); // 覆盖"按空格继续"
        fflush(stdout);

        // 恢复暂停后，重新绘制所有食物（防止意外覆盖）
        redraw_all_foods();
    }
}

// 检查加速按键
void check_speed_key()
{
    // Shift键是否被按下
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
    {
        if (!is_speeding)
        {
            is_speeding = 1;
            speed = fast_speed;
            // 加速提示也移到游戏区域外，避免干扰
            setCursorPosition(width + 5, 3);
            setColor(COLOR_YELLOW);
            printf("加速中!");
            setColor(COLOR_WHITE);
            fflush(stdout);
        }
    }
    else
    {
        if (is_speeding)
        {
            is_speeding = 0;
            speed = normal_speed;
            // 清除加速提示
            setCursorPosition(width + 5, 3);
            printf("       ");
            fflush(stdout);
        }
    }
}

// 绘制游戏界面
void draw()
{
    playSoundEffect("system.WAV");

    // 清屏
    system("cls");

    // 打印分数板
    update_score_display();

    // 打印蛇长度
    update_length_display();

    // 上边界
    setCursorPosition(0, 1);
    setColor(COLOR_GRAY);
    for (int i = 0; i < width + 2; i++)
    {
        printf("#");
    }

    // 中间部分
    for (int n = 0; n < height; n++)
    {
        setCursorPosition(0, n + 2);
        setColor(COLOR_GRAY);
        printf("#");
        setColor(COLOR_WHITE);
        for (int j = 0; j < width; j++)
        {
            printf(" ");
        }
        setColor(COLOR_GRAY);
        printf("#");
    }

    // 下边界
    setCursorPosition(0, height + 2);
    for (int i = 0; i < width + 2; i++)
    {
        printf("#");
    }

    // 将光标移回左上角
    setCursorPosition(0, 0);
    setColor(COLOR_WHITE);
}

// 设置光标位置
void setCursorPosition(int x, int y)
{
    COORD Cursor;
    Cursor.X = x;
    Cursor.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cursor);
}

// 设置颜色
void setColor(int color)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// 检查位置是否有效（不靠近边界1格）
int is_position_valid(int x, int y)
{
    if (x <= 0 || x >= width - 1 || y <= 0 || y >= height - 1)
    {
        return 0;
    }

    snake *cur = head_node;
    while (cur)
    {
        if (cur->snake_x == x && cur->snake_y == y)
            return 0;
        cur = cur->next_node;
    }

    wall *w_cur = wall_head;
    while (w_cur)
    {
        if (w_cur->wall_x == x && w_cur->wall_y == y)
            return 0;
        w_cur = w_cur->next;
    }

    if ((special_food_active && x == special_food_x && y == special_food_y) ||
        (moldy_food_active && x == moldy_food_x && y == moldy_food_y) ||
        (x == point_x && y == point_y))
    {
        return 0;
    }
    return 1;
}

// 生成普通食物
void setpoint()
{
    srand((unsigned int)time(NULL));

    int attempts = 0;
    do
    {
        point_x = rand() % width;
        point_y = rand() % height;
        attempts++;

        if (!is_position_valid(point_x, point_y))
        {
            continue;
        }

        snake *current = head_node;
        int valid = 1;
        while (current != NULL)
        {
            if (point_x == current->snake_x && point_y == current->snake_y)
            {
                valid = 0;
                break;
            }
            current = current->next_node;
        }
        if (!valid)
        {
            continue;
        }

        wall *w_current = wall_head;
        while (w_current != NULL)
        {
            if (point_x == w_current->wall_x && point_y == w_current->wall_y)
            {
                valid = 0;
                break;
            }
            w_current = w_current->next;
        }
        if (!valid)
        {
            continue;
        }

        if (special_food_active && point_x == special_food_x && point_y == special_food_y)
        {
            valid = 0;
        }
        if (!valid)
        {
            continue;
        }

        if (moldy_food_active && point_x == moldy_food_x && point_y == moldy_food_y)
        {
            valid = 0;
        }

        if (valid)
        {
            break;
        }

    } while (attempts < 500);

    setCursorPosition(point_x + 1, point_y + 2);
    setColor(COLOR_RED);
    printf("G");
    setColor(COLOR_WHITE);
    fflush(stdout);
}

// 生成特殊食物T
void generate_special_food()
{
    int attempts = 0;
    do
    {
        special_food_x = rand() % width;
        special_food_y = rand() % height;
        attempts++;

        if (!is_position_valid(special_food_x, special_food_y))
        {
            continue;
        }

        snake *current = head_node;
        int valid = 1;
        while (current != NULL)
        {
            if (special_food_x == current->snake_x && special_food_y == current->snake_y)
            {
                valid = 0;
                break;
            }
            current = current->next_node;
        }
        if (!valid)
        {
            continue;
        }

        wall *w_current = wall_head;
        while (w_current != NULL)
        {
            if (special_food_x == w_current->wall_x && special_food_y == w_current->wall_y)
            {
                valid = 0;
                break;
            }
            w_current = w_current->next;
        }
        if (!valid)
        {
            continue;
        }

        if (special_food_x == point_x && special_food_y == point_y)
        {
            valid = 0;
        }
        if (!valid)
        {
            continue;
        }

        if (moldy_food_active && special_food_x == moldy_food_x && special_food_y == moldy_food_y)
        {
            valid = 0;
        }

        if (valid)
        {
            break;
        }

    } while (attempts < 500);

    special_food_active = 1;
    setCursorPosition(special_food_x + 1, special_food_y + 2);
    setColor(COLOR_RED);
    printf("T");
    setColor(COLOR_WHITE);
    fflush(stdout);
}

// 生成发霉食物W
void generate_moldy_food()
{
    int attempts = 0;
    do
    {
        moldy_food_x = rand() % width;
        moldy_food_y = rand() % height;
        attempts++;

        if (!is_position_valid(moldy_food_x, moldy_food_y))
        {
            continue;
        }

        snake *current = head_node;
        int valid = 1;
        while (current != NULL)
        {
            if (moldy_food_x == current->snake_x && moldy_food_y == current->snake_y)
            {
                valid = 0;
                break;
            }
            current = current->next_node;
        }
        if (!valid)
        {
            continue;
        }

        wall *w_current = wall_head;
        while (w_current != NULL)
        {
            if (moldy_food_x == w_current->wall_x && moldy_food_y == w_current->wall_y)
            {
                valid = 0;
                break;
            }
            w_current = w_current->next;
        }
        if (!valid)
        {
            continue;
        }

        if (moldy_food_x == point_x && moldy_food_y == point_y)
        {
            valid = 0;
        }
        if (!valid)
        {
            continue;
        }

        if (special_food_active && moldy_food_x == special_food_x && moldy_food_y == special_food_y)
        {
            valid = 0;
        }

        if (valid)
        {
            break;
        }

    } while (attempts < 500);

    moldy_food_active = 1;
    setCursorPosition(moldy_food_x + 1, moldy_food_y + 2);
    setColor(COLOR_RED);
    printf("W");
    setColor(COLOR_WHITE);
    fflush(stdout);
}

// 移动蛇头
void move_snake()
{
    // 保存旧位置用于清除
    int old_x = snake_x;
    int old_y = snake_y;

    // 根据方向移动
    switch (mode)
    {
    case 1:
        snake_y--;
        break; // 上
    case 2:
        snake_y++;
        break; // 下
    case 3:
        snake_x--;
        break; // 左
    case 4:
        snake_x++;
        break; // 右
    }

    // 清除旧蛇头位置
    setCursorPosition(old_x + 1, old_y + 2);
    printf(" ");
    fflush(stdout);

    // 绘制新蛇头
    setCursorPosition(snake_x + 1, snake_y + 2);
    setColor(COLOR_GREEN);
    switch (mode)
    {
    case 1:
        printf("↑");
        break;
    case 2:
        printf("↓");
        break;
    case 3:
        printf("←");
        break;
    case 4:
        printf("→");
        break;
    }
    setColor(COLOR_WHITE);
    fflush(stdout);
}

// 键盘输入处理
void key_scanf()
{
    if (_kbhit())
    {
        int key = _getch();

        if (key == 0 || key == 0xE0)
        {
            key = _getch();
            switch (key)
            {
            case 75:
                if (mode != 4)
                    mode = 3;
                break;
            case 77:
                if (mode != 3)
                    mode = 4;
                break;
            case 72:
                if (mode != 2)
                    mode = 1;
                break;
            case 80:
                if (mode != 1)
                    mode = 2;
                break;
            }
            return;
        }

        switch (tolower(key))
        {
        case 'a':
            if (mode != 4)
                mode = 3;
            break;
        case 'd':
            if (mode != 3)
                mode = 4;
            break;
        case 'w':
            if (mode != 2)
                mode = 1;
            break;
        case 's':
            if (mode != 1)
                mode = 2;
            break;
        case ' ':
            pause_game();
            break;
        }
    }
}

// 游戏结束
void gameover()
{
    // 关闭音乐
    mciSendStringFunc("close bgmusic", NULL, 0, NULL);
    cleanupAudio();
    playSoundEffect("fail.WAV");

    system("cls");
    // 在游戏区域内显示结束信息
    setCursorPosition(width / 2, height / 2 + 2);
    setColor(COLOR_RED);
    printf("游戏结束");
    setCursorPosition(width / 2 - 3, height / 2 + 3);
    setColor(COLOR_YELLOW);
    printf("您的分数为: %d", score);
    setCursorPosition(0, height + 4);
    setColor(COLOR_WHITE);
    Sleep(3000);
}

// 添加蛇身节点
void add_bodynode(int x, int y)
{
    snake *node = (snake *)malloc(sizeof(snake));
    node->snake_x = x;
    node->snake_y = y;
    node->next_node = head_node;
    head_node = node;
}

// 清除内存
void clear_node()
{
    snake *temp = head_node;
    while (temp != NULL)
    {
        snake *next = temp->next_node;
        free(temp);
        temp = next;
    }
    head_node = NULL;
}

// 绘制蛇身
void show_body()
{
    if (head_node == NULL)
    {
        return;
    }

    snake *temp = head_node->next_node; // 跳过蛇头
    int count = 0;

    while (temp != NULL && count < length - 1)
    {
        setCursorPosition(temp->snake_x + 1, temp->snake_y + 2);
        setColor(COLOR_GREEN);
        printf("O");
        setColor(COLOR_WHITE);
        temp = temp->next_node;
        count++;
    }

    // 清除多余的蛇身
    while (temp != NULL)
    {
        setCursorPosition(temp->snake_x + 1, temp->snake_y + 2);
        printf(" ");
        fflush(stdout);
        temp = temp->next_node;
    }
}

// 清除多余节点
void clearafter(snake *last)
{
    if (last == NULL || last->next_node == NULL)
    {
        return;
    }

    snake *temp = last->next_node;
    last->next_node = NULL;

    while (temp != NULL)
    {
        setCursorPosition(temp->snake_x + 1, temp->snake_y + 2);
        printf(" ");
        fflush(stdout);
        snake *next_node = temp->next_node;
        free(temp);
        temp = next_node;
    }
}

// 更新蛇身位置
void update_snake_body()
{
    // 添加新的蛇头位置
    add_bodynode(snake_x, snake_y);

    // 如果超过长度，删除多余的节点
    if (length > 0)
    {
        snake *temp = head_node;
        for (int i = 0; i < length && temp != NULL; i++)
        {
            if (temp->next_node == NULL)
            {
                break;
            }
            temp = temp->next_node;
        }

        if (temp != NULL)
        {
            clearafter(temp);
        }
    }
}

// 添加墙
void add_wall(int x, int y)
{
    wall *new_wall = (wall *)malloc(sizeof(wall));
    new_wall->wall_x = x;
    new_wall->wall_y = y;
    new_wall->next = wall_head;
    wall_head = new_wall;
}

// 清除墙
void clear_walls()
{
    wall *temp = wall_head;
    while (temp != NULL)
    {
        setCursorPosition(temp->wall_x + 1, temp->wall_y + 2);
        printf(" ");
        fflush(stdout);
        wall *next = temp->next;
        free(temp);
        temp = next;
    }
    wall_head = NULL;
}

// 显示墙
void show_walls()
{
    wall *temp = wall_head;
    while (temp != NULL)
    {
        setCursorPosition(temp->wall_x + 1, temp->wall_y + 2);
        setColor(COLOR_GRAY);
        printf("#");
        setColor(COLOR_WHITE);
        temp = temp->next;
    }
}

// 检查是否撞到墙
int check_wall_collision(int x, int y)
{
    wall *temp = wall_head;
    while (temp != NULL)
    {
        if (temp->wall_x == x && temp->wall_y == y)
        {
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}

// 蜕皮功能
void shed_skin()
{
    playSoundEffect("up.WAV");

    if (length < 2)
    {
        return;
    }

    // 计算蜕皮点
    int shed_point = length / 2;

    // 找到蜕皮点
    snake *temp = head_node;
    for (int i = 0; i < shed_point && temp != NULL; i++)
    {
        if (temp->next_node == NULL)
        {
            break;
        }
        temp = temp->next_node;
    }

    // 将蜕皮点后的蛇身变为墙
    if (temp != NULL && temp->next_node != NULL)
    {
        snake *wall_start = temp->next_node;
        temp->next_node = NULL;

        // 将后半段蛇身添加到墙中
        snake *current = wall_start;
        while (current != NULL)
        {
            setCursorPosition(current->snake_x + 1, current->snake_y + 2);
            printf(" ");
            fflush(stdout);

            add_wall(current->snake_x, current->snake_y);

            setCursorPosition(current->snake_x + 1, current->snake_y + 2);
            setColor(COLOR_GRAY);
            printf("#");
            setColor(COLOR_WHITE);
            fflush(stdout);

            snake *next = current->next_node;
            free(current);
            current = next;
        }

        // 更新蛇速度
        if (shed_point + 1 < 10 && length >= 10)
        {
            speed *= rate;
            fast_speed *= rate;
            normal_speed *= rate;
        }

        // 更新蛇身长度
        length = shed_point + 1;

        // 更新长度显示
        update_length_display();

        // 显示蜕皮提示（也移到游戏区域外）
        setCursorPosition(width + 5, 5);
        setColor(COLOR_YELLOW);
        printf("蜕皮!");
        Sleep(1500);
        setCursorPosition(width + 5, 5);
        setColor(COLOR_WHITE);
        printf("      ");
        fflush(stdout);
    }
}

// 创建随机墙方块
void create_random_walls()
{
    playSoundEffect("jieduan.WAV");

    int walls_created = 0;
    int attempts = 0;

    while (walls_created < 10 && attempts < 200)
    {
        int wall_x = rand() % width;
        int wall_y = rand() % height;
        attempts++;

        if (!is_position_valid(wall_x, wall_y))
        {
            continue;
        }

        snake *current = head_node;
        int valid = 1;
        while (current != NULL)
        {
            if (wall_x == current->snake_x && wall_y == current->snake_y)
            {
                valid = 0;
                break;
            }
            current = current->next_node;
        }
        if (!valid)
        {
            continue;
        }

        wall *w_current = wall_head;
        while (w_current != NULL)
        {
            if (wall_x == w_current->wall_x && wall_y == w_current->wall_y)
            {
                valid = 0;
                break;
            }
            w_current = w_current->next;
        }
        if (!valid)
        {
            continue;
        }

        if (wall_x == point_x && wall_y == point_y)
        {
            valid = 0;
        }
        if (!valid)
        {
            continue;
        }
        if (special_food_active && wall_x == special_food_x && wall_y == special_food_y)
        {
            valid = 0;
        }
        if (!valid)
        {
            continue;
        }

        if (moldy_food_active && wall_x == moldy_food_x && wall_y == moldy_food_y)
        {
            valid = 0;
        }

        if (valid)
        {
            add_wall(wall_x, wall_y);
            walls_created++;

            // 显示新墙
            setCursorPosition(wall_x + 1, wall_y + 2);
            setColor(COLOR_GRAY);
            printf("#");
            setColor(COLOR_WHITE);
            fflush(stdout);
        }
    }

    // 显示发霉食物提示（移到游戏区域外）
    setCursorPosition(width + 5, 5);
    setColor(COLOR_YELLOW);
    printf("发霉食物!");
    Sleep(1500);
    setCursorPosition(width + 5, 5);
    setColor(COLOR_WHITE);
    printf("          ");
    fflush(stdout);
}

// 以下为大佬提供的音频播放方式awa------------------------
// 初始化音频系统
int initAudio()
{
    if (!hWinMM)
    {
        hWinMM = LoadLibrary("winmm.dll");
        if (!hWinMM)
            return 0;

        mciSendStringFunc = (MCI_SEND_STRING)GetProcAddress(hWinMM, "mciSendStringA");
        if (!mciSendStringFunc)
            return 0;

        srand((unsigned int)time(NULL));
    }
    return 1;
}

// 清理音频系统
void cleanupAudio()
{
    if (hWinMM)
    {
        FreeLibrary(hWinMM);
        hWinMM = NULL;
        mciSendStringFunc = NULL;
    }
}

// 播放音效
void playSoundEffect(const char *filename)
{
    if (!initAudio())
        return;

    static int counter = 0;
    char alias[32];
    sprintf(alias, "sound%d", counter++);

    char command[256];
    sprintf(command, "open \"%s\" alias %s", filename, alias);
    mciSendStringFunc(command, NULL, 0, NULL);

    sprintf(command, "play %s", alias);
    mciSendStringFunc(command, NULL, 0, NULL);
}

// 播放背景音乐
void playBackgroundMusic(const char *filename)
{
    if (!initAudio())
        return;

    // 关闭前背景音乐
    mciSendStringFunc("close bgmusic", NULL, 0, NULL);

    // 确保设备释放
    Sleep(50);

    // 打开背景音乐文件
    char command[256];
    sprintf(command, "open \"%s\" alias bgmusic", filename);
    DWORD result = mciSendStringFunc(command, NULL, 0, NULL);

    if (result != 0)
    {
        return;
    }

    // 播放背景音乐
    result = mciSendStringFunc("play bgmusic notify", NULL, 0, NULL);

    if (result != 0)
    {
        mciSendStringFunc("close bgmusic", NULL, 0, NULL);
        return;
    }
}

// 手动循环背景音乐
void restartBackgroundMusicIfEnded(const char *filename)
{
    if (!initAudio())
        return;

    char status[64];
    mciSendStringFunc("status bgmusic mode", status, sizeof(status), NULL);

    // 重新播放
    if (strcmp(status, "stopped") == 0)
    {
        playBackgroundMusic(filename);
    }
}