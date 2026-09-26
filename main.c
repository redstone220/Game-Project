#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "raylib.h"

#define DEBUG 0
#define HITBOX 0
#define PLATFORM_WINDOW 1

#define WIDTH 1080
#define HEIGHT 720

#define MAX_LEADERBOARD 5   // how many top scores we keep
#define MAX_NAME_LEN 16     // max characters in a player's name (including null terminator)


// THIS AREA HOLDS SETTINGS VARIABLES THAT CAN BE TWEAKED TO BALANCE THE GAME
const float gravity = 1200; // px/s^2
const float pipe_vertical_distance = 200; // how much distance pipes are apart vertically
const float pipe_horizontal_distance = 400; // how much distance pipes are apart horizontally
const float flap_velocity = -450 ; // upward. That's why -ve
float game_speed = 300; // pipe speed. 
float difficulty = 2.1; // how much to increase after erach score


// THIS AREA DEALS WITH VARIABLE THAT NEEDS TO BE PASS IN EVERY FUNCTION
float dt = 0; // I hate passing it to every function
int inputPressed = 0; // global input tracking. updated in main loops
Vector2 mouse_position = {0, 0}; // tracks mouse position


float bird_rotation = 0; // current bird rotation. Updated in draw bird
float rotation_speed = 75; // how much to rotate per second

int animate = 0; // 1 - animates bird, base, backgrooound, controls rotation. 0 - stop all animation and rotation
int sound_on = 1; // 1 - sound on. 0 - sound off


typedef struct 
{
    Font determination; // konwing that the mouse might come out one day for the cheese fills you up with determination
} FontList;

typedef struct
{
    // holds all frames and total frame count for each bird
    int total_frames;
    Texture2D frames[3];
} BirdAnimation;

typedef struct 
{
    // Bundles texture together
    Texture2D background[2];
    Texture2D ground[1];
    Texture2D pipe[2];
    Texture2D numbers[10];

    Texture2D game_over_txt;
    Texture2D begin_menu;

    BirdAnimation bird[3];

    // ui
    Texture2D exit_ui;
    Texture2D pencil;
    Texture2D sound_on;
    Texture2D sound_off;
    Texture2D pause;
    Texture2D play;

    Texture2D credits_btn;    // generated flat-color background for the CREDITS button
    Texture2D how_to_play_btn; // generated flat-color background for the HOW TO PLAY button
    Texture2D edit_user;      // pencil-on-person icon used to edit the player's name
} Assets;

typedef struct 
{
    // which asset to load
    int background; // 0-day 1-night
    int ground; // 0-regular
    int bird; // 0-yellow 1-blue 2-red
    int pipe; // 0-green 1-red
} CurrentAssets;

typedef struct
{
    // pipe hold the position of gap between the pipes. top left corner of gap
    float x;
    float y;
    int passed;
} Pipe;

typedef struct
{
    // one row of the leaderboard
    char name[MAX_NAME_LEN];
    int score;
} LeaderboardEntry;

typedef struct
{
    // all sfx in one place
    Sound flap;
    Sound death;
    Sound point;
    Sound hit;

    // music
    Music bg;
} Sfx;

typedef struct {
    // all assets needs to be scaled. all are contained here
    Vector2 bird;
    Vector2 bird_hitbox; // smaller than visual bird
    Vector2 pipe;
    Vector2 background;
    Vector2 ground;
    Vector2 number;
    Vector2 game_over_txt;
    Vector2 menu;

    Vector2 exit_ui;
    Vector2 ui_icon;
    Vector2 pause_icon;
    Vector2 play_icon;
} Scale;

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER,
    STATE_END,
    STATE_CUSTOMIZATION,
    STATE_PAUSED,
    STATE_COUNTDOWN,
    STATE_COOLDOWN,
    STATE_NAME_ENTRY,
    STATE_CREDITS,
    STATE_HOW_TO_PLAY
} GameState;

Sfx sfx;
Scale scale;
Assets assets;
FontList font;
CurrentAssets current_assets;
GameState gamestate = STATE_MENU;

LeaderboardEntry leaderboard[MAX_LEADERBOARD]; // top scorers, sorted descending by score
int leaderboard_count = 0;                     // how many entries are currently filled in

char player_name[MAX_NAME_LEN] = "";           // name being typed on the name-entry screen
int name_length = 0;

void show_fps(void);

void load_textures(void);
Sfx load_sound(void);
void load_fonts(void);
Scale set_scales(void);
void set_current_asset(void);
void init_pipes(Pipe pipes[], int total_pipes);
void free_memory(void);

void draw_bird(int x, int y, float velocity);
void move_bird(float *pos_y, float velocity, float dt);
void update_velocity(float *velocity, float dt);

void draw_pipes(Pipe pipes[], int total_pipes);
void move_pipe(Pipe pipes[], int total_pipes, float dt);

void update_score(int *score, float bird_x, int total_pipes, Pipe pipes[]);
int check_death(float pos_x, float pos_y, Pipe pipes[], int total_pipes);

void menu(void);
void draw_menu_ui(int *start_game);
void draw_menu_extra_ui(int *start_game);

void draw_background(void);
void draw_ground(void);
void draw_score(int score);
void draw_game_over(void);
void draw_game_over_score(int score);
void draw_customization(void);
void draw_countdown(int count);

void load_leaderboard(void);
void save_leaderboard(void);
int qualifies_for_leaderboard(int score);
void insert_leaderboard_entry(const char *name, int score);
void draw_leaderboard(int start_y);
void update_name_entry(void);
void draw_name_entry(void);

void load_player_name(void);
void save_player_name(void);

Rectangle draw_info_panel(const char *title, const char *lines[], int line_count, Color box_color, Color text_color);
Rectangle draw_credits_panel(void);
Rectangle draw_how_to_play_panel(void);

void draw_text_outlined(Font font, const char *text, Vector2 position, Vector2 origin, float fontSize, float spacing, Color textColor, Color outlineColor, float outlineThickness);
void button(int *button_tracker, Texture2D tex, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color hover_tint);
void play_sound_pitch_variation(Sound sfx, float variation);

