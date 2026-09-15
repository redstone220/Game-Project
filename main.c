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
const float gravity = 1200;
const float pipe_vertical_distance = 200; // how much distance pipes are apart vertically
const float pipe_horizontal_distance = 400; // how much distance pipes are apart horizontally
const float flap_velocity = -450 ; // upward. That's why -ve
float game_speed = 300; // pipe speed. 
float background_speed = 70; // for parallex effect

float dt = 0; // I hate passing it to every function

int inputPressed = 0; // global input tracking. updated in main loops

float base_poition = 0; // for parallex
float backgroung_position = 0; // for parallex

float bird_rotation = 0; // current bird rotation. Updated in draw bird
float rotation_speed = 75; // how much to rotate per second

int animate = 0; // 1 - animates bird, base, backgrooound, controlrotation. 0 - stop all animation and rotation
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
    Vector2 pipe;
    Vector2 background;
    Vector2 ground;
    Vector2 number;
    Vector2 game_over_txt;
    Vector2 menu;
} Scale;

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER,
    STATE_END,
    STATE_CUSTOMIZATION
} GameState;

Sfx sfx;
Scale scale;
Assets assets;
FontList font;
CurrentAssets current_assets;
GameState gamestate = STATE_CUSTOMIZATION;

void show_fps(void);
void load_textures(void);
Sfx load_sound(void);
Scale set_scales(void);
void load_fonts(void);
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
void draw_text_outlined(Font font, const char *text, Vector2 position, Vector2 origin, float fontSize, float spacing, Color textColor, Color outlineColor, float outlineThickness);
 
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
    load_fonts();
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
                bird_rotation = 0; 
            }
        }
        else if (gamestate == STATE_CUSTOMIZATION){
            
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
        .pipe = (Vector2){1.5f, 2.0f},
        .background = (Vector2){(float)HEIGHT/(float)assets.background[0].height, (float)HEIGHT/(float)assets.background[0].height}, // fills up the whole height
        .ground = {1.0f, 1.0f},
        .number = {1.5f, 1.5f},
        .game_over_txt = {2.5f, 2.5f},
        .menu = {2.0f, 2.0f}
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

    #if HITBOX
        DrawRectanglePro(dest, origin, 0.0f, (Color){100, 100, 100, 50});
    #endif

    DrawTexturePro(bird, source, dest, origin, bird_rotation, WHITE);
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
        // printf("%f %f \n", pipe.x + green_pipe.width * horizontal_scale, pipe.y - green_pipe.height*vertical_scale);
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
        updates score. (Also changes game velocity after scoring-Todo)
    */
    for (int i = 0; i < total_pipes; i++){
        if (pipes[i].x <= bird_x && !pipes[i].passed){
            *score += 1;
            pipes[i].passed = 1;
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
    float high_score_font_size = 100;
    float score_spacing = 2.0f;
    float high_score_spacing = 1.75;

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
        GetColor(0xbd1748aa),
        (Color){50, 50, 50, 255},
        // WHITE,
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
        (Vector2){WIDTH/2, game_over_height + 265},
        high_score_ancor,
        high_score_font_size,
        high_score_spacing,
        GetColor(0xbd1748aa),
        (Color){50, 50, 50, 255},
        // WHITE,
        3.2
    );
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
