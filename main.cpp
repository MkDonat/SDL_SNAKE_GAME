#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

// App Meta-datas stuffs
const char *appname{"snake\0"};
const char *appversion{"0.0.1\0"};
const char *appidentifier{"game.snake.cm\0"};

// Delta Time stuffs
Uint64 now{SDL_GetPerformanceCounter()};
Uint64 last{0U};
double deltaTime{0.0f};

// Window stuffs
SDL_Window *window = NULL;
const char *title = "Snake\0";
int width = 960; // default: 720U - psvita: 960
int height = 544;// default: 468U - psvita: 544

// Renderer stuffs
SDL_Renderer *renderer = NULL;
const char *renderer_backend_name = NULL;//"vulkan\0";

// Frect
SDL_FRect frect;

// Gamepad stuffs
SDL_Gamepad *gamepad0{NULL};
Sint16 gamepad_axis_leftx{};

// SDL_FPoint custom function          
SDL_FPoint Vector2_normalized(SDL_FPoint v) {
    float l = v.x * v.x + v.y * v.y;
	if (l != 0) {
		l = SDL_sqrtf(l);
        return (SDL_FPoint){
            v.x /= l,
		    v.y /= l
        };
	}
    else return (SDL_FPoint){0.0f,0.0f};
}

// World cells stuffs
struct World{
    int MIN_COLUMNS_COUNT = 0;
    int MAX_COLUMNS_COUNT = 0;
    int MIN_LINES_COUNT = 0;
    int MAX_LINES_COUNT = 0;
    int CELLS_PORTION = 5;
    int WIDTH_CELLS_SIZE = 
        (int)SDL_floorf((float)width * (float)CELLS_PORTION / 100.0f);
    int HEIGHT_CELLS_SIZE = 
        (int)SDL_floorf((float)height * (float)CELLS_PORTION * 1.5f / 100.0f );
};
World world;

// Snake stuffs - WASD config
struct Snake{
    std::vector <SDL_FPoint> points;
    uint32_t speed = 150; // 150
    SDL_FPoint direction = {0.0f , 0.0f};
    SDL_FPoint orientation = {1.0f , 0.0f};
    float size_scale_w = 1.0f;
    float size_scale_h = 1.0f;
    bool there_is_a_point_to_add = false;
    bool change_orientation_flag = false;
};
Snake snake;

struct BONNUS{
    std::vector<SDL_FPoint>points;
    uint8_t r = 38;
    uint8_t g = 65;
    uint8_t b = 153;
    int MAX_COUNT = 5;
    int POP_PERIOD = 1500;
};
BONNUS bonnus;

// Custom functions declaration and implementation
SDL_FPoint get_global_position(SDL_FPoint world_pos, World world){
    return (SDL_FPoint){
        world_pos.x * world.WIDTH_CELLS_SIZE 
        ,
        world_pos.y * world.HEIGHT_CELLS_SIZE
    };
}

SDL_FPoint next_world_position(
    World world, SDL_FPoint orientation, SDL_FPoint world_position
)
{
    std::array<std::array<float, 4>, 2> d = {
        {
            {
                world_position.x, Vector2_normalized(orientation).x, 
                (float)world.MAX_COLUMNS_COUNT-1.0f, (float)world.MIN_COLUMNS_COUNT
            }
            ,
            {
                world_position.y, Vector2_normalized(orientation).y, 
                (float)world.MAX_LINES_COUNT-1.0f, (float)world.MIN_LINES_COUNT
            }  
        }
    };
     
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
    return (SDL_FPoint){d[0][0], d[1][0]};
}

