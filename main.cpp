#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <vector>
#include <array>

// App Meta-datas
const char *appname = "snake\0";
const char *appversion = "0.0.1\0";
const char *appidentifier = "game.snake.cm\0";

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

// DEBUG
Vector2 test = {0.0f , 0.0f};

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

// World cells stuffs
typedef struct{
    int MIN_COLUMNS_COUNT = 0;
    int MAX_COLUMNS_COUNT;
    int MIN_LINES_COUNT = 0;
    int MAX_LINES_COUNT;
    int CELLS_PORTION = 5;
    int WIDTH_CELLS_SIZE = 
        SDL_floorf(width * CELLS_PORTION / 100.0f) ;
    int HEIGHT_CELLS_SIZE = 
        SDL_floorf(WIDTH_CELLS_SIZE * 1.0f);
}World;

World world;

// Snake stuffs - WASD config
typedef struct{
    SDL_FRect frect = {};
    std::vector <Vector2> points;
    uint32_t speed = 150;
    Vector2 direction = {0.0f , 0.0f};
    Vector2 orientation = {1.0f , 0.0f};
    float size_scale_w = 1.0f;
    float size_scale_h = 1.0f;
}Snake;

Snake snake;

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

Uint32 snake_next_frame(void *userdata, SDL_TimerID timerID, Uint32 interval){
    // for each snake point
    for(int i = (int)snake.points.size()-1; i >= 0; i--){
        if(i == 0){ // snake head point
            Vector2 orientation{snake.orientation.x , snake.orientation.y};
            snake.points[i].x = next_world_position(world, orientation, snake.points[i]).x;
            snake.points[i].y = next_world_position(world, orientation, snake.points[i]).y;
        }else{ // snake body points
            snake.points[i].x = snake.points[i-1].x;
            snake.points[i].y = snake.points[i-1].y;
        }
    }

    return interval;
}

void snake_update(){

    snake.frect.w = world.WIDTH_CELLS_SIZE * snake.size_scale_w;
    snake.frect.h = world.HEIGHT_CELLS_SIZE * snake.size_scale_h;
    
    // we get the global position to a frect for drawing
    for(size_t i = 0; i < snake.points.size(); i++){
        
        snake.frect.x = get_global_position(snake.points[i], world).x;
        snake.frect.y = get_global_position(snake.points[i], world).y;
        
        if(i == 0){
            SDL_SetRenderDrawColor(renderer, 158, 130, 52, SDL_ALPHA_OPAQUE);
        }else{
            SDL_SetRenderDrawColor(renderer, 18, 97, 42, SDL_ALPHA_OPAQUE);
        }

        SDL_RenderFillRect(renderer, &snake.frect);
    }
    
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
    for(int i = 0; i > -3; i--){
        snake.points.push_back(
            (Vector2){ (float)i , 0.0f }
        );
    }

    //Snake Timer stuffs
    if(
        (
            SDL_AddTimer(
                snake.speed, // Uint32 interval, 
                snake_next_frame, // SDL_TimerCallback callback, 
                NULL // void *userdata
            )
        ) == 0
    ){
        SDL_SetError("ADD TIMER FAILED: %s", SDL_GetError());
    }


    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    
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
            if(event->key.scancode == SDL_SCANCODE_W){
                //SDL_Log("W");
                snake.direction.y = -1.0f;
                snake.orientation.y = -1.0f;
                snake.orientation.x = 0.0f;
            }
            else if(event->key.scancode == SDL_SCANCODE_S){
                //SDL_Log("S");
                snake.direction.y = 1.0f;
                snake.orientation.y = 1.0f;
                snake.orientation.x = 0.0f;
            }
            else if(event->key.scancode == SDL_SCANCODE_A){
                //SDL_Log("A");
                snake.direction.x = -1.0f;
                snake.orientation.x = -1.0f;
                snake.orientation.y = 0.0f;
            }
            else if(event->key.scancode == SDL_SCANCODE_D){
                //SDL_Log("D");
                snake.direction.x = 1.0f;
                snake.orientation.x = 1.0f;
                snake.orientation.y = 0.0f;
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
    
    // Snake
    snake_update();

    SDL_RenderPresent(renderer); // show the result
    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result){
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Log("PROGRAM ENDED");
}
