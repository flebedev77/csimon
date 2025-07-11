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
#define BUTTON_SIZE 50
#define BUTTON_LIT_SIZE 55
#define BUTTON_COLOR_INTERPOLATION 0.4
#define BUTTON_SIZE_INTERPOLATION 0.4

#define SAVEFILE_FILEPATH ".csimon"

#define MENU_TITLE "PRESS START"
#define GAMEOVER_TITLE "GAME OVER!"
#define GAMEOVER_BLINK_AMOUNT 3

#define AUTHOR "Made by flebedev77"

#define GAMEPAD_AXISREGISTERTHRESHOLD 0.6

static int score = 0;
static int highScore = 0;

#ifdef __EMSCRIPTEN__
  // Make it lowest resolution people generally use
  static int screenWidth = 1366;
  static int screenHeight = 768;
#else
  static int screenWidth = 0;
  static int screenHeight = 0;
#endif

static int sequence[SEQUENCE_CAPACITY];
static int sequenceLength = 1;
static int sequenceDisplayIndex = 0;

static float sequenceDisplayRateAcceleration = SEQUENCE_DISPLAY_RATE_ACCELERATION;
static float sequenceDisplayRate = INITIAL_SEQUENCE_DISPLAY_RATE;
static float sequenceDisplayDelay = 0.f;

static float runDuration = 0.f;
static float menuRunDuration = 0.f;

static int playerSequence[SEQUENCE_CAPACITY]; 
static int playerSequenceIndex = 0;
static int playerSequenceLength = 0;

static bool buttonsLit[BUTTON_AMOUNT];
static bool gamepadButtonsDown[BUTTON_AMOUNT];
static int gamepadButtonPressed = -1;

// This is for animations
static float buttonSizes[BUTTON_AMOUNT];
static Color buttonColors[BUTTON_AMOUNT];

static bool isShowingSequence = false;
static bool isWaitingBetweenButton = false;
static bool isShowingButtonAnimation = false;

static int animationType;
enum { ANIMATION_TYPE_GAMEOVER, ANIMATION_TYPE_WIN };

static int gameoverAnimationBlinkCount = 0;
static int gameoverBlinkAnimationState = 0;

static int gameState;
static int gameStateAfterWait;
static float gameStateWaitDuration;
static float gameStateWaitRate = 0.f;
enum {
 GAMESTATE_MENU,
 GAMESTATE_MENU_GAMEOVER,
 GAMESTATE_GAME,
 GAMESTATE_WAITING
};

static const Color BUTTON_UNLIT_COLOR = { 200, 200, 200, 255 };

static Font fontSm;
static Font font;
static Font fontLg;

static float deltaTime;

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

bool IsGamepadAxisDownAny(int button)
{
  for (size_t i = 0; i < MAX_CONTROLLER_AMOUNT; i++)
  {
    if (IsGamepadAvailable(i) && GetGamepadAxisMovement(i, button) > GAMEPAD_AXISREGISTERTHRESHOLD)
      return true;
  }
  return false;
}

static bool prevGamepadAxisState = false;

bool IsGamepadAxisPressedAny(int button)
{
  for (size_t i = 0; i < MAX_CONTROLLER_AMOUNT; i++)
  {
    if (IsGamepadAvailable(i) && GetGamepadAxisMovement(i, button) > GAMEPAD_AXISREGISTERTHRESHOLD)
    {
      bool ret = prevGamepadAxisState == false; 
      prevGamepadAxisState = true;
      return ret;
    } else if (GetGamepadAxisMovement(i, button) <= GAMEPAD_AXISREGISTERTHRESHOLD)
    {
      prevGamepadAxisState = false;
    }
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

float LerpFloat(float a, float b, float t)
{
  return a + (b-a) * t;
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
  for (size_t i = 0; i < BUTTON_AMOUNT; i++)
  {
    buttonSizes[i] = BUTTON_SIZE;
  }
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

  menuRunDuration = 0.f;
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
  //Jank af
#ifdef __EMSCRIPTEN__
  gamepadButtonsDown[1] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_LEFT);  // X
  gamepadButtonsDown[0] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_UP);    // Y
#else
  gamepadButtonsDown[0] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_LEFT) ||
    IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_TRIGGER_1);  // X / RShoulder
  gamepadButtonsDown[1] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_UP) ||
    IsGamepadAxisDownAny(GAMEPAD_AXIS_LEFT_TRIGGER);    // Y / LTrigger