Uint32 bonnus_next_pop(
    void *userdata, SDL_TimerID timerID, Uint32 interval
)
{
    // setting limits
    int bonnus_max_col = world.MAX_COLUMNS_COUNT;// - 1;
    int bonnus_max_li = world.MAX_LINES_COUNT ;//- 1;
    // checking bonnus number
    if(bonnus.points.size() < bonnus.MAX_COUNT){
        // adding a random point
        bonnus.points.push_back(
            (SDL_FPoint){
                (float)SDL_rand(bonnus_max_col),
                (float)SDL_rand(bonnus_max_li)
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
            (SDL_FPoint){ (float)i , 0.0f }
        );
    }
}

void render_bonnus(){
    // Drawing all bonnus points
    for(size_t i = 0; i < bonnus.points.size(); i++){
        frect.x = get_global_position(bonnus.points[i], world).x;
        frect.y = get_global_position(bonnus.points[i], world).y;
        SDL_SetRenderDrawColor(
            renderer, bonnus.r, bonnus.g, bonnus.b, 255
        );
        SDL_RenderFillRect(renderer, &frect);
    }
}

void snake_ckeck_autobite(){
    for(size_t i = 1; i < snake.points.size(); i++){
        if(
            (snake.points[0].x == snake.points[i].x) &&
            (snake.points[0].y == snake.points[i].y) 
        ){
            //SDL_Log("Auto-bite detected\n");
            snake.points.clear();
            snake_add_points(3);
            snake.orientation = (SDL_FPoint){1.0f, 0.0f};
            break;
        }
    }
}

Uint32 snake_next_frame(
    void *userdata, SDL_TimerID timerID, Uint32 interval
)
{
    Snake* snake = (Snake *)userdata;
    const SDL_FPoint ORIENTATION = snake->orientation;
    SDL_FPoint point_to_add = {};
    // for each snake point
    for(int i = std::size(snake->points)-1; i >= 0; i--){
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

void render_snake(){
    // we get the global position to a frect for drawing
    for(size_t i = 0; i < snake.points.size(); i++){
        
        frect.x = get_global_position(snake.points[i], world).x;
        frect.y = get_global_position(snake.points[i], world).y;
        
        if(i == 0){
            SDL_SetRenderDrawColor(
                renderer, 158, 130, 52, SDL_ALPHA_OPAQUE);
        }else{
            SDL_SetRenderDrawColor(
                renderer, 18, 97, 42, SDL_ALPHA_OPAQUE);
        }

        SDL_RenderFillRect(renderer, &frect);
    }
}

int there_is_a_match_point_between(
    std::vector<SDL_FPoint>va, std::vector<SDL_FPoint>vb
)
{
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

bool erase_from_collection(auto &collection, auto &p){
    constexpr float epsilon = 1e-04f;
    auto new_end{
        std::remove_if(
            std::begin(collection),
            std::end(collection), 
            [&](const auto &e){
                if(
                    std::fabs(e.x-p.x)<epsilon &&
                    std::fabs(e.y-p.y)<epsilon
                ){
                    return true;
                }
                else return false;
            }
        )
    };
    bool result{
        new_end!=std::end(collection)
    };
    collection.erase(new_end,std::end(collection));
    return result;
}


SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]){

    // SDL_Log("RENDERERS LIST:");
    // for(int i = 0; i<=SDL_GetNumRenderDrivers()-1; i++){
    //     SDL_Log("%d- %s",i,SDL_GetRenderDriver(i));
    // }

    // PS VITA STUFFS
    SDL_SetHint(SDL_HINT_VITA_RESOLUTION, "720");
    SDL_SetHint(SDL_HINT_VITA_PVR_OPENGL, "1");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_VITA_ENABLE_FRONT_TOUCH, "0");
    SDL_SetHint(SDL_HINT_VITA_ENABLE_BACK_TOUCH, "0");
    
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

    //Window initial ratio
    float w_ini_ratio{(float)width/(float)height};
    // Getting columns count
    world.MAX_COLUMNS_COUNT = {
        (int)SDL_floorf(width) / world.WIDTH_CELLS_SIZE
    };
    world.MAX_LINES_COUNT = {
        (int)SDL_floorf(height) / world.HEIGHT_CELLS_SIZE
    };
    // Re-calculation window size 
    width = world.MAX_COLUMNS_COUNT * world.WIDTH_CELLS_SIZE;
    height = world.MAX_LINES_COUNT * world.HEIGHT_CELLS_SIZE;
    // Window final ratio
    float w_fi_ratio{(float)width/(float)height};
    SDL_Log(
        "init ratio: %.2f, final ratio: %.2f, columns: %d, lines: %d",
        w_ini_ratio, w_fi_ratio,
        world.MAX_COLUMNS_COUNT, world.MAX_LINES_COUNT
    );
    // Create window
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
    // Current Backend
    SDL_Log("RENDERER: %s", SDL_GetRendererName(renderer));

    // snake firsts points
    snake_add_points(3);

    //firsts Bonnus points
    for(int i = 0; i < 2; i++){
        bonnus.points.push_back(
            (SDL_FPoint){
                (float)SDL_rand(world.MIN_COLUMNS_COUNT),
                (float)SDL_rand(world.MAX_LINES_COUNT)
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
        SDL_SetError(
            "ADD SNAKE NEXT FRAME TIMER FAILED: %s", 
            SDL_GetError()
        );
    }

     // Bonnus point popping timer init
    if(
        (
            SDL_AddTimer(
                bonnus.POP_PERIOD, // Uint32 interval, 
                bonnus_next_pop, // SDL_TimerCallback callback, 
                &bonnus // void *userdata
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
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            SDL_Log("GAMEPAD REMOVED");
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            if(
                event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP &&
                *orentationFlag
            ){
                //SDL_Log("W");
                if(snake.orientation.y == 0.0f){
                    snake.orientation.y = -1.0f;
                }
                snake.orientation.x = 0.0f;
                *orentationFlag = false;
            }else if(
                event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN &&
                *orentationFlag                
            ){
                //SDL_Log("S");
                if(snake.orientation.y == 0.0f){
                    snake.orientation.y = 1.0f;
                }
                snake.orientation.x = 0.0f;
                *orentationFlag = false;
            }else if(
                event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT &&
                *orentationFlag
            ){
                //SDL_Log("A");
                if(snake.orientation.x == 0.0f){
                    snake.orientation.x = -1.0f;
                }
                snake.orientation.y = 0.0f;
                *orentationFlag = false;
            }else if(
                event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT &&
                *orentationFlag
            ){
                //SDL_Log("D");
                if(snake.orientation.x == 0.0f){
                    snake.orientation.x = 1.0f;
                }
                snake.orientation.y = 0.0f;
                *orentationFlag = false;
            };
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            if(
                event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN ||
                event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP
            ){
                snake.direction.y = 0.0f;
            };
            if(
                event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT ||
                event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT
            ){
                snake.direction.x = 0.0f;
            };
            if(
                event->gbutton.button == SDL_GAMEPAD_BUTTON_START
            ){
                return SDL_APP_SUCCESS;
            }
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
    render_snake();

    // Bonnus
    render_bonnus();
    // Check if bonnus was taken by snake
    int match_index = there_is_a_match_point_between(
        bonnus.points, snake.points
    );
    if(
        match_index >= 0 
    ){
        // erase bonnus point
        erase_from_collection(
            bonnus.points,
            bonnus.points[match_index]
        );
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
