#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "raylib.h"

#define DEBUG 0
#define HITBOX 0
#define PLATFORM_WINDOW 1

#define WIDTH 1080
#define HEIGHT 720

/*
shift all scales to scale struct
shift all global Texture to struct
Store high score in a file and use it. move it to a function
*/

// THIS AREA HOLDS SETTINGS VARIABLES
float gravity = 1000;
const float pipe_vertical_distance = 200;
const float pipe_horizontal_distance = 400;
const float flap_velocity = -425 ; // upward. That's why -ve
float game_speed = 300;
float background_speed = 70; // for parallex effect

float dt = 0; // I hate passing it to every function

float base_poition = 0;
float backgroung_position = 0;

float bird_rotation = 0;
float rotation_speed = 75;

int animate = 1;
int sound_on = 1;

// LOADING TEXTURES (should've used a struct)
Texture2D green_pipe;
int bird_animation_frame = 3;
Texture2D bird_frames[3]; // sadly global variable has to be defined this way
Texture2D background;
Texture2D ground;
Texture2D game_over_text;

typedef struct
{
    int total_frames;
    Texture2D frames[];
} Animation;

// typedef struct 
// {
//     Animation bird;
//     Animation pipes; // animation struct can hold assets and total_size together. It can be reused
//     Animation background;
//     Animation numbers; // Use this to draw score

//     Texture2D ground;
//     Texture2D game_over_text;
// } Assets;

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
} Scale;

Sfx sfx;
Scale scale;
// Assets textures;


