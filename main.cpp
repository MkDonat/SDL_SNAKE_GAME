#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <vector>
#include <array>

// App Meta-datas stuffs
const char *appname = "snake\0";
const char *appversion = "0.0.1\0";
const char *appidentifier = "game.snake.cm\0";

// Delta Time stuffs
Uint64 now = SDL_GetPerformanceCounter();
Uint64 last = 0U;
double deltaTime = 0.0f;

// Window stuffs
SDL_Window *window = NULL;
const char *title = "Snake\0";
int width = 720U;
int height = 468U;

// Renderer stuffs
SDL_Renderer *renderer = NULL;
const char *renderer_backend_name = NULL;//"vulkan\0";

// Frect
SDL_FRect frect;

// Gamepad stuffs
SDL_Gamepad *gamepad0 = NULL;
Sint16 gamepad_axis_leftx = {};

// Math Vector
typedef struct{
    float x;
    float y;
}Vector2;

Vector2 Vector2_normalized(Vector2 v) {
    float l = v.x * v.x + v.y * v.y;
	if (l != 0) {
		l = SDL_sqrtf(l);
        return (Vector2){
            v.x /= l,
		    v.y /= l
        };
	}
    else return (Vector2){0.0f,0.0f};
}

// World cells stuffs
struct World{
    int MIN_COLUMNS_COUNT = 0;
    int MAX_COLUMNS_COUNT;
    int MIN_LINES_COUNT = 0;
    int MAX_LINES_COUNT;
    int CELLS_PORTION = 5; //5
    int WIDTH_CELLS_SIZE = 
        SDL_floorf(width * CELLS_PORTION / 100.0f) ;
    int HEIGHT_CELLS_SIZE = 
        SDL_floorf(WIDTH_CELLS_SIZE * 1.0f);
};
World world;

// Snake stuffs - WASD config
struct Snake{
    std::vector <Vector2> points;
    uint32_t speed = 150; // 150
    Vector2 direction = {0.0f , 0.0f};
    Vector2 orientation = {1.0f , 0.0f};
    float size_scale_w = 1.0f;
    float size_scale_h = 1.0f;
    bool there_is_a_point_to_add = false;
    bool change_orientation_flag = false;
};
Snake snake;

struct BONNUS{
    std::vector<Vector2>points;
    uint8_t r = 38;
    uint8_t g = 65;
    uint8_t b = 153;
    int MAX_COUNT = 5;
    int POP_PERIOD = 1500;
};
BONNUS bonnus;

// Custom functions declaration and implementation
Vector2 get_global_position(Vector2 world_pos, World world){
    int x = {}, y = {};
    x = world_pos.x * world.WIDTH_CELLS_SIZE;
    y = world_pos.y * world.HEIGHT_CELLS_SIZE;

    return (Vector2){x , y};
}
Vector2 next_world_position(World world, Vector2 orientation, Vector2 world_position){
    std::array<std::array<int, 4>, 2> d = {{
        {world_position.x, Vector2_normalized(orientation).x, world.MAX_COLUMNS_COUNT-1, world.MIN_COLUMNS_COUNT},
        {world_position.y, Vector2_normalized(orientation).y, world.MAX_LINES_COUNT-1, world.MIN_LINES_COUNT}
    }};
     
    for (int i = 0; i < 2; ++i){
        if(d[i][1] != 0.0f){ // if orientation.x/y != 0.0f
            if(d[i][1] == 1.0f){ // if orientation.x/y == 1.0f
                if(
                    // if worl_pos_x/y > world.MAX_COLUMNS/LINES_COUNT
                    d[i][0]++ >= d[i][2] 
                ){
                    // worl_pos_x/y = world.MIN_COLUMNS/LINES_COUNT;
                    d[i][0] = d[i][3]; 
                }
            }
            else if (d[i][1] == -1.0f){ // else if orientation.x/y == -1.0f
                if(
                    // if worl_pos_x/y < world.MIN_COLUMNS/LINES_COUNT
                    d[i][0]-- <= d[i][3]
                ){
                    // worl_pos_x/y = world.MAX_COLUMNS/LINES_COUNT;
                    d[i][0] = d[i][2];
                }
            }
        }
    }
    return (Vector2){d[0][0], d[1][0]};
}

