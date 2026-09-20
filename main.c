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


// THIS AREA HOLDS SETTINGS VARIABLES THAT CAN BE TWEAKED TO BALANCE THE GAME
const float gravity = 1200; // px/s^2
const float pipe_vertical_distance = 200; // how much distance pipes are apart vertically
const float pipe_horizontal_distance = 400; // how much distance pipes are apart horizontally
const float flap_velocity = -450 ; // upward. That's why -ve
float game_speed = 300; // pipe speed. 
float background_speed = 70; // for parallex effect
float difficulty = 2.1; // how much to increase after erach score

// THIS AREA DEALS WITH VARIABLE THAT NEEDS TO BE PASS IN EVERY FUNCTION
float dt = 0; // I hate passing it to every function
int inputPressed = 0; // global input tracking. updated in main loops
Vector2 mouse_position = {0, 0}; // tracks mouse position

// RANDOM GLOBAL VARIABLES(should've used a struct)
float base_poition = 0; // for parallex
float backgroung_position = 0; // for parallex

float bird_rotation = 0; // current bird rotation. Updated in draw bird
float rotation_speed = 75; // how much to rotate per second

int animate = 0; // 1 - animates bird, base, backgrooound, controls rotation. 0 - stop all animation and rotation
int sound_on = 1; // 1 - sound on. 0 - sound off

// Outline animation variables for customization screen
float anim_bird_x = 0;
float anim_pipe_x = 0;
float anim_bg_x = 0;
int first_cust_load = 1;

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
    // all sfx in one place
    Sound flap;
    Sound death;
    Sound point;
    Sound hit;
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
    STATE_COOLDOWN
} GameState;

Sfx sfx;
Scale scale;
Assets assets;
FontList font;
CurrentAssets current_assets;
GameState gamestate = STATE_MENU;

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

void draw_background(void);
void draw_ground(void);
void draw_score(int score);
void draw_high_score(int high_score);
void draw_game_over(void);
void draw_game_over_score(int score, int high_score);
int load_and_save_high_score(int high_score, char load_or_save);
void draw_customization(void);
void draw_countdown(int count);

void draw_text_outlined(Font font, const char *text, Vector2 position, Vector2 origin, float fontSize, float spacing, Color textColor, Color outlineColor, float outlineThickness);
void button(int *button_tracker, Texture2D tex, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color hover_tint);

int main(){
    InitWindow(WIDTH, HEIGHT, "Flappy Bird");
    InitAudioDevice();
    if (PLATFORM_WINDOW == 1) SetWindowIcon((Image) LoadImage("icons/favicon-5.png"));
    SetTargetFPS(60); // vsync handle this automatically. but still miss one or two frames
    SetWindowState(FLAG_VSYNC_HINT);
    srand(time(NULL));
    
    int high_score = load_and_save_high_score(0, 'l'); // loading. so first value does not matter
    
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
        
        if (gamestate == STATE_MENU)
        {
            draw_background();
            draw_ground();
            menu();
            
            int start_game = inputPressed;
            draw_menu_ui(&start_game);

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
            if (score > high_score) high_score = score;
            
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
            draw_game_over_score(score, high_score);

            cooldown_timer -= dt;
            if (cooldown_timer <= 0) gamestate = STATE_END;
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
            );
            if (exit_pressed){
                gamestate = STATE_MENU;
                animate = 0;
                bird_rotation = 0;
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
    // for debugging. Also looks cool
    int fps = GetFPS();
    char fps_info[50];
    snprintf(fps_info, sizeof(fps_info), "FPS = %d", fps);
    DrawText(fps_info, 20, HEIGHT-30, 20, LIGHTGRAY);
}


int load_and_save_high_score(int high_score, char load_or_save){
    /*
    Loads and Saves high score. If load_or_save = 'l' then loads. if load_or_save = 's' saves.
    When loading high_score needs to be passed. Default it to 0 even though it has no job here
    */
    int high_score_load = 0;
    if (load_or_save == 'l'){
        FILE *fp = fopen("saves/high_score.txt", "r");
        if (fp != NULL){
            fscanf(fp, "%d", &high_score_load);
            fclose(fp);
        }
        else high_score_load = 0;
        return high_score_load;
    }

    if (load_or_save == 's'){
        FILE *high_score_file = fopen("saves/high_score.txt", "w"); 
        if (high_score_file != NULL){
            fprintf(high_score_file, "%d", high_score);
            fclose(high_score_file);
        }
    }
    return 0;
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

    assets.exit_ui = LoadTexture("icons/exit.png");
    assets.pencil = LoadTexture("icons/pencil-solid.png");
    assets.sound_on = LoadTexture("icons/sound-on-solid.png");
    assets.sound_off = LoadTexture("icons/sound-mute-solid.png");
    assets.pause = LoadTexture("icons/pause.png");
    assets.play = LoadTexture("icons/play.png");
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
    /*
        Responsible for loading all sounds. Must be called after InitAudioDevice()
    */
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

    UnloadTexture(assets.exit_ui);
    UnloadTexture(assets.pencil);
    UnloadTexture(assets.sound_on);
    UnloadTexture(assets.sound_off);
    UnloadTexture(assets.pause);
    UnloadTexture(assets.play);

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
        draws backgground. If animate is on, meves background to left.
    */
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
            if (sound_on) PlaySound(sfx.point);
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


void draw_high_score(int high_score){
    // used to draw high score. (replaced with game over score)
    char high_score_string[20];
    snprintf(high_score_string, sizeof(high_score_string), "High Score = %d", high_score);
    DrawText(high_score_string, 0, 50, 30, RED);
}


void draw_game_over_score(int score, int high_score){
    /*
        draws score and high_score in state end
    */
    int game_over_height = 120; // change in draw_game_over() if changed here.

    float score_font_size = 130;
    float high_score_font_size = 105;
    float score_spacing = 2.0f;
    float high_score_spacing = 1.7;

    // draws the score
    char score_string[50];
    snprintf(score_string, sizeof(score_string), "Score = %d", score);

    Vector2 score_text_size = MeasureTextEx(font.determination, score_string, score_font_size, score_spacing);
    Vector2 score_ancor = {score_text_size.x/2, score_text_size.y/2}; // ancoring to center point
    
    draw_text_outlined(
        font.determination,
        score_string,
        (Vector2){WIDTH/2, game_over_height + 150},
        score_ancor,
        score_font_size,
        score_spacing,
        // GetColor(0xbd1748aa),
        // (Color){50, 50, 50, 255},
        // WHITE,
        GetColor(0xFCA048FF),
        GetColor(0x543847FF),
        3.2
    );

    // draws the high_score
    char high_score_string[50];
    snprintf(high_score_string, sizeof(high_score_string), "High Score = %d", high_score);
    
    Vector2 high_score_text_size = MeasureTextEx(font.determination, high_score_string, high_score_font_size, high_score_spacing);
    Vector2 high_score_ancor = {high_score_text_size.x/2, high_score_text_size.y/2}; // ancoring to center point
    
    draw_text_outlined(
        font.determination,
        high_score_string,
        (Vector2){WIDTH/2, game_over_height + 285},
        high_score_ancor,
        high_score_font_size,
        high_score_spacing,
        // GetColor(0xbd1748aa),
        // (Color){50, 50, 50, 255},
        // WHITE,
        GetColor(0xFCA048FF),
        GetColor(0x543847FF),
        3.2
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
    if (sound_on) if (death) PlaySound(sfx.death);
    
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


void draw_customization(void) {
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
