/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <math.h>

#include "ch.h"
#include "hal.h"

#include "gfx.h"
#include "src/gwin/gwin_keyboard_layout.h"

#include "lwipthread.h"
#include <lwip/dhcp.h>

#include "chprintf.h"

#include "web/web.h"

/* From nosys.specs */
extern double atof (const char* str);

#define DEG2RAD(x)  (x * (M_PI / 180.0))

static void screen_draw_ethernet_up(void *p)
{
  dhcp_start((struct netif*)p);

  font_t font = gdispOpenFont("UI2");
  gdispFillStringBox(200, 10, 70, 25, "UP", font, Black, White, justifyLeft);
  gdispCloseFont(font);
}

static void screen_draw_ethernet_down(void *p)
{
  dhcp_stop((struct netif*)p);

  font_t font = gdispOpenFont("UI2");
  gdispFillStringBox(200, 10, 70, 25, "DOWN", font, Black, White, justifyLeft);
  gdispCloseFont(font);
}

void screen_draw_httpd_string(char *string_ptr)
{
  font_t font = gdispOpenFont("UI2");
  gdispFillStringBox(200, 35, 250, 25, string_ptr, font, Black, White, justifyLeft);
  gdispCloseFont(font);
}

static uint8_t _macAddress[] = {0xC2, 0xAF, 0x51, 0x03, 0xCF, 0x46};
static const lwipthread_opts_t lwip_opts = {
  .macaddress = _macAddress,
  .address = 0,
  .netmask = 0,
  .gateway = 0,
  .addrMode = NET_ADDRESS_DHCP,
  .ourHostName = "scot-controller",
  .link_up_cb = screen_draw_ethernet_up,
  .link_down_cb = screen_draw_ethernet_down
};

/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    //palSetLine(LINE_ARD_D13);
    chThdSleepMilliseconds(500);
    //palClearLine(LINE_ARD_D13);
    chThdSleepMilliseconds(500);
  }
}

  static const GVSpecialKey NumPadSKeys[] = {
    { "Cancel", "X", 0, 0 },             // \001 (1) = Backspace
    { "Submit", "S", 0, 0 }             // \002 (2) = Enter
  };
  static const char *NumPadSet[] = { "123",     "456",    "789",      "0.\001\002",  0 };
  static const GVKeySet NumPadSets[] = { NumPadSet, 0 };
  static const GVKeyTable VirtualKeyboard_NumPad = { NumPadSKeys, NumPadSets };

