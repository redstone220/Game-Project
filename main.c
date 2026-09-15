#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define DEBUG 0
#define HITBOX 0
#define PLATFORM_WINDOW 0

#define WIDTH 1080
#define HEIGHT 720


// THIS AREA HOLDS SETTINGS VARIABLES
float gravity = 1200;
const float pipe_vertical_distance = 200;
const float pipe_horizontal_distance = 400;
const float flap_velocity = -450 ; // upward. That's why -ve
float game_speed = 300;
float background_speed = 70; // for parallex effect

float dt = 0; // I hate passing it to every function

int inputPressed = 0; // global input tracking. updated in main loops

float base_poition = 0; // for parallex
float backgroung_position = 0; // for parallex

float bird_rotation = 0;
float rotation_speed = 75;

int animate = 0; // 1 - animates bird, base, backgrooound, controlrotation. 0 - stop all animation and rotation
int sound_on = 1; // 1 - sound on. 0 - sound off


// LOADING TEXTURES (should've used a struct)
// Texture2D green_pipe;
// int bird_animation_frame = 3;
// Texture2D bird_frames[3]; // sadly global variable has to be defined this way
// Texture2D ground;
// Texture2D game_over_text;
// Texture2D begin_menu;
// Texture2D numbers[10];

typedef struct
{
    int total_frames;
    Texture2D frames[3];
} Animation;

typedef struct 
{
    Texture2D background[2];
    Texture2D ground[1];
    Texture2D pipe[2];
    Texture2D numbers[10];

    Texture2D game_over_txt;
    Texture2D begin_menu;

    Animation bird[3];
} Assets;

typedef struct 
{
    int background;
    int ground;
    int bird;
    int pipe;
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
    Vector2 bird;
    Vector2 pipe;
    Vector2 background;
    Vector2 ground;
    Vector2 number;
    Vector2 game_over_txt;
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
Scale set_scales(void);
void set_current_asset(void);
void menu(void);
void free_memory(void);
void draw_bird(int x, int y, float velocity);
void move_bird(float *pos_y, float velocity, float dt);
void update_velocity(float *velocity, float dt);
void draw_pipes(Pipe pipes[], int total_pipes);
void move_pipe(Pipe pipes[], int total_pipes, float dt);
void draw_background(void);
void draw_ground(void);
void update_score(int *score, float bird_x, int total_pipes, Pipe pipes[]);
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

    int score =  0; // current score

    // THIS AREA DEALS WITH PIPES
    int total_pipes = WIDTH/pipe_horizontal_distance + 1;
    Pipe pipes[total_pipes];
    init_pipes(pipes, total_pipes);

    // main game loop
    while (!WindowShouldClose()){
        BeginDrawing();
        inputPressed = IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        dt = GetFrameTime();
        
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

            update_velocity(&velocity, dt); // passing by reference. much cleaner
            move_bird(&pos_y, velocity, dt);
            draw_bird(pos_x, pos_y, velocity);

            move_pipe(pipes, total_pipes, dt);
            draw_pipes(pipes, total_pipes);
    
            if (check_death(pos_x, pos_y, pipes, total_pipes)){
                draw_game_over();
                gamestate = STATE_END;
                animate = 0;
            }

            update_score(&score, pos_x, total_pipes, pipes);
            draw_score(score);
            if (score > high_score) high_score = score;
            
            draw_ground();
        }
        else if (gamestate == STATE_END){
            draw_background();
            draw_pipes(pipes, total_pipes);
            draw_bird(pos_x, pos_y, velocity);
            draw_ground();
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
                animate = 0;
                bird_rotation = -30; 
            }
        }
        show_fps();
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
    DrawText(fps_info, WIDTH-95, 0, 20, LIGHTGRAY);
}


