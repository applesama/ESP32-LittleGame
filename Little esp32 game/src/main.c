
#include <driver/gpio.h>
#include <esp_adc_cal.h>
#include <esp_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <nvs_flash.h>
#include "fonts.h"
#include "graphics.h"




#define USE_WIFI 0
#define DISPLAY_VOLTAGE 1
#define DISPLAY_IMAGE_WAVE 1
// this makes it faster but uses so much extra memory
// that you can't use wifi at the same time
#define COPY_IMAGE_TO_RAM 1

uint32_t vref;

extern image_header spaceship_image;       
extern image_header enemy_plant;           
extern image_header Boom_image1;           
extern image_header cannonball_blue_image; 
extern image_header Enemy1;                
extern image_header Enemy1_Boom;           
extern image_header Self_image;
extern image_header Self_Boom_image;
typedef struct pos
{
    int x;
    int y;
    int z;
    int h;
    int speed;
    int colour;
    bool allowedGo;
} pos;
uint32_t vref;


char number_str[100];
bool godMode = false;
int score = 0;
int defaultHpOfBoss = 5;//default HP of boss.
int life = 3; //hit points of the player


void app_main()
{
    
    *((unsigned *)0x3FF49018) |= (1 << 9);
    graphics_init();
    set_orientation(PORTRAIT);

    while (true)
    {
        
        vTaskDelay(500 / portTICK_RATE_MS);//without this, when we exit the grade board after the game is finished, then the system may conside us are still pressng the button and may start the game directly(the system runs the game too fast)
        godMode = false;

        cls(rgbToColour(232, 192, 232));
        setFontColour(0, 0, 200);
        setFont(FONT_UBUNTU16);
        print_xy("Space Warfare", 10, 30);

        setFontColour(235, 23, 220);
        setFont(FONT_SMALL);
        print_xy("Last Score:", 3, 50);
        sprintf(number_str, "%d", score);
        print_xy(number_str, 70, LASTY);

        print_xy("Last Boss Killed:", 3, LASTY + 10);
        sprintf(number_str, "%d", (defaultHpOfBoss - 5) / 5);
        print_xy(number_str, 100, LASTY);
        print_xy("L Button: Normal Mode", 3, LASTY + 10);
        print_xy("R Button: God Mode", 3, LASTY + 10);

        flip_frame();
        while ((gpio_get_level(0) == 1) && (gpio_get_level(35) == 1))
        {
        }
        if (gpio_get_level(35) == 0)
            godMode = true;

        if(godMode)life = 20;
        else life = 3;

        cls(rgbToColour(232, 192, 232));
        setFont(FONT_SMALL);
        print_xy("Get Ready!", (display_width / 2) - 30, display_height / 2);
        flip_frame();
        vTaskDelay(3000 / portTICK_RATE_MS);
        setFontColour(255, 255, 255);
        
        
        defaultHpOfBoss = 5;
        int enemySpeed = 1;
        int x = (display_width / 2); //enemy x coordinate
        int y = 40;                  //enemy y coordinate
        int level = 0; //player level
        score = 0;

        pos stars[7]; //0-2:enemy bullets properties; 3: player position; 4-6: player bullets
        for (int i = 0; i < 3; i++)
        {
            stars[i].x = x;
            stars[i].y = y;
            stars[i].z = 5;
            stars[i].h = 5;
            stars[i].speed = 4;
            stars[i].colour = rgbToColour(255, 0, 0);
            stars[i].allowedGo = false;
        }

        stars[3].x = 15; //this is properties the ship for player controlled
        stars[3].y = (display_height - 10);
        stars[3].z = 20;
        stars[3].h = 5;

        for (int i = 4; i < 7; i++) // properties of player's bullets
        {
            stars[i].x = stars[3].x;
            stars[i].y = stars[3].y - 10;
            stars[i].speed = 3;
            stars[i].allowedGo = false;
        }

        bool goToRight = true; //enemy moves to the right when it's true and false for moves left
        bool killedEnemy = false;
        bool bossBattle = false;
        int killTime = 0;     //counts how many enemy kills to decide if boss appear.
        int boomDuration = 0; //make sure the boom displayed for a while.
        int hpOfBoss = 5;     //hit points of the boss

        while (1)
        {
            cls(rgbToColour(232, 192, 232));
            print_xy("Score:", 65, 6);
            sprintf(number_str, "%d", score);
            print_xy(number_str, 105, 6);

            if (life == 1)
                setFontColour(255, 0, 0);
            if (life == 2)
                setFontColour(255, 255, 0);
            if (life >= 3)
                setFontColour(0, 255, 0); //the player's HP will shows in different colors.

            print_xy("Life: ", 0, 6);
            sprintf(number_str, "%d", life);
            print_xy(number_str, 30, 6);

            setFontColour(255, 255, 255);

            print_xy("Level: ", 0, LASTY + 10);
            sprintf(number_str, "%d", level);
            print_xy(number_str, 35, LASTY);

            print_xy("BossLv. ", 65, LASTY);
            sprintf(number_str, "%d", (defaultHpOfBoss / 5));
            print_xy(number_str, 110, LASTY); // boss will become harder and harder to kill as its HP increases every time it got killed

            draw_image(&Self_image, stars[3].x, stars[3].y); //your own ship

            if (!(GPIO.in & 1))
            {
                stars[3].x = stars[3].x - 2;
                //if(stars[3].x<0) stars[3].x=0;
                if (stars[3].x < 15)
                    stars[3].x = 15;
            }
            if (!(GPIO.in1.data & 8))
            {
                stars[3].x = stars[3].x + 2;
                //if(stars[3].x>135-20) stars[3].x=135-20;
                if (stars[3].x > 135 - 15)
                    stars[3].x = 135 - 15;
            }
            if (boomDuration == 100)
            { //enemyShip respawn
                if (bossBattle)
                {
                    x = 14 + rand() % (display_width - 14);
                    bossBattle = false;
                }
                else
                { //normal battle
                    x = 10 + rand() % (display_width - 10);
                }
            }

            if (level == 1)
            {
                for (int i = 4; i < 7; i++)
                {
                    stars[i].speed = 5;
                }
            } //award  higher bullet speed once kill enough enemy
            if ((level == 3) && (!godMode))
            {
                for (int i = 4; i < 7; i++)
                {
                    stars[i].speed = 7;
                }
            } //award  higher bullet speed once kill enough enemy
            if ((level == 7) || (godMode))
            {
                for (int i = 4; i < 7; i++)
                {
                    stars[i].speed = 10;
                }
            } //award  higher bullet speed once kill enough enemy

            if (killTime == 5) //kill enough enemy to enter bossbattle
            {
                bossBattle = true;
                for (int i = 0; i < 3; i++)
                {
                    if (level < 3)
                        stars[i].speed = 3;
                    if (level == 3)
                    {
                        stars[i].speed = 5;
                        enemySpeed = 3;
                    }
                    if (level == 5)
                        stars[i].speed = 7;
                    if (level == 7)
                    {
                        stars[i].speed = 9;
                        enemySpeed = 4; // different normal enemy firepower and mobility in different levels
                    }
                }
            }

            if ((killedEnemy) && (boomDuration < 100))
            {
                draw_image(&Enemy1_Boom, x, y); //make sure the boom image displayed for a while
            }
            else
            {
                if (!bossBattle) // in normal battle
                {
                    stars[0].colour = rgbToColour(0, 100, 0);

                    if ((level <= 7) && (level >= 5))
                        enemySpeed = 2;
                    if (level > 7)
                        enemySpeed = 3; //speed up for normal enemy as level increase.
                    killedEnemy = false;
                    boomDuration = 0;
                    if (x <= (display_width - 10) && goToRight) //enemy moves to right
                    {
                        x += enemySpeed;
                    }
                    else
                    {
                        if (x >= (display_width - 10))
                            x = (display_width - 10);
                        goToRight = false;
                    }
                    if (x >= 10 && !goToRight) //enemy moves to left
                    {
                        x -= enemySpeed;
                    }
                    else
                    {
                        if (x <= 10)
                            x = 10; //make sure enemy does not fly out the screen
                        goToRight = true;
                    }
                    draw_image(&Enemy1, x, y);
                }
                else if (bossBattle) // in boss battle
                {

                    stars[0].colour = rgbToColour(255, 0, 0);
                    stars[1].colour = rgbToColour(255, 165, 0);
                    stars[2].colour = rgbToColour(255, 255, 0);
                    killedEnemy = false;
                    boomDuration = 0;
                    if (x <= (display_width - 14) && goToRight)
                    {
                        x += enemySpeed;
                    }
                    else
                    {
                        if (x >= (display_width - 14))
                            x = (display_width - 14);
                        goToRight = false;
                    }
                    if (x >= 14 && !goToRight)
                    {
                        x -= enemySpeed;
                    }
                    else
                    {
                        if (x <= 14)
                            x = 14;
                        goToRight = true;
                    }
                    draw_image(&enemy_plant, x, y);
                }
            }

            if ((stars[0].allowedGo == false) && (stars[0].y == y) && (killedEnemy == false))
            {
                stars[0].allowedGo = true;
            }
            if (stars[0].allowedGo == true)
                stars[0].y += stars[0].speed;

            if ((stars[1].allowedGo == false) && (stars[1].y == y) && (stars[0].y > display_height / 4) && (bossBattle) && (killedEnemy == false))
            {
                stars[1].allowedGo = true;
            }
            if (stars[1].allowedGo == true)
                stars[1].y += stars[1].speed;

            if ((stars[2].allowedGo == false) && (stars[2].y == y) && (stars[1].y > display_height / 4) && (bossBattle) && (killedEnemy == false))
            {
                stars[2].allowedGo = true;
            }
            if (stars[2].allowedGo == true)
                stars[2].y += stars[2].speed;

            for (int i = 0; i < 3; i++)
            {
                if (stars[i].allowedGo == true)
                    draw_rectangle(stars[i].x, stars[i].y, stars[i].z, stars[i].h, stars[i].colour);

                if (stars[i].y >= display_height)
                {
                    stars[i].x = x;
                    stars[i].y = y;
                    stars[i].allowedGo = false;
                }
            }

            if ((stars[4].allowedGo == false) && (stars[4].y == stars[3].y - 10))
            {
                stars[4].allowedGo = true;
            }
            if (stars[4].allowedGo == true)
                stars[4].y -= stars[4].speed;

            if (((stars[5].allowedGo == false) && (stars[5].y == stars[3].y - 10) && (stars[4].y < (display_height * 3 / 4)) && ((level >= 5) || (godMode))))
            {
                stars[5].allowedGo = true;
            }
            if (stars[5].allowedGo == true)
                stars[5].y -= stars[5].speed;

            if (((stars[6].allowedGo == false) && (stars[6].y == stars[3].y - 10) && (stars[5].y < (display_height * 3 / 4)) && ((level >= 9) || (godMode))))
            {
                stars[6].allowedGo = true;
            }
            if (stars[6].allowedGo == true)
                stars[6].y -= stars[6].speed;

            for (int i = 4; i < 7; i++)
            {
                if (stars[i].allowedGo == true)
                    draw_image(&cannonball_blue_image, stars[i].x, stars[i].y);

                if (stars[i].y < 0)
                {
                    stars[i].x = stars[3].x;
                    stars[i].y = stars[3].y - 10;
                    stars[i].allowedGo = false;
                }
            } //bullets of player ship

            if (stars[0].y >= (display_height - 25))
            { //the moving rectangle moves to the player-controlled area. same as blew
                if ((stars[0].x >= stars[3].x && stars[3].x + 30 >= stars[0].x + 10) || (stars[0].x <= stars[3].x && stars[3].x <= stars[0].x + 15) || (stars[0].x <= stars[3].x + 30 && stars[3].x + 20 <= stars[0].x + 10))
                {
                    stars[0].allowedGo = false;
                    stars[0].x = x;
                    stars[0].y = y;
                    if (life > 1)
                        life--;
                    else
                        break;
                    //stop the loop when meet the collision conditions
                }
            }
            if (stars[1].y >= (display_height - 25))
            {
                if ((stars[1].x >= stars[3].x && stars[3].x + 30 >= stars[1].x + 10) || (stars[1].x <= stars[3].x && stars[3].x <= stars[1].x + 15) || (stars[1].x <= stars[3].x + 30 && stars[3].x + 20 <= stars[1].x + 10))
                {
                    stars[1].allowedGo = false;
                    stars[1].x = x;
                    stars[1].y = y;
                    if (life > 1)
                        life--;
                    else
                        break;
                }
            }
            if (stars[2].y >= (display_height - 25))
            {
                if ((stars[2].x >= stars[3].x && stars[3].x + 30 >= stars[2].x + 10) || (stars[2].x <= stars[3].x && stars[3].x <= stars[2].x + 15) || (stars[2].x <= stars[3].x + 30 && stars[3].x + 20 <= stars[2].x + 10))
                {
                    stars[2].allowedGo = false;
                    stars[2].x = x;
                    stars[2].y = y;
                    if (life > 1)
                        life--;
                    else
                        break;
                }
            }

            for (int i = 4; i < 7; i++)
            {
                if (stars[i].y <= (y + 8 + 5) && stars[i].y >= (y - 8 - 5))
                {
                    if (stars[i].x <= (x + 17) && stars[i].x >= (x - 17))
                    {
                        if (!killedEnemy)
                        {

                            score = score + 100; //every time you hit the enemy will award 100 points whatever if you killed it
                            if ((bossBattle) && (hpOfBoss > 0))
                            {
                                hpOfBoss--;
                            }
                            else if ((bossBattle) && (hpOfBoss == 0))
                            {
                                life += ((defaultHpOfBoss / 5) * 3); //award 3 life
                                killTime = 0;
                                killedEnemy = true;
                                bossBattle = false;
                                defaultHpOfBoss += 5;
                                hpOfBoss = defaultHpOfBoss;
                                level++;
                                if (level >= 9)
                                    level = 9;
                                score = score + ((defaultHpOfBoss / 5) * 100);
                            }
                            else
                            {
                                killTime++;
                                killedEnemy = true;
                                level++;
                                if (level >= 12)
                                    level = 12;
                            }

                            stars[i].y = stars[3].y - 10; //if hit the enemy, the bullet will disapeared
                        }
                    }
                }
            }

            if (killedEnemy)
                boomDuration++;
            
            flip_frame();
        }
        draw_image(&Self_Boom_image, stars[3].x, stars[3].y);
        flip_frame();
        vTaskDelay(1000 / portTICK_RATE_MS);
        draw_rectangle(0, 0, display_width, display_height / 4, rgbToColour(0, 0, 0));
        flip_frame();
        vTaskDelay(750 / portTICK_RATE_MS);
        draw_rectangle(0, display_height / 4, display_width, display_height / 4, rgbToColour(0, 0, 0));
        flip_frame();
        vTaskDelay(750 / portTICK_RATE_MS);
        draw_rectangle(0, display_height / 2, display_width, display_height / 4, rgbToColour(0, 0, 0));
        flip_frame();
        vTaskDelay(750 / portTICK_RATE_MS);
        draw_rectangle(0, display_height * 3 / 4, display_width, display_height / 4, rgbToColour(0, 0, 0));
        flip_frame();
        vTaskDelay(750 / portTICK_RATE_MS);

        setFont(FONT_SMALL);
        
        print_xy("Final Score:", 5, 30);
        sprintf(number_str, "%d", score);

        print_xy(number_str, 85, 30);

        print_xy("Boss Killed:", 5, LASTY + 10);
        sprintf(number_str, "%d", (defaultHpOfBoss - 5) / 5);
        print_xy(number_str, 85, LASTY);
        
        print_xy("Press any key to exit", 5, LASTY + 10);
        setFont(FONT_UBUNTU16);
        setFontColour(255,210,0);
        print_xy("Game Over", 25, display_height/2);
        flip_frame();
        while ((gpio_get_level(0) == 1) && (gpio_get_level(35) == 1))
        {
        }
    }
}