static GHandle    ghConsole;
static  GHandle   ghKeyboard;
static void createKeyboard(void) {
  GWidgetInit   wi;

  gwinWidgetClearInit(&wi);

  // Create the console - set colors before making it visible
  wi.g.show = gFalse;
  wi.g.x = 0; wi.g.y = 0;
  wi.g.width = gdispGetWidth(); wi.g.height = gdispGetHeight()/4;
  ghConsole = gwinConsoleCreate(0, &wi.g);
  gwinSetColor(ghConsole, GFX_BLACK);
  gwinSetBgColor(ghConsole, HTML2COLOR(0xF0F0F0));

  // Create the keyboard
  wi.g.show = gFalse;
  wi.g.x = 0; wi.g.y = gdispGetHeight()/4;
  wi.g.width = gdispGetWidth(); wi.g.height = gdispGetHeight() - (gdispGetHeight()/4);
  ghKeyboard = gwinKeyboardCreate(0, &wi);
  gwinKeyboardSetLayout(ghKeyboard, &VirtualKeyboard_NumPad);
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /* Initialise screen */
  /* Screen is 480 in x, 272 in y */
  gfxInit();
  gdispClear(White);

  /* Initialise Network */
  lwipInit(&lwip_opts);
  chThdCreateStatic(wa_http_server, sizeof(wa_http_server), NORMALPRIO + 1,
                    http_server, NULL);

  font_t default_font;
  default_font = gdispOpenFont("DejaVuSans20");      // Get the first defined font.
  gwinSetDefaultFont(default_font);
  gwinSetDefaultStyle(&WhiteWidgetStyle, gFalse);

  font_t font;

  font = gdispOpenFont("UI2");
  gdispDrawStringBox(410, 247, 70, 25, "Phil Crump", font, Black, justifyLeft);
  gdispCloseFont(font);

  #define EL_GRAPHIC_ORIGIN_X 80
  #define EL_GRAPHIC_ORIGIN_Y 90
  #define EL_GRAPHIC_RADIUS   60
  gdispDrawArc(EL_GRAPHIC_ORIGIN_X, EL_GRAPHIC_ORIGIN_Y, EL_GRAPHIC_RADIUS, 0, 180, Black);
  gdispDrawLine(EL_GRAPHIC_ORIGIN_X, EL_GRAPHIC_ORIGIN_Y, EL_GRAPHIC_ORIGIN_X, EL_GRAPHIC_ORIGIN_Y-(EL_GRAPHIC_RADIUS+10), Black);
  gdispDrawLine(EL_GRAPHIC_ORIGIN_X+(EL_GRAPHIC_RADIUS+10), EL_GRAPHIC_ORIGIN_Y, EL_GRAPHIC_ORIGIN_X-(EL_GRAPHIC_RADIUS+10), EL_GRAPHIC_ORIGIN_Y, Black);

  float el_angle = 50.0;
  gdispDrawThickLine(
    EL_GRAPHIC_ORIGIN_X,
    EL_GRAPHIC_ORIGIN_Y,
    EL_GRAPHIC_ORIGIN_X - (EL_GRAPHIC_RADIUS * cos(DEG2RAD(el_angle))),
    EL_GRAPHIC_ORIGIN_Y - (EL_GRAPHIC_RADIUS * sin(DEG2RAD(el_angle))),
    Red, 2, false
  );

  gdispDrawLine(20, 105, 460, 105, HTML2COLOR(0xC0C0C0));

  #define AZ_GRAPHIC_ORIGIN_X 80
  #define AZ_GRAPHIC_ORIGIN_Y 180
  #define AZ_GRAPHIC_RADIUS   60
  gdispDrawCircle(AZ_GRAPHIC_ORIGIN_X, AZ_GRAPHIC_ORIGIN_Y, AZ_GRAPHIC_RADIUS, Black);
  gdispDrawLine(
    AZ_GRAPHIC_ORIGIN_X + (AZ_GRAPHIC_RADIUS + 10),
    AZ_GRAPHIC_ORIGIN_Y,
    AZ_GRAPHIC_ORIGIN_X - (AZ_GRAPHIC_RADIUS + 10),
    AZ_GRAPHIC_ORIGIN_Y,
    Black
  );
  gdispDrawLine(
    AZ_GRAPHIC_ORIGIN_X,
    AZ_GRAPHIC_ORIGIN_Y + (AZ_GRAPHIC_RADIUS + 10),
    AZ_GRAPHIC_ORIGIN_X,
    AZ_GRAPHIC_ORIGIN_Y - (AZ_GRAPHIC_RADIUS + 10),
    Black
  );

  float az_angle = 200.0;
  gdispDrawThickLine(
    AZ_GRAPHIC_ORIGIN_X,
    AZ_GRAPHIC_ORIGIN_Y,
    AZ_GRAPHIC_ORIGIN_X - (AZ_GRAPHIC_RADIUS * cos(DEG2RAD(az_angle))),
    AZ_GRAPHIC_ORIGIN_Y - (AZ_GRAPHIC_RADIUS * sin(DEG2RAD(az_angle))),
    Red, 2, false
  );

  char el_text_string[14];
  char az_text_string[14];

  chsnprintf(el_text_string, 13, "EL:  %4.2fd", el_angle);
  chsnprintf(az_text_string, 13, "AZ: %5.2fd", az_angle);

  font = gdispOpenFont("DejaVuSans32");
  gdispDrawString(160, EL_GRAPHIC_ORIGIN_Y, el_text_string, font, Black);
  gdispDrawString(160, AZ_GRAPHIC_ORIGIN_Y, az_text_string, font, Black);
  gdispCloseFont(font);

  /*** Create Button ***/
  GWidgetInit wi;
  GHandle   ghButton1;

  // Apply some default values for GWIN
  gwinWidgetClearInit(&wi);
  wi.g.show = gTrue;

  // Apply the button parameters
  wi.g.width = 30;
  wi.g.height = 30;
  wi.g.y = EL_GRAPHIC_ORIGIN_Y;
  wi.g.x = 350;
  wi.text = "C";

  // Create the actual button
  ghButton1 = gwinButtonCreate(0, &wi);

  createKeyboard();

  /* Set up event handler */
  static GListener gl;
  geventListenerInit(&gl);
  gwinAttachListener(&gl);

  geventAttachSource(&gl, gwinKeyboardGetEventSource(ghKeyboard), GLISTEN_KEYTRANSITIONS|GLISTEN_KEYUP);

  /*
   * Blink a LED to show lethality.
   */
  palClearLine(LINE_ARD_D13);
  palSetLineMode(LINE_ARD_D13, PAL_MODE_OUTPUT_PUSHPULL);
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO+1, Thread1, NULL);

  int i;
  GEvent *pe;
  GEventKeyboard *pk;
  char kb_input_buffer[128];
  int kb_input_index = 0;
  while (true) {
    //chThdSleepMilliseconds(500);
    //gdispSetBacklight(0);

    // Get an Event
    pe = geventEventWait(&gl, gDelayForever);

    switch(pe->type) {
      case GEVENT_GWIN_BUTTON:
        if (((GEventGWinButton*)pe)->gwin == ghButton1) {
          // Our button has been pressed

          /* Show Keyboard */
          gwinShow(ghConsole);
          gwinClear(ghConsole);
          gwinShow(ghKeyboard);

          kb_input_index = 0;
        }
        break;

      case GEVENT_KEYBOARD:
        // This is a keyboard event from a keyboard source which must be separately listened to.
        // It is not sent on the gwin event source even though in this case it was generated by a gwin widget.
        pk = (GEventKeyboard *)pe;

        for (i = 0; i < pk->bytecount; i++)
        {
          if(pk->c[i] == 'X')
          {
            // Cancel, close keyboard.
            gwinClear(ghConsole);
            gwinHide(ghConsole);
            gwinHide(ghKeyboard);
          }
          else if(pk->c[i] == 'S')
          {
            // Submit, close keyboard.
            gwinClear(ghConsole);
            gwinHide(ghConsole);
            gwinHide(ghKeyboard);

            /* Set value */
            kb_input_buffer[kb_input_index] = '\0';
            el_angle = (float)atof(kb_input_buffer);

            chsnprintf(el_text_string, 12, "EL: %4.2fd", el_angle);

            font = gdispOpenFont("DejaVuSans32");
            gdispDrawString(160, EL_GRAPHIC_ORIGIN_Y, el_text_string, font, Black);
            gdispCloseFont(font);

            font = gdispOpenFont("UI2");
            gdispDrawString(60, EL_GRAPHIC_ORIGIN_Y, kb_input_buffer, font, Black);
            gdispCloseFont(font);
          }
          else
          {
            gwinPrintf(ghConsole, "%c", pk->c[i]);

            kb_input_buffer[kb_input_index] = pk->c[i];
            kb_input_index++;
            if(kb_input_index > 127)
            {
              /* String too long, user is an idiot, just exit */
              gwinClear(ghConsole);
              gwinHide(ghConsole);
              gwinHide(ghKeyboard);
            }
          }
        }
      break;

      default:
        break;
    }
  }
}
