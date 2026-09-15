#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "raylib.h"

#define DEBUG 0
#define HITBOX 1
#define PLATFORM_WINDOW 0

#define WIDTH 1080
#define HEIGHT 720

// Design-time reference resolution. All visual scale factors are computed
// relative to this resolution, so changing WIDTH/HEIGHT above automatically
// rescales everything (bird, pipes, ground, UI, text) proportionally.
#define BASE_WIDTH 1080
#define BASE_HEIGHT 720

#define BIRD_FRAMES 3   // downflap, midflap, upflap - same for every bird color
#define BIRD_COLORS 3   // yellow, blue, red
#define PIPE_COLORS 2   // green, red

/*
Store high score in a file and use it. move it to a function   [DONE]
shift all scales to scale struct                                [DONE]
shift all global Texture to struct                               [DONE]
*/

// THIS AREA HOLDS SETTINGS VARIABLES
float gravity = 1200;
const float pipe_vertical_distance = 200;
const float pipe_horizontal_distance = 400;
const float flap_velocity = -450 ; // upward. That's why -ve
float game_speed = 300;
float background_speed = 70; // for parallex effect

float dt = 0; // I hate passing it to every function

int inputPressed = 0;

float base_poition = 0;
float backgroung_position = 0;

float bird_rotation = 0;
float rotation_speed = 75;

int animate = 1;
int sound_on = 1;

// Base (design-time) UI offsets - actual drawn position is these * scale.resolution
int score_height = 30;
int game_over_height = 100;

typedef struct
{
    // one bird color's flap-cycle frames: down, mid, up
    Texture2D frames[BIRD_FRAMES];
} BirdAnimation;

typedef struct
{
    Texture2D background[2];   // 0 = day, 1 = night
    Texture2D ground;
    Texture2D pipe[PIPE_COLORS];   // 0 = green, 1 = red
    Texture2D numbers[10];

    Texture2D game_over_txt;
    Texture2D message;

    BirdAnimation bird[BIRD_COLORS]; // 0 = yellow, 1 = blue, 2 = red
} Assets;

typedef struct
{
    // indices selecting which variant of each asset is currently active
    int background;
    int pipe;
    int bird;
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
    // all sfx in one place
    Sound flap;
    Sound death;
    Sound point;
    Sound hit;
} Sfx;

typedef struct {
    // all assets needs to be scaled. all are contained here
    Vector2 bird_draw;
    Vector2 bird_hitbox;
    Vector2 pipe;
    Vector2 background;
    Vector2 ground;

    // UI draw scales (used to be hardcoded literals scattered around)
    float menu;
    float game_over;
    float digit;

    // overall scale factor derived from current HEIGHT vs BASE_HEIGHT.
    // Multiply any "design-time" pixel value (font sizes, offsets) by this
    // to keep it proportional when WIDTH/HEIGHT change.
    float resolution;
} Scale;

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER,
    STATE_END
} GameState;

Sfx sfx;
Scale scale;
Assets assets;
CurrentAssets current_assets;
GameState gamestate = STATE_MENU;

void show_fps(void);
void load_textures(void);
Sfx load_sound(void);
void set_current_asset(void);
void menu(void);
void free_memory(void);
Scale set_scales(void);
void draw_bird(int x, int y, float velocity);
float move_bird(float pos_y, float velocity, float dt);
float update_velocity(float velocity, float dt);
void draw_pipes(Pipe pipes[], int total_pipes);
void move_pipe(Pipe pipes[], int total_pipes, float dt);
void draw_background(void);
void draw_ground(void);
int update_score(int score, float bird_x, int total_pipes, Pipe pipes[]);
void draw_score(int score);
void draw_high_score(int high_score);
int check_death(float pos_x, float pos_y, Pipe pipes[], int total_pipes);
void draw_game_over(void);
void init_pipes(Pipe pipes[], int total_pipes);
int load_and_save_high_score(int high_score, char load_or_save);
void draw_game_over_score(int score, int high_score);