int main(){
    InitWindow(WIDTH, HEIGHT, "Flappy Bird");
    InitAudioDevice();
    if (PLATFORM_WINDOW == 1) SetWindowIcon((Image) LoadImage("icons/favicon-5.png"));
    SetTargetFPS(60); // vsync handle this automatically. but still miss one or two frames
    SetWindowState(FLAG_VSYNC_HINT);
    srand(time(NULL));
    
    load_leaderboard();
    load_player_name(); // remembers the name from the last time the game was played
    
    load_textures();
    set_current_asset();
    load_fonts();
    sfx = load_sound();
    scale = set_scales();

    // THIS AREA HOLDS VARIABLES FOR BIRD
    float pos_x = WIDTH * 0.212;
    float pos_y = HEIGHT/2;
    float velocity = -400; // bird upward or downward velocity

    int score =  0; // current score

    float countdown_timer = 0.0f; // tracks the time until play resumes
    float cooldown_timer = 0.0f; // turns off all controls for the duration.
    float initial_game_speed = game_speed;

    // THIS AREA DEALS WITH PIPES
    int total_pipes = WIDTH/pipe_horizontal_distance + 1;
    Pipe pipes[total_pipes];
    init_pipes(pipes, total_pipes);

    // main game loop
    while (!WindowShouldClose()){
        BeginDrawing();

        inputPressed = IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        mouse_position = GetMousePosition();
        dt = GetFrameTime();
    
        if (sound_on)
        {
            UpdateMusicStream(sfx.bg);

            if (!IsMusicStreamPlaying(sfx.bg)) {
                PlayMusicStream(sfx.bg);
            }
        }
        else
        {
            if (IsMusicStreamPlaying(sfx.bg))
            {
                PauseMusicStream(sfx.bg);
            }
        }       

        if (gamestate == STATE_MENU)
        {
            draw_background();
            draw_ground();
            menu();
            
            int start_game = inputPressed;
            draw_menu_ui(&start_game);
            draw_menu_extra_ui(&start_game);

            if (start_game && gamestate == STATE_MENU) {
                gamestate = STATE_PLAYING;
                animate = 1;
                bird_rotation = -30;
            }
        }

        else if (gamestate == STATE_PLAYING){
            draw_background();

            // Setup pause button dimensions to check collisions before movement updates
            Rectangle pause_dest = {WIDTH - assets.pause.width * scale.pause_icon.x - 20, 20, assets.pause.width * scale.pause_icon.x, assets.pause.height * scale.pause_icon.x};
            if (CheckCollisionPointRec(mouse_position, pause_dest)) {
                inputPressed = 0;
            }

            update_velocity(&velocity, dt); // passing by reference. much cleaner
            move_bird(&pos_y, velocity, dt);
            draw_bird(pos_x, pos_y, velocity);

            move_pipe(pipes, total_pipes, dt);
            draw_pipes(pipes, total_pipes);
    
            if (check_death(pos_x, pos_y, pipes, total_pipes)){
                draw_game_over();
                gamestate = STATE_COOLDOWN;
                cooldown_timer = 0.25f; // prevents the player from immediatetly getting into menu when dies
                animate = 0;
            }

            update_score(&score, pos_x, total_pipes, pipes);
            draw_score(score);
            
            draw_ground();

            // Draw pause button
            int pause_clicked = 0;
            button(&pause_clicked, assets.pause, (Rectangle){0, 0, assets.pause.width, assets.pause.height}, pause_dest, (Vector2){0,0}, 0.0f, (Color){200, 200, 200, 150});

            if (pause_clicked) {
                gamestate = STATE_PAUSED;
                animate = 0;
            }
        }

        else if (gamestate == STATE_PAUSED){
            draw_background();
            draw_pipes(pipes, total_pipes);
            draw_bird(pos_x, pos_y, velocity);
            draw_score(score);
            draw_ground();

            // Dim the screen slightly for visual feedback
            DrawRectangle(0, 0, WIDTH, HEIGHT, (Color){0, 0, 0, 100});

            int unpaused = 0;
            float icon_scale = scale.play_icon.x;
            Rectangle play_src = {0, 0, assets.play.width, assets.play.height};
            Rectangle play_dest = {WIDTH/2.0f, HEIGHT/2.0f, assets.play.width * icon_scale, assets.play.height * icon_scale};

            button(&unpaused, assets.play, play_src, play_dest, (Vector2){assets.play.width/2.0f, assets.play.height/2.0f}, 0.0f, (Color){200, 200, 200, 150});

            if (unpaused) {
                gamestate = STATE_COUNTDOWN;
                countdown_timer = 3.0f; // Start 3 second countdown
            }
        }

        else if (gamestate == STATE_COUNTDOWN){
            // Keep game state frozen while counting down
            draw_background();
            draw_pipes(pipes, total_pipes);
            draw_bird(pos_x, pos_y, velocity);
            draw_score(score);
            draw_ground();

            // Dim screen slightly to highlight countdown text
            DrawRectangle(0, 0, WIDTH, HEIGHT, (Color){0, 0, 0, 100});

            countdown_timer -= dt;
            int current_count = (int)ceil(countdown_timer);

            if (current_count > 0) {
                draw_countdown(current_count);
            }

            // Once timer finishes, return to PLAYING
            if (countdown_timer <= 0.0f) {
                gamestate = STATE_PLAYING;
                animate = 1;
            }
        }

        else if (gamestate == STATE_COOLDOWN){
            draw_background();
            draw_pipes(pipes, total_pipes);
            draw_bird(pos_x, pos_y, velocity);
            draw_ground();
            draw_game_over();
            draw_game_over_score(score);

            cooldown_timer -= dt;
            if (cooldown_timer <= 0) {
                // The player name was entered before the run started.
                // If the score qualifies, save it automatically.
                if (qualifies_for_leaderboard(score)) {
                    insert_leaderboard_entry(player_name, score);
                    save_leaderboard();
                }
                gamestate = STATE_END;
            }
        }

        else if (gamestate == STATE_NAME_ENTRY){
            draw_background();
            draw_ground();

            update_name_entry();
            draw_name_entry();

            if (IsKeyPressed(KEY_ENTER)) {
                if (name_length == 0) { // don't allow saving a blank name
                    snprintf(player_name, MAX_NAME_LEN, "Player");
                    name_length = (int)strlen(player_name);
                }
                save_player_name();
                gamestate = STATE_MENU;
            }
        }

        else if (gamestate == STATE_CREDITS){
            draw_background();
            draw_ground();
            menu();

            Rectangle panel_box = draw_credits_panel();

            float exit_h = assets.exit_ui.height * scale.exit_ui.y;
            float exit_center_y = panel_box.y + panel_box.height + 20.0f + exit_h / 2.0f;
            float max_center_y = HEIGHT - exit_h / 2.0f - 20.0f; // keep it on-screen even if the panel grows tall
            if (exit_center_y > max_center_y) exit_center_y = max_center_y;

            int exit_pressed = 0;
            button(
                &exit_pressed,
                assets.exit_ui, 
                (Rectangle){0,0,assets.exit_ui.width,assets.exit_ui.height}, 
                (Rectangle){(WIDTH/2), exit_center_y, 
                (assets.exit_ui.width*scale.exit_ui.x), 
                (assets.exit_ui.height*scale.exit_ui.y)}, 
                (Vector2){assets.exit_ui.width * scale.exit_ui.x/2.0f, assets.exit_ui.height * scale.exit_ui.y/2.0f},
                0.0f,
                (Color){100, 100, 100, 100}
            ); // <-- exit.png (assets.exit_ui) button #1: closes the CREDITS panel, back to STATE_MENU
            if (exit_pressed){
                gamestate = STATE_MENU;
            }
        }

        else if (gamestate == STATE_HOW_TO_PLAY){
            draw_background();
            draw_ground();
            menu();

            Rectangle panel_box = draw_how_to_play_panel();

            float exit_h = assets.exit_ui.height * scale.exit_ui.y;
            float exit_center_y = panel_box.y + panel_box.height + 20.0f + exit_h / 2.0f;
            float max_center_y = HEIGHT - exit_h / 2.0f - 20.0f; // keep it on-screen even if the panel grows tall
            if (exit_center_y > max_center_y) exit_center_y = max_center_y;

            int exit_pressed = 0;
            button(
                &exit_pressed,
                assets.exit_ui, 
                (Rectangle){0,0,assets.exit_ui.width,assets.exit_ui.height}, 
                (Rectangle){(WIDTH/2), exit_center_y, 
                (assets.exit_ui.width*scale.exit_ui.x), 
                (assets.exit_ui.height*scale.exit_ui.y)}, 
                (Vector2){assets.exit_ui.width * scale.exit_ui.x/2.0f, assets.exit_ui.height * scale.exit_ui.y/2.0f},
                0.0f,
                (Color){100, 100, 100, 100}
            ); // <-- exit.png (assets.exit_ui) button #2: closes the HOW TO PLAY panel, back to STATE_MENU
            if (exit_pressed){
                gamestate = STATE_MENU;
            }
        }

        else if (gamestate == STATE_END){
            draw_background();
            draw_pipes(pipes, total_pipes);
            draw_bird(pos_x, pos_y, velocity);
            draw_ground();
            draw_game_over();
            draw_game_over_score(score);
            draw_leaderboard(455);
            
            // re-setting
            if (inputPressed){
                init_pipes(pipes, total_pipes);
                score = 0;
                pos_y = HEIGHT/2;
                velocity = -400;
                dt = 0;
                gamestate = STATE_MENU;
                animate = 0;
                bird_rotation = 0; 
                game_speed = initial_game_speed;
            }
        }

        else if (gamestate == STATE_CUSTOMIZATION){
            animate = 0;
            draw_background();
            draw_ground();

            int exit_pressed = 0;
            draw_customization();
            button(
                &exit_pressed,
                assets.exit_ui, 
                (Rectangle){0,0,assets.exit_ui.width,assets.exit_ui.height}, 
                (Rectangle){(WIDTH/2), (HEIGHT - assets.exit_ui.height*scale.exit_ui.y/2 -20), 
                (assets.exit_ui.width*scale.exit_ui.x), 
                (assets.exit_ui.height*scale.exit_ui.y)}, 
                (Vector2){assets.exit_ui.width * scale.exit_ui.x/2.0f, assets.exit_ui.height * scale.exit_ui.y/2.0f},
                0.0f,
                (Color){100, 100, 100, 100}
            ); // <-- exit.png (assets.exit_ui) button #3: closes the CUSTOMIZATION screen, back to STATE_MENU (this one was already in your original code)
            if (exit_pressed){
                gamestate = STATE_MENU;
                animate = 0;
                bird_rotation = 0;
            }
        }

        show_fps();
        EndDrawing();
    }
    
    free_memory();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}


