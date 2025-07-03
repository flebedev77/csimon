#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>

#define APP_TITLE "Simon"
#define SEQUENCE_CAPACITY 100
#define SEQUENCE_DISPLAY_RATE 1

#define MAX_CONTROLLER_AMOUNT 8

#define BUTTON_AMOUNT 4

static int screenWidth = 0;
static int screenHeight = 0;

static int sequence[SEQUENCE_CAPACITY];
static int sequenceLength = 1;
static int sequenceDisplayIndex = 0;

static float sequenceDisplayDelay = 0.f;

static int playerSequence[SEQUENCE_CAPACITY]; 
static int playerSequenceLength = 0;

static bool buttonsLit[BUTTON_AMOUNT];
static bool gamepadButtonsDown[BUTTON_AMOUNT];

static bool isShowingSequence = false;
static bool isWaitingBetweenButton = false;

int RandomButton(unsigned int seed)
{
  srand(time(0) + seed);
  return rand() % 4;
}

void ResetButtons()
{
  for (size_t i = 0; i < BUTTON_AMOUNT; i++)
  {
    buttonsLit[i] = false;
    gamepadButtonsDown[i] = false;
  }
}

// Ran whenever the game boots up / reset
void Reset()
{
  for (size_t i = 0; i < SEQUENCE_CAPACITY; i++)
  {
    sequence[i] = 0;
    playerSequence[i] = 0;
  }
  sequenceLength = 10;
  playerSequenceLength = 0;

  for (size_t i = 0; i < sequenceLength; i++)
  {
    sequence[i] = RandomButton(i*100);
  }

  ResetButtons(); 

  isShowingSequence = true;
  sequenceDisplayIndex = 0;
  sequenceDisplayDelay = 0.f;
}

// Ran whenever player messes up, etc.
void Init()
{
  for (size_t i = 0; i < SEQUENCE_CAPACITY; i++)
  {
    playerSequence[i] = 0;
  } 
  playerSequenceLength = 0;
}


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

void DrawButtons()
{
  gamepadButtonsDown[0] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_LEFT);  // X
  gamepadButtonsDown[1] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_UP);    // Y
  gamepadButtonsDown[2] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT); // B
  gamepadButtonsDown[3] = IsGamepadButtonDownAny(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);  // A

  if (!isShowingSequence)
  {
    for (size_t i = 0; i < BUTTON_AMOUNT; i++)
    {
      buttonsLit[i] = gamepadButtonsDown[i];
    }
  }

  DrawCircle(screenWidth/2 - 100, screenHeight/2, 50, buttonsLit[0] ? GREEN : WHITE);
  DrawCircle(screenWidth/2, screenHeight/2 - 100, 50, buttonsLit[1] ? YELLOW : WHITE);
  DrawCircle(screenWidth/2 + 100, screenHeight/2, 50, buttonsLit[2] ? RED : WHITE);
  DrawCircle(screenWidth/2, screenHeight/2 + 100, 50, buttonsLit[3] ? ORANGE : WHITE);
}

int main(void)
{
    Reset();

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, APP_TITLE);

    screenWidth = GetRenderWidth();
    screenHeight = GetRenderHeight();

    SetTargetFPS(60);               

    SetWindowState(FLAG_FULLSCREEN_MODE);
    
    while (!WindowShouldClose())    
    {
      float deltaTime = GetFrameTime();

      BeginDrawing();
      ClearBackground(RAYWHITE);
      DrawButtons();
      EndDrawing();

      if (IsKeyPressed(KEY_ZERO))
      {
        isShowingSequence = !isShowingSequence;
      }


      if (isShowingSequence)
      {
        sequenceDisplayDelay += (!isWaitingBetweenButton) ? deltaTime * 3.f : deltaTime;

        if (sequenceDisplayDelay > SEQUENCE_DISPLAY_RATE)
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
            isShowingSequence = false;
          }
        }
      }

      if (IsGamepadButtonDownAny(GAMEPAD_BUTTON_MIDDLE_LEFT))
        break;
    }

    CloseWindow();        
    return 0;
}