int main(){
    InitWindow(WIDTH, HEIGHT, "Flappy Bird");
    InitAudioDevice();
    if (PLATFORM_WINDOW == 1) SetWindowIcon((Image) LoadImage("sprites/yellowbird-midflap.png"));
    SetTargetFPS(60); // vsync handle this automatically. but still miss one or two frames
    SetWindowState(FLAG_VSYNC_HINT);
    srand(time(NULL));

    int high_score = load_and_save_high_score(0, 'l'); // loading. so first value does not matter

    load_textures();
    set_current_asset();
    sfx = load_sound();
    scale = set_scales();


    // THIS AREA HOLDS VARIABLES FOR BIRD
    float pos_x = WIDTH * 0.212;
    float pos_y = HEIGHT/2;
    float velocity = -400;

    int score =  0;

    // THIS AREA DEALS WITH PIPES
    int total_pipes = WIDTH/pipe_horizontal_distance + 1;
    Pipe pipes[total_pipes];
    init_pipes(pipes, total_pipes);

    // main game loop
    while (!WindowShouldClose()){
        BeginDrawing();
        inputPressed = IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        if (gamestate == STATE_MENU)
        {
            draw_background();
            draw_ground();
            menu();
            if (inputPressed) {
                gamestate = STATE_PLAYING;
                animate = 1;
                bird_rotation = -30;
            }
        }
        else if (gamestate == STATE_PLAYING){
            draw_background();

            dt = GetFrameTime();
            velocity = update_velocity(velocity, dt);
            pos_y = move_bird(pos_y, velocity, dt);
            draw_bird(pos_x, pos_y, velocity);

            move_pipe(pipes, total_pipes, dt);
            draw_pipes(pipes, total_pipes);

            if (check_death(pos_x, pos_y, pipes, total_pipes)){
                draw_game_over();
                gamestate = STATE_END;
                animate = 0;
            }

            score = update_score(score, pos_x, total_pipes, pipes);
            draw_score(score);
            if (score > high_score) high_score = score;
            draw_high_score(high_score);

            show_fps();
            draw_ground();
        }
        else if (gamestate == STATE_END){
            draw_background();
            draw_pipes(pipes, total_pipes);
            draw_bird(pos_x, pos_y, velocity);
            draw_ground();
            show_fps();
            draw_game_over();
            draw_game_over_score(score, high_score);

            // re-setting
            if (inputPressed){
                init_pipes(pipes, total_pipes);
                score = 0;
                pos_y = HEIGHT/2;
                velocity = -400;
                dt = 0;
                gamestate = STATE_MENU;
                animate = 1;
                bird_rotation = -30;
            }
        }
        EndDrawing();
    }

    load_and_save_high_score(high_score, 's');
    free_memory();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}


void show_fps(void){
    int fps = GetFPS();
    char fps_info[50];
    snprintf(fps_info, sizeof(fps_info), "FPS = %d", fps);
    int font_size = (int)(20 * scale.resolution);
    DrawText(fps_info, WIDTH - (int)(95 * scale.resolution), 0, font_size, LIGHTGRAY);
}


int load_and_save_high_score(int high_score, char load_or_save){
    /*
    Loads and Saves high score. If load_or_save = 'l' then loads. if load_or_save = 's' saves.
    When loading high_score needs to be passed. Default it to 0 even though it has no job here
    */
    int high_score_load = 0;
    if (load_or_save == 'l'){
        FILE *fp = fopen("high_score.txt", "r");
        if (fp != NULL) fscanf(fp, "%d", &high_score_load);
        else high_score_load = 0;
        fclose(fp);
        return high_score_load;
    }

    if (load_or_save == 's'){
        FILE *high_score_file = fopen("high_score.txt", "w");
        if (high_score_file != NULL){
            fprintf(high_score_file, "%d", high_score);
        }
        fclose(high_score_file);
    }
    return 0;
}


