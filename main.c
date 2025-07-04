#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>

#include "res/roboto.h"

#define APP_TITLE "Simon"
#define SEQUENCE_CAPACITY 100
#define INITIAL_SEQUENCE_DISPLAY_RATE 0.6
#define SEQUENCE_DISPLAY_RATE_ACCELERATION 0.1
#define SEQUENCE_DISPLAY_RATE_ACCELERATION_DECCELERATION 0.01

#define OFF_TO_ON_SHOWING_SEQUENCE_RATIO 3

#define MAX_CONTROLLER_AMOUNT 8

#define BUTTON_AMOUNT 4

#define SAVEFILE_FILEPATH ".csimon"

#define MENU_TITLE "PRESS START"
#define GAMEOVER_TITLE "GAME OVER!"
#define GAMEOVER_BLINK_AMOUNT 3

static int score = 0;
static int highScore = 0;

static int screenWidth = 0;
static int screenHeight = 0;

static int sequence[SEQUENCE_CAPACITY];
static int sequenceLength = 1;
static int sequenceDisplayIndex = 0;

static float sequenceDisplayRateAcceleration = SEQUENCE_DISPLAY_RATE_ACCELERATION;
static float sequenceDisplayRate = INITIAL_SEQUENCE_DISPLAY_RATE;
static float sequenceDisplayDelay = 0.f;

static float runDuration = 0.f;

static int playerSequence[SEQUENCE_CAPACITY]; 
static int playerSequenceIndex = 0;
static int playerSequenceLength = 0;

static bool buttonsLit[BUTTON_AMOUNT];
static bool gamepadButtonsDown[BUTTON_AMOUNT];
static int gamepadButtonPressed = -1;

static bool isShowingSequence = false;
static bool isWaitingBetweenButton = false;
static bool isShowingButtonAnimation = false;

static int animationType;
static enum { ANIMATION_TYPE_GAMEOVER };

static int gameoverAnimationBlinkCount = 0;
static int gameoverBlinkAnimationState = 0;

static int gameState;
static int gameStateAfterWait;
static float gameStateWaitDuration;
static float gameStateWaitRate = 0.f;
static enum {
 GAMESTATE_MENU,
 GAMESTATE_MENU_GAMEOVER,
 GAMESTATE_GAME,
 GAMESTATE_WAITING
};

static const Color BUTTON_UNLIT_COLOR = { 200, 200, 200, 255 };

static Font font;
static Font fontSm;
static Font fontLg;

//Helpers
bool IsGamepadButtonDownAny(int button)
{
  for (size_t i = 0; i < MAX_CONTROLLER_AMOUNT; i++)
  {
    if (IsGamepadAvailable(i) && IsGamepadButtonDown(i, button))
      return true;
  }
  return false;
}

bool IsGamepadButtonPressedAny(int button)
{
  for (size_t i = 0; i < MAX_CONTROLLER_AMOUNT; i++)
  {
    if (IsGamepadAvailable(i) && IsGamepadButtonPressed(i, button))
      return true;
  }
  return false;
}

int RandomButton(unsigned int seed)
{
  srand(time(0) + seed);
  return rand() % 4;
}

void AddButtonToSequence()
{
  sequenceLength++;
  sequence[sequenceLength-1] = RandomButton(0);
}

void ResetButtons()
{
  for (size_t i = 0; i < BUTTON_AMOUNT; i++)
  {
    buttonsLit[i] = false;
    gamepadButtonsDown[i] = false;
  }
}

void LightButtons()
{
  for (size_t i = 0; i < BUTTON_AMOUNT; i++)
  {
    buttonsLit[i] = true;
  }
}

// We read / writing in binary mode, to prevent skids from editing the savefile
void WriteSave()
{
  printf("Writing savefile at %s\n", SAVEFILE_FILEPATH);
  FILE* file = fopen(SAVEFILE_FILEPATH, "wb");

  if (file == NULL)
  {
    perror("Error writing savefile");
    return;
  }

  // Add paddings and bit shifts to discourage persistent skids
  int writeData[] = {
    10,
    255,
    15,

    highScore << 4,

    15,
    255,
    10
  };
  size_t count = sizeof(writeData) / sizeof(int);

  size_t writtenCount = fwrite(writeData, sizeof(int), count, file);

  if (writtenCount != count)
  {
    perror("Could not write entire savefile");
  }
  
  fclose(file);
}