void show_fps(void){
    // for debugging. Also looks cool
    int fps = GetFPS();
    char fps_info[50];
    snprintf(fps_info, sizeof(fps_info), "FPS = %d", fps);
    DrawText(fps_info, 20, HEIGHT-30, 20, LIGHTGRAY);
}


void load_leaderboard(void){
    /*
        Loads the saved leaderboard from saves/leaderboard.txt.
        File format is one "name score" pair per line, already sorted highest first.
        If the file doesn't exist yet (first run), leaderboard just stays empty.
    */
    leaderboard_count = 0;

    FILE *fp = fopen("saves/leaderboard.txt", "r");
    if (fp == NULL) return;

    char name_buf[MAX_NAME_LEN];
    int score_buf;
    while (leaderboard_count < MAX_LEADERBOARD && fscanf(fp, "%15s %d", name_buf, &score_buf) == 2){
        strncpy(leaderboard[leaderboard_count].name, name_buf, MAX_NAME_LEN - 1);
        leaderboard[leaderboard_count].name[MAX_NAME_LEN - 1] = '\0';
        leaderboard[leaderboard_count].score = score_buf;
        leaderboard_count++;
    }
    fclose(fp);
}


void save_leaderboard(void){
    /*
        Writes the current leaderboard array to saves/leaderboard.txt, one "name score" per line.
    */
    FILE *fp = fopen("saves/leaderboard.txt", "w");
    if (fp == NULL) return;

    for (int i = 0; i < leaderboard_count; i++){
        fprintf(fp, "%s %d\n", leaderboard[i].name, leaderboard[i].score);
    }
    fclose(fp);
}


void load_player_name(void){
    /*
        Loads the player's saved name from saves/player_name.txt.The very first time
        the game is run (no save file yet) it just falls back to "Player" - the
        player can rename themselves any time from the menu's edit button.
    */
    FILE *fp = fopen("saves/player_name.txt", "r");
    if (fp == NULL || fscanf(fp, "%15s", player_name) != 1){
        snprintf(player_name, MAX_NAME_LEN, "Player");
    }
    if (fp != NULL) fclose(fp);

    name_length = (int)strlen(player_name);
}


void save_player_name(void){
    /*
        Saves the player's current name to saves/player_name.txt, so the same
        name is remembered next time the game is opened.
    */
    FILE *fp = fopen("saves/player_name.txt", "w");
    if (fp == NULL) return;
    fprintf(fp, "%s", player_name);
    fclose(fp);
}


int qualifies_for_leaderboard(int score){
    /*
        If this name already exists, only a higher score should update it.
        This prevents the same player from appearing multiple times.

        If the name does not exist, use the normal Top 5 qualification rules.
    */
    if (score <= 0) return 0;

    for (int i = 0; i < leaderboard_count; i++){
        if (strcmp(leaderboard[i].name, player_name) == 0){
            return score > leaderboard[i].score;
        }
    }

    if (leaderboard_count < MAX_LEADERBOARD) return 1;

    return score > leaderboard[MAX_LEADERBOARD - 1].score;
}


void insert_leaderboard_entry(const char *name, int score){
    /*
        If the name already exists, replace its old score only when the
        new score is higher.

        If the name does not exist, add a new entry and keep the
        leaderboard sorted from highest score to lowest score.
    */

    // First check for an existing player with the same name.
    for (int i = 0; i < leaderboard_count; i++){
        if (strcmp(leaderboard[i].name, name) == 0){

            // Same name: update only if the new score is higher.
            if (score > leaderboard[i].score){
                leaderboard[i].score = score;

                // Move the updated entry upward if necessary.
                for (int j = i; j > 0 && leaderboard[j].score > leaderboard[j - 1].score; j--){
                    LeaderboardEntry temp = leaderboard[j];
                    leaderboard[j] = leaderboard[j - 1];
                    leaderboard[j - 1] = temp;
                }
            }

            return;
        }
    }

    // Name does not exist, so add a new player.
    int insert_at = leaderboard_count < MAX_LEADERBOARD ? leaderboard_count : MAX_LEADERBOARD - 1;

    strncpy(leaderboard[insert_at].name, name, MAX_NAME_LEN - 1);
    leaderboard[insert_at].name[MAX_NAME_LEN - 1] = '\0';
    leaderboard[insert_at].score = score;

    if (leaderboard_count < MAX_LEADERBOARD) leaderboard_count++;

    // Keep the leaderboard sorted highest-score-first.
    for (int i = insert_at; i > 0 && leaderboard[i].score > leaderboard[i - 1].score; i--){
        LeaderboardEntry temp = leaderboard[i];
        leaderboard[i] = leaderboard[i - 1];
        leaderboard[i - 1] = temp;
    }
}


