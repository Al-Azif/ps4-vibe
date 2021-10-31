// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#include <cstddef>
#include <cstdlib>

#include <orbis/Pad.h>
#include <orbis/SystemService.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

#include "graphics.h"
#include "libLog_stub.h"
#include "notifi.h"

// Handle for the library
int g_LibLogHandle = -1;

// Handle for the gamepad
int g_PadHandle = -1;

// Dimensions
#define FRAME_WIDTH 1920
#define FRAME_HEIGHT 1080
#define FRAME_DEPTH 4

// Font information
#define FONT_SIZE_LARGE 64
#define FONT_SIZE_SMALL 32

int frameID = 0;

// Background and foreground colors
Color g_BackgroundColor;
Color g_ForegroundColor;

// Font faces
FT_Face g_FontLarge;
FT_Face g_FontSmall;

// Globals
OrbisPadVibeParam g_Vibra;
OrbisPadColor g_LightBarColor;
bool g_ExitFlag = false;
bool g_Egg = false;

// Unload loaded libraries
void terminate() {
  g_Vibra.lgMotor = 0;
  g_Vibra.smMotor = 0;
  scePadSetVibration(g_PadHandle, &g_Vibra);

  if (g_LibLogHandle < 0) {
    sceKernelStopUnloadModule(g_LibLogHandle, 0, 0, 0, NULL, NULL);
  }
}

// This function will display p_Message, then "cleanly" close the application
void fail(const char *p_Message) {
  printf("%s", p_Message); // Flawfinder: ignore
  notifi(NULL, "%s", p_Message);
  terminate();
  sceSystemServiceLoadExec("exit", NULL);
}

// Initalize
void initialize() {
  if ((g_LibLogHandle = sceKernelLoadStartModule("/app0/sce_module/libLog.prx", 0, 0, 0, NULL, NULL)) < 0) {
    fail("Failed to start the libLog library");
  }

  if (logInitalize(g_LibLogHandle) != 0) {
    sceKernelStopUnloadModule(g_LibLogHandle, 0, 0, 0, NULL, NULL);
    fail("Failed to initialize the libLog library's functions");
  }

  // We know the logging is initalized here so we can use it after this point
  logKernel(LL_Info, "%s", "libLog.prx Initialized");

  // Initialize the ScePad library
  if (scePadInit() != 0) {
    logKernel(LL_Fatal, "%s", "Failed to initialize the ScePad library");
    fail("Failed to initialize the ScePad library's functions");
  }

  // Get the user ID
  OrbisUserServiceInitializeParams s_Param;
  s_Param.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
  sceUserServiceInitialize(&s_Param);
  int s_UserId;
  sceUserServiceGetInitialUser(&s_UserId);

  // Open a handle for the controller
  if ((g_PadHandle = scePadOpen(s_UserId, 0, 0, NULL)) < 0) {
    logKernel(LL_Fatal, "%s", "Failed to open pad");
    fail("Failed to open pad");
  }

  g_BackgroundColor = {0, 0, 0};
  g_ForegroundColor = {255, 255, 255};

  g_LightBarColor.r = 255;
  g_LightBarColor.g = 0;
  g_LightBarColor.b = 144;

  g_Vibra.lgMotor = 0;
  g_Vibra.smMotor = 0;
}

void updatePad(int p_PadHandle) {
  OrbisPadData s_PadData;
  scePadReadState(p_PadHandle, &s_PadData);

  if (s_PadData.buttons & ORBIS_PAD_BUTTON_OPTIONS && s_PadData.buttons & ORBIS_PAD_BUTTON_L3) {
    g_ExitFlag = true;
  }

  if (s_PadData.buttons & ORBIS_PAD_BUTTON_TRIANGLE) {
    if (g_Egg) {
      g_Egg = false;
    } else {
      g_Egg = true;
    }
  }

  if (s_PadData.buttons & ORBIS_PAD_BUTTON_UP) {
    if (g_Vibra.lgMotor < 255) {
      g_Vibra.lgMotor++;
    }
  }

  if (s_PadData.buttons & ORBIS_PAD_BUTTON_L1) {
    if (g_Vibra.lgMotor <= 245) {
      g_Vibra.lgMotor += 10;
    } else {
      g_Vibra.lgMotor = 255;
    }
  }

  if (s_PadData.buttons & ORBIS_PAD_BUTTON_DOWN) {
    if (g_Vibra.lgMotor > 0) {
      g_Vibra.lgMotor--;
    }
  }

  if (s_PadData.buttons & ORBIS_PAD_BUTTON_L2) {
    if (g_Vibra.lgMotor >= 10) {
      g_Vibra.lgMotor -= 10;
    } else {
      g_Vibra.lgMotor = 0;
    }
  }

  if (s_PadData.buttons & ORBIS_PAD_BUTTON_RIGHT) {
    if (g_Vibra.smMotor < 255) {
      g_Vibra.smMotor++;
    }
  }

  if (s_PadData.buttons & ORBIS_PAD_BUTTON_R1) {
    if (g_Vibra.smMotor <= 245) {
      g_Vibra.smMotor += 10;
    } else {
      g_Vibra.smMotor = 255;
    }
  }

  if (s_PadData.buttons & ORBIS_PAD_BUTTON_LEFT) {
    if (g_Vibra.smMotor > 0) {
      g_Vibra.smMotor--;
    }
  }

  if (s_PadData.buttons & ORBIS_PAD_BUTTON_R2) {
    if (g_Vibra.smMotor >= 10) {
      g_Vibra.smMotor -= 10;
    } else {
      g_Vibra.smMotor = 0;
    }
  }

  if (g_Vibra.lgMotor < 255 || g_Vibra.smMotor < 255) {
    g_Egg = false;
  }
}