void ReadSave()
{
  printf("Reading savefile at %s\n", SAVEFILE_FILEPATH);
  FILE* file = fopen(SAVEFILE_FILEPATH, "rb");

  if (file == NULL)
  {
    perror("Error reading savefile");
    return;
  }

  int readData[7];
  size_t count = sizeof(readData) / sizeof(int);

  size_t readCount = fread(readData, sizeof(int), count, file);
  if (readCount != count)
  {
    perror("Could not read entire savefile, expect your progress not loaded");
  }

  bool readableSave = true;

  if (readData[0] != 10) readableSave = false;
  if (readData[1] != 255) readableSave = false;
  if (readData[2] != 15) readableSave = false;
  if (readData[4] != 15) readableSave = false;
  if (readData[5] != 255) readableSave = false;
  if (readData[6] != 10) readableSave = false;

  if (readableSave)
  {
    highScore = readData[3] >> 4;
  } else
  {
    perror("Savefile is corrupt");
  }

  fclose(file);
}

//Reseting

// Ran whenever player messes up, etc.
void SoftReset()
{
  for (size_t i = 0; i < SEQUENCE_CAPACITY; i++)
  {
    playerSequence[i] = 0;
  } 
  playerSequenceLength = 0;
  playerSequenceIndex = 0;

  isShowingSequence = true;
  isShowingButtonAnimation = false;
  sequenceDisplayIndex = 0;
  sequenceDisplayDelay = 0.f;

  gameoverAnimationBlinkCount = 0;
  gameoverBlinkAnimationState = 0;

  gameStateWaitDuration = 0.f;
  gameStateWaitRate = 0.f;
}

// Ran whenever the game boots up / reset
void Reset()
{

  ResetButtons();

  score = 0;

  runDuration = 0.f;

  for (size_t i = 0; i < SEQUENCE_CAPACITY; i++)
  {
    sequence[i] = 0;
    playerSequence[i] = 0;
  }
  sequenceLength = 1;

  playerSequenceLength = 0;
  playerSequenceIndex = 0;

  for (size_t i = 0; i < sequenceLength; i++)
  {
    sequence[i] = RandomButton(i*100);
  }

  ResetButtons(); 
  SoftReset();

  sequenceDisplayRate = INITIAL_SEQUENCE_DISPLAY_RATE;
  sequenceDisplayRateAcceleration = SEQUENCE_DISPLAY_RATE_ACCELERATION;

  gameState = GAMESTATE_MENU;
  gameStateAfterWait = GAMESTATE_MENU;
}