void update_name_entry(void){
    /*
        Reads keyboard input on the name-entry screen and builds up player_name.
        Only letters and digits are accepted, so the "name score" file format never
        breaks on a stray space.
    */
    int key = GetCharPressed();
    while (key > 0){
        int is_letter = (key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z');
        int is_digit = (key >= '0' && key <= '9');

        if ((is_letter || is_digit) && name_length < MAX_NAME_LEN - 1){
            player_name[name_length] = (char)key;
            name_length++;
            player_name[name_length] = '\0';
        }
        key = GetCharPressed(); // there can be more than one character typed per frame
    }

    if (IsKeyPressed(KEY_BACKSPACE) && name_length > 0){
        name_length--;
        player_name[name_length] = '\0';
    }
}


void draw_name_entry(void){
    /*
        Lets the player type/edit their name. Reached from the menu's edit button,
        pre-filled with their current name. Same determination font used
        everywhere else (score, leaderboard, panels).
    */
    DrawRectangle(0, 0, WIDTH, HEIGHT, (Color){0, 0, 0, 120});

    const float title_font_size = 72.0f;
    const float name_font_size = 58.0f;
    const float hint_font_size = 30.0f;
    const float spacing = 2.0f;

    const char *title = "EDIT YOUR NAME";
    Vector2 title_size = MeasureTextEx(font.determination, title, title_font_size, spacing);
    draw_text_outlined(
        font.determination, title,
        (Vector2){WIDTH / 2.0f, 245.0f},
        (Vector2){title_size.x / 2.0f, title_size.y / 2.0f},
        title_font_size, spacing,
        GOLD, BLACK, 3.0f
    );

    Rectangle box = {WIDTH / 2.0f - 330.0f, 320.0f, 660.0f, 105.0f};
    DrawRectangleRec(box, WHITE);
    DrawRectangleLinesEx(box, 4.0f, BLACK);

    Vector2 name_size = MeasureTextEx(font.determination, player_name, name_font_size, spacing);
    draw_text_outlined(
        font.determination, player_name,
        (Vector2){WIDTH / 2.0f, box.y + box.height / 2.0f},
        (Vector2){name_size.x / 2.0f, name_size.y / 2.0f},
        name_font_size, spacing,
        BLACK, WHITE, 0.0f
    );

    // Blinking cursor after the typed name.
    if (((int)(GetTime() * 2.0)) % 2 == 0){
        float cursor_x = WIDTH / 2.0f + name_size.x / 2.0f + 5.0f;
        DrawRectangle((int)cursor_x, (int)box.y + 20, 4, (int)box.height - 40, BLACK);
    }

    const char *hint = "TYPE YOUR NAME  •  PRESS ENTER TO SAVE";
    Vector2 hint_size = MeasureTextEx(font.determination, hint, hint_font_size, spacing);
    draw_text_outlined(
        font.determination, hint,
        (Vector2){WIDTH / 2.0f, 485.0f},
        (Vector2){hint_size.x / 2.0f, hint_size.y / 2.0f},
        hint_font_size, spacing,
        WHITE, BLACK, 2.0f
    );
}

void draw_leaderboard(int start_y){
    /*
        Draws the saved Top 5 leaderboard using the determination font.
    */
    const float title_font_size = 62.0f;
    const float row_font_size = 43.0f;
    const float spacing = 1.5f;
    const int row_height = 40;

    const char *title = "TOP 5";
    Vector2 title_size = MeasureTextEx(font.determination, title, title_font_size, spacing);
    draw_text_outlined(
        font.determination, title,
        (Vector2){WIDTH / 2.0f, (float)start_y},
        (Vector2){title_size.x / 2.0f, title_size.y / 2.0f},
        title_font_size, spacing,
        GOLD, BLACK, 2.5f
    );

    if (leaderboard_count == 0){
        const char *empty_msg = "NO SCORES YET";
        Vector2 empty_size = MeasureTextEx(font.determination, empty_msg, row_font_size, spacing);
        draw_text_outlined(
            font.determination, empty_msg,
            (Vector2){WIDTH / 2.0f, (float)(start_y + row_height)},
            (Vector2){empty_size.x / 2.0f, empty_size.y / 2.0f},
            row_font_size, spacing,
            WHITE, BLACK, 1.5f
        );
        return;
    }

    for (int i = 0; i < leaderboard_count; i++){
        char row[64];
        snprintf(row, sizeof(row), "%d. %-16s %d", i + 1, leaderboard[i].name, leaderboard[i].score);

        Vector2 row_size = MeasureTextEx(font.determination, row, row_font_size, spacing);
        draw_text_outlined(
            font.determination, row,
            (Vector2){WIDTH / 2.0f, (float)(start_y + row_height * (i + 1))},
            (Vector2){row_size.x / 2.0f, row_size.y / 2.0f},
            row_font_size, spacing,
            WHITE, BLACK, 1.5f
        );
    }
}


Rectangle draw_info_panel(const char *title, const char *lines[], int line_count, Color box_color, Color text_color){
    /*
        Generic "box with a title and a few lines of text" popup, used by both
        the credits screen and the how-to-play screen. Each line's font size is
        auto-shrunk to fit inside the box's width, so long lines (like URLs)
        never spill past the box edges. Returns the box rectangle so callers
        can attach other UI (like an exit button) right below it.
    */
    DrawRectangle(0, 0, WIDTH, HEIGHT, (Color){0, 0, 0, 150}); // dim whatever is behind the panel

    float line_height = 36.0f;
    float box_w = 940.0f;
    float box_h = 150.0f + line_count * line_height;
    Rectangle box = {WIDTH / 2.0f - box_w / 2.0f, HEIGHT / 2.0f - box_h / 2.0f, box_w, box_h};

    DrawRectangleRec(box, box_color);
    DrawRectangleLinesEx(box, 5.0f, BLACK);

    float title_font_size = 50.0f;
    float title_spacing = 2.0f;
    Vector2 title_size = MeasureTextEx(font.determination, title, title_font_size, title_spacing);
    draw_text_outlined(
        font.determination, title,
        (Vector2){WIDTH / 2.0f, box.y + 55.0f},
        (Vector2){title_size.x / 2.0f, title_size.y / 2.0f},
        title_font_size, title_spacing,
        GOLD, BLACK, 3.0f
    );

    float max_line_font_size = 28.0f;
    float min_line_font_size = 14.0f; // never shrink smaller than this - too small to read
    float line_spacing = 1.2f;
    float text_max_width = box_w - 60.0f; // small margin inside the box's left/right edges

    for (int i = 0; i < line_count; i++){
        if (lines[i][0] == '\0') continue; // blank line used just as a spacer

        float fitted_size = max_line_font_size;
        while (fitted_size > min_line_font_size &&
               MeasureTextEx(font.determination, lines[i], fitted_size, line_spacing).x > text_max_width){
            fitted_size -= 1.0f;
        }

        Vector2 line_size = MeasureTextEx(font.determination, lines[i], fitted_size, line_spacing);
        draw_text_outlined(
            font.determination, lines[i],
            (Vector2){WIDTH / 2.0f, box.y + 115.0f + i * line_height},
            (Vector2){line_size.x / 2.0f, line_size.y / 2.0f},
            fitted_size, line_spacing,
            text_color, BLACK, 1.2f
        );
    }

    return box;
}


Rectangle draw_credits_panel(void){
    const char *lines[] = {
        "Assets from github - https://github.com/samuelcust/flappy-bird-assets",
        "Font from - https://www.dafont.com/pix32.font",
        "pixeleted icons from - https://pixeliconlibrary.com/",
        "BGM- https://soundcloud.com/flappybirdagain",
        "",
        "Code by - ",
        "Eftehar Ahmed Shifat",
        "Md. Shahriar Ibne Alam"
    };
    int line_count = sizeof(lines) / sizeof(lines[0]); // always matches the array above, however many lines you add
    return draw_info_panel("CREDITS", lines, line_count, GetColor(0x2B2743FF), GetColor(0xFCE38AFF));
}


Rectangle draw_how_to_play_panel(void){
    const char *lines[] = {
        "PRESS SPACE TO FLY",
        "AVOID HITTING THE PIPES",
        "PASS A PIPE GAP FOR +1 SCORE"
    };
    int line_count = sizeof(lines) / sizeof(lines[0]);
    return draw_info_panel("HOW TO PLAY", lines, line_count, GetColor(0x1F4E4EFF), GetColor(0xA8E6CFFF));
}


void load_textures(void){
    /*
        Responsible for loading all texture. Must be called after InitWindow()
    */
    assets.background[0] = LoadTexture("sprites/background-day.png");
    assets.background[1] = LoadTexture("sprites/background-night.png");

    assets.ground[0] = LoadTexture("sprites/base.png");

    assets.pipe[0] = LoadTexture("sprites/pipe-green.png");
    assets.pipe[1] = LoadTexture("sprites/pipe-red.png");

    for (int i = 0; i < 10; i++){
        char file_name[20];
        snprintf(file_name, sizeof(file_name), "sprites/%d.png", i);
        assets.numbers[i] = LoadTexture(file_name);
    }

    BirdAnimation bird1 = {
        .frames = {
            LoadTexture("sprites/yellowbird-downflap.png"),
            LoadTexture("sprites/yellowbird-midflap.png"),
            LoadTexture("sprites/yellowbird-upflap.png")
        },
        .total_frames = 3
    };

    BirdAnimation bird2 = {
        .frames = {
            LoadTexture("sprites/bluebird-downflap.png"),
            LoadTexture("sprites/bluebird-midflap.png"),
            LoadTexture("sprites/bluebird-upflap.png")
        },
        .total_frames = 3
    };

    BirdAnimation bird3 = {
        .frames = {
            LoadTexture("sprites/redbird-downflap.png"),
            LoadTexture("sprites/redbird-midflap.png"),
            LoadTexture("sprites/redbird-upflap.png")
        },
        .total_frames = 3
    };

    assets.bird[0] = bird1;
    assets.bird[1] = bird2;
    assets.bird[2] = bird3;

    assets.game_over_txt = LoadTexture("sprites/gameover.png");
    assets.begin_menu = LoadTexture("sprites/message.png");

    assets.exit_ui = LoadTexture("icons/exit.png"); // exit.png loaded here - used by 3 buttons: CREDITS, HOW TO PLAY, CUSTOMIZATION
    assets.pencil = LoadTexture("icons/pencil-solid.png");
    assets.sound_on = LoadTexture("icons/sound-on-solid.png");
    assets.sound_off = LoadTexture("icons/sound-mute-solid.png");
    assets.pause = LoadTexture("icons/pause.png");
    assets.play = LoadTexture("icons/play.png");

    // small flat-color images, stretched by button() to whatever size the button needs
    Image credits_img = GenImageColor(4, 4, GetColor(0x5B3A29FF));   // warm brown
    assets.credits_btn = LoadTextureFromImage(credits_img);
    UnloadImage(credits_img);

    Image how_to_play_img = GenImageColor(4, 4, GetColor(0x1F4E4EFF)); // dark teal
    assets.how_to_play_btn = LoadTextureFromImage(how_to_play_img);
    UnloadImage(how_to_play_img);

    assets.edit_user = LoadTexture("icons/edit-user.png"); // save the uploaded icon here in your project
}


void load_fonts(){
    font.determination = LoadFontEx("fonts/determination/determination.ttf", 250, NULL, 0);
}


void set_current_asset(void){
    /*
        initiate asset handeler struct.
    */
    current_assets.background = 0; // 0-day 1-night
    current_assets.ground = 0; // 0-regular
    current_assets.bird = 0; // 0-yellow 1-blue 2-red
    current_assets.pipe = 0; // 0-green 1-red
}


Sfx load_sound(void){
    Sfx s = {
        .death = LoadSound("audio/die.wav"),
        .flap = LoadSound("audio/wing.wav"),
        .hit = LoadSound("audio/hit.wav"),
        .point = LoadSound("audio/point.wav"),
        .bg = LoadMusicStream("audio/bgm.mp3")
    };

    SetSoundVolume(s.point, 0.5f);
    SetMusicVolume(s.bg, 1.5f);

    return s;
}


Scale set_scales(void){
    /*
        Single source of truth for all scale. Used to dynamically scale assest without changing a lot of code.
    */
    Scale s = {
        .bird = (Vector2){2.0f, 2.0f},
        .bird_hitbox = (Vector2){1.5f, 1.5f}, // slightly smaller than the 2.0f visual scale for forgiveness
        .pipe = (Vector2){1.5f, 2.0f},
        .background = (Vector2){(float)HEIGHT/(float)assets.background[0].height, (float)HEIGHT/(float)assets.background[0].height}, // fills up the whole height
        .ground = {1.0f, 1.0f},
        .number = {1.5f, 1.5f},
        .game_over_txt = {2.5f, 2.5f},
        .menu = {2.0f, 2.0f},

        .exit_ui = {0.5f, 0.5f},
        .ui_icon = {0.4f, 0.4f},  // all ui have same scale
        .pause_icon = {0.33f, 0.33f},
        .play_icon = {1.5f, 1.5f}
    };
    return s;
}


void free_memory(void){
    /*
        Frees all textures and sound from VRAM
    */
    UnloadTexture(assets.background[0]);
    UnloadTexture(assets.background[1]);

    UnloadTexture(assets.ground[0]);

    UnloadTexture(assets.pipe[0]);
    UnloadTexture(assets.pipe[1]);
    
    UnloadTexture(assets.game_over_txt);
    UnloadTexture(assets.begin_menu);

    UnloadTexture(assets.exit_ui); // matches the LoadTexture("icons/exit.png") in load_textures()
    UnloadTexture(assets.pencil);
    UnloadTexture(assets.sound_on);
    UnloadTexture(assets.sound_off);
    UnloadTexture(assets.pause);
    UnloadTexture(assets.play);

    UnloadTexture(assets.credits_btn);
    UnloadTexture(assets.how_to_play_btn);
    UnloadTexture(assets.edit_user);

    for (int i = 0; i < 10; i++) {
        UnloadTexture(assets.numbers[i]);
    }

    for (int i=0; i<3; i++){
        for (int j=0; j<3; j++){
            UnloadTexture(assets.bird[i].frames[j]);
        }
    }

    UnloadSound(sfx.death);
    UnloadSound(sfx.flap);
    UnloadSound(sfx.hit);
    UnloadSound(sfx.point);
    UnloadMusicStream(sfx.bg);
    
    UnloadFont(font.determination);
}


void init_pipes(Pipe pipes[], int total_pipes){
    /*
        set-up pipe for the first time
    */
    for (int i = 0; i < total_pipes; i++){
        pipes[i].x =  WIDTH + (i+1)*pipe_horizontal_distance;
        // pipes[i].y = rand() % (HEIGHT - 400) + 50; // bar should be between 50 and (WIDTH-350) as bottom portion is ground
        pipes[i].y = GetRandomValue(50, HEIGHT - assets.ground[0].height*scale.ground.y - 60 - pipe_vertical_distance);
        pipes[i].passed = 0;
    }
}


void draw_background(void){
    /*
        draws background. If animate is on, meves background to left.
    */
    static float background_speed = 70; // for parallex effect
    static float backgroung_position = 0; // for parallex

    Texture2D background = assets.background[current_assets.background];
    float width = background.width;
    float height = background.height;

    float background_scale = scale.background.x;
    // printf("bgs = %f\n", scale.bird.x); // fu**k integer division

    Rectangle source = {0.0f, 0.0f, width, height};
    Vector2 orign = {0.0f, 0.0f};
    
    for (int i = 0; i < (int)(WIDTH/(width*background_scale)) + 2; i++){
        Rectangle dest = {
            width*background_scale*i - backgroung_position,
            0,
            width*background_scale,
            height*background_scale
        };
        DrawTexturePro(background, source, dest, orign, 0.0f, WHITE);
    }

    if (animate){
        backgroung_position += background_speed*dt;
        if (backgroung_position > width*background_scale) backgroung_position -= width*background_scale;
    }
}


void draw_ground(void){
    /*
        draws and moves ground. ground velocity is same as pipe velocity
    */
    static float base_poition = 0; // for parallex

    Texture2D ground = assets.ground[current_assets.ground];
    float width = ground.width;
    float height = ground.height;

    float ground_scale = scale.ground.x; // how much to scale height. Later math is used to not distort the image

    Rectangle source = {0.0f, 0.0f, width, height};
    Vector2 orign = {0.0f, height};
    
    for (int i = 0; i < (int)(WIDTH/(width*ground_scale)) + 2; i++){
        Rectangle dest = {
            width*ground_scale*i-base_poition,
            HEIGHT,
            width*ground_scale,
            height*ground_scale
        };
        DrawTexturePro( ground, source, dest, orign, 0.0f, WHITE);
    }
    // if it works do not touch it
    if (animate){
        base_poition += game_speed*dt;
        if (base_poition > width*ground_scale) base_poition -= width*ground_scale;
    }    
}


void draw_bird(int x, int y, float velocity){
    /*
        draws bird. If animate is on animates bird. Handels rotation. Draws bird hitbox.
    */
    float bird_scale = scale.bird.x;
    int bird_animation_frame = assets.bird[current_assets.bird].total_frames;
    // Texture2D bird_frames[] = assets.bird[current_assets.bird].frames;

    float animation_time = 0.4; // total time to finish an animation
    int frame_no = 1;
    if (animate) frame_no = (int)(GetTime()/(animation_time/bird_animation_frame)) % bird_animation_frame;
    Texture2D bird = assets.bird[current_assets.bird].frames[frame_no];
 
    if (animate && bird_rotation < 25){// temporary fix
        if (bird_rotation < 0) bird_rotation += rotation_speed  * dt;
        else if (bird_rotation >= 0 && bird_rotation < 7) bird_rotation +=  (rotation_speed/2.25) * dt;
        else if (bird_rotation >= 7 && bird_rotation < 15) bird_rotation +=  (rotation_speed/2) * dt;
        else bird_rotation += (rotation_speed/(log10(bird_rotation)*1.2)) * dt;
    }

    Rectangle source =  {
        0.0f,
        0.0f,
        (float)bird.width,
        (float)bird.height
    };

    Rectangle dest = {
        (float)x,
        (float)y,
        bird.width * bird_scale,
        bird.height * bird_scale
    };

    Vector2 origin = {bird.width*bird_scale/2, bird.height*bird_scale/2}; // ancoring to the middle point

    DrawTexturePro(bird, source, dest, origin, bird_rotation, WHITE);

    #if HITBOX
        float hw = bird.width * scale.bird_hitbox.x;
        float hh = bird.height * scale.bird_hitbox.y;
        DrawRectanglePro((Rectangle){(float)x, (float)y, hw, hh}, (Vector2){hw/2, hh/2}, 0.0f, (Color){100, 100, 100, 150});
    #endif
}


void menu(void){
    /*
        draws main menu.
    */
    float menu_scale = scale.menu.x;
    DrawTexturePro(
        assets.begin_menu,
        (Rectangle){0.0f, 0.0f, assets.begin_menu.width, assets.begin_menu.height},
        (Rectangle){WIDTH/2, HEIGHT/2, assets.begin_menu.width*menu_scale, assets.begin_menu.height*menu_scale},
        (Vector2){assets.begin_menu.width*menu_scale/2, assets.begin_menu.height*menu_scale/2},
        0.0f,
        WHITE
    );

    float swing_speed = 4.0f;       // How fast the bird swings up and down
    float swing_amplitude = 15.0f;  // How far it moves from the center (10px up, 10px down)
    float base_position = 460.0f;   // The center point of the hover
    float bird_position = base_position + (sin(GetTime() * swing_speed) * swing_amplitude);

    float bird_scale = scale.bird.x;
    int bird_animation_frame = assets.bird[current_assets.bird].total_frames;

    float animation_time = 0.4; // total time to finish an animation
    int frame_no = 1;
    frame_no = (int)(GetTime()/(animation_time/bird_animation_frame)) % bird_animation_frame;

    Texture2D bird = assets.bird[current_assets.bird].frames[frame_no];
    Rectangle source =  {0.0f, 0.0f, (float)bird.width, (float)bird.height};
    Rectangle dest = {(float)WIDTH/2, (float)bird_position, bird.width * bird_scale, bird.height * bird_scale};
    Vector2 origin = {bird.width*bird_scale/2, bird.height*bird_scale/2}; // ancoring to the middle point

    DrawTexturePro(bird, source, dest, origin, bird_rotation, WHITE);
}


void update_velocity(float *velocity, float dt){
    // uses v = u + gt to get velocity. If key pressed velocity instantly changes  to flap_velocity
    if (!inputPressed){
        *velocity += gravity * dt;
    }
    else{
        if (sound_on) play_sound_pitch_variation(sfx.flap, 0.15);
        bird_rotation = -30;
        *velocity = flap_velocity;
    }
}


void move_bird(float *pos_y, float velocity, float dt){
    // uses s = vt to calculate posion.
    // y = yo + vt; as coordinate system is inversed
    *pos_y += velocity*dt;
}


void draw_pipes(Pipe pipes[], int total_pipes){
    // Draws pipe in the pipes list
    float vertical_scale = scale.pipe.y;
    float horizontal_scale = scale.pipe.x;

    for (int i = 0; i < total_pipes; i++){
        Pipe pipe = pipes[i];
        
        // drawing the bottom portion
        DrawTexturePro(
            assets.pipe[current_assets.pipe],
            (Rectangle){0.0f, 0.0f, (float)assets.pipe[0].width, (float)assets.pipe[0].height},
            (Rectangle){pipe.x, (pipe.y + pipe_vertical_distance), (assets.pipe[0].width * horizontal_scale), (assets.pipe[0].height * vertical_scale)},
            (Vector2){0.0f, 0.0f},
            0.0f,
            WHITE
        );

        // drawing the top portion
        // shifting the origin to middle of pipe. Then rotating by 180° . Since now origin is middle point we have to draw with reference to the middle point
        DrawTexturePro(
            assets.pipe[current_assets.pipe],
            (Rectangle){0.0f, 0.0f, (float)assets.pipe[0].width, (float)assets.pipe[0].height},
            (Rectangle){(pipe.x + (assets.pipe[0].width * horizontal_scale)/2), (pipe.y - (assets.pipe[0].height*vertical_scale)/2), (assets.pipe[0].width * horizontal_scale), (assets.pipe[0].height * vertical_scale)},
            (Vector2){(assets.pipe[0].width*horizontal_scale)/2, (assets.pipe[0].height*vertical_scale)/2},
            180.0f,
            WHITE
        );
        
        #if HITBOX
            // Middle green debug rect (the safe gap)
            DrawRectangle(pipe.x, pipe.y, assets.pipe[0].width * horizontal_scale, pipe_vertical_distance, (Color){0, 100, 0, 50});
            
            // Top pipe red debug rect
            DrawRectangleLinesEx((Rectangle){pipe.x, 0, assets.pipe[0].width * horizontal_scale, pipe.y}, 3, (Color){200, 0, 0, 150});
            
            // Bottom pipe red debug rect
            DrawRectangleLinesEx((Rectangle){pipe.x, pipe.y + pipe_vertical_distance, assets.pipe[0].width * horizontal_scale, HEIGHT - (pipe.y + pipe_vertical_distance)}, 3, (Color){200, 0, 0, 150});
        #endif
    }
}


void move_pipe(Pipe pipes[], int total_pipes, float dt){
    // moves pipes to the left. Uses game_speed as pipe velocity. Recycles pipes that are out of screen
    // (Also draws debug rect )
    float furthest = 0;
    int move = -1;
    for (int i = 0; i < total_pipes; i++){
        pipes[i].x -= game_speed * dt;
        if (pipes[i].x + (assets.pipe[0].width * scale.pipe.x) <= 0) {
            move = i;
        }
        if (pipes[i].x > furthest) furthest = pipes[i].x;
    } 

    if (move != -1){
        pipes[move].x = furthest + pipe_horizontal_distance;
        pipes[move].y = GetRandomValue(50, HEIGHT - assets.ground[0].height*scale.ground.y - 60 - pipe_vertical_distance);
        pipes[move].passed = 0;
    }
}


void update_score(int *score, float bird_x, int total_pipes, Pipe pipes[]){
    /*
        updates score. (Also changes game velocity after scoring-Todo[Done])
    */
    for (int i = 0; i < total_pipes; i++){
        if (pipes[i].x <= bird_x && !pipes[i].passed){
            *score += 1;
            pipes[i].passed = 1;
            game_speed += difficulty;
            if (sound_on) play_sound_pitch_variation(sfx.point, 0.10);
        }
    }
}


void draw_score(int score) {
    /*
        uses number textures to draw score. update the score_height to change position. Aligned at center. 
        Update digit_scale to change scale
    */
    int score_height = 30; // where to draw. reffered from top
    float digit_scale = scale.number.x; // scale

    int digits[10];
    int count = 0;
    int temp = score;

    if (temp == 0) {
        digits[count++] = 0;
    } else {
        while (temp > 0) {
            digits[count++] = temp % 10;
            temp /= 10;
        }
    }

    float total_width = 0.0f; 
    for (int i = count - 1; i >= 0; i--) {
        int digit = digits[i];
        total_width += assets.numbers[digit].width * digit_scale;
    }

    float current_x = (WIDTH / 2.0f) - (total_width / 2.0f);

    for (int i = count - 1; i >= 0; i--) {
        int digit = digits[i];
        Texture2D tex = assets.numbers[digit];

        DrawTexturePro(
            tex,
            (Rectangle){0.0f, 0.0f, (float)tex.width, (float)tex.height},
            (Rectangle){current_x, (float)score_height, tex.width * digit_scale, tex.height * digit_scale}, 
            (Vector2){0.0f, 0.0f},
            0.0f,
            WHITE
        );

        current_x += tex.width * digit_scale;
    }
}


void draw_game_over_score(int score){
    /*
        Draws only the current score on the game-over screen.
        High score has been replaced by the Top 5 leaderboard.
    */
    int game_over_height = 120; // change in draw_game_over() if changed here.

    const float score_font_size = 82.0f;
    const float score_spacing = 2.0f;

    char score_string[50];
    snprintf(score_string, sizeof(score_string), "SCORE  %d", score);

    Vector2 score_text_size = MeasureTextEx(font.determination, score_string, score_font_size, score_spacing);
    Vector2 score_anchor = {score_text_size.x / 2.0f, score_text_size.y / 2.0f};

    draw_text_outlined(
        font.determination,
        score_string,
        (Vector2){WIDTH / 2.0f, game_over_height + 155},
        score_anchor,
        score_font_size,
        score_spacing,
        GetColor(0xFCA048FF),
        GetColor(0x543847FF),
        3.2f
    );
}

int check_death(float pos_x, float pos_y, Pipe pipes[], int total_pipes){
    /*
        checks if bird collides with ceiling, ground or pipe. Also draws pipe debug box(should've done this in draw pipes)
    */
    float bird_width = assets.bird[current_assets.bird].frames[0].width * scale.bird_hitbox.x;
    float bird_height = assets.bird[current_assets.bird].frames[0].height * scale.bird_hitbox.y;

    Rectangle bird_rec = {pos_x - bird_width/2, pos_y - bird_height/2, bird_width, bird_height};

    int death = 0;

    if (pos_y - bird_height/2 <= 0) death = 1; // checking if bird hits ceiling. Bird position is it's center point coordinate
    else if (pos_y + bird_height/2  >= (HEIGHT - assets.ground[0].height*scale.ground.x)) death = 1; // checking if hits floor. This behabiour is buggy. I'll fix it later

     // Pipe collision checks
    float pipe_w = assets.pipe[0].width * scale.pipe.x;
    for (int i=0; i < total_pipes; i++){
        Rectangle top_pipe_rec = {pipes[i].x, 0, pipe_w, pipes[i].y};
        Rectangle bottom_pipe_rec = {pipes[i].x, pipes[i].y + pipe_vertical_distance, pipe_w, HEIGHT - (pipes[i].y + pipe_vertical_distance)};
        
        if (CheckCollisionRecs(bird_rec, top_pipe_rec) || CheckCollisionRecs(bird_rec, bottom_pipe_rec)) {
            death = 1;
            break;
        }
    }
    if (sound_on) if (death) play_sound_pitch_variation(sfx.death, 0.15);
    
    return death;
}


void draw_game_over(void){
    // draws game over from texture
    int game_over_height = 120; // change in draw_game_over_score()

    float game_over_scale = scale.game_over_txt.x;
    DrawTexturePro(
        assets.game_over_txt,
        (Rectangle){0.0f, 0.0f, assets.game_over_txt.width, assets.game_over_txt.height},
        (Rectangle){WIDTH/2,game_over_height, assets.game_over_txt.width*game_over_scale, assets.game_over_txt.height*game_over_scale},
        (Vector2){assets.game_over_txt.width*game_over_scale/2, assets.game_over_txt.height*game_over_scale/2},
        0.0f,
        WHITE
    );
}


void draw_text_outlined(Font font, const char *text, Vector2 position, Vector2 origin, float fontSize, float spacing, Color textColor, Color outlineColor, float outlineThickness) {
    // Draw the outline by shifting the text in 8 directions (Up, Down, Left, Right, and Diagonals)
    Vector2 offsets[8] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1},   // Up, Down, Left, Right
        {-1, -1}, {1, -1}, {-1, 1}, {1, 1}  // Diagonals
    };

    for (int i = 0; i < 8; i++) {
        Vector2 offset_pos = {
            position.x + (offsets[i].x * outlineThickness),
            position.y + (offsets[i].y * outlineThickness)
        };
        DrawTextPro(font, text, offset_pos, origin, 0.0f, fontSize, spacing, outlineColor);
    }

    // Draw the main text perfectly centered on top
    DrawTextPro(font, text, position, origin, 0.0f, fontSize, spacing, textColor);
}