void load_textures(void){
    // pipes
    assets.pipe[0] = LoadTexture("sprites/pipe-green.png");
    assets.pipe[1] = LoadTexture("sprites/pipe-red.png");

    // bird flap-cycle frames, one BirdAnimation per color
    assets.bird[0].frames[0] = LoadTexture("sprites/yellowbird-downflap.png");
    assets.bird[0].frames[1] = LoadTexture("sprites/yellowbird-midflap.png");
    assets.bird[0].frames[2] = LoadTexture("sprites/yellowbird-upflap.png");

    assets.bird[1].frames[0] = LoadTexture("sprites/bluebird-downflap.png");
    assets.bird[1].frames[1] = LoadTexture("sprites/bluebird-midflap.png");
    assets.bird[1].frames[2] = LoadTexture("sprites/bluebird-upflap.png");

    assets.bird[2].frames[0] = LoadTexture("sprites/redbird-downflap.png");
    assets.bird[2].frames[1] = LoadTexture("sprites/redbird-midflap.png");
    assets.bird[2].frames[2] = LoadTexture("sprites/redbird-upflap.png");

    assets.game_over_txt = LoadTexture("sprites/gameover.png");
    assets.message = LoadTexture("sprites/message.png");

    for (int i = 0; i < 10; i++){
        char file_name[20];
        snprintf(file_name, sizeof(file_name), "sprites/%d.png", i);
        assets.numbers[i] = LoadTexture(file_name);
    }

    assets.background[0] = LoadTexture("sprites/background-day.png");
    assets.background[1] = LoadTexture("sprites/background-night.png");

    assets.ground = LoadTexture("sprites/base.png");
}


void set_current_asset(void){
    // Defaults for a fresh game. Change these (or expose a menu) to let the
    // player pick a different bird/pipe/background variant later.
    current_assets.background = 0; // 0 = day, 1 = night
    current_assets.pipe = 0;       // 0 = green, 1 = red
    current_assets.bird = 0;       // 0 = yellow, 1 = blue, 2 = red
}


Sfx load_sound(void){
    Sfx s = {
       .death = LoadSound("audio/die.wav"),
       .flap = LoadSound("audio/wing.wav"),
       .hit = LoadSound("audio/hit.wav"),
       .point = LoadSound("audio/point.wav")
    };
    SetSoundVolume(s.point, 0.5f);
    return s;
}


Scale set_scales(void){
    // Single source of truth for "how much bigger/smaller is the current
    // window than the resolution this game was designed at". Every other
    // scale below is derived from it, so bumping WIDTH/HEIGHT rescales the
    // whole game consistently instead of needing every literal retuned.
    float resolution_scale = (float)HEIGHT / (float)BASE_HEIGHT;

    Scale s = {
        .bird_draw = (Vector2){2.0f * resolution_scale, 2.0f * resolution_scale},
        .bird_hitbox = (Vector2){1.5f * resolution_scale, 1.75f * resolution_scale},
        .pipe = (Vector2){1.5f * resolution_scale, 2.0f * resolution_scale},
        .background = (Vector2){(float)HEIGHT/(float)assets.background[0].height, (float)HEIGHT/(float)assets.background[0].height}, // fills up the whole height
        .ground = (Vector2){resolution_scale, resolution_scale},
        .menu = 2.0f * resolution_scale,
        .game_over = 2.0f * resolution_scale,
        .digit = 1.5f * resolution_scale,
        .resolution = resolution_scale
    };
    return s;
}


void free_memory(void){
    UnloadTexture(assets.background[0]);
    UnloadTexture(assets.background[1]);

    UnloadTexture(assets.ground);

    for (int i = 0; i < PIPE_COLORS; i++){
        UnloadTexture(assets.pipe[i]);
    }

    UnloadTexture(assets.game_over_txt);
    UnloadTexture(assets.message);

    for (int i = 0; i < 10; i++) {
        UnloadTexture(assets.numbers[i]);
    }

    for (int color = 0; color < BIRD_COLORS; color++){
        for (int frame = 0; frame < BIRD_FRAMES; frame++){
            UnloadTexture(assets.bird[color].frames[frame]);
        }
    }

    UnloadSound(sfx.death);
    UnloadSound(sfx.flap);
    UnloadSound(sfx.hit);
    UnloadSound(sfx.point);
}