void show_fps(void);
void load_textures(void);
Sfx load_sound(void);
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

 
int main(){
    InitWindow(WIDTH, HEIGHT, "Flappy Bird");
    InitAudioDevice();
    if (PLATFORM_WINDOW == 1) SetWindowIcon((Image) LoadImage("sprites/yellowbird-midflap.png"));
    SetTargetFPS(60); // vsync handle this automatically. but still miss one or two frames
    SetWindowState(FLAG_VSYNC_HINT);
    srand(time(NULL));
    
    int high_score = load_and_save_high_score(0, 'l'); // loading. so first value does not matter
    
    load_textures();
    sfx = load_sound();
    scale = set_scales(); // implement later

    // THIS AREA HOLDS VARIABLES FOR BIRD
    float pos_x = WIDTH * 0.212;
    float pos_y = HEIGHT/2;
    float velocity = -400;

    int score =  0;
    // int high_score = 0;
    int game_over = 0; 

    // THIS AREA DEALS WITH PIPES
    int total_pipes = WIDTH/pipe_horizontal_distance + 1;
    Pipe pipes[total_pipes];
    init_pipes(pipes, total_pipes);

    // main game loop
    while (!WindowShouldClose()){
        BeginDrawing();
        if (!game_over){
            draw_background();

            dt = GetFrameTime();

            velocity = update_velocity(velocity, dt);
            pos_y = move_bird(pos_y, velocity, dt);
            draw_bird(pos_x, pos_y, velocity);

            move_pipe(pipes, total_pipes, dt);
            draw_pipes(pipes, total_pipes);
    
            if (check_death(pos_x, pos_y, pipes, total_pipes)){
                draw_game_over();
                game_over = 1;
                animate = 0;
            }

            score = update_score(score, pos_x, total_pipes, pipes);
            draw_score(score);
            if (score > high_score) high_score = score;
            draw_high_score(high_score);
            
            show_fps();
            draw_ground();
        }
        else{
            draw_background();
            draw_pipes(pipes, total_pipes);
            draw_bird(pos_x, pos_y, velocity);
            draw_ground();
            show_fps();
            draw_score(score);
            draw_high_score(high_score);
            draw_game_over();

            // re-setting
            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(0)){
                init_pipes(pipes, total_pipes);
                score = 0;
                pos_y = HEIGHT/2;
                velocity = -400;
                dt = 0;
                game_over = 0;
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
    green_pipe = LoadTexture("sprites/pipe-green.png");

    bird_frames[0] = LoadTexture("sprites/yellowbird-downflap.png");
    bird_frames[1] = LoadTexture("sprites/yellowbird-midflap.png");
    bird_frames[2] = LoadTexture("sprites/yellowbird-upflap.png");

    background = LoadTexture("sprites/background-day.png");

    ground = LoadTexture("sprites/base.png");

    game_over_text = LoadTexture("sprites/gameover.png");
}


Sfx load_sound(void){
    Sfx  s = {
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
        .background = (Vector2){(float)HEIGHT/(float)background.height, (float)HEIGHT/(float)background.height}, // fills up the whole height
        .ground = {1.0f, 1.0f}
    };
    return s;
}


void free_memory(void){
    UnloadTexture(green_pipe);
    UnloadTexture(background);
    UnloadTexture(ground);
    UnloadTexture(game_over_text);
    for (int i = 0; i < bird_animation_frame; i++) UnloadTexture(bird_frames[i]);

    UnloadSound(sfx.death);
    UnloadSound(sfx.flap);
    UnloadSound(sfx.hit);
    UnloadSound(sfx.point);
}


void init_pipes(Pipe pipes[], int total_pipes){
    for (int i = 0; i < total_pipes; i++){
        pipes[i].x =  WIDTH + (i+1)*pipe_horizontal_distance;
        // pipes[i].y = rand() % (HEIGHT - 400) + 50; // bar should be between 50 and (WIDTH-350) as bottom portion is ground
        pipes[i].y = GetRandomValue(50, HEIGHT - ground.height*scale.ground.y - 60 - pipe_vertical_distance);
        pipes[i].passed = 0;
    }
}


void draw_background(void){
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

    float animation_time = 0.4; // total time to finish an animation
    int frame_no = 1;
    if (animate) frame_no = (int)(GetTime()/(animation_time/bird_animation_frame)) % bird_animation_frame;
    Texture2D bird = bird_frames[frame_no];
 
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


float update_velocity(float velocity, float dt){
    // uses v = u + gt to get velocity. If key pressed velocity instantly changes  to flap_velocity
    if (!(IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(0))){
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

    for (int i = 0; i < total_pipes; i++){
        Pipe pipe = pipes[i];
        
        // drawing the bottom portion
        DrawTexturePro(
            green_pipe,
            (Rectangle){0.0f, 0.0f, (float)green_pipe.width, (float)green_pipe.height},
            (Rectangle){pipe.x, (pipe.y + pipe_vertical_distance), (green_pipe.width * horizontal_scale), (green_pipe.height * vertical_scale)},
            (Vector2){0.0f, 0.0f},
            0.0f,
            WHITE
        );

        // drawing the top portion
        // shifting the origin to middle of pipe. Then rotating by 180° . Since now origin is middle point we have to draw with reference to the middle point
        DrawTexturePro(
            green_pipe,
            (Rectangle){0.0f, 0.0f, (float)green_pipe.width, (float)green_pipe.height},
            (Rectangle){(pipe.x + (green_pipe.width * horizontal_scale)/2), (pipe.y - (green_pipe.height*vertical_scale)/2), (green_pipe.width * horizontal_scale), (green_pipe.height * vertical_scale)},
            (Vector2){(green_pipe.width*horizontal_scale)/2, (green_pipe.height*vertical_scale)/2},
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
        if (pipes[i].x + green_pipe.width*2 <= 0) {
            move = i;
        }
        if (pipes[i].x > furthest) furthest = pipes[i].x;
    } 

    if (move != -1){
        pipes[move].x = furthest + pipe_horizontal_distance;
        pipes[move].y = GetRandomValue(50, HEIGHT - ground.height*scale.ground.y - 60 - pipe_vertical_distance); // if changed here , change in basic decleration
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


void draw_score(int score){
    char score_string[15];
    snprintf(score_string, sizeof(score_string), "Score = %d", score);
    DrawText(score_string, 0, 0, 45, GREEN);
}


void draw_high_score(int high_score){
    char high_score_string[20];
    snprintf(high_score_string, sizeof(high_score_string), "High Score = %d", high_score);
    DrawText(high_score_string, 0, 50, 30, RED);
}


int check_death(float pos_x, float pos_y, Pipe pipes[], int total_pipes){
    float bird_scale = scale.bird.x;
    float bird_width = bird_frames[0].width*bird_scale;
    float bird_height = bird_frames[0].height*bird_scale;
    Rectangle bird = {pos_x-bird_width/2, pos_y-bird_height/2, bird_width, bird_height};

    int death = 0;

    if (pos_y - bird_height/2 <= 0) death = 1; // checking if bird hits ceiling. Bird position is it's center point coordinate
    else if (pos_y + bird_height/2  >= (HEIGHT - ground.height*scale.ground.x)) death = 1; // checking if hits floor. This behabiour is buggy. I'll fix it later

    for  (int i=0; i < total_pipes; i++){
        int within_pipe = (pipes[i].x <= pos_x + bird_width/2 && pipes[i].x + green_pipe.width*1.5 >= pos_x - bird_width/2);
        #if HITBOX
            DrawRectangle(pipes[i].x, pipes[i].y, green_pipe.width*scale.pipe.x, pipe_vertical_distance, (Color){0, 100, 0, 50}); // (fixed)why offset by 5 though? because you are dumb. you are moving the pipe then drawing hitbox. I works for now
        #endif
        if (within_pipe && pos_y - bird_height/2 <= pipes[i].y) death = 1; // for top pipe
        else if (within_pipe && pos_y + bird_height/2 >= pipes[i].y + pipe_vertical_distance) death = 1; // for top pipe
    }

    if (sound_on) if (death) PlaySound(sfx.death);
    
    return death;
}   


void draw_game_over(void){
    float scale = 2;
    DrawTexturePro(
        game_over_text,
        (Rectangle){0.0f, 0.0f, game_over_text.width, game_over_text.height},
        (Rectangle){WIDTH/2, HEIGHT/2, game_over_text.width*scale, game_over_text.height*scale},
        (Vector2){game_over_text.width*scale/2, game_over_text.height*scale/2},
        0.0f,
        WHITE
    );
}