int main() {
  initialize();

  auto scene = new Scene2D(FRAME_WIDTH, FRAME_HEIGHT, FRAME_DEPTH);

  if (!scene->Init(0xC000000, 2)) {
    logKernel(LL_Fatal, "%s", "Failed to initialize 2D scene");
    fail("Failed to initialize 2D scene");
  }

  const char *font = "/app0/Solomon.ttf";

  logKernel(LL_Info, "Initializing font (%s)", font);
  if (!scene->InitFont(&g_FontLarge, font, FONT_SIZE_LARGE)) {
    logKernel(LL_Fatal, "Failed to initialize large font '%s'", font);
    fail("Failed to initialize large font");
  }

  if (!scene->InitFont(&g_FontSmall, font, FONT_SIZE_SMALL)) {
    logKernel(LL_Fatal, "Failed to initialize small font '%s'", font);
    fail("Failed to initialize small font");
  }

  logKernel(LL_Info, "%s", "Entering draw loop...");

  sceSystemServiceHideSplashScreen();
  while (!g_ExitFlag) {
    updatePad(g_PadHandle);

    // Sooper sekrit Easter egg
    if (g_Egg && g_Vibra.lgMotor == 255 && g_Vibra.smMotor == 255) {
      OrbisPadColor s_NewLightBarColor;
      s_NewLightBarColor.r = rand() % 256;
      s_NewLightBarColor.g = rand() % 256;
      s_NewLightBarColor.b = rand() % 256;

      scePadSetLightBar(g_PadHandle, &s_NewLightBarColor);
    } else {
      scePadSetLightBar(g_PadHandle, &g_LightBarColor);
    }

    char s_LargeMotorStatus[0x20];                                                                      // Flawfinder: ignore. This is bigger than it will ever need to be
    char s_SmallMotorStatus[0x20];                                                                      // Flawfinder: ignore. This is bigger than it will ever need to be
    snprintf(s_LargeMotorStatus, sizeof(s_LargeMotorStatus), "Large Motor Speed: %i", g_Vibra.lgMotor); // Flawfinder: ignore. Constant format
    snprintf(s_SmallMotorStatus, sizeof(s_SmallMotorStatus), "Small Motor Speed: %i", g_Vibra.smMotor); // Flawfinder: ignore. Constant format

    scene->DrawText((char *)"PSVibe: PS4 Edition by Al Azif", g_FontLarge, 40, 80, g_BackgroundColor, g_ForegroundColor);
    scene->DrawText((char *)"Press UP/DOWN to control large motor speed", g_FontSmall, 40, 140, g_BackgroundColor, g_ForegroundColor);
    scene->DrawText((char *)"Press LEFT/RIGHT to control small motor speed", g_FontSmall, 40, 175, g_BackgroundColor, g_ForegroundColor);
    scene->DrawText((char *)s_LargeMotorStatus, g_FontSmall, 40, 235, g_BackgroundColor, g_ForegroundColor);
    scene->DrawText((char *)s_SmallMotorStatus, g_FontSmall, 40, 270, g_BackgroundColor, g_ForegroundColor);
    scene->DrawText((char *)"Press OPTIONS + L3 to exit", g_FontSmall, 40, 335, g_BackgroundColor, g_ForegroundColor);

    scene->SubmitFlip(frameID);
    scene->FrameWait(frameID);

    scene->FrameBufferSwap();
    frameID++;

    scene->FrameBufferClear();
    scene->FrameBufferFill(g_BackgroundColor);

    scePadSetVibration(g_PadHandle, &g_Vibra);
  }

  terminate();

  sceSystemServiceLoadExec("exit", NULL);

  return 0;
}