void button(int *button_tracker, Texture2D tex, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color hover_tint){
    Rectangle collision_rect = {
        dest.x - origin.x,
        dest.y - origin.y,
        dest.width,
        dest.height
    };

    float click_scale = 0.7f;

    if (CheckCollisionPointRec(mouse_position, collision_rect)){
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            *button_tracker = 1;
            DrawTexturePro(
                tex, 
                (Rectangle){source.x, source.y, source.width, source.height}, 
                (Rectangle){dest.x, dest.y, dest.width*click_scale, dest.height*click_scale}, 
                origin, 
                rotation, 
                WHITE
            );
            DrawRectanglePro(
                (Rectangle){dest.x, dest.y, dest.width*click_scale, dest.height*click_scale}, 
                origin, 
                rotation, 
                // (Color){50, 50, 50, 100}
                hover_tint
            );
        }
        else {
            DrawTexturePro(tex, source, dest, origin, rotation, WHITE);
            DrawRectanglePro(dest, origin, rotation, hover_tint);
        }
        return;
    }

    else DrawTexturePro(tex, source, dest, origin, rotation, WHITE);
    *button_tracker = 0;
}


void draw_menu_ui(int *start_game) {
    float icon_scale = scale.ui_icon.x;
    int pencil_clicked = 0;
    int sound_clicked = 0;
    
    // Draw Sound Button (Top Right)
    Texture2D sound_tex = sound_on ? assets.sound_on : assets.sound_off;
    Rectangle sound_src = {0, 0, sound_tex.width, sound_tex.height};
    float sound_w = sound_tex.width * icon_scale;
    float sound_h = sound_tex.height * icon_scale;
    Rectangle sound_dest = {WIDTH - sound_w - 20, 20, sound_w, sound_h};
    
    button(&sound_clicked, sound_tex, sound_src, sound_dest, (Vector2){0,0}, 0.0f, (Color){200, 200, 200, 150});

    // Draw Pencil Button (Below Sound)
    Rectangle pencil_src = {0, 0, assets.pencil.width, assets.pencil.height};
    float pencil_w = assets.pencil.width * icon_scale;
    float pencil_h = assets.pencil.height * icon_scale;
    Rectangle pencil_dest = {WIDTH - pencil_w - 20, sound_dest.y + sound_h + 20, pencil_w, pencil_h};
    
    button(&pencil_clicked, assets.pencil, pencil_src, pencil_dest, (Vector2){0,0}, 0.0f, (Color){200, 200, 200, 150});

    // Prevent game from starting when clicking UI
    if (CheckCollisionPointRec(mouse_position, pencil_dest) || CheckCollisionPointRec(mouse_position, sound_dest)) {
        *start_game = 0;
    }

    if (pencil_clicked) {
        gamestate = STATE_CUSTOMIZATION;
    } else if (sound_clicked) {
        sound_on = !sound_on;
    }
}