Uint32 bonnus_next_pop(void *userdata, SDL_TimerID timerID, Uint32 interval){

    // setting limits
    int bonnus_max_col = world.MAX_COLUMNS_COUNT;// - 1;
    int bonnus_max_li = world.MAX_LINES_COUNT ;//- 1;
    // checking bonnus nomber
    if(bonnus.points.size() < bonnus.MAX_COUNT){
        // adding a random point
        bonnus.points.push_back(
            (Vector2){
                SDL_rand(bonnus_max_col),
                SDL_rand(bonnus_max_li)
            }
        );
    }
    //SDL_Log("BONNUS ADDED!");

    return interval;
}

void snake_add_points(int nbr){
    // snake firsts points
    for(int i = nbr; i > 0; i--){
        snake.points.push_back(
            (Vector2){ (float)i , 0.0f }
        );
    }
}

void bonnus_update(){

    // Drawing all bonnus points
    for(size_t i = 0; i < bonnus.points.size(); i++){
        frect.x = get_global_position(bonnus.points[i], world).x;
        frect.y = get_global_position(bonnus.points[i], world).y;
        SDL_SetRenderDrawColor(renderer, bonnus.r, bonnus.g, bonnus.b, 255);
        SDL_RenderFillRect(renderer, &frect);
    }
}

void snake_ckeck_autobite(){
    // Snake - check auto-bite
    for(size_t i = 1; i < snake.points.size(); i++){
        if(
            (snake.points[0].x == snake.points[i].x) &&
            (snake.points[0].y == snake.points[i].y) 
        ){ // auto-bite detected
            SDL_Log("Auto-bite detected\n");
            snake.points.clear();
            snake_add_points(3);
            snake.orientation = (Vector2){1.0f, 0.0f};
            break;
        }
    }
}

Uint32 snake_next_frame(void *userdata, SDL_TimerID timerID, Uint32 interval){
    Snake* snake = (Snake *)userdata;
    const Vector2 ORIENTATION = snake->orientation;
    Vector2 point_to_add = {};
    // for each snake point
    for(int i = (int)snake->points.size()-1; i >= 0; i--){

        if(snake->there_is_a_point_to_add){
            point_to_add.x = snake->points[i].x;
            point_to_add.y = snake->points[i].y;
        }

        if(i == 0){ // snake head point
            snake->points[i].x = next_world_position(world, ORIENTATION, snake->points[i]).x;
            snake->points[i].y = next_world_position(world, ORIENTATION, snake->points[i]).y;
        }else{ // snake body points
            snake->points[i].x = snake->points[i-1].x;
            snake->points[i].y = snake->points[i-1].y;
        }

        if(snake->there_is_a_point_to_add){
            snake->points.push_back(
                point_to_add
            );
        }
        snake->there_is_a_point_to_add = false;
    }
    snake->change_orientation_flag = true;

    snake_ckeck_autobite();

    return interval;
}

void snake_update(){
    
    // we get the global position to a frect for drawing
    for(size_t i = 0; i < snake.points.size(); i++){
        
        frect.x = get_global_position(snake.points[i], world).x;
        frect.y = get_global_position(snake.points[i], world).y;
        
        if(i == 0){
            SDL_SetRenderDrawColor(renderer, 158, 130, 52, SDL_ALPHA_OPAQUE);
        }else{
            SDL_SetRenderDrawColor(renderer, 18, 97, 42, SDL_ALPHA_OPAQUE);
        }

        SDL_RenderFillRect(renderer, &frect);
    }
    
}

int there_is_a_match_point_between(std::vector<Vector2>va, std::vector<Vector2>vb){
    for(size_t i = 0; i < va.size(); i++){
        for(size_t j = 0; j < vb.size(); j++){
            if(
                (va[i].x == vb[j].x)&&
                (va[i].y == vb[j].y)
            ){
                return i;
            }
        }
    }
    return -1;
}

bool RumbleGamepad(SDL_Gamepad *gamepad){
   return SDL_RumbleGamepad(
        gamepad,
        1000, //Uint16 low_frequency_rumble
        4000, //Uint16 high_frequency_rumble
        5000 //Uint32 duration_ms
    );
}

SDL_Gamepad *OpenGamepad(int *gamepad_index){
   return SDL_OpenGamepad(
        *SDL_GetGamepads(gamepad_index)
    );  
}


SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]){

    if(
        SDL_Init(SDL_INIT_VIDEO) == true
    ){
        SDL_Log("VIDEO INIT: PASS");
    }else{
        SDL_SetError("VIDEO INIT FAILED: %s", SDL_GetError());
    }

	if(
        SDL_Init(SDL_INIT_GAMEPAD) == true
    ){
		SDL_Log("GAMEPAD INIT: PASS");
	}else{
		SDL_SetError("GAMEPAD INIT FAILLED: %s", SDL_GetError());
	}

    bool SDL_SetAppMetadata_result = SDL_SetAppMetadata(
		appname,//const char *appname,
		appversion,//const char *appversion,
		appidentifier//const char *appidentifier
    );
	if(SDL_SetAppMetadata_result == true){
        	SDL_Log("SET METADATA: PASS");
	}else{
		SDL_SetError("SET METADATA FAILED: %s", SDL_GetError());
	}

    window = SDL_CreateWindow(
        title,
        width,
        height,
        //SDL_WINDOW_MAXIMIZED,
        SDL_WINDOW_RESIZABLE
    );
    if(window != NULL){
        SDL_Log("WINDOW CREATION: PASS");
    }else{
        SDL_SetError("WINDOW CREATION FAILED: %s", SDL_GetError());
    }
    // Window first update
    world.MAX_COLUMNS_COUNT = SDL_floorf(width / world.WIDTH_CELLS_SIZE) - 0U;
    world.MAX_LINES_COUNT = SDL_floorf(height / world.HEIGHT_CELLS_SIZE) - 0U;

    renderer = SDL_CreateRenderer(
        window,//SDL_Window *window,
        renderer_backend_name//const char *name
    );
    if(renderer != NULL){
        SDL_Log("RENDERER CREATION: PASS");
    }else{
        SDL_SetError("RENDERER CREATION FAILLED: %s", SDL_GetError());
    }

    //SDL_Log("Available renderer drivers:");
    //for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) {
    //    SDL_Log("%d. %s", i + 1, SDL_GetRenderDriver(i));
    //}
    SDL_Log("RENDERER: %s", SDL_GetRendererName(renderer));

    // snake firsts points
    snake_add_points(3);

    //firsts Bonnus points
    for(int i = 0; i < 2; i++){
        bonnus.points.push_back(
            (Vector2){
                SDL_rand(world.MIN_COLUMNS_COUNT),
                SDL_rand(world.MAX_LINES_COUNT)
            }
        );
    }
    
    //Snake Next frame Timer init
    if(
        (
            SDL_AddTimer(
                snake.speed, // Uint32 interval, 
                snake_next_frame, // SDL_TimerCallback callback, 
                &snake//NULL // void *userdata
            )
        ) == 0
    ){
        SDL_SetError("ADD SNAKE NEXT FRAME TIMER FAILED: %s", SDL_GetError());
    }

     // Bonnus point popping timer init
    if(
        (
            SDL_AddTimer(
                bonnus.POP_PERIOD, // Uint32 interval, 
                bonnus_next_pop, // SDL_TimerCallback callback, 
                &bonnus//NULL // void *userdata
            )
        ) == 0
    ){
        SDL_SetError("ADD BONNUS TIMER FAILED: %s", SDL_GetError());
    }

    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    
    bool *orentationFlag = &snake.change_orientation_flag;

    //SDL_Log("%i\n", event->type);
    switch(event->type){
        case SDL_EVENT_GAMEPAD_ADDED:
            if(
                (gamepad0 = OpenGamepad(0)) != NULL
            ){
                SDL_Log("GAMEPAD OPEN: PASS");
            }else{
                SDL_SetError("GAMEPAD OPEN FAILLED: %s", SDL_GetError());
            }

            if(
                RumbleGamepad(gamepad0) == true
            ){
                SDL_Log("RUMBLE GAMEPAD: PASS");
            }else{
                SDL_SetError("RUMBLE GAMEPAD FAILLED: %s", SDL_GetError());
            }
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            gamepad_axis_leftx = SDL_GetGamepadAxis(
                gamepad0, //SDL_Gamepad *gamepad
                SDL_GAMEPAD_AXIS_LEFTX //SDL_GamepadAxis axis
            );
            SDL_Log("AXIS LEFT X: %i", gamepad_axis_leftx);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            SDL_Log("GAMEPAD REMOVED");
            break;
        case SDL_EVENT_KEY_DOWN:
            if(
                (event->key.scancode == SDL_SCANCODE_W) &&
                *orentationFlag
            ){
                //SDL_Log("W");
                if(snake.orientation.y == 0.0f){
                    snake.orientation.y = -1.0f;
                }
                snake.orientation.x = 0.0f;
                *orentationFlag = false;
            }
            else if(
                (event->key.scancode == SDL_SCANCODE_S) &&
                *orentationFlag
            ){
                //SDL_Log("S");
                if(snake.orientation.y == 0.0f){
                    snake.orientation.y = 1.0f;
                }
                snake.orientation.x = 0.0f;
                *orentationFlag = false;
            }
            else if(
                (event->key.scancode == SDL_SCANCODE_A) &&
                *orentationFlag
            ){
                //SDL_Log("A");
                if(snake.orientation.x == 0.0f){
                    snake.orientation.x = -1.0f;
                }
                snake.orientation.y = 0.0f;
                *orentationFlag = false;
            }
            else if(
                (event->key.scancode == SDL_SCANCODE_D) &&
                *orentationFlag
            ){
                //SDL_Log("D");
                if(snake.orientation.x == 0.0f){
                    snake.orientation.x = 1.0f;
                }
                snake.orientation.y = 0.0f;
                *orentationFlag = false;
            }
            if(event->key.scancode == SDL_SCANCODE_ESCAPE){
                return SDL_APP_SUCCESS;
            }
            break;
        case SDL_EVENT_KEY_UP:
            if(event->key.scancode == SDL_SCANCODE_W){
                //SDL_Log("D");
                snake.direction.y = 0.0f;
            }
            else if(event->key.scancode == SDL_SCANCODE_S){
                //SDL_Log("D");
                snake.direction.y = 0.0f;
            }
            if(event->key.scancode == SDL_SCANCODE_D){
                //SDL_Log("D");
                snake.direction.x = 0.0f;
            }
            else if(event->key.scancode == SDL_SCANCODE_A){
                //SDL_Log("D");
                snake.direction.x = 0.0f;
            }
            break;
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
            break;
        default:
            break;
    }
    

	return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void *appstate){

    last = now;
    now = SDL_GetPerformanceCounter();
    deltaTime = (double)((now - last) * 1.0 / SDL_GetPerformanceFrequency());

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(renderer); // Drawing Black screen
    
    // world cells update
    world.MAX_COLUMNS_COUNT = SDL_floorf(width / world.WIDTH_CELLS_SIZE) - 0U;
    world.MAX_LINES_COUNT = SDL_floorf(height / world.HEIGHT_CELLS_SIZE) - 0U;
    
    // Drawing cells
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, SDL_ALPHA_OPAQUE);
    // Vertical lines
    for(int i = world.MAX_COLUMNS_COUNT; i > 0; i--){
        int x_pos = world.WIDTH_CELLS_SIZE * i;
        if(
            ( 
                SDL_RenderLine(
                    renderer,//SDL_Renderer *renderer, 
                    x_pos, 0.0f,//float x1, float y1, 
                    x_pos, height//float x2, float y2
                )
            )== false
        ){
            SDL_SetError("RENDER LINE FAILED AT v_LINE %d: %s\n", i, SDL_GetError());
        }
    }
    // Horizontal lines
    for(int j = world.MAX_LINES_COUNT; j > 0; j--){
        int y_pos = world.HEIGHT_CELLS_SIZE * j;
        if(
            ( 
                SDL_RenderLine(
                    renderer,//SDL_Renderer *renderer, 
                    0.0f, y_pos,//float x1, float y1, 
                    width, y_pos//float x2, float y2
                )
            ) == false
        ){
            SDL_SetError("RENDER LINE FAILED AT h_LINE %d: %s\n", j, SDL_GetError());
        }
    }
    
    // FRect size update for drawing snake and bonnus
    frect.w = world.WIDTH_CELLS_SIZE * snake.size_scale_w;
    frect.h = world.HEIGHT_CELLS_SIZE * snake.size_scale_h;

    // Snake
    snake_update();

    // Bonnus
    bonnus_update();
    // Check match point to erase
    int match_index = there_is_a_match_point_between(
        bonnus.points, snake.points
    );
    if(
        match_index >= 0
    ){
        // erase bonnus point
        bonnus.points.erase(bonnus.points.begin() + match_index);
        // add a snake point
        snake.there_is_a_point_to_add = true;
    }

    // Show the result
    SDL_RenderPresent(renderer); // show the result
    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result){
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Log("PROGRAM ENDED");
}
