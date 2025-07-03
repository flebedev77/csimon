#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>

#define APP_TITLE "Simon"
#define SEQUENCE_CAPACITY 100

#define MAX_CONTROLLER_AMOUNT 8

#define BUTTON_AMOUNT 4

static int screenWidth = 0;
static int screenHeight = 0;

static int sequence[SEQUENCE_CAPACITY];
static int sequenceLength = 1;

static int playerSequence[SEQUENCE_CAPACITY]; 
static int playerSequenceLength = 0;

static bool buttonsLit[BUTTON_AMOUNT];
static bool gamepadButtonsDown[BUTTON_AMOUNT];

static bool isShowingSequence = false;

int RandomButton(unsigned int seed)
{
  srand(time(0) + seed);
  return rand() % 4;
}

// Ran whenever the game boots up / reset
void Reset()
{
  for (size_t i = 0; i < SEQUENCE_CAPACITY; i++)
  {
    sequence[i] = 0;
    playerSequence[i] = 0;
  }
  sequenceLength = 1;
  playerSequenceLength = 0;

  sequence[0] = RandomButton(0);

  for (size_t i = 0; i < BUTTON_AMOUNT; i++)
  {
    buttonsLit[i] = false;
    gamepadButtonsDown[i] = false;
  }
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
      BeginDrawing();
      ClearBackground(RAYWHITE);
      DrawButtons();
      EndDrawing();

      if (IsKeyPressed(KEY_ZERO))
      {
        isShowingSequence = !isShowingSequence;
      }

      if (IsGamepadButtonDownAny(GAMEPAD_BUTTON_MIDDLE_LEFT))
        break;
    }

    CloseWindow();        
    return 0;
}
