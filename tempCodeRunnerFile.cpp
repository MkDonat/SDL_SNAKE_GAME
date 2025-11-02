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