// Input / Drawing
void DrawButtons()
{
  gamepadButtonsDown[0] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_LEFT);  // X
  gamepadButtonsDown[1] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_UP);    // Y
  gamepadButtonsDown[2] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT); // B
  gamepadButtonsDown[3] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);  // A
  
  gamepadButtonPressed = -1;
  if (IsGamepadButtonPressedAny(GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) gamepadButtonPressed = 0;
  if (IsGamepadButtonPressedAny(GAMEPAD_BUTTON_RIGHT_FACE_UP)) gamepadButtonPressed = 1;
  if (IsGamepadButtonPressedAny(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) gamepadButtonPressed = 2;
  if (IsGamepadButtonPressedAny(GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) gamepadButtonPressed = 3;


  if (
      (!isShowingSequence && !isShowingButtonAnimation) ||
      (gameState == GAMESTATE_WAITING && !isShowingSequence)
     )
  {
    for (size_t i = 0; i < BUTTON_AMOUNT; i++)
    {
      buttonsLit[i] = gamepadButtonsDown[i];
    }
  }

  DrawCircle(screenWidth/2 - 100, screenHeight/2, 50, buttonsLit[0] ? GREEN : BUTTON_UNLIT_COLOR);
  DrawCircle(screenWidth/2, screenHeight/2 - 100, 50, buttonsLit[1] ? YELLOW : BUTTON_UNLIT_COLOR);
  DrawCircle(screenWidth/2 + 100, screenHeight/2, 50, buttonsLit[2] ? RED : BUTTON_UNLIT_COLOR);
  DrawCircle(screenWidth/2, screenHeight/2 + 100, 50, buttonsLit[3] ? ORANGE : BUTTON_UNLIT_COLOR);
}

void DrawMenu(bool isGameoverMenu)
{
  if (IsGamepadButtonDownAny(GAMEPAD_BUTTON_MIDDLE_RIGHT))
  {
    gameState = GAMESTATE_GAME;
  }

  if (isGameoverMenu)
  {
    Vector2 gameOverTitleDimensions = MeasureTextEx(fontLg, GAMEOVER_TITLE, (float)fontLg.baseSize, 2);  
    DrawTextEx(fontLg, GAMEOVER_TITLE, (Vector2){
          (float)(screenWidth/2 - gameOverTitleDimensions.x/2),
          (float)(screenHeight/2 - gameOverTitleDimensions.y/2) - 250.f
        }, (float)fontLg.baseSize, 2, DARKGRAY);
  }

  if ((int)(runDuration * 15.f) % 15 > 7)
  {
    Vector2 menuTitleDimensions = MeasureTextEx(fontLg, MENU_TITLE, (float)fontLg.baseSize, 2);
    DrawTextEx(fontLg, MENU_TITLE, (Vector2){
        (float)(screenWidth/2 - menuTitleDimensions.x/2),
        (float)(screenHeight/2 - fontLg.baseSize/2)
        }, (float)fontLg.baseSize, 2, DARKGRAY);

  }
}

// Logic
int main(void)
{
    Reset();

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, APP_TITLE);

    screenWidth = GetRenderWidth();
    screenHeight = GetRenderHeight();

    SetTargetFPS(60);               

    SetWindowState(FLAG_FULLSCREEN_MODE);

    ReadSave();

    font = LoadFontFromMemory(".ttf", RobotoRegular, RobotoRegular_len, 30, 0, 0);
    fontSm = LoadFontFromMemory(".ttf", RobotoRegular, RobotoRegular_len, 20, 0, 0);
    fontLg = LoadFontFromMemory(".ttf", RobotoRegular, RobotoRegular_len, 50, 0, 0);

    
    while (!WindowShouldClose())    
    {
      char buf[100];
      float deltaTime = GetFrameTime();
      runDuration += deltaTime;

      BeginDrawing();
      ClearBackground(RAYWHITE);
      DrawButtons();

      if (IsKeyPressed(KEY_ZERO))
      {
        isShowingSequence = !isShowingSequence;
      }

      switch (gameState)
      {
        case GAMESTATE_GAME:
          if (isShowingSequence && !isShowingButtonAnimation)
          {
            sequenceDisplayDelay += (!isWaitingBetweenButton) ? deltaTime * OFF_TO_ON_SHOWING_SEQUENCE_RATIO : deltaTime;

            if (sequenceDisplayDelay > sequenceDisplayRate)
            {
              sequenceDisplayDelay = 0.f;

              if (sequenceDisplayIndex < sequenceLength)
              {
                if (isWaitingBetweenButton)
                {
                  ResetButtons();
                  isWaitingBetweenButton = false;
                } else
                {
                  buttonsLit[sequence[sequenceDisplayIndex]] = true;
                  isWaitingBetweenButton = true;

                  sequenceDisplayIndex++;
                }
              } else
              {
                sequenceDisplayIndex = 0;
                playerSequenceIndex = 0;
                isShowingSequence = false;
              }
            }
          } else if (!isShowingButtonAnimation)
          {
            if (gamepadButtonPressed != -1)
            {
              if (gamepadButtonPressed == sequence[playerSequenceIndex])
              {
                playerSequenceIndex++;

                if (playerSequenceIndex >= sequenceLength)
                {
                  if (sequenceDisplayRate > 0.3f)
                  {
                    sequenceDisplayRate -= sequenceDisplayRateAcceleration;
                  }
                  if (sequenceDisplayRateAcceleration > 0.f)
                  {
                    sequenceDisplayRateAcceleration -= SEQUENCE_DISPLAY_RATE_ACCELERATION_DECCELERATION;
                  }
                  if (sequenceDisplayRateAcceleration < 0.f)
                  {
                    sequenceDisplayRateAcceleration = 0.f;
                  }
                  score += playerSequenceIndex;
                  int prevPlayerSequenceIndex = playerSequenceIndex;
                  SoftReset(); 
                  AddButtonToSequence();
                  playerSequenceIndex = prevPlayerSequenceIndex;

                  gameState = GAMESTATE_WAITING;
                  gameStateAfterWait = GAMESTATE_GAME;
                  gameStateWaitDuration = 0.2f;
                }
              } else
              {
                Reset();
                gameState = GAMESTATE_WAITING;
                gameStateAfterWait = GAMESTATE_MENU_GAMEOVER;
                gameStateWaitDuration = 2.f;
                isShowingButtonAnimation = true;
                animationType = ANIMATION_TYPE_GAMEOVER;
              }
            }
          }

          if (score > highScore)
          {
            highScore = score;
          }

          break;

        case GAMESTATE_WAITING:
          gameStateWaitRate += deltaTime;
          if (gameStateWaitRate > gameStateWaitDuration)
          {
            gameStateWaitDuration = 0;
            gameStateWaitRate = 0;
            gameState = gameStateAfterWait;
            playerSequenceIndex = 0;
            ResetButtons();
          } 
          break;

        case GAMESTATE_MENU:
          DrawMenu(false);
          break;
        case GAMESTATE_MENU_GAMEOVER:
          DrawMenu(true); 
          break;

      }

      snprintf(buf, sizeof(buf), "%d/%d", playerSequenceIndex, sequenceLength);
      Vector2 texDimensions = MeasureTextEx(font, buf, (float)font.baseSize, 2);
      DrawTextEx(font, buf, (Vector2){ (float)(screenWidth / 2 - texDimensions.x / 2), (float)(screenHeight - 100) }, (float)font.baseSize, 2, DARKGRAY);

      snprintf(buf, sizeof(buf), "Score: %d", score + playerSequenceIndex);
      DrawTextEx(fontSm, buf, (Vector2){ 10.f, 10.f }, (float)fontSm.baseSize, 2, DARKGRAY);

      snprintf(buf, sizeof(buf), "Best: %d", highScore);
      DrawTextEx(fontSm, buf, (Vector2){ 10.0f, 30.0f }, (float)fontSm.baseSize, 2, DARKGRAY);


      if (isShowingButtonAnimation)
      {
        if (animationType == ANIMATION_TYPE_GAMEOVER)
        {
          if ((int)(runDuration*5.f) % 3 < 2) 
          {
            if (gameoverBlinkAnimationState)
            {
              gameoverAnimationBlinkCount++;
            }
            LightButtons();
            gameoverBlinkAnimationState = 0;
          } else
          {
            gameoverBlinkAnimationState = 1;
            ResetButtons();

            if (gameoverAnimationBlinkCount >= GAMEOVER_BLINK_AMOUNT-1)
            {
              gameState = GAMESTATE_MENU_GAMEOVER;
              gameStateWaitRate = 0.f;
              gameoverBlinkAnimationState = 0;
              gameoverAnimationBlinkCount = 0;
              isShowingButtonAnimation = false;
            }
          }
        }
      }

      EndDrawing();

      if (IsGamepadButtonDownAny(GAMEPAD_BUTTON_MIDDLE_LEFT))
        break;
    }

    WriteSave();

    UnloadFont(font);
    UnloadFont(fontSm);
    UnloadFont(fontLg);
    CloseWindow();        
    return 0;
}