int load_and_save_high_score(int high_score, char load_or_save){
    /*
    Loads and Saves high score. If load_or_save = 'l' then loads. if load_or_save = 's' saves.
    When loading high_score needs to be passed. Default it to 0 even though it has no job here
    */
    int high_score_load = 0;
    if (load_or_save == 'l'){
        FILE *fp = fopen("high_score.txt", "r");
        if (fp != NULL){
            fscanf(fp, "%d", &high_score_load);
            fclose(fp);
        }
        else high_score_load = 0;
        return high_score_load;
    }

    if (load_or_save == 's'){
        FILE *high_score_file = fopen("high_score.txt", "w"); 
        if (high_score_file != NULL){
            fprintf(high_score_file, "%d", high_score);
            fclose(high_score_file);
        }
    }
    return 0;
}


void load_textures(void){
    // green_pipe = LoadTexture("sprites/pipe-green.png");

    // bird_frames[0] = LoadTexture("sprites/yellowbird-downflap.png");
    // bird_frames[1] = LoadTexture("sprites/yellowbird-midflap.png");
    // bird_frames[2] = LoadTexture("sprites/yellowbird-upflap.png");


    // ground = LoadTexture("sprites/base.png");

    // game_over_text = LoadTexture("sprites/gameover.png");
    // begin_menu = LoadTexture("sprites/message.png");

    // for (int i = 0; i < 10; i++){
    //     char file_name[20];
    //     snprintf(file_name, sizeof(file_name), "sprites/%d.png", i);
    //     numbers[i] = LoadTexture(file_name);
    // }

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

    Animation bird1 = {
        .frames = {
            LoadTexture("sprites/yellowbird-downflap.png"),
            LoadTexture("sprites/yellowbird-midflap.png"),
            LoadTexture("sprites/yellowbird-upflap.png")
        },
        .total_frames = 3
    };

    Animation bird2 = {
        .frames = {
            LoadTexture("sprites/bluebird-downflap.png"),
            LoadTexture("sprites/bluebird-midflap.png"),
            LoadTexture("sprites/bluebird-upflap.png")
        },
        .total_frames = 3
    };

    Animation bird3 = {
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
}


void set_current_asset(void){
    current_assets.background = 1; // 0-day 1-night
    current_assets.ground = 0; // 0-regular
    current_assets.bird = 1; // 0-yellow 1-blue 2-red
    current_assets.pipe = 1; // 0-green 1-red
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
    Scale s = {
        .bird = (Vector2){2.0f, 2.0f},
        .pipe = (Vector2){1.5f, 2.0f},
        .background = (Vector2){(float)HEIGHT/(float)assets.background[0].height, (float)HEIGHT/(float)assets.background[0].height}, // fills up the whole height
        .ground = {1.0f, 1.0f},
        .number = {1.5f, 1.5f},
        .game_over_txt = {2.0f, 2.0f}
    };
    return s;
}


void free_memory(void){
    UnloadTexture(assets.background[0]);
    UnloadTexture(assets.background[1]);

    UnloadTexture(assets.ground[0]);

    UnloadTexture(assets.pipe[0]);
    UnloadTexture(assets.pipe[1]);
    
    UnloadTexture(assets.game_over_txt);
    UnloadTexture(assets.begin_menu);

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
}


void init_pipes(Pipe pipes[], int total_pipes){
    for (int i = 0; i < total_pipes; i++){
        pipes[i].x =  WIDTH + (i+1)*pipe_horizontal_distance;
        // pipes[i].y = rand() % (HEIGHT - 400) + 50; // bar should be between 50 and (WIDTH-350) as bottom portion is ground
        pipes[i].y = GetRandomValue(50, HEIGHT - assets.ground[0].height*scale.ground.y - 60 - pipe_vertical_distance);
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

    #if HITBOX
        DrawRectanglePro(dest, origin, 0.0f, (Color){100, 100, 100, 50});
    #endif

    DrawTexturePro(bird, source, dest, origin, bird_rotation, WHITE);
}

void menu(void){
    float menu_scale = 2;
    DrawTexturePro(
        assets.begin_menu,
        (Rectangle){0.0f, 0.0f, assets.begin_menu.width, assets.begin_menu.height},
        (Rectangle){WIDTH/2, HEIGHT/2, assets.begin_menu.width*menu_scale, assets.begin_menu.height*menu_scale},
        (Vector2){assets.begin_menu.width*menu_scale/2, assets.begin_menu.height*menu_scale/2},
        0.0f,
        WHITE
    );
}

void update_velocity(float *velocity, float dt){
    // uses v = u + gt to get velocity. If key pressed velocity instantly changes  to flap_velocity
    if (!inputPressed){
        *velocity += gravity * dt;
    }
    else{
        if (sound_on) PlaySound(sfx.flap);
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
        // printf("%f %f \n", pipe.x + green_pipe.width * horizontal_scale, pipe.y - green_pipe.height*vertical_scale);
    }
}


void move_pipe(Pipe pipes[], int total_pipes, float dt){
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
    for (int i = 0; i < total_pipes; i++){
        if (pipes[i].x <= bird_x && !pipes[i].passed){
            *score += 1;
            pipes[i].passed = 1;
            if (sound_on) PlaySound(sfx.point);
        }
    }
}


void draw_score(int score) {
    int digits[10];
    int count = 0;
    int temp = score;

    int score_height = 30; // where to draw. reffered from top

    if (temp == 0) {
        digits[count++] = 0;
    } else {
        while (temp > 0) {
            digits[count++] = temp % 10;
            temp /= 10;
        }
    }

    float digit_scale = scale.number.x;
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


void draw_high_score(int high_score){
    char high_score_string[20];
    snprintf(high_score_string, sizeof(high_score_string), "High Score = %d", high_score);
    DrawText(high_score_string, 0, 50, 30, RED);
}


void draw_game_over_score(int score, int high_score){
    int game_over_height = 100; // change in draw_gamw_over() if changed here

    char score_string[50];
    snprintf(score_string, sizeof(score_string), "Score = %d", score);
    DrawText(score_string, WIDTH/2 - 100, game_over_height + 100, 30, RED);

    char high_score_string[50];
    snprintf(high_score_string, sizeof(high_score_string), "High Score = %d", high_score);
    DrawText(high_score_string, WIDTH/2 - 100, game_over_height + 150, 30, RED);
}


int check_death(float pos_x, float pos_y, Pipe pipes[], int total_pipes){
    float bird_scale = scale.bird.x;
    // Texture2D bird_frames[] = assets.bird[current_assets.bird].frames;

    float bird_width = assets.bird[current_assets.bird].frames[0].width*bird_scale; // good luck reading this
    float bird_height = assets.bird[current_assets.bird].frames[0].height*bird_scale;
    Rectangle bird = {pos_x-bird_width/2, pos_y-bird_height/2, bird_width, bird_height};

    int death = 0;

    if (pos_y - bird_height/2 <= 0) death = 1; // checking if bird hits ceiling. Bird position is it's center point coordinate
    else if (pos_y + bird_height/2  >= (HEIGHT - assets.ground[0].height*scale.ground.x)) death = 1; // checking if hits floor. This behabiour is buggy. I'll fix it later

    for  (int i=0; i < total_pipes; i++){
        int within_pipe = (pipes[i].x <= pos_x + bird_width/2 && pipes[i].x + assets.pipe[0].width*scale.pipe.x >= pos_x - bird_width/2);
        #if HITBOX
            DrawRectangle(pipes[i].x, pipes[i].y, assets.pipe[0].width*scale.pipe.x, pipe_vertical_distance, (Color){0, 100, 0, 50}); 
        #endif
        if (within_pipe && pos_y - bird_height/2 <= pipes[i].y) death = 1; // for top pipe
        else if (within_pipe && pos_y + bird_height/2 >= pipes[i].y + pipe_vertical_distance) death = 1; 
    }

    if (sound_on) if (death) PlaySound(sfx.death);
    
    return death;
}   


void draw_game_over(void){
    int game_over_height = 100; // change in draw_game_over_score()

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