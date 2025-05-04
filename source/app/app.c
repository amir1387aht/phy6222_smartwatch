#include "app/app.h"

#define BALL_RADIUS     3
#define PADDLE_WIDTH    4
#define PADDLE_HEIGHT   20
#define PLAYER_PADDLE_X 2
#define AI_PADDLE_X     (ST7735_TFTWIDTH_128 - PADDLE_WIDTH - 2)
#define BALL_COLOR      ST77XX_BLUE // or display_get_color(0, 0, 255)
#define BALL_FLASH_COLOR ST77XX_RED // or display_get_color(255, 0, 0)
#define PADDLE_COLOR    ST77XX_RED
#define BG_COLOR        ST77XX_BLACK
#define TEXT_COLOR      ST77XX_WHITE
#define TOP_BOUNDARY    11

static int ball_x = ST7735_TFTWIDTH_128 / 2;
static int ball_y = ST7735_TFTHEIGHT_128 / 2;
static int ball_vx = 1;
static int ball_vy = 1;
static int player_paddle_y = ST7735_TFTHEIGHT_128 / 2 - PADDLE_HEIGHT / 2;
static int ai_paddle_y = ST7735_TFTHEIGHT_128 / 2 - PADDLE_HEIGHT / 2;
static int player_paddle_dir = 1;
static bool game_active = true;

// Variables for ball flashing and reset
static bool reset_in_progress = false;
static int flash_counter = 0;
#define FLASH_TOTAL_COUNT 12

// Direction alternator for resets (instead of using rand())
static int direction_toggle = 0;

void start_ball_reset() {
    // Start flashing
    reset_in_progress = true;
    flash_counter = 0;
    
    // Erase the ball at its current position
    gfx_fill_circle(ball_x, ball_y, BALL_RADIUS, BG_COLOR);
    
    // Reset ball position immediately to center
    ball_x = ST7735_TFTWIDTH_128 / 2;
    ball_y = ST7735_TFTHEIGHT_128 / 2;
    
    // Alternate directions each reset without using rand()
    direction_toggle++;
    ball_vx = (direction_toggle % 2 == 0) ? 1 : -1;
    ball_vy = ((direction_toggle / 2) % 2 == 0) ? 1 : -1;
}

void update_ball_reset() {
    if (!reset_in_progress) return;
    
    // Determine color based on flash counter
    uint16_t current_flash_color = (flash_counter % 2 == 0) ? BALL_COLOR : BALL_FLASH_COLOR;
    
    // Draw ball with the current flash color
    gfx_fill_circle(ball_x, ball_y, BALL_RADIUS, current_flash_color);
    
    // Increment flash counter
    flash_counter++;
    
    // Check if flashing is complete
    if (flash_counter >= FLASH_TOTAL_COUNT) {
        reset_in_progress = false;
        // Ensure ball is drawn with the correct final color
        gfx_fill_circle(ball_x, ball_y, BALL_RADIUS, BALL_COLOR);
    }

    WaitMs(100);
}

