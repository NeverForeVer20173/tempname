#ifndef APPLICATIONS_OLED_UI_H_
#define APPLICATIONS_OLED_UI_H_

#define LOW_SPEED           1   // 一帧滑动1px
#define MID_SPEED           3   // 一帧滑动3px
#define HIGH_SPEED          4   // 一帧滑动4px
#define FAST_SPEED          6   // 一帧滑动6px
#define SO_FAST_SPEED       8   // 一帧滑动8px

typedef enum {
    low_speed = LOW_SPEED,
    mid_speed = MID_SPEED,
    high_speed = HIGH_SPEED,
    fast_speed = FAST_SPEED,
    so_fast_speed = SO_FAST_SPEED
} Speed_ENUM;

typedef enum {
    UI_IDLE = 0,
    UI_ANIM_LEFT,
    UI_ANIM_RIGHT,
    UI_ANIM_UP,   // 二级菜单
    UI_ANIM_DOWN  // 二级菜单
} ui_anim_state_t;

typedef enum {
    UI_STATE_WATCH = 0,
    UI_STATE_WEATHER_MENU,
    UI_STATE_MENU,
    UI_STATE_PLANT_MENU,
    UI_STATE_NOTE_MENU,
    UI_STATE_MOTOR_MENU,
    UI_STATE_MENU_TO_WEATHER,
    UI_STATE_WEATHER_TO_MENU,
    UI_STATE_MENU_TO_PLANT,
    UI_STATE_PLANT_TO_MENU,
    UI_STATE_MENU_TO_NOTE,
    UI_STATE_NOTE_TO_MENU,
    UI_STATE_MENU_TO_MOTOR,
    UI_STATE_MOTOR_TO_MENU,
    UI_STATE_MOTOR_OPENED,
    UI_STATE_MOTOR_CLOSED,
    UI_STATE_MOTOR_AUTO

} UIState_ENUM;

extern const char* words[];
extern void ui_run(char* a ,char* a_trg,int b);
extern void Show_Menu(Speed_ENUM Speed_choose);
extern void Game_Menu(void);
extern void To_Game_Menu_Display(void);
extern void Game_To_Menu_Display(void);
extern void update_menu_to_game_anim(void);
extern void update_game_to_menu_anim(void);
extern int oled_ui_thread(void);

#endif /* APPLICATIONS_OLED_UI_H_ */