void init_pipes(Pipe pipes[], int total_pipes){
    for (int i = 0; i < total_pipes; i++){
        pipes[i].x =  WIDTH + (i+1)*pipe_horizontal_distance;
        // pipes[i].y = rand() % (HEIGHT - 400) + 50; // bar should be between 50 and (WIDTH-350) as bottom portion is ground
        pipes[i].y = GetRandomValue(50, HEIGHT - assets.ground.height*scale.ground.y - 60 - pipe_vertical_distance);
        pipes[i].passed = 0;
    }
}


void draw_background(void){
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
    Texture2D ground = assets.ground;
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
    float bird_scale = scale.bird_draw.x;

    float animation_time = 0.4; // total time to finish an animation
    int frame_no = 1;
    if (animate) frame_no = (int)(GetTime()/(animation_time/BIRD_FRAMES)) % BIRD_FRAMES;
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

    #if HITBOX
        DrawRectanglePro((Rectangle){(float)x, (float)y, bird.width * scale.bird_hitbox, bird.height * scale.bird_hitbox}, origin, 0.0f, (Color){100, 100, 100, 50});
    #endif

    DrawTexturePro(bird, source, dest, origin, bird_rotation, WHITE);
}

void menu(void){
    float menu_scale = scale.menu;
    Texture2D begin_menu = assets.message;
    DrawTexturePro(
        begin_menu,
        (Rectangle){0.0f, 0.0f, begin_menu.width, begin_menu.height},
        (Rectangle){WIDTH/2, HEIGHT/2, begin_menu.width*menu_scale, begin_menu.height*menu_scale},
        (Vector2){begin_menu.width*menu_scale/2, begin_menu.height*menu_scale/2},
        0.0f,
        WHITE
    );
}

float update_velocity(float velocity, float dt){
    // uses v = u + gt to get velocity. If key pressed velocity instantly changes  to flap_velocity
    if (!inputPressed){
        return velocity + gravity * dt;
    }
    else{
        if (sound_on) PlaySound(sfx.flap);
        bird_rotation = -30;
        return flap_velocity;
    }
}


float move_bird(float pos_y, float velocity, float dt){
    // uses s = vt to calculate posion.
    // y = yo + vt; as coordinate system is inversed
    return pos_y + velocity*dt;
}


void draw_pipes(Pipe pipes[], int total_pipes){
    float vertical_scale = scale.pipe.y;
    float horizontal_scale = scale.pipe.x;
    Texture2D pipe_texture = assets.pipe[current_assets.pipe];

    for (int i = 0; i < total_pipes; i++){
        Pipe pipe = pipes[i];

        // drawing the bottom portion
        DrawTexturePro(
            pipe_texture,
            (Rectangle){0.0f, 0.0f, (float)pipe_texture.width, (float)pipe_texture.height},
            (Rectangle){pipe.x, (pipe.y + pipe_vertical_distance), (pipe_texture.width * horizontal_scale), (pipe_texture.height * vertical_scale)},
            (Vector2){0.0f, 0.0f},
            0.0f,
            WHITE
        );

        // drawing the top portion
        // shifting the origin to middle of pipe. Then rotating by 180° . Since now origin is middle point we have to draw with reference to the middle point
        DrawTexturePro(
            pipe_texture,
            (Rectangle){0.0f, 0.0f, (float)pipe_texture.width, (float)pipe_texture.height},
            (Rectangle){(pipe.x + (pipe_texture.width * horizontal_scale)/2), (pipe.y - (pipe_texture.height*vertical_scale)/2), (pipe_texture.width * horizontal_scale), (pipe_texture.height * vertical_scale)},
            (Vector2){(pipe_texture.width*horizontal_scale)/2, (pipe_texture.height*vertical_scale)/2},
            180.0f,
            WHITE
        );
        // printf("%f %f \n", pipe.x + pipe_texture.width * horizontal_scale, pipe.y - pipe_texture.height*vertical_scale);
    }
}


void move_pipe(Pipe pipes[], int total_pipes, float dt){
    float furthest = 0;
    int move = -1;
    Texture2D pipe_texture = assets.pipe[current_assets.pipe];

    for (int i = 0; i < total_pipes; i++){
        pipes[i].x -= game_speed * dt;
        if (pipes[i].x + pipe_texture.width*2 <= 0) {
            move = i;
        }
        if (pipes[i].x > furthest) furthest = pipes[i].x;
    }

    if (move != -1){
        pipes[move].x = furthest + pipe_horizontal_distance;
        pipes[move].y = GetRandomValue(50, HEIGHT - assets.ground.height*scale.ground.y - 60 - pipe_vertical_distance);
        pipes[move].passed = 0;
    }
}


