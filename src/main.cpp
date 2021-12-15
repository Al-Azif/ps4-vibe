// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#include "controller.h"
#include "graphics.h"

#include "libLog.h"
#include "notifi.h"

#include <orbis/Pad.h>
#include <orbis/SystemService.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

#include <cstddef>
#include <cstdlib>

// Dimensions for the 2D scene
#define FRAME_WIDTH 1920
#define FRAME_HEIGHT 1080
#define FRAME_DEPTH 4

// Font information
const char *g_FontFace = "/app0/Solomon.ttf";
#define FONT_SIZE_LARGE 64
#define FONT_SIZE_SMALL 32

// Background and foreground colors
Color g_BackgroundColor = {0, 0, 0};
Color g_ForegroundColor = {255, 255, 255};

// Controller data
auto g_Controller = new Controller();
OrbisPadVibeParam g_Vibra;
OrbisPadColor g_LightBarColor;

// Other globals
bool g_ExitFlag = false;
bool g_Egg = false;

// Unload loaded libraries
void terminate() {
  g_Vibra.lgMotor = 0;
  g_Vibra.smMotor = 0;
  g_Controller->SetVibration(&g_Vibra);

  g_Controller->ResetLightBar();
}

// This function will display p_Message, then "cleanly" close the application
void fail(const char *p_Message) {
  logKernel(LL_Fatal, "%s", p_Message);
  notifi(NULL, "%s", p_Message);
  terminate();
  sceSystemServiceLoadExec("exit", NULL);
}

// Initalize
void initialize() {
  g_Vibra.lgMotor = 0;
  g_Vibra.smMotor = 0;

  g_LightBarColor.r = 255;
  g_LightBarColor.g = 0;
  g_LightBarColor.b = 144;
}

