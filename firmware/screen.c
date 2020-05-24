#include "main.h"

#include <math.h>
#include "chprintf.h"
extern double atof (const char* str);

#include "gfx.h"
#include "src/gwin/gwin_keyboard_layout.h"

#include "screen_images/image_network_up_bmp.h"
#include "screen_images/image_network_down_bmp.h"

static font_t font_ui2;
static font_t font_dejavusans20;
static font_t font_dejavusans32;

static GDisplay* pixmap_azimuth_graphic;
static GHandle label_azimuth_value;
static GHandle label_azimuth_demand;
static GHandle label_azimuth_error;

//static GDisplay* pixmap_elevation_graphic;
//static GHandle label_elevation_value;

static bool screen_ip_link_is_up = false;

#define EL_GRAPHIC_ORIGIN_X 80
#define EL_GRAPHIC_ORIGIN_Y 80
#define EL_GRAPHIC_RADIUS   60

#define AZ_GRAPHIC_ORIGIN_X 80
#define AZ_GRAPHIC_ORIGIN_Y 180
#define AZ_GRAPHIC_RADIUS   60

static GHandle screen_image_ethernet_up;
static GHandle screen_image_ethernet_down;

static void screen_draw_ethernet_up(void)
{
  gwinHide(screen_image_ethernet_down);
  gwinShow(screen_image_ethernet_up);
}

static void screen_draw_ethernet_down(void)
{
  gwinHide(screen_image_ethernet_up);
  gwinShow(screen_image_ethernet_down);
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

static void widgets_init(void)
{
  GWidgetInit   wi;

  gwinWidgetClearInit(&wi);
  gwinSetDefaultFont(font_dejavusans32);

  /* Create "AZ:" label */

  wi.g.x = 160;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y-65;
  wi.g.width = 70;
  wi.g.height = 30;
  wi.text = "AZ:";
  wi.customDraw = NULL;
  wi.g.show = true;

  gwinLabelCreate(NULL, &wi);
 
  /* Create Az Value label */

  wi.g.x = 230;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y-65;
  wi.g.width = 110;
  wi.g.height = 30;
  wi.text = "---.--";
  wi.customDraw = gwinLabelDrawJustifiedRight;
  wi.g.show = true;
 
  label_azimuth_value = gwinLabelCreate(NULL, &wi);

  gwinWidgetClearInit(&wi);
  gwinSetDefaultFont(font_dejavusans20);

  /* Create Az "Demand:" label */

  wi.g.x = 160;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y-30;
  wi.g.width = 120;
  wi.g.height = 30;
  wi.text = "Demand:";
  wi.customDraw = NULL;
  wi.g.show = true;

  gwinLabelCreate(NULL, &wi);

  /* Create Az Demand Value label */

  wi.g.x = 260;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y-30;
  wi.g.width = 80;
  wi.g.height = 30;
  wi.text = "---.--";
  wi.customDraw = gwinLabelDrawJustifiedRight;
  wi.g.show = true;
 
  label_azimuth_demand = gwinLabelCreate(NULL, &wi);

  gwinWidgetClearInit(&wi);

  /* Create Az "Error" label */

  wi.g.x = 160;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y;
  wi.g.width = 100;
  wi.g.height = 30;
  wi.text = "-   Error:";
  wi.customDraw = NULL;
  wi.g.show = true;

  gwinLabelCreate(NULL, &wi);

  /* Create Az Error value label */

  wi.g.x = 260;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y;
  wi.g.width = 80;
  wi.g.height = 30;
  wi.text = "---.--";
  wi.customDraw = gwinLabelDrawJustifiedRight;
  wi.g.show = true;
 
  label_azimuth_error = gwinLabelCreate(NULL, &wi);

  /** TODO: Elevation **/

  pixmap_azimuth_graphic = gdispPixmapCreate(140, 140);

  /* Ethernet Status */
  wi.g.x = 300;
  wi.g.y = 250;
  wi.g.width = 21;
  wi.g.height = 19;
  wi.customDraw = NULL;
  wi.text = "";

  wi.g.show = false;
  screen_image_ethernet_up = gwinImageCreate(NULL, &wi.g);
  gwinImageOpenMemory(screen_image_ethernet_up, image_network_up_bmp);
  gwinImageCache(screen_image_ethernet_up);

  wi.g.show = true;
  screen_image_ethernet_down = gwinImageCreate(NULL, &wi.g);
  gwinImageOpenMemory(screen_image_ethernet_down, image_network_down_bmp);
  gwinImageCache(screen_image_ethernet_down);
}

static float graphic_azimuth_degrees_last = -999.9;
static char text_azimuth_string[14];
void graphic_azimuth_draw(void)
{

  if(azimuth_degrees == graphic_azimuth_degrees_last)
  {
    return;
  }
  graphic_azimuth_degrees_last = azimuth_degrees;

  gdispGFillArea(pixmap_azimuth_graphic, 0, 0, 140, 140, White);
  gdispGDrawCircle(pixmap_azimuth_graphic, 70, 70, AZ_GRAPHIC_RADIUS, Black);
  gdispGDrawLine(
    pixmap_azimuth_graphic,
    70 + (AZ_GRAPHIC_RADIUS + 10),
    70,
    70 - (AZ_GRAPHIC_RADIUS + 10),
    70,
    Black
  );
  gdispGDrawLine(
    pixmap_azimuth_graphic,
    70,
    70 + (AZ_GRAPHIC_RADIUS + 10),
    70,
    70 - (AZ_GRAPHIC_RADIUS + 10),
    Black
  );
  float azimuth_line_radians = DEG2RAD(azimuth_degrees + 90);
  gdispGDrawThickLine(
    pixmap_azimuth_graphic,
    70,
    70,
    70 - (AZ_GRAPHIC_RADIUS * cos(azimuth_line_radians)),
    70 - (AZ_GRAPHIC_RADIUS * sin(azimuth_line_radians)),
    Red, 3, false
  );
  gdispBlitArea(AZ_GRAPHIC_ORIGIN_X-70, AZ_GRAPHIC_ORIGIN_Y-70, 140, 140, gdispPixmapGetBits(pixmap_azimuth_graphic));

  chsnprintf(text_azimuth_string, 13, "%.2f", azimuth_degrees);
  gwinSetText(label_azimuth_value, text_azimuth_string, true);

  chsnprintf(text_azimuth_string, 13, "%.2f", azimuth_demand_degrees);
  gwinSetText(label_azimuth_demand, text_azimuth_string, true);

  chsnprintf(text_azimuth_string, 13, "%.2f", azimuth_error_degrees);
  gwinSetText(label_azimuth_error, text_azimuth_string, true);
}

THD_FUNCTION(screen_service_thread, arg)
{
  (void)arg;

  /* Initialise screen */
  /* Screen is 480 in x, 272 in y */
  gfxInit();
  gdispClear(White);

  /* Set up fonts */
  font_ui2 = gdispOpenFont("UI2");
  font_dejavusans20 = gdispOpenFont("DejaVuSans20");
  font_dejavusans32 = gdispOpenFont("DejaVuSans32");

  gwinSetDefaultFont(font_ui2);
  gwinSetDefaultStyle(&WhiteWidgetStyle, gFalse);

  widgets_init();
  graphic_azimuth_draw();

  gdispDrawStringBox(370, 250, 108, 20, "Phil Crump M0DNY", font_ui2, Black, justifyLeft);

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

  /* Grey divider */
  gdispDrawLine(20, 105, 460, 105, HTML2COLOR(0xC0C0C0));



  char el_text_string[14];

  /*** Create Button ***/
  GWidgetInit wi;
  GHandle   ghButton1;

  // Apply some default values for GWIN
  gwinWidgetClearInit(&wi);
  wi.g.show = gTrue;

  // Apply the button parameters
  wi.g.width = 25;
  wi.g.height = 25;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y-70;
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

  int i;
  GEvent *pe;
  GEventKeyboard *pk;
  char kb_input_buffer[128];
  int kb_input_index = 0;

  while(true)
  {
    //chThdSleepMilliseconds(500);
    //gdispSetBacklight(0);

    azimuth_degrees = (float)azimuth_raw * (360.0 / 65536.0);
    if((azimuth_degrees - azimuth_demand_degrees) > 180.0)
    {
      azimuth_error_degrees = (azimuth_degrees - (360.0 + azimuth_demand_degrees));
    }
    else
    {
      azimuth_error_degrees = azimuth_degrees - azimuth_demand_degrees;
    }
    graphic_azimuth_draw();

    if(!screen_ip_link_is_up && ip_txrx_link_is_up())
    {
      screen_draw_ethernet_up();
      screen_ip_link_is_up = true;
    }
    else if(screen_ip_link_is_up && !ip_txrx_link_is_up())
    {
      screen_draw_ethernet_down();
      screen_ip_link_is_up = false;
    }

    //elevation_degrees = (float)elevation_raw * (360.0 / 65536.0);
    //chsnprintf(el_text_string, 13, "EL:  %5.2f", elevation_degrees);
    //gwinSetText(label_elevation_value, el_text_string, false);

    // Get an Event
    pe = geventEventWait(&gl, 20); // milliseconds

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

            gdispDrawString(160, EL_GRAPHIC_ORIGIN_Y, el_text_string, font_dejavusans32, Black);

            gdispDrawString(60, EL_GRAPHIC_ORIGIN_Y, kb_input_buffer, font_ui2, Black);
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