int update_score(int score, float bird_x, int total_pipes, Pipe pipes[]){
    for (int i = 0; i < total_pipes; i++){
        if (pipes[i].x <= bird_x && !pipes[i].passed){
            score += 1;
            pipes[i].passed = 1;
            if (sound_on) PlaySound(sfx.point);
        }
    }
    return score;
}


void draw_score(int score) {
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

    float digit_scale = scale.digit;
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
            (Rectangle){current_x, (float)score_height * scale.resolution, tex.width * digit_scale, tex.height * digit_scale},
            (Vector2){0.0f, 0.0f},
            0.0f,
            WHITE
        );

        current_x += tex.width * digit_scale;
    }
}


void draw_high_score(int high_score){
    char high_score_string[20];
    snprintf(high_score_string, sizeof(high_score_string), "High Score = %d", high_score);
    int font_size = (int)(30 * scale.resolution);
    DrawText(high_score_string, 0, (int)(50 * scale.resolution), font_size, RED);
}


void draw_game_over_score(int score, int high_score){
    int font_size = (int)(30 * scale.resolution);
    int x = WIDTH/2 - (int)(100 * scale.resolution);

    char score_string[50];
    snprintf(score_string, sizeof(score_string), "Score = %d", score);
    DrawText(score_string, x, (int)((game_over_height + 100) * scale.resolution), font_size, RED);

    char high_score_string[50];
    snprintf(high_score_string, sizeof(high_score_string), "High Score = %d", high_score);
    DrawText(high_score_string, x, (int)((game_over_height + 150) * scale.resolution), font_size, RED);
}


int check_death(float pos_x, float pos_y, Pipe pipes[], int total_pipes){
    float bird_scale = scale.bird_hitbox.x;
    Texture2D bird_texture = assets.bird[current_assets.bird].frames[0];
    float bird_width = bird_texture.width*bird_scale;
    float bird_height = bird_texture.height*bird_scale;
    Rectangle bird = {pos_x-bird_width/2, pos_y-bird_height/2, bird_width, bird_height};

    Texture2D pipe_texture = assets.pipe[current_assets.pipe];

    int death = 0;

    if (pos_y - bird_height/2 <= 0) death = 1; // checking if bird hits ceiling. Bird position is it's center point coordinate
    else if (pos_y + bird_height/2  >= (HEIGHT - assets.ground.height*scale.ground.x)) death = 1; // checking if hits floor. This behabiour is buggy. I'll fix it later

    for  (int i=0; i < total_pipes; i++){
        int within_pipe = (pipes[i].x <= pos_x + bird_width/2 && pipes[i].x + pipe_texture.width*scale.pipe.x >= pos_x - bird_width/2);
        #if HITBOX
            DrawRectangle(pipes[i].x, pipes[i].y, pipe_texture.width*scale.pipe.x, pipe_vertical_distance, (Color){0, 100, 0, 50});
        #endif
        if (within_pipe && pos_y - bird_height/2 <= pipes[i].y) death = 1; // for top pipe
        else if (within_pipe && pos_y + bird_height/2 >= pipes[i].y + pipe_vertical_distance) death = 1;
    }

    if (sound_on) if (death) PlaySound(sfx.death);

    return death;
}


void draw_game_over(void){
    float game_over_scale = scale.game_over;
    Texture2D game_over_text = assets.game_over_txt;
    DrawTexturePro(
        game_over_text,
        (Rectangle){0.0f, 0.0f, game_over_text.width, game_over_text.height},
        (Rectangle){WIDTH/2, game_over_height * scale.resolution, game_over_text.width*game_over_scale, game_over_text.height*game_over_scale},
        (Vector2){game_over_text.width*game_over_scale/2, game_over_text.height*game_over_scale/2},
        0.0f,
        WHITE
    );
}