int main() {
  initialize();

  logKernel(LL_Info, "%s", "Initializing scene");

  auto s_Scene = new Scene2D(FRAME_WIDTH, FRAME_HEIGHT, FRAME_DEPTH);
  if (!s_Scene->Init(0xC000000, 2)) {
    logKernel(LL_Fatal, "%s", "Failed to initialize 2D scene");
    fail("Failed to initialize 2D scene");
  }

  logKernel(LL_Info, "Initializing font (%s)", g_FontFace);

  FT_Face s_FontLarge;
  if (!s_Scene->InitFont(&s_FontLarge, g_FontFace, FONT_SIZE_LARGE)) {
    logKernel(LL_Fatal, "Failed to initialize large font '%s'", g_FontFace);
    fail("Failed to initialize large font");
  }

  FT_Face s_FontSmall;
  if (!s_Scene->InitFont(&s_FontSmall, g_FontFace, FONT_SIZE_SMALL)) {
    logKernel(LL_Fatal, "Failed to initialize small font '%s'", g_FontFace);
    fail("Failed to initialize small font");
  }

  logKernel(LL_Info, "%s", "Initializing controller");

  if (!g_Controller->Init(-1)) {
    logKernel(LL_Fatal, "%s", "Failed to initialize controller");
    fail("Failed to initialize controller");
  }

  logKernel(LL_Info, "%s", "Entering draw loop...");

  sceSystemServiceHideSplashScreen();

  int s_FrameId = 0;
  while (!g_ExitFlag) {
    // Large Motor Up
    if (g_Controller->DpadUpPressed()) {
      if (g_Vibra.lgMotor < 255) {
        g_Vibra.lgMotor++;
      }
    }
    if (g_Controller->L1Pressed()) {
      if (g_Vibra.lgMotor <= 245) {
        g_Vibra.lgMotor += 10;
      } else {
        g_Vibra.lgMotor = 255;
      }
    }

    // Large Motor Down
    if (g_Controller->DpadDownPressed()) {
      if (g_Vibra.lgMotor > 0) {
        g_Vibra.lgMotor--;
      }
    }
    if (g_Controller->L2Pressed()) {
      if (g_Vibra.lgMotor >= 10) {
        g_Vibra.lgMotor -= 10;
      } else {
        g_Vibra.lgMotor = 0;
      }
    }

    // Small Motor Up
    if (g_Controller->DpadRightPressed()) {
      if (g_Vibra.smMotor < 255) {
        g_Vibra.smMotor++;
      }
    }
    if (g_Controller->R1Pressed()) {
      if (g_Vibra.smMotor <= 245) {
        g_Vibra.smMotor += 10;
      } else {
        g_Vibra.smMotor = 255;
      }
    }

    // Small Motor Down
    if (g_Controller->DpadLeftPressed()) {
      if (g_Vibra.smMotor > 0) {
        g_Vibra.smMotor--;
      }
    }
    if (g_Controller->R2Pressed()) {
      if (g_Vibra.smMotor >= 10) {
        g_Vibra.smMotor -= 10;
      } else {
        g_Vibra.smMotor = 0;
      }
    }

    if (g_Controller->L3Pressed() && g_Controller->StartPressed()) {
      g_ExitFlag = true;
    }

    if (g_Controller->TrianglePressed() && g_Vibra.lgMotor == 69 && g_Vibra.smMotor == 69) {
      g_Egg = true;
    }

    if (g_Vibra.lgMotor != 69 || g_Vibra.smMotor != 69) {
      g_Egg = false;
    }

    // Sooper sekrit Easter egg
    if (g_Egg && g_Vibra.lgMotor == 69 && g_Vibra.smMotor == 69) {
      OrbisPadColor s_NewLightBarColor;
      s_NewLightBarColor.r = rand() % 256;
      s_NewLightBarColor.g = rand() % 256;
      s_NewLightBarColor.b = rand() % 256;

      g_Controller->SetLightBar(&s_NewLightBarColor);
    } else {
      g_Controller->SetLightBar(&g_LightBarColor);
    }

    char s_LargeMotorStatus[0x20];                                                                      // Flawfinder: ignore. This is bigger than it will ever need to be
    char s_SmallMotorStatus[0x20];                                                                      // Flawfinder: ignore. This is bigger than it will ever need to be
    snprintf(s_LargeMotorStatus, sizeof(s_LargeMotorStatus), "Large Motor Speed: %i", g_Vibra.lgMotor); // Flawfinder: ignore. Constant format
    snprintf(s_SmallMotorStatus, sizeof(s_SmallMotorStatus), "Small Motor Speed: %i", g_Vibra.smMotor); // Flawfinder: ignore. Constant format

    s_Scene->DrawText((char *)"PSVibe: PS4 Edition by Al Azif", s_FontLarge, 40, 80, g_BackgroundColor, g_ForegroundColor);
    s_Scene->DrawText((char *)"Press UP/DOWN to control large motor speed", s_FontSmall, 40, 140, g_BackgroundColor, g_ForegroundColor);
    s_Scene->DrawText((char *)"Press LEFT/RIGHT to control small motor speed", s_FontSmall, 40, 175, g_BackgroundColor, g_ForegroundColor);
    s_Scene->DrawText((char *)s_LargeMotorStatus, s_FontSmall, 40, 235, g_BackgroundColor, g_ForegroundColor);
    s_Scene->DrawText((char *)s_SmallMotorStatus, s_FontSmall, 40, 270, g_BackgroundColor, g_ForegroundColor);
    s_Scene->DrawText((char *)"Press OPTIONS + L3 to exit", s_FontSmall, 40, 335, g_BackgroundColor, g_ForegroundColor);

    s_Scene->SubmitFlip(s_FrameId);
    s_Scene->FrameWait(s_FrameId);

    s_Scene->FrameBufferSwap();
    s_FrameId++;

    s_Scene->FrameBufferClear();
    s_Scene->FrameBufferFill(g_BackgroundColor);

    g_Controller->SetVibration(&g_Vibra);
  }

  terminate();

  sceSystemServiceLoadExec("exit", NULL);

  return 0;
}