void draw_menu_extra_ui(int *start_game) {
    /*
        Draws the two extra menu buttons (top-left) and the player's name with
        its edit icon (top-center). Uses button() for every clickable element,
        same as the sound/pencil buttons above.
    */
    float btn_w = 230.0f, btn_h = 60.0f, gap = 15.0f;
    float start_x = 20.0f, start_y = 20.0f;
    float label_font_size = 26.0f, label_spacing = 1.5f;

    // HOW TO PLAY button
    Rectangle howto_dest = {start_x, start_y, btn_w, btn_h};
    int howto_clicked = 0;
    button(&howto_clicked, assets.how_to_play_btn, (Rectangle){0, 0, assets.how_to_play_btn.width, assets.how_to_play_btn.height}, howto_dest, (Vector2){0, 0}, 0.0f, (Color){255, 255, 255, 60});
    DrawRectangleLinesEx(howto_dest, 3, BLACK);

    const char *howto_label = "HOW TO PLAY";
    Vector2 howto_label_size = MeasureTextEx(font.determination, howto_label, label_font_size, label_spacing);
    draw_text_outlined(
        font.determination, howto_label,
        (Vector2){howto_dest.x + btn_w / 2.0f, howto_dest.y + btn_h / 2.0f},
        (Vector2){howto_label_size.x / 2.0f, howto_label_size.y / 2.0f},
        label_font_size, label_spacing, WHITE, BLACK, 1.5f
    );

    // CREDITS button (below How To Play)
    Rectangle credits_dest = {start_x, start_y + btn_h + gap, btn_w, btn_h};
    int credits_clicked = 0;
    button(&credits_clicked, assets.credits_btn, (Rectangle){0, 0, assets.credits_btn.width, assets.credits_btn.height}, credits_dest, (Vector2){0, 0}, 0.0f, (Color){255, 255, 255, 60});
    DrawRectangleLinesEx(credits_dest, 3, BLACK);

    const char *credits_label = "CREDITS";
    Vector2 credits_label_size = MeasureTextEx(font.determination, credits_label, label_font_size, label_spacing);
    draw_text_outlined(
        font.determination, credits_label,
        (Vector2){credits_dest.x + btn_w / 2.0f, credits_dest.y + btn_h / 2.0f},
        (Vector2){credits_label_size.x / 2.0f, credits_label_size.y / 2.0f},
        label_font_size, label_spacing, WHITE, BLACK, 1.5f
    );

    // Player name + edit icon (top-center)
    float name_font_size = 30.0f, name_spacing = 1.5f;
    char name_display[40];
    snprintf(name_display, sizeof(name_display), "PLAYER: %s", player_name);
    Vector2 name_size = MeasureTextEx(font.determination, name_display, name_font_size, name_spacing);

    float edit_scale = 0.12f; // edit-user.png is a large square icon, scaled down for inline use
    float edit_w = assets.edit_user.width * edit_scale;
    float edit_h = assets.edit_user.height * edit_scale;

    float cluster_gap = 14.0f;
    float cluster_w = name_size.x + cluster_gap + edit_w;
    float cluster_x = WIDTH / 2.0f - cluster_w / 2.0f;
    float cluster_y = 20.0f;

    draw_text_outlined(
        font.determination, name_display,
        (Vector2){cluster_x, cluster_y + edit_h / 2.0f},
        (Vector2){0.0f, name_size.y / 2.0f},
        name_font_size, name_spacing, WHITE, BLACK, 2.0f
    );

    Rectangle edit_dest = {cluster_x + name_size.x + cluster_gap, cluster_y, edit_w, edit_h};
    int edit_clicked = 0;
    button(&edit_clicked, assets.edit_user, (Rectangle){0, 0, assets.edit_user.width, assets.edit_user.height}, edit_dest, (Vector2){0, 0}, 0.0f, (Color){200, 200, 200, 150});

    // Prevent the game from starting when clicking any of this UI
    if (CheckCollisionPointRec(mouse_position, howto_dest) ||
        CheckCollisionPointRec(mouse_position, credits_dest) ||
        CheckCollisionPointRec(mouse_position, edit_dest)) {
        *start_game = 0;
    }

    if (howto_clicked) {
        gamestate = STATE_HOW_TO_PLAY;
    } else if (credits_clicked) {
        gamestate = STATE_CREDITS;
    } else if (edit_clicked) {
        gamestate = STATE_NAME_ENTRY; // pre-filled with the current name, ready to edit
    }
}