void update_ball() {
    // If reset is in progress, handle flashing instead of normal movement
    if (reset_in_progress) {
        update_ball_reset();
        return;
    }
    
    gfx_fill_circle(ball_x, ball_y, BALL_RADIUS, BG_COLOR);
    ball_x += ball_vx;
    ball_y += ball_vy;
    
    // Collision with top boundary
    if (ball_y - BALL_RADIUS <= TOP_BOUNDARY) {
        ball_y = TOP_BOUNDARY + BALL_RADIUS;
        ball_vy = -ball_vy;
    }
    
    // Collision with bottom border
    if ((ball_y + BALL_RADIUS) >= ST7735_TFTHEIGHT_128) {
        ball_vy = -ball_vy;
    }
    
    // Collision with player's paddle
    if (ball_vx < 0 && (ball_x - BALL_RADIUS) <= (PLAYER_PADDLE_X + PADDLE_WIDTH)) {
        if ((ball_y + BALL_RADIUS >= player_paddle_y) && (ball_y - BALL_RADIUS <= player_paddle_y + PADDLE_HEIGHT)) {
            ball_x = PLAYER_PADDLE_X + PADDLE_WIDTH + BALL_RADIUS; // reposition ball outside paddle
            ball_vx = -ball_vx;
        } else if (ball_x - BALL_RADIUS < 0) {
            // Player missed the ball
            start_ball_reset();
            return;
        }
    }
    
    // Collision with AI's paddle
    if (ball_vx > 0 && (ball_x + BALL_RADIUS) >= AI_PADDLE_X) {
        if ((ball_y + BALL_RADIUS >= ai_paddle_y) && (ball_y - BALL_RADIUS <= ai_paddle_y + PADDLE_HEIGHT)) {
            ball_x = AI_PADDLE_X - BALL_RADIUS; // reposition ball outside paddle
            ball_vx = -ball_vx;
        } else if (ball_x + BALL_RADIUS > ST7735_TFTWIDTH_128) {
            // AI missed the ball
            start_ball_reset();
            return;
        }
    }
    
    gfx_fill_circle(ball_x, ball_y, BALL_RADIUS, BALL_COLOR);
}

void update_paddles() {
    gfx_fill_rect(PLAYER_PADDLE_X, player_paddle_y, PADDLE_WIDTH, PADDLE_HEIGHT, BG_COLOR);
    gfx_fill_rect(AI_PADDLE_X, ai_paddle_y, PADDLE_WIDTH, PADDLE_HEIGHT, BG_COLOR);
    
    // Move player's paddle automatically, change direction on button press (handled in callback)
    player_paddle_y += player_paddle_dir * 2;
    if (player_paddle_y < TOP_BOUNDARY) {
        player_paddle_y = TOP_BOUNDARY;
        player_paddle_dir = 1;
    } else if (player_paddle_y + PADDLE_HEIGHT > ST7735_TFTHEIGHT_128) {
        player_paddle_y = ST7735_TFTHEIGHT_128 - PADDLE_HEIGHT;
        player_paddle_dir = -1;
    }
    
    // Simple AI to follow ball
    if (ai_paddle_y + PADDLE_HEIGHT / 2 < ball_y && (ai_paddle_y + PADDLE_HEIGHT) < ST7735_TFTHEIGHT_128)
        ai_paddle_y += 1;
    else if (ai_paddle_y + PADDLE_HEIGHT / 2 > ball_y && ai_paddle_y > TOP_BOUNDARY)
        ai_paddle_y -= 1;
    
    gfx_fill_rect(PLAYER_PADDLE_X, player_paddle_y, PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_COLOR);
    gfx_fill_rect(AI_PADDLE_X, ai_paddle_y, PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_COLOR);
}

void posedge_callback_wakeup(GPIO_Pin_e pin, IO_Wakeup_Pol_e type) {
    // Invert the player's paddle movement direction on button press
    player_paddle_dir = -player_paddle_dir;
}

void app_init() {
    hal_gpioin_register(BUTTON_PIN, posedge_callback_wakeup, NULL);
    gfx_init(ST7735_TFTWIDTH_128, ST7735_TFTHEIGHT_128, 0);
    gfx_set_font(&FreeMono9pt7b);
    gfx_set_text_color(TEXT_COLOR);
    gfx_set_text_size(1);
    gfx_set_text_wrap(true);
    gfx_set_cursor(14, 0);
    gfx_print("PONG GAME");
    gfx_fill_rect(PLAYER_PADDLE_X, player_paddle_y, PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_COLOR);
    gfx_fill_rect(AI_PADDLE_X, ai_paddle_y, PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_COLOR);
    gfx_fill_circle(ball_x, ball_y, BALL_RADIUS, BALL_COLOR);
}

void app_update() {
    if (!game_active)
        return;
    update_ball();
    update_paddles();
    WaitMs(10);
}