#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>

#define APP_TITLE "Simon"
#define SEQUENCE_CAPACITY 100
#define INITIAL_SEQUENCE_DISPLAY_RATE 0.6
#define SEQUENCE_DISPLAY_RATE_ACCELERATION 0.1
#define SEQUENCE_DISPLAY_RATE_ACCELERATION_DECCELERATION 0.02

#define OFF_TO_ON_SHOWING_SEQUENCE_RATIO 3

#define MAX_CONTROLLER_AMOUNT 8

#define BUTTON_AMOUNT 4

#define SAVEFILE_FILEPATH ".csimon"

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

static int playerSequence[SEQUENCE_CAPACITY]; 
static int playerSequenceIndex = 0;
static int playerSequenceLength = 0;

static bool buttonsLit[BUTTON_AMOUNT];
static bool gamepadButtonsDown[BUTTON_AMOUNT];
static int gamepadButtonPressed = -1;

static bool isShowingSequence = false;
static bool isWaitingBetweenButton = false;

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

  // Add paddings to discourage persistent skids
  int writeData[] = {
    10,
    255,
    15,

    highScore,

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
    highScore = readData[3];
  } else
  {
    perror("Savefile is corrupt");
  }

  fclose(file);
}

//Reseting
void ResetButtons()
{
  for (size_t i = 0; i < BUTTON_AMOUNT; i++)
  {
    buttonsLit[i] = false;
    gamepadButtonsDown[i] = false;
  }
}

// Ran whenever player messes up, etc.
void SoftReset()
{
  ResetButtons();
  
  for (size_t i = 0; i < SEQUENCE_CAPACITY; i++)
  {
    playerSequence[i] = 0;
  } 
  playerSequenceLength = 0;
  playerSequenceIndex = 0;

  isShowingSequence = true;
  sequenceDisplayIndex = 0;
  sequenceDisplayDelay = 0.f;
}

// Ran whenever the game boots up / reset
void Reset()
{
  score = 0;

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
      } else
      {
        if (gamepadButtonPressed != -1)
        {
          if (gamepadButtonPressed == sequence[playerSequenceIndex])
          {
            playerSequenceIndex++;

            if (playerSequenceIndex >= sequenceLength)
            {
              sequenceDisplayRate -= sequenceDisplayRateAcceleration;
              sequenceDisplayRateAcceleration -= SEQUENCE_DISPLAY_RATE_ACCELERATION_DECCELERATION;
              score += playerSequenceIndex;
              SoftReset(); 
              AddButtonToSequence();
            }
          } else
          {
            Reset();
          }
        }
      }

      if (score > highScore)
      {
        highScore = score;
      }
      
      char buf[100];
      snprintf(buf, sizeof(buf), "Score: %d", score + playerSequenceIndex);
      DrawText(buf, 10, 10, 20, DARKGRAY);

      snprintf(buf, sizeof(buf), "Best: %d", highScore);
      DrawText(buf, 10, 30, 20, DARKGRAY);


      if (IsGamepadButtonDownAny(GAMEPAD_BUTTON_MIDDLE_LEFT))
        break;
    }

    WriteSave();

    CloseWindow();        
    return 0;
}