void draw_customization(void) {
    // Outline animation variables for customization screen
    static float anim_bird_x = 0;
    static float anim_pipe_x = 0;
    static float anim_bg_x = 0;
    static int first_cust_load = 1;

    float bird_y = 150, pipe_y = 350, bg_y = 550; // change here to change position of customization options
    float spacing = 200; // horizontal spacing

    Vector2 bird_outline_size = {90.0f, 90.0f};
    Vector2 pipe_outline_size = {90.0f, 120.0f};
    Vector2 bg_outline_size = {150.0f, 80.0f};

    float start_x_bird = WIDTH/2.0f - spacing;
    float start_x_pipe = WIDTH/2.0f - spacing/2.0f;
    float start_x_bg = WIDTH/2.0f - spacing/2.0f;

    if (first_cust_load) {
        // set-up these variables for the first time
        anim_bird_x = start_x_bird + current_assets.bird * spacing;
        anim_pipe_x = start_x_pipe + current_assets.pipe * spacing;
        anim_bg_x = start_x_bg + current_assets.background * spacing;
        first_cust_load = 0;
    }

    // Draw Section Titles
    float text_offset = 80; // how much above the text is drawn
    Color text_color = ORANGE;
    Color outline_color = BLACK;
    float font_size = 40;
    float text_spacing = 2;
    float outline_thickness = 1;
    draw_text_outlined(font.determination, "BIRD STYLE", (Vector2){WIDTH/2 - MeasureTextEx(font.determination, "BIRD STYLE", font_size, text_spacing).x/2 , bird_y - text_offset}, (Vector2){0.0f, 0.0f}, font_size, text_spacing, text_color, outline_color, outline_thickness);
    draw_text_outlined(font.determination, "PIPE STYLE", (Vector2){WIDTH/2 - MeasureTextEx(font.determination, "PIPE STYLE", font_size, text_spacing).x/2 , pipe_y - text_offset}, (Vector2){0.0f, 0.0f}, font_size, text_spacing, text_color, outline_color, outline_thickness);
    draw_text_outlined(font.determination, "BACKGROUND", (Vector2){WIDTH/2 - MeasureTextEx(font.determination, "BACKGROUND", font_size, text_spacing).x/2 , bg_y - text_offset}, (Vector2){0.0f, 0.0f}, font_size, text_spacing, text_color, outline_color, outline_thickness);

    float lerp_speed = 12.0f; // does what it says. creates a no linier animation
    anim_bird_x += ((start_x_bird + current_assets.bird * spacing) - anim_bird_x) * lerp_speed * dt;
    anim_pipe_x += ((start_x_pipe + current_assets.pipe * spacing) - anim_pipe_x) * lerp_speed * dt;
    anim_bg_x += ((start_x_bg + current_assets.background * spacing) - anim_bg_x) * lerp_speed * dt;

    // Selection outlines
    DrawRectangleLinesEx((Rectangle){anim_bird_x - 45, bird_y - 45, bird_outline_size.x, bird_outline_size.y}, 5, WHITE);
    DrawRectangleLinesEx((Rectangle){anim_pipe_x - 45, pipe_y - 20, pipe_outline_size.x, pipe_outline_size.y}, 5, WHITE);
    DrawRectangleLinesEx((Rectangle){anim_bg_x - 75, bg_y - 15, bg_outline_size.x, bg_outline_size.y}, 5, WHITE);

    // Draw Birds Selection (Animated & Scaled)
    for (int i = 0; i < 3; i++) {
        float x = start_x_bird + i * spacing;
        Rectangle hover_rect = {x - 45, bird_y - 45, bird_outline_size.x, bird_outline_size.y};
        
        int total_frames = assets.bird[i].total_frames;
        float anim_time = 0.4;
        int frame_no = (int)(GetTime() / (anim_time / total_frames)) % total_frames;
        
        float b_width = assets.bird[i].frames[frame_no].width * scale.bird.x;
        float b_height = assets.bird[i].frames[frame_no].height * scale.bird.y;
        
        Rectangle dest = {x - b_width/2, bird_y - b_height/2, b_width, b_height};
        DrawTexturePro(assets.bird[i].frames[frame_no], (Rectangle){0,0,assets.bird[i].frames[frame_no].width,assets.bird[i].frames[frame_no].height}, dest, (Vector2){0,0}, 0, WHITE);
        
        if (CheckCollisionPointRec(mouse_position, hover_rect) && i != current_assets.bird) {
            DrawRectangleLinesEx(hover_rect, 3, GRAY);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) current_assets.bird = i;
        }
    }

    // Draw Pipes Selection
    for (int i = 0; i < 2; i++) {
        float x = start_x_pipe + i * spacing;
        Rectangle hover_rect = {x - 45, pipe_y - 20, pipe_outline_size.x, pipe_outline_size.y};
        
        float p_width = assets.pipe[i].width * scale.pipe.x;
        Rectangle dest = {x - p_width/2, pipe_y - 5, p_width, 100}; 
        DrawTexturePro(assets.pipe[i], (Rectangle){0,0,assets.pipe[i].width,100}, dest, (Vector2){0,0}, 0, WHITE);
        
        if (CheckCollisionPointRec(mouse_position, hover_rect) && current_assets.pipe != i) {
            DrawRectangleLinesEx(hover_rect, 3, GRAY);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) current_assets.pipe = i;
        }
    }

    // Draw Backgrounds Selection
    for (int i = 0; i < 2; i++) {
        float x = start_x_bg + i * spacing;
        Rectangle hover_rect = {x - 75, bg_y - 15, bg_outline_size.x, bg_outline_size.y};
        Rectangle dest = {x - 65, bg_y - 5, 130, 60};
        
        DrawTexturePro(assets.background[i], (Rectangle){80,300,150, 90}, dest, (Vector2){0,0}, 0, WHITE);
        
        if (CheckCollisionPointRec(mouse_position, hover_rect) && current_assets.background != i) {
            DrawRectangleLinesEx(hover_rect, 3, GRAY);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) current_assets.background = i;
        }
    }
}


void draw_countdown(int count) {
    char num_str[10];
    snprintf(num_str, sizeof(num_str), "%d", count);

    float font_size = 150.0f;
    float spacing = 5.0f;
    Vector2 text_size = MeasureTextEx(font.determination, num_str, font_size, spacing);
    Vector2 position = {WIDTH / 2.0f, HEIGHT / 2.0f};
    Vector2 origin = {text_size.x / 2.0f, text_size.y / 2.0f};

    draw_text_outlined(
        font.determination,  
        num_str, 
        position, 
        origin, 
        font_size, 
        spacing, 
        WHITE, 
        BLACK, 
        4.0f
    );
}


void play_sound_pitch_variation(Sound sfx, float variation) {
    // varies pitch that makes repetative sound less annoying
    int range = (int)(variation * 1000.0f);
    int offset = GetRandomValue(-range, range);

    float pitch = 1.0f + ((float)offset / 1000.0f);

    if (pitch < 0.1f) pitch = 0.1f;

    SetSoundPitch(sfx, pitch);
    PlaySound(sfx);
}