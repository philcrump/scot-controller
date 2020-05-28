#include "main.h"

#include <math.h>
#include "chprintf.h"
extern double atof (const char* str);

#include "gfx.h"
#include "src/gwin/gwin_keyboard_layout.h"

#include "screen_images/image_network_red_bmp.h"
#include "screen_images/image_network_amber_bmp.h"
#include "screen_images/image_network_green_bmp.h"

#include "screen_images/image_clock_red_bmp.h"
#include "screen_images/image_clock_amber_bmp.h"
#include "screen_images/image_clock_green_bmp.h"

static font_t font_ui2;
static font_t font_dejavusans20;
static font_t font_dejavusans32;

typedef enum {
  SCREEN_STATE_DASHBOARD,
  SCREEN_STATE_ENTRY_AZ,
  SCREEN_STATE_ENTRY_EL
} screen_state_t;

static screen_state_t screen_state = SCREEN_STATE_DASHBOARD;

static GDisplay* pixmap_azimuth_graphic;
static GHandle label_azimuth_value;
static GHandle label_azimuth_demand;
static GHandle label_azimuth_error;
static GHandle control_azimuth_button;

static GDisplay* pixmap_elevation_graphic;
static GHandle label_elevation_value;
static GHandle label_elevation_demand;
static GHandle label_elevation_error;
static GHandle control_elevation_button;

#define EL_GRAPHIC_ORIGIN_X 80
#define EL_GRAPHIC_ORIGIN_Y 80
#define EL_GRAPHIC_RADIUS   60

#define AZ_GRAPHIC_ORIGIN_X 80
#define AZ_GRAPHIC_ORIGIN_Y 180
#define AZ_GRAPHIC_RADIUS   60

static uint32_t screen_ip_link_status = APP_IP_LINK_STATUS_DOWN;

static GHandle screen_image_ethernet_up;
static GHandle screen_image_ethernet_warn;
static GHandle screen_image_ethernet_down;

static void screen_draw_ethernet_up(void)
{
  gwinHide(screen_image_ethernet_down);
  gwinHide(screen_image_ethernet_warn);
  gwinShow(screen_image_ethernet_up);
}

static void screen_draw_ethernet_warn(void)
{
  gwinHide(screen_image_ethernet_up);
  gwinHide(screen_image_ethernet_down);
  gwinShow(screen_image_ethernet_warn);
}

static void screen_draw_ethernet_down(void)
{
  gwinHide(screen_image_ethernet_up);
  gwinHide(screen_image_ethernet_warn);
  gwinShow(screen_image_ethernet_down);
}

static uint32_t screen_app_ip_service_sntp_status = APP_IP_SERVICE_SNTP_STATUS_DOWN;


static GHandle screen_data_clock;
static GHandle screen_image_clock_up;
static GHandle screen_image_clock_polling;
static GHandle screen_image_clock_down;

static void screen_draw_clock_up(void)
{
  gwinHide(screen_image_clock_down);
  gwinHide(screen_image_clock_polling);
  gwinShow(screen_image_clock_up);
}

static void screen_draw_clock_polling(void)
{
  gwinHide(screen_image_clock_up);
  gwinHide(screen_image_clock_down);
  gwinShow(screen_image_clock_polling);
}

static void screen_draw_clock_down(void)
{
  gwinHide(screen_image_clock_up);
  gwinHide(screen_image_clock_polling);
  gwinShow(screen_image_clock_down);
}

static char _screen_clock_string_buffer[32];
static char *screen_clock_string(void)
{
  static uint32_t _ms;
  static struct tm _tm;
  static RTCDateTime _screen_clock_string_datetime;

  rtcGetTime(&RTCD1, &_screen_clock_string_datetime);
  rtcConvertDateTimeToStructTm(&_screen_clock_string_datetime, &_tm, &_ms);
  chsnprintf(_screen_clock_string_buffer, 31, "%02d/%02d/%04d %02d:%02d:%02d.%03d",
    _tm.tm_mday, _tm.tm_mon+1, _tm.tm_year+1900,
    _tm.tm_hour, _tm.tm_min, _tm.tm_sec, _ms
  );
  return (char *)_screen_clock_string_buffer;
}

static const GVSpecialKey NumPadSKeys[] = {
  { "Cancel", "X", 0, 0 },             // \001 (1) = Backspace
  { "Submit", "S", 0, 0 }             // \002 (2) = Enter
};
static const char *NumPadSet[] = { "123",     "456",    "789",      "0.\001\002",  0 };
static const GVKeySet NumPadSets[] = { NumPadSet, 0 };
static const GVKeyTable VirtualKeyboard_NumPad = { NumPadSKeys, NumPadSets };

static GHandle ghConsole;
static GHandle ghKeyboard;
static void createKeyboard(void)
{
  GWidgetInit   wi;
  gwinWidgetClearInit(&wi);

  gwinSetDefaultFont(font_dejavusans20);

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

  /* Create Azimuth Graphic pixmap */

  pixmap_azimuth_graphic = gdispPixmapCreate(140, 140);

  /* Create "AZ:" label */

  gwinSetDefaultStyle(&WhiteWidgetStyle, false);
  gwinSetDefaultFont(font_dejavusans32);

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

  gwinSetDefaultFont(font_dejavusans20);

  /* Create Az "Demand:" label */

  wi.g.x = 160;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y-35;
  wi.g.width = 120;
  wi.g.height = 25;
  wi.text = "Demand:";
  wi.customDraw = NULL;
  wi.g.show = true;

  gwinLabelCreate(NULL, &wi);

  /* Create Az Demand Value label */

  wi.g.x = 260;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y-35;
  wi.g.width = 80;
  wi.g.height = 25;
  wi.text = "---.--";
  wi.customDraw = gwinLabelDrawJustifiedRight;
  wi.g.show = true;
 
  label_azimuth_demand = gwinLabelCreate(NULL, &wi);

 /* Create Az Control button */

  wi.g.x = 350;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y-35;
  wi.g.width = 25;
  wi.g.height = 25;
  wi.text = "C";
  wi.customDraw = NULL;
  wi.g.show = true;

  control_azimuth_button = gwinButtonCreate(0, &wi);

  /* Create Az "Error" label */

  wi.g.x = 160;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y-10;
  wi.g.width = 100;
  wi.g.height = 25;
  wi.text = "-   Error:";
  wi.customDraw = NULL;
  wi.g.show = true;

  gwinLabelCreate(NULL, &wi);

  /* Create Az Error value label */

  wi.g.x = 260;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y-10;
  wi.g.width = 80;
  wi.g.height = 25;
  wi.text = "---.--";
  wi.customDraw = gwinLabelDrawJustifiedRight;
  wi.g.show = true;
 
  label_azimuth_error = gwinLabelCreate(NULL, &wi);

  /* Create Elevation Graphic pixmap */

  pixmap_elevation_graphic = gdispPixmapCreate(140, 90);

  /* Create "EL:" label */

  gwinSetDefaultFont(font_dejavusans32);

  wi.g.x = 160;
  wi.g.y = EL_GRAPHIC_ORIGIN_Y-65;
  wi.g.width = 70;
  wi.g.height = 30;
  wi.text = "EL:";
  wi.customDraw = NULL;
  wi.g.show = true;

  gwinLabelCreate(NULL, &wi);
 
  /* Create El Value label */

  wi.g.x = 230;
  wi.g.y = EL_GRAPHIC_ORIGIN_Y-65;
  wi.g.width = 110;
  wi.g.height = 30;
  wi.text = "---.--";
  wi.customDraw = gwinLabelDrawJustifiedRight;
  wi.g.show = true;
 
  label_elevation_value = gwinLabelCreate(NULL, &wi);

  gwinSetDefaultFont(font_dejavusans20);

  /* Create El "Demand:" label */

  wi.g.x = 160;
  wi.g.y = EL_GRAPHIC_ORIGIN_Y-35;
  wi.g.width = 120;
  wi.g.height = 25;
  wi.text = "Demand:";
  wi.customDraw = NULL;
  wi.g.show = true;

  gwinLabelCreate(NULL, &wi);

  /* Create El Demand Value label */

  wi.g.x = 260;
  wi.g.y = EL_GRAPHIC_ORIGIN_Y-35;
  wi.g.width = 80;
  wi.g.height = 25;
  wi.text = "---.--";
  wi.customDraw = gwinLabelDrawJustifiedRight;
  wi.g.show = true;
 
  label_elevation_demand = gwinLabelCreate(NULL, &wi);

 /* Create El Control button */

  wi.g.x = 350;
  wi.g.y = EL_GRAPHIC_ORIGIN_Y-35;
  wi.g.width = 25;
  wi.g.height = 25;
  wi.text = "C";
  wi.customDraw = NULL;
  wi.g.show = true;

  control_elevation_button = gwinButtonCreate(0, &wi);

  /* Create El "Error" label */

  wi.g.x = 160;
  wi.g.y = EL_GRAPHIC_ORIGIN_Y-10;
  wi.g.width = 100;
  wi.g.height = 25;
  wi.text = "-   Error:";
  wi.customDraw = NULL;
  wi.g.show = true;

  gwinLabelCreate(NULL, &wi);

  /* Create El Error value label */

  wi.g.x = 260;
  wi.g.y = EL_GRAPHIC_ORIGIN_Y-10;
  wi.g.width = 80;
  wi.g.height = 25;
  wi.text = "---.--";
  wi.customDraw = gwinLabelDrawJustifiedRight;
  wi.g.show = true;
 
  label_elevation_error = gwinLabelCreate(NULL, &wi);

  /* Ethernet Status Graphic */
  wi.g.x = 300;
  wi.g.y = 250;
  wi.g.width = 21;
  wi.g.height = 19;
  wi.text = "-   Error:";
  wi.customDraw = NULL;

  wi.g.show = false;
  screen_image_ethernet_up = gwinImageCreate(NULL, &wi.g);
  gwinImageOpenMemory(screen_image_ethernet_up, image_network_green_bmp);
  gwinImageCache(screen_image_ethernet_up);

  wi.g.show = false;
  screen_image_ethernet_warn = gwinImageCreate(NULL, &wi.g);
  gwinImageOpenMemory(screen_image_ethernet_warn, image_network_amber_bmp);
  gwinImageCache(screen_image_ethernet_warn);

  wi.g.show = true;
  screen_image_ethernet_down = gwinImageCreate(NULL, &wi.g);
  gwinImageOpenMemory(screen_image_ethernet_down, image_network_red_bmp);
  gwinImageCache(screen_image_ethernet_down);

  /* RT Clock Status Graphic */
  wi.g.x = 275;
  wi.g.y = 250;
  wi.g.width = 19;
  wi.g.height = 19;
  wi.text = "-   Error:";
  wi.customDraw = NULL;

  wi.g.show = false;
  screen_image_clock_up = gwinImageCreate(NULL, &wi.g);
  gwinImageOpenMemory(screen_image_clock_up, image_clock_green_bmp);
  gwinImageCache(screen_image_clock_up);

  wi.g.show = false;
  screen_image_clock_polling = gwinImageCreate(NULL, &wi.g);
  gwinImageOpenMemory(screen_image_clock_polling, image_clock_amber_bmp);
  gwinImageCache(screen_image_clock_polling);

  wi.g.show = true;
  screen_image_clock_down = gwinImageCreate(NULL, &wi.g);
  gwinImageOpenMemory(screen_image_clock_down, image_clock_red_bmp);
  gwinImageCache(screen_image_clock_down);

  /* Clock Data */
  gwinSetDefaultFont(font_ui2);
  wi.g.x = 110;
  wi.g.y = 250;
  wi.g.width = 150;
  wi.g.height = 20;
  wi.text = "";
  wi.customDraw = NULL;

  wi.g.show = true;
  screen_data_clock = gwinLabelCreate(NULL, &wi);
}

static float graphic_azimuth_degrees_last = -999.9;
static char text_azimuth_string[14];
void screen_dashboard_azimuth_draw(bool force_redraw)
{
  if(!force_redraw && azimuth_degrees == graphic_azimuth_degrees_last)
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

static float screen_dashboard_elevation_degrees_last = -999.9;
static char screen_dashboard_elevation_string[14];
void screen_dashboard_elevation_draw(bool force_redraw)
{
  if(!force_redraw && elevation_degrees == screen_dashboard_elevation_degrees_last)
  {
    return;
  }
  screen_dashboard_elevation_degrees_last = elevation_degrees;

  gdispGFillArea(pixmap_elevation_graphic, 0, 0, 140, 90, White);

  gdispGDrawArc(pixmap_elevation_graphic, 70, 70, EL_GRAPHIC_RADIUS, 50, 180+15, Black);
  gdispGDrawLine(pixmap_elevation_graphic, 70, 70, 70, 70-(EL_GRAPHIC_RADIUS+10), Black);
  gdispGDrawLine(pixmap_elevation_graphic, 70+(EL_GRAPHIC_RADIUS+10), 70, 70-(EL_GRAPHIC_RADIUS+10), 70, Black);

  gdispGDrawThickLine(
    pixmap_elevation_graphic,
    70,
    70,
    70 - (EL_GRAPHIC_RADIUS * cos(DEG2RAD(elevation_degrees))),
    70 - (EL_GRAPHIC_RADIUS * sin(DEG2RAD(elevation_degrees))),
    Red, 3, false
  );

  gdispBlitArea(EL_GRAPHIC_ORIGIN_X-(EL_GRAPHIC_RADIUS+10), EL_GRAPHIC_ORIGIN_Y-(EL_GRAPHIC_RADIUS+10), 140, 90, gdispPixmapGetBits(pixmap_elevation_graphic));

  chsnprintf(screen_dashboard_elevation_string, 13, "%.2f", elevation_degrees);
  gwinSetText(label_elevation_value, screen_dashboard_elevation_string, true);

  chsnprintf(screen_dashboard_elevation_string, 13, "%.2f", elevation_demand_degrees);
  gwinSetText(label_elevation_demand, screen_dashboard_elevation_string, true);

  chsnprintf(screen_dashboard_elevation_string, 13, "%.2f", elevation_error_degrees);
  gwinSetText(label_elevation_error, screen_dashboard_elevation_string, true);
}

static GListener screen_keyboard_glistener;
static char screen_keyboard_buffer[16];
static int32_t screen_keyboard_buffer_index = 0;

THD_FUNCTION(screen_service_thread, arg)
{
  (void)arg;

  chRegSetThreadName("screen");

  /* Initialise screen */
  /* Screen is 480 in x, 272 in y */
  gfxInit();
  gdispClear(White);

  watchdog_feed(WATCHDOG_DOG_SCREEN);

  /* Set up fonts */
  font_ui2 = gdispOpenFont("UI2");
  font_dejavusans20 = gdispOpenFont("DejaVuSans20");
  font_dejavusans32 = gdispOpenFont("DejaVuSans32");

  widgets_init();

  screen_dashboard_elevation_draw(true);
  screen_dashboard_azimuth_draw(true);
  /* Grey divider */
  gdispDrawLine(20, 105, 460, 105, HTML2COLOR(0xC0C0C0));

  gdispDrawStringBox(370, 250, 108, 20, "Phil Crump M0DNY", font_ui2, Black, justifyLeft);

  createKeyboard();

  geventListenerInit(&screen_keyboard_glistener);
  gwinAttachListener(&screen_keyboard_glistener);
  geventAttachSource(&screen_keyboard_glistener, gwinKeyboardGetEventSource(ghKeyboard), GLISTEN_KEYTRANSITIONS|GLISTEN_KEYUP);

  int i;
  GEvent *pe;
  GEventKeyboard *pk;

  while(true)
  {
    if(screen_state == SCREEN_STATE_DASHBOARD)
    {
      screen_dashboard_elevation_draw(false);

      screen_dashboard_azimuth_draw(false);

      if(screen_ip_link_status != app_ip_link_status())
      {
        /* Update IP Link Status */
        screen_ip_link_status = app_ip_link_status();

        if(screen_ip_link_status == APP_IP_LINK_STATUS_DOWN)
        {
          screen_draw_ethernet_down();
        }
        else if(screen_ip_link_status == APP_IP_LINK_STATUS_UPBUTNOIP)
        {
          screen_draw_ethernet_warn();
        }
        else if(screen_ip_link_status == APP_IP_LINK_STATUS_BOUND)
        {
          screen_draw_ethernet_up();
        }
      }


      if(screen_app_ip_service_sntp_status != app_ip_service_sntp_status())
      {
        /* Update SNTP app status */
        screen_app_ip_service_sntp_status = app_ip_service_sntp_status();

        if(screen_app_ip_service_sntp_status == APP_IP_SERVICE_SNTP_STATUS_DOWN)
        {
          screen_draw_clock_down();
        }
        else if(screen_app_ip_service_sntp_status == APP_IP_SERVICE_SNTP_STATUS_POLLING)
        {
          screen_draw_clock_polling();
        }
        else if(screen_app_ip_service_sntp_status == APP_IP_SERVICE_SNTP_STATUS_SYNCED)
        {
          screen_draw_clock_up();
        }
      }
      gwinSetText(screen_data_clock, screen_clock_string(), false);
    }

    // Get an Event or wait 20 milliseconds
    pe = geventEventWait(&screen_keyboard_glistener, 20);

    switch(pe->type) {
      case GEVENT_GWIN_BUTTON:
        if (((GEventGWinButton*)pe)->gwin == control_elevation_button
          || ((GEventGWinButton*)pe)->gwin == control_azimuth_button)
        {
          // Our button has been pressed

          /* Show Keyboard */
          gwinShow(ghConsole);
          gwinClear(ghConsole);
          gwinShow(ghKeyboard);

          if(((GEventGWinButton*)pe)->gwin == control_elevation_button)
          {
            screen_state = SCREEN_STATE_ENTRY_EL;
          }
          else if(((GEventGWinButton*)pe)->gwin == control_azimuth_button)
          {
            screen_state = SCREEN_STATE_ENTRY_AZ;
          }

          screen_keyboard_buffer_index = 0;
        }
        break;

      case GEVENT_KEYBOARD:
        /* Abort if we're not in the right screen for this */
        if(screen_state != SCREEN_STATE_ENTRY_AZ && screen_state != SCREEN_STATE_ENTRY_EL)
        {
          break;
        }
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

            screen_state = SCREEN_STATE_DASHBOARD;

            screen_dashboard_elevation_draw(true);
            screen_dashboard_azimuth_draw(true);
            /* Grey divider */
            gdispDrawLine(20, 105, 460, 105, HTML2COLOR(0xC0C0C0));
          }
          else if(pk->c[i] == 'S')
          {
            // Submit

            screen_keyboard_buffer[screen_keyboard_buffer_index] = '\0';

            if(screen_state == SCREEN_STATE_ENTRY_EL)
            {
              tracking_elevation_set_demand((float)atof(screen_keyboard_buffer));
            }
            else if(screen_state == SCREEN_STATE_ENTRY_AZ)
            {
              tracking_azimuth_set_demand((float)atof(screen_keyboard_buffer));
            }

            // close keyboard.
            gwinClear(ghConsole);
            gwinHide(ghConsole);
            gwinHide(ghKeyboard);

            screen_state = SCREEN_STATE_DASHBOARD;

            screen_dashboard_elevation_draw(true);
            screen_dashboard_azimuth_draw(true);
            /* Grey divider */
            gdispDrawLine(20, 105, 460, 105, HTML2COLOR(0xC0C0C0));
          }
          else
          {
            gwinPrintf(ghConsole, "%c", pk->c[i]);

            screen_keyboard_buffer[screen_keyboard_buffer_index] = pk->c[i];
            screen_keyboard_buffer_index++;
            if(screen_keyboard_buffer_index > 12)
            {
              /* String too long, user is an idiot, just exit */
              screen_keyboard_buffer_index = 0;

              gwinClear(ghConsole);
              gwinHide(ghConsole);
              gwinHide(ghKeyboard);

              screen_state = SCREEN_STATE_DASHBOARD;

              screen_dashboard_elevation_draw(true);
              screen_dashboard_azimuth_draw(true);
              /* Grey divider */
              gdispDrawLine(20, 105, 460, 105, HTML2COLOR(0xC0C0C0));
            }
          }
        }
      break;

      default:
        break;
    }
    watchdog_feed(WATCHDOG_DOG_SCREEN);
  }
}