#endif
  gamepadButtonsDown[2] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT); // B
  gamepadButtonsDown[3] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);  // A

  if (!gamepadButtonsDown[0]) gamepadButtonsDown[0] = IsKeyDown(KEY_LEFT);
  if (!gamepadButtonsDown[1]) gamepadButtonsDown[1] = IsKeyDown(KEY_UP);
  if (!gamepadButtonsDown[2]) gamepadButtonsDown[2] = IsKeyDown(KEY_RIGHT);
  if (!gamepadButtonsDown[3]) gamepadButtonsDown[3] = IsKeyDown(KEY_DOWN);

  
  gamepadButtonPressed = -1;
#ifdef __EMSCRIPTEN__
  if (IsGamepadButtonPressedAny(GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) gamepadButtonPressed = 1;
  if (IsGamepadButtonPressedAny(GAMEPAD_BUTTON_RIGHT_FACE_UP)) gamepadButtonPressed = 0;
#else
  if (IsGamepadButtonPressedAny(GAMEPAD_BUTTON_RIGHT_FACE_LEFT) ||
      IsGamepadButtonPressedAny(GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) gamepadButtonPressed = 0;
  if (IsGamepadButtonPressedAny(GAMEPAD_BUTTON_RIGHT_FACE_UP) ||
      IsGamepadAxisPressedAny(GAMEPAD_AXIS_LEFT_TRIGGER)) gamepadButtonPressed = 1;
#endif
  if (IsGamepadButtonPressedAny(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) gamepadButtonPressed = 2;
  if (IsGamepadButtonPressedAny(GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) gamepadButtonPressed = 3;

  if (IsKeyPressed(KEY_LEFT)) gamepadButtonPressed = 0;
  if (IsKeyPressed(KEY_UP)) gamepadButtonPressed = 1;
  if (IsKeyPressed(KEY_RIGHT)) gamepadButtonPressed = 2;
  if (IsKeyPressed(KEY_DOWN)) gamepadButtonPressed = 3;


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

  Color targetButtonColors[4];
  targetButtonColors[0] = buttonsLit[0] ? GREEN  : BUTTON_UNLIT_COLOR;
  targetButtonColors[1] = buttonsLit[1] ? BLUE   : BUTTON_UNLIT_COLOR;
  targetButtonColors[2] = buttonsLit[2] ? RED    : BUTTON_UNLIT_COLOR;
  targetButtonColors[3] = buttonsLit[3] ? ORANGE : BUTTON_UNLIT_COLOR;

  buttonColors[0] = ColorLerp(buttonColors[0], targetButtonColors[0], BUTTON_COLOR_INTERPOLATION);
  buttonColors[1] = ColorLerp(buttonColors[1], targetButtonColors[1], BUTTON_COLOR_INTERPOLATION);
  buttonColors[2] = ColorLerp(buttonColors[2], targetButtonColors[2], BUTTON_COLOR_INTERPOLATION);
  buttonColors[3] = ColorLerp(buttonColors[3], targetButtonColors[3], BUTTON_COLOR_INTERPOLATION);

  float targetButtonSizes[4];
  targetButtonSizes[0] = buttonsLit[0] ? BUTTON_LIT_SIZE : BUTTON_SIZE;
  targetButtonSizes[1] = buttonsLit[1] ? BUTTON_LIT_SIZE : BUTTON_SIZE;
  targetButtonSizes[2] = buttonsLit[2] ? BUTTON_LIT_SIZE : BUTTON_SIZE;
  targetButtonSizes[3] = buttonsLit[3] ? BUTTON_LIT_SIZE : BUTTON_SIZE;

  buttonSizes[0] = LerpFloat(buttonSizes[0], targetButtonSizes[0], BUTTON_SIZE_INTERPOLATION);
  buttonSizes[1] = LerpFloat(buttonSizes[1], targetButtonSizes[1], BUTTON_SIZE_INTERPOLATION);
  buttonSizes[2] = LerpFloat(buttonSizes[2], targetButtonSizes[2], BUTTON_SIZE_INTERPOLATION);
  buttonSizes[3] = LerpFloat(buttonSizes[3], targetButtonSizes[3], BUTTON_SIZE_INTERPOLATION);

  DrawCircle(screenWidth/2 - 100, screenHeight/2, buttonSizes[0], buttonColors[0]);
  DrawCircle(screenWidth/2, screenHeight/2 - 100, buttonSizes[1], buttonColors[1]);
  DrawCircle(screenWidth/2 + 100, screenHeight/2, buttonSizes[2], buttonColors[2]);
  DrawCircle(screenWidth/2, screenHeight/2 + 100, buttonSizes[3], buttonColors[3]);
}

void DrawMenu(bool isGameoverMenu)
{
  menuRunDuration += GetFrameTime();
  if (
    IsGamepadButtonDownAny(GAMEPAD_BUTTON_MIDDLE_RIGHT) ||
    IsKeyDown(KEY_ENTER) )
  {
    gameState = GAMESTATE_GAME;
  }

  if (isGameoverMenu && menuRunDuration < 3.f)
  {
    Vector2 gameOverTitleDimensions = MeasureTextEx(font, GAMEOVER_TITLE, (float)font.baseSize, 2);  
    DrawTextEx(font, GAMEOVER_TITLE, (Vector2){
          (float)(screenWidth/2 - gameOverTitleDimensions.x/2),
          (float)(screenHeight/2 + 30.f)
        }, (float)font.baseSize, 2, DARKGRAY);
  }

  if ((int)(runDuration * 15.f) % 15 > 7)
  {
    Vector2 menuTitleDimensions = MeasureTextEx(fontLg, MENU_TITLE, (float)fontLg.baseSize, 2);
    DrawTextEx(fontLg, MENU_TITLE, (Vector2){
        (float)(screenWidth/2 - menuTitleDimensions.x/2),
        (float)(screenHeight/2 - fontLg.baseSize/2)
        }, (float)fontLg.baseSize, 2, DARKGRAY);
  }

  Vector2 creditDimensions = MeasureTextEx(fontSm, AUTHOR, (float)fontSm.baseSize, 2);
  DrawTextEx(fontSm, AUTHOR, (Vector2){
      (float)(screenWidth/2 - creditDimensions.x/2),
      (float)(screenHeight - fontSm.baseSize) - 10.f
      }, (float)fontSm.baseSize, 2, DARKGRAY);
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
    HideCursor();

    SetWindowState(FLAG_FULLSCREEN_MODE);

    ReadSave();

    font = LoadFontFromMemory(".ttf", RobotoRegular, RobotoRegular_len, 30, 0, 0);
    fontSm = LoadFontFromMemory(".ttf", RobotoRegular, RobotoRegular_len, 20, 0, 0);
    fontLg = LoadFontFromMemory(".ttf", RobotoRegular, RobotoRegular_len, 50, 0, 0);

    
    while (!WindowShouldClose())    
    {
      //DrawFPS(10, screenHeight - 50);
      char buf[100];
      deltaTime = GetFrameTime();
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
                  SoftReset(); 
                  AddButtonToSequence();

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

      if (gameState != GAMESTATE_MENU && gameState != GAMESTATE_MENU_GAMEOVER)
      {
        snprintf(buf, sizeof(buf), "%d/%d", playerSequenceIndex, sequenceLength);
        Vector2 texDimensions = MeasureTextEx(font, buf, (float)font.baseSize, 2);
        DrawTextEx(font, buf, (Vector2){ (float)(screenWidth / 2 - texDimensions.x / 2), (float)(screenHeight - 100) }, (float)font.baseSize, 2, DARKGRAY);
      }

      snprintf(buf, sizeof(buf), "Score: %d", score);
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
