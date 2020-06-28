#include "main.h"

#include <math.h>
#include "chprintf.h"
extern double atof (const char* str);
#include <string.h>

#include "gfx.h"
#include "src/gwin/gwin_keyboard_layout.h"

static font_t font_ui1;
static font_t font_ui2;
static font_t font_dejavusans10;
static font_t font_dejavusans12;
static font_t font_dejavusans16;
static font_t font_dejavusans20;
static font_t font_dejavusans32;

typedef enum {
  SCREEN_STATE_DASHBOARD,
  SCREEN_STATE_ENTRY_AZ,
  SCREEN_STATE_ENTRY_EL
} screen_state_t;

static screen_state_t screen_state = SCREEN_STATE_DASHBOARD;

/** Elevation Elements **/

static GDisplay* pixmap_elevation_graphic;
static GHandle label_elevation_value;
static GHandle label_elevation_demand;
static GHandle label_elevation_error;
static GHandle control_elevation_button;


static GDisplay* pixmap_elevation_pwm;
static GHandle label_elevation_motor_pwm;
static GDisplay* pixmap_elevation_current;
static GHandle label_elevation_motor_current;

/** Azimuth Elements **/

static GDisplay* pixmap_azimuth_graphic;
static GHandle label_azimuth_value;
static GHandle label_azimuth_demand;
static GHandle label_azimuth_error;
static GHandle control_azimuth_button;

static GDisplay* pixmap_azimuth_pwm;
static GHandle label_azimuth_motor_pwm;
static GDisplay* pixmap_azimuth_current;
static GHandle label_azimuth_motor_current;

#define EL_GRAPHIC_ORIGIN_X 80
#define EL_GRAPHIC_ORIGIN_Y 80
#define EL_GRAPHIC_RADIUS   60

#define AZ_GRAPHIC_ORIGIN_X 80
#define AZ_GRAPHIC_ORIGIN_Y 180
#define AZ_GRAPHIC_RADIUS   60

static GDisplay* pixmap_clock;
static GHandle label_clock;

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
  gwinSetDefaultStyle(&WhiteWidgetStyle, false);

  /* Create Elevation pixmaps */

  pixmap_elevation_graphic = gdispPixmapCreate(140, 90);
  pixmap_elevation_pwm = gdispPixmapCreate(35, 80);
  pixmap_elevation_current = gdispPixmapCreate(35, 80);

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
  wi.g.width = 95;
  wi.g.height = 25;
  wi.text = "Demand:";
  wi.customDraw = gwinLabelDrawJustifiedRight;
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

  wi.g.x = 355;
  wi.g.y = 10;
  wi.g.width = 30;
  wi.g.height = 15;
  wi.text = "C";
  wi.customDraw = NULL;
  wi.g.show = true;

  control_elevation_button = gwinButtonCreate(0, &wi);

  /* Create El "Error" label */

  wi.g.x = 160;
  wi.g.y = EL_GRAPHIC_ORIGIN_Y-10;
  wi.g.width = 95;
  wi.g.height = 25;
  wi.text = "Error:";
  wi.customDraw = gwinLabelDrawJustifiedRight;
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

  /* Create Elevation PWM Label */

  gwinSetDefaultFont(font_dejavusans10);

  wi.g.x = 395;
  wi.g.y = 90;
  wi.g.width = 35;
  wi.g.height = 10;
  wi.text = "Brake";
  wi.customDraw = gwinLabelDrawJustifiedCenter;
  wi.g.show = true;
 
  label_elevation_motor_pwm = gwinLabelCreate(NULL, &wi);

  /* Create Elevation Current Label */

  wi.g.x = 435;
  wi.g.y = 90;
  wi.g.width = 35;
  wi.g.height = 10;
  wi.text = "0.00A";
  wi.customDraw = gwinLabelDrawJustifiedCenter;
  wi.g.show = true;
 
  label_elevation_motor_current = gwinLabelCreate(NULL, &wi);

  /* Create Azimuth pixmaps */

  pixmap_azimuth_graphic = gdispPixmapCreate(140, 140);
  pixmap_azimuth_pwm = gdispPixmapCreate(35, 80);
  pixmap_azimuth_current = gdispPixmapCreate(35, 80);

  /* Create "AZ:" label */

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
  wi.g.width = 95;
  wi.g.height = 25;
  wi.text = "Demand:";
  wi.customDraw = gwinLabelDrawJustifiedRight;
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

  wi.g.x = 355;
  wi.g.y = 110;
  wi.g.width = 30;
  wi.g.height = 15;
  wi.text = "C";
  wi.customDraw = NULL;
  wi.g.show = true;

  control_azimuth_button = gwinButtonCreate(0, &wi);

  /* Create Az "Error" label */

  wi.g.x = 160;
  wi.g.y = AZ_GRAPHIC_ORIGIN_Y-10;
  wi.g.width = 95;
  wi.g.height = 25;
  wi.text = "Error:";
  wi.customDraw = gwinLabelDrawJustifiedRight;
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

  /* Create Azimuth PWM Label */

  gwinSetDefaultFont(font_dejavusans10);

  wi.g.x = 395;
  wi.g.y = 190;
  wi.g.width = 35;
  wi.g.height = 10;
  wi.text = "Brake";
  wi.customDraw = gwinLabelDrawJustifiedCenter;
  wi.g.show = true;
 
  label_azimuth_motor_pwm = gwinLabelCreate(NULL, &wi);

  /* Create Azimuth Current Label */

  wi.g.x = 435;
  wi.g.y = 190;
  wi.g.width = 35;
  wi.g.height = 10;
  wi.text = "0.00A";
  wi.customDraw = gwinLabelDrawJustifiedCenter;
  wi.g.show = true;
 
  label_azimuth_motor_current = gwinLabelCreate(NULL, &wi);

  /* Clock Data */
  pixmap_clock = gdispPixmapCreate(150, 20);

  gwinSetDefaultFont(font_ui2);
  wi.g.x = 0;
  wi.g.y = 0;
  wi.g.width = 150;
  wi.g.height = 20;
  wi.text = "";
  wi.customDraw = NULL;

  wi.g.show = true;
  label_clock = gwinGLabelCreate(pixmap_clock, NULL, &wi);
}


static float screen_dashboard_elevation_degrees_last = -999.9;
static char screen_dashboard_elevation_string[14];
static color_t screen_dashboard_elevation_limit1_color;
static color_t screen_dashboard_elevation_limit2_color;
void screen_dashboard_elevation_draw(bool force_redraw)
{
  if(!force_redraw && elevation_degrees == screen_dashboard_elevation_degrees_last)
  {
    return;
  }
  screen_dashboard_elevation_degrees_last = elevation_degrees;

  gdispGFillArea(pixmap_elevation_graphic, 0, 0, 140, 90, White);

  gdispGDrawArc(pixmap_elevation_graphic, 70, 70, EL_GRAPHIC_RADIUS, 58, 180+15, Black);
  gdispGDrawLine(pixmap_elevation_graphic, 70, 70, 70, 70-(EL_GRAPHIC_RADIUS+10), Black);
  gdispGDrawLine(pixmap_elevation_graphic, 70+(20), 70, 70-(EL_GRAPHIC_RADIUS+10), 70, Black);

  if(tracking_elevation_local_control || tracking_elevation_remote_control)
  { 
    gdispGDrawThickLine(
      pixmap_elevation_graphic,
      70,
      70,
      70 - (EL_GRAPHIC_RADIUS * cos(DEG2RAD(elevation_demand_degrees))),
      70 - (EL_GRAPHIC_RADIUS * sin(DEG2RAD(elevation_demand_degrees))),
      Lime, 3, false
    );
  }

  gdispGDrawThickLine(
    pixmap_elevation_graphic,
    70,
    70,
    70 - (EL_GRAPHIC_RADIUS * cos(DEG2RAD(elevation_degrees))),
    70 - (EL_GRAPHIC_RADIUS * sin(DEG2RAD(elevation_degrees))),
    Red, 3, false
  );

  /* Upper Limit Switch Status */
  gdispGFillRoundedBox(
    pixmap_elevation_graphic,
    100, 5,
    9, 9,
    2,
    screen_dashboard_elevation_limit1_color 
  );

  /* Lower Limit Switch Status */
  gdispGFillRoundedBox(
    pixmap_elevation_graphic,
    0, 81,
    9, 9,
    2,
    screen_dashboard_elevation_limit2_color 
  );

  gdispBlitArea(EL_GRAPHIC_ORIGIN_X-(EL_GRAPHIC_RADIUS+10), EL_GRAPHIC_ORIGIN_Y-(EL_GRAPHIC_RADIUS+10), 140, 90, gdispPixmapGetBits(pixmap_elevation_graphic));

  chsnprintf(screen_dashboard_elevation_string, 13, "%.2f", (elevation_degrees < 180) ? elevation_degrees : (elevation_degrees - 360));
  gwinSetText(label_elevation_value, screen_dashboard_elevation_string, true);

  chsnprintf(screen_dashboard_elevation_string, 13, "%.2f", elevation_demand_degrees);
  gwinSetText(label_elevation_demand, screen_dashboard_elevation_string, true);

  chsnprintf(screen_dashboard_elevation_string, 13, "%.2f", elevation_error_degrees);
  gwinSetText(label_elevation_error, screen_dashboard_elevation_string, true);
}

static limit_t screen_dashboard_elevation_limit1_last;
static void screen_dashboard_elevation_limit1_draw(bool force_redraw)
{
  if(!force_redraw && (el_limit_1 == screen_dashboard_elevation_limit1_last))
  {
    return;
  }

  if(el_limit_1 == LIMIT_OK)
  {
    /* OK */
    screen_dashboard_elevation_limit1_color = Lime;
  }
  else if(el_limit_1 == LIMIT_HALT)
  {
    /* Hit Switch */
    screen_dashboard_elevation_limit1_color = Red;
  }
  else // if(el_limit_1 == LIMIT_UNKNOWN)
  {
    /* Unknown! */
    screen_dashboard_elevation_limit1_color = Purple;
  }

  /* Upper Limit Switch Status */
  gdispGFillRoundedBox(
    pixmap_elevation_graphic,
    100, 5,
    9, 9,
    2,
    screen_dashboard_elevation_limit1_color 
  );

  gdispBlitArea(EL_GRAPHIC_ORIGIN_X-(EL_GRAPHIC_RADIUS+10), EL_GRAPHIC_ORIGIN_Y-(EL_GRAPHIC_RADIUS+10), 140, 90, gdispPixmapGetBits(pixmap_elevation_graphic));

  screen_dashboard_elevation_limit1_last = el_limit_1;
}

static limit_t screen_dashboard_elevation_limit2_last;
static void screen_dashboard_elevation_limit2_draw(bool force_redraw)
{
  if(!force_redraw && (el_limit_2 == screen_dashboard_elevation_limit2_last))
  {
    return;
  }

  if(el_limit_2 == LIMIT_OK)
  {
    /* OK */
    screen_dashboard_elevation_limit2_color = Lime;
  }
  else if(el_limit_2 == LIMIT_HALT)
  {
    /* Hit Switch */
    screen_dashboard_elevation_limit2_color = Red;
  }
  else // if(el_limit_2 == LIMIT_UNKNOWN)
  {
    /* Unknown! */
    screen_dashboard_elevation_limit2_color = Purple;
  }

  /* Lower Limit Switch Status */
  gdispGFillRoundedBox(
    pixmap_elevation_graphic,
    0, 81,
    9, 9,
    2,
    screen_dashboard_elevation_limit2_color 
  );
  
  gdispBlitArea(EL_GRAPHIC_ORIGIN_X-(EL_GRAPHIC_RADIUS+10), EL_GRAPHIC_ORIGIN_Y-(EL_GRAPHIC_RADIUS+10), 140, 90, gdispPixmapGetBits(pixmap_elevation_graphic));

  screen_dashboard_elevation_limit2_last = el_limit_2;
}

static tracking_control_t screen_dashboard_elevation_control_last;
void screen_dashboard_elevation_control_draw(bool force_redraw)
{
  if(!force_redraw && (control_elevation == screen_dashboard_elevation_control_last))
  {
    return;
  }

  static color_t l_vel_color;
  static color_t l_pos_color;
  static color_t r_pos_color;
  static color_t r_mot_color;

  if(control_elevation == CONTROL_LOCAL_VELOCITY)
  {
    l_vel_color = Lime;
    l_pos_color = Grey;
    r_pos_color = Grey;
    r_mot_color = Grey;
  }
  else if(control_elevation == CONTROL_LOCAL_POSITION)
  {
    l_vel_color = Grey;
    l_pos_color = Lime;
    r_pos_color = Grey;
    r_mot_color = Grey;
  }
  else if(control_elevation == CONTROL_REMOTE_POSITION)
  {
    l_vel_color = Grey;
    l_pos_color = Grey;
    r_pos_color = Lime;
    r_mot_color = Grey;
  }
  else if(control_elevation == CONTROL_REMOTE_MOTOR)
  {
    l_vel_color = Grey;
    l_pos_color = Grey;
    r_pos_color = Grey;
    r_mot_color = Lime;
  }
  else // CONTROL_NONE
  {
    l_vel_color = Grey;
    l_pos_color = Grey;
    r_pos_color = Grey;
    r_mot_color = Grey;
  }

  /* Local - Vel */
  gdispFillRoundedBox(
    350, 25,
    40, 15,
    2,
    l_vel_color
  );
  gdispDrawStringBox(
    350, 25,
    40, 15,
    "L-Vel",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  /* Local - Position */
  gdispFillRoundedBox(
    350, 45,
    40, 15,
    2,
    l_pos_color
  );
  gdispDrawStringBox(
    350, 45,
    40, 15,
    "L-Pos",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  /* Remote - Position */
  gdispFillRoundedBox(
    350, 65,
    40, 15,
    2,
    r_pos_color
  );
  gdispDrawStringBox(
    350, 65,
    40, 15,
    "R-Pos",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  /* Remote - Motor */
  gdispFillRoundedBox(
    350, 85,
    40, 15,
    2,
    r_mot_color
  );
  gdispDrawStringBox(
    350, 85,
    40, 15,
    "R-Mot",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  screen_dashboard_elevation_control_last = control_elevation;
}

static int16_t screen_dashboard_elevation_pwm_last = 0;
static char screen_dashboard_elevation_pwm_string[8];
static int32_t screen_dashboard_elevation_pwm_percent;
void screen_dashboard_elevation_pwm_draw(bool force_redraw)
{
  if(!force_redraw && (elevation_pwm == screen_dashboard_elevation_pwm_last))
  {
    return;
  }

  screen_dashboard_elevation_pwm_percent = (100 * (int32_t)elevation_pwm) / 1024;

  /* El PWM bargraph box */
  gdispGDrawBox(
    pixmap_elevation_pwm,
    0, 0,
    35, 80,
    Black
  );

  /* Blank out box */
  gdispGFillArea(
    pixmap_elevation_pwm,
    1, 1,
    33, 78,
    White
  );

  /* Draw center line */
  gdispGDrawLine(pixmap_elevation_pwm, 1, 40, 34, 40, Black);

  if(elevation_pwm > 0)
  {
    gdispGFillArea(
      pixmap_elevation_pwm,
      2, 40-((39*screen_dashboard_elevation_pwm_percent)/100),
      31, ((39*screen_dashboard_elevation_pwm_percent)/100),
      Orange
    );
    chsnprintf(screen_dashboard_elevation_pwm_string, 7, "%+d%%", screen_dashboard_elevation_pwm_percent);
  }
  else if(elevation_pwm < 0)
  {
    gdispGFillArea(
      pixmap_elevation_pwm,
      2, 40,
      31, ((-1*39*screen_dashboard_elevation_pwm_percent)/100),
      Orange
    );
    chsnprintf(screen_dashboard_elevation_pwm_string, 7, "%+d%%", screen_dashboard_elevation_pwm_percent);
  }
  else
  {
    strcpy(screen_dashboard_elevation_pwm_string, "Brake");
  }

  gdispBlitArea(395, 10, 35, 80, gdispPixmapGetBits(pixmap_elevation_pwm));

  gwinSetText(label_elevation_motor_pwm, screen_dashboard_elevation_pwm_string, false);

  screen_dashboard_elevation_pwm_last = elevation_pwm;
}

static int16_t screen_dashboard_elevation_current_last = 0;
static char screen_dashboard_elevation_current_string[8];
static int32_t screen_dashboard_elevation_current_milliamps;
void screen_dashboard_elevation_current_draw(bool force_redraw)
{
  if(!force_redraw && (elevation_current == screen_dashboard_elevation_current_last))
  {
    return;
  }

  screen_dashboard_elevation_current_milliamps = 1.25 * (int32_t)elevation_current;

  /* El Current bargraph box */
  gdispGDrawBox(
    pixmap_elevation_current,
    0, 0,
    35, 80,
    Black
  );

  /* Blank out box */
  gdispGFillArea(
    pixmap_elevation_current,
    1, 1,
    33, 78,
    White
  );

  /* El Current Value */
  gdispGFillArea(
    pixmap_elevation_current,
    2, 79-((78*screen_dashboard_elevation_current_milliamps)/4000),
    31, ((78*screen_dashboard_elevation_current_milliamps)/4000),
    Red
  );

  gdispBlitArea(435, 10, 35, 80, gdispPixmapGetBits(pixmap_elevation_current));

  chsnprintf(screen_dashboard_elevation_current_string, 7, "%.2fA", (float)screen_dashboard_elevation_current_milliamps/1000);

  gwinSetText(label_elevation_motor_current, screen_dashboard_elevation_current_string, false);

  screen_dashboard_elevation_current_last = elevation_current;
}

/*** Azimuth Elements Draw Methods ***/

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

  if(tracking_azimuth_local_control || tracking_azimuth_remote_control)
  { 
    gdispGDrawThickLine(
      pixmap_azimuth_graphic,
      70,
      70,
      70 - (AZ_GRAPHIC_RADIUS * cos(DEG2RAD(azimuth_demand_degrees + 90))),
      70 - (AZ_GRAPHIC_RADIUS * sin(DEG2RAD(azimuth_demand_degrees + 90))),
      Lime, 3, false
    );
  }

  gdispGDrawThickLine(
    pixmap_azimuth_graphic,
    70,
    70,
    70 - (AZ_GRAPHIC_RADIUS * cos(DEG2RAD(azimuth_degrees + 90))),
    70 - (AZ_GRAPHIC_RADIUS * sin(DEG2RAD(azimuth_degrees + 90))),
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

static tracking_control_t screen_dashboard_azimuth_control_last;
void screen_dashboard_azimuth_control_draw(bool force_redraw)
{
  if(!force_redraw && (control_azimuth == screen_dashboard_azimuth_control_last))
  {
    return;
  }

  static color_t l_vel_color;
  static color_t l_pos_color;
  static color_t r_pos_color;
  static color_t r_mot_color;

  if(control_azimuth == CONTROL_LOCAL_VELOCITY)
  {
    l_vel_color = Lime;
    l_pos_color = Grey;
    r_pos_color = Grey;
    r_mot_color = Grey;
  }
  else if(control_azimuth == CONTROL_LOCAL_POSITION)
  {
    l_vel_color = Grey;
    l_pos_color = Lime;
    r_pos_color = Grey;
    r_mot_color = Grey;
  }
  else if(control_azimuth == CONTROL_REMOTE_POSITION)
  {
    l_vel_color = Grey;
    l_pos_color = Grey;
    r_pos_color = Lime;
    r_mot_color = Grey;
  }
  else if(control_azimuth == CONTROL_REMOTE_MOTOR)
  {
    l_vel_color = Grey;
    l_pos_color = Grey;
    r_pos_color = Grey;
    r_mot_color = Lime;
  }
  else // CONTROL_NONE
  {
    l_vel_color = Grey;
    l_pos_color = Grey;
    r_pos_color = Grey;
    r_mot_color = Grey;
  }

  /* Local - Vel */
  gdispFillRoundedBox(
    350, 125,
    40, 15,
    2,
    l_vel_color
  );
  gdispDrawStringBox(
    350, 125,
    40, 15,
    "L-Vel",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  /* Local - Position */
  gdispFillRoundedBox(
    350, 145,
    40, 15,
    2,
    l_pos_color
  );
  gdispDrawStringBox(
    350, 145,
    40, 15,
    "L-Pos",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  /* Remote - Position */
  gdispFillRoundedBox(
    350, 165,
    40, 15,
    2,
    r_pos_color
  );
  gdispDrawStringBox(
    350, 165,
    40, 15,
    "R-Pos",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  /* Remote - Motor */
  gdispFillRoundedBox(
    350, 185,
    40, 15,
    2,
    r_mot_color
  );
  gdispDrawStringBox(
    350, 185,
    40, 15,
    "R-Mot",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  screen_dashboard_azimuth_control_last = control_azimuth;
}

static int16_t screen_dashboard_azimuth_pwm_last = 0;
static char screen_dashboard_azimuth_pwm_string[8];
static int32_t screen_dashboard_azimuth_pwm_percent;
void screen_dashboard_azimuth_pwm_draw(bool force_redraw)
{
  if(!force_redraw && (azimuth_pwm == screen_dashboard_azimuth_pwm_last))
  {
    return;
  }

  screen_dashboard_azimuth_pwm_percent = (100 * (int32_t)azimuth_pwm) / 1024;

  /* Az PWM bargraph box */
  gdispGDrawBox(
    pixmap_azimuth_pwm,
    0, 0,
    35, 80,
    Black
  );

  /* Blank out box */
  gdispGFillArea(
    pixmap_azimuth_pwm,
    1, 1,
    33, 78,
    White
  );

  /* Draw center line */
  gdispGDrawLine(pixmap_azimuth_pwm, 1, 40, 34, 40, Black);

  if(azimuth_pwm > 0)
  {
    gdispGFillArea(
      pixmap_azimuth_pwm,
      2, 40-((39*screen_dashboard_azimuth_pwm_percent)/100),
      31, ((39*screen_dashboard_azimuth_pwm_percent)/100),
      Orange
    );
    chsnprintf(screen_dashboard_azimuth_pwm_string, 7, "%+d%%", screen_dashboard_azimuth_pwm_percent);
  }
  else if(azimuth_pwm < 0)
  {
    gdispGFillArea(
      pixmap_azimuth_pwm,
      2, 41,
      31, ((-1*39*screen_dashboard_azimuth_pwm_percent)/100),
      Orange
    );
    chsnprintf(screen_dashboard_azimuth_pwm_string, 7, "%+d%%", screen_dashboard_azimuth_pwm_percent);
  }
  else
  {
    strcpy(screen_dashboard_azimuth_pwm_string, "Brake");
  }

  gdispBlitArea(395, 110, 35, 80, gdispPixmapGetBits(pixmap_azimuth_pwm));

  gwinSetText(label_azimuth_motor_pwm, screen_dashboard_azimuth_pwm_string, false);

  screen_dashboard_azimuth_pwm_last = azimuth_pwm;
}

static int16_t screen_dashboard_azimuth_current_last = 0;
static char screen_dashboard_azimuth_current_string[8];
static int32_t screen_dashboard_azimuth_current_milliamps;
void screen_dashboard_azimuth_current_draw(bool force_redraw)
{
  if(!force_redraw && (azimuth_current == screen_dashboard_azimuth_current_last))
  {
    return;
  }

  screen_dashboard_azimuth_current_milliamps = 1.25 * (int32_t)azimuth_current;

  /* Az Current bargraph box */
  gdispGDrawBox(
    pixmap_azimuth_current,
    0, 0,
    35, 80,
    Black
  );

  /* Blank out box */
  gdispGFillArea(
    pixmap_azimuth_current,
    1, 1,
    33, 78,
    White
  );

  /* Az Current Value */
  gdispGFillArea(
    pixmap_azimuth_current,
    2, 79-((78*screen_dashboard_azimuth_current_milliamps)/4000),
    31, ((78*screen_dashboard_azimuth_current_milliamps)/4000),
    Red
  );

  gdispBlitArea(435, 110, 35, 80, gdispPixmapGetBits(pixmap_azimuth_current));

  chsnprintf(screen_dashboard_azimuth_current_string, 7, "%.2fA", (float)screen_dashboard_azimuth_current_milliamps/1000);

  gwinSetText(label_azimuth_motor_current, screen_dashboard_azimuth_current_string, false);

  screen_dashboard_azimuth_current_last = azimuth_current;
}

/*** System Boxes ***/
static remotedevice_t screen_dashboard_device_elresolver_last;
static void screen_dashboard_device_elresolver_draw(bool force_redraw)
{
  if(!force_redraw && (device_elresolver == screen_dashboard_device_elresolver_last))
  {
    return;
  }

  static color_t screen_dashboard_device_elresolver_color;

  if(device_elresolver == DEVICE_OK)
  {
    /* OK */
    screen_dashboard_device_elresolver_color = Lime;
  }
  else if(device_elresolver == DEVICE_FAULT)
  {
    /* Fault */
    screen_dashboard_device_elresolver_color = Red;
  }
  else // if(device_elresolver == DEVICE_UNKNOWN)
  {
    /* Unknown! */
    screen_dashboard_device_elresolver_color = Purple;
  }

  gdispFillRoundedBox(
    160, 210,
    90, 15,
    2,
    screen_dashboard_device_elresolver_color
  );
  gdispDrawStringBox(
    160, 210,
    90, 15,
    "EL Resolver",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  screen_dashboard_device_elresolver_last = device_elresolver;
}

static remotedevice_t screen_dashboard_device_azresolver_last;
static void screen_dashboard_device_azresolver_draw(bool force_redraw)
{
  if(!force_redraw && (device_azresolver == screen_dashboard_device_azresolver_last))
  {
    return;
  }

  static color_t screen_dashboard_device_azresolver_color;

  if(device_azresolver == DEVICE_OK)
  {
    /* OK */
    screen_dashboard_device_azresolver_color = Lime;
  }
  else if(device_azresolver == DEVICE_FAULT)
  {
    /* Fault */
    screen_dashboard_device_azresolver_color = Red;
  }
  else // if(device_azresolver == DEVICE_UNKNOWN)
  {
    /* Unknown! */
    screen_dashboard_device_azresolver_color = Purple;
  }

  gdispFillRoundedBox(
    160, 230,
    90, 15,
    2,
    screen_dashboard_device_azresolver_color
  );
  gdispDrawStringBox(
    160, 230,
    90, 15,
    "AZ Resolver",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  screen_dashboard_device_azresolver_last = device_azresolver;
}

static remotedevice_t screen_dashboard_device_elmotor_last;
static void screen_dashboard_device_elmotor_draw(bool force_redraw)
{
  if(!force_redraw && (device_elmotor == screen_dashboard_device_elmotor_last))
  {
    return;
  }

  static color_t screen_dashboard_device_elmotor_color;

  if(device_elmotor == DEVICE_OK)
  {
    /* OK */
    screen_dashboard_device_elmotor_color = Lime;
  }
  else if(device_elmotor == DEVICE_FAULT)
  {
    /* Fault */
    screen_dashboard_device_elmotor_color = Red;
  }
  else if(device_elmotor == DEVICE_INACTIVE)
  {
    /* Inactive (control timed out) */
    screen_dashboard_device_elmotor_color = Grey;
  }
  else // if(device_elmotor == DEVICE_UNKNOWN)
  {
    /* Unknown! */
    screen_dashboard_device_elmotor_color = Purple;
  }

  gdispFillRoundedBox(
    270, 210,
    90, 15,
    2,
    screen_dashboard_device_elmotor_color
  );
  gdispDrawStringBox(
    270, 210,
    90, 15,
    "EL Motor",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  screen_dashboard_device_elmotor_last = device_elmotor;
}

static remotedevice_t screen_dashboard_device_azmotor_last;
static void screen_dashboard_device_azmotor_draw(bool force_redraw)
{
  if(!force_redraw && (device_azmotor == screen_dashboard_device_azmotor_last))
  {
    return;
  }

  static color_t screen_dashboard_device_azmotor_color;

  if(device_azmotor == DEVICE_OK)
  {
    /* OK */
    screen_dashboard_device_azmotor_color = Lime;
  }
  else if(device_azmotor == DEVICE_FAULT)
  {
    /* Fault */
    screen_dashboard_device_azmotor_color = Red;
  }
  else if(device_azmotor == DEVICE_INACTIVE)
  {
    /* Inactive (control timed out) */
    screen_dashboard_device_azmotor_color = Grey;
  }
  else // if(device_azmotor == DEVICE_UNKNOWN)
  {
    /* Unknown! */
    screen_dashboard_device_azmotor_color = Purple;
  }

  gdispFillRoundedBox(
    270, 230,
    90, 15,
    2,
    screen_dashboard_device_azmotor_color
  );
  gdispDrawStringBox(
    270, 230,
    90, 15,
    "AZ Motor",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  screen_dashboard_device_azmotor_last = device_azmotor;
}

static estop_t screen_dashboard_estop_last;
static void screen_dashboard_draw_estop(bool force_redraw)
{
  if(!force_redraw && (estop == screen_dashboard_estop_last))
  {
    return;
  }

  static color_t screen_dashboard_estop_color;

  if(estop == ESTOP_OK)
  {
    /* OK */
    screen_dashboard_estop_color = Lime;
  }
  else if(estop == ESTOP_HALT)
  {
    /* ESTOP! */
    screen_dashboard_estop_color = Red;
  }
  else // if(estop == ESTOP_UNKNOWN)
  {
    /* Unknown */
    screen_dashboard_estop_color = Purple;
  }

  /* ESTOP Box */
  gdispFillRoundedBox(
    380, 210,
    90, 35,
    2,
    screen_dashboard_estop_color
  );

  gdispDrawStringBox(
    380, 210,
    90, 35,
    "E-STOP",
    font_dejavusans16,
    Black,
    justifyCenter
  );

  screen_dashboard_estop_last = estop;
}

static uint32_t screen_dashboard_network_last;
static void screen_dashboard_network_draw(bool force_redraw)
{
  if(!force_redraw && (app_ip_link_status() == screen_dashboard_network_last))
  {
    return;
  }

  static color_t screen_dashboard_network_color;

  if(app_ip_link_status() == APP_IP_LINK_STATUS_DOWN)
  {
    /* No Link */
    screen_dashboard_network_color = Red;
  }
  else if(app_ip_link_status() == APP_IP_LINK_STATUS_UPBUTNOIP)
  {
    /* Link but no IP */
    screen_dashboard_network_color = Grey;
  }
  else if(app_ip_link_status() == APP_IP_LINK_STATUS_BOUND)
  {
    /* Link & IP */
    screen_dashboard_network_color = Lime;
  }

  /* Network Box */
  gdispFillRoundedBox(
    160, 250,
    90, 15,
    2,
    screen_dashboard_network_color
  );
  gdispDrawStringBox(
    160, 250,
    90, 15,
    "Network",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  screen_dashboard_network_last = app_ip_link_status();
}

static uint32_t screen_dashboard_timesync_last;
static void screen_dashboard_timesync_draw(bool force_redraw)
{
  if(!force_redraw && (app_ip_service_sntp_status() == screen_dashboard_timesync_last))
  {
    return;
  }

  static color_t screen_dashboard_timesync_color;

  if(app_ip_service_sntp_status() == APP_IP_SERVICE_SNTP_STATUS_DOWN)
  {
    /* No NTP Reachable */
    screen_dashboard_timesync_color = Red;
  }
  else if(app_ip_service_sntp_status() == APP_IP_SERVICE_SNTP_STATUS_POLLING)
  {
    /* NTP reached, syncing system clock */
    screen_dashboard_timesync_color = Grey;
  }
  else if(app_ip_service_sntp_status() == APP_IP_SERVICE_SNTP_STATUS_SYNCED)
  {
    /* NTP reached, system clock synced */
    screen_dashboard_timesync_color = Lime;
  }

  /* Time Sync Box */
  gdispFillRoundedBox(
    270, 250,
    90, 15,
    2,
    screen_dashboard_timesync_color
  );
  gdispDrawStringBox(
    270, 250,
    90, 15,
    "Time Sync",
    font_dejavusans12,
    Black,
    justifyCenter
  );

  screen_dashboard_timesync_last = app_ip_service_sntp_status();
}

static void screen_dashboard_clock_draw(void)
{
  gwinSetText(label_clock, screen_clock_string(), true);

  gdispBlitArea(5, 251, 150, 20, gdispPixmapGetBits(pixmap_clock));
}

static void screen_dashboard_drawall(bool force_redraw)
{
  if(force_redraw)
  {
    /* El/Az divider line */
    gdispDrawLine(20, 105, 460, 105, HTML2COLOR(0xC0C0C0));
      
    /* Az/System divider line */
    gdispDrawLine(160, 205, 470, 205, HTML2COLOR(0xC0C0C0));

    gdispDrawStringBox(370, 250, 108, 20, "Phil Crump M0DNY", font_ui2, Black, justifyLeft);
  }

  screen_dashboard_elevation_draw(force_redraw);
  screen_dashboard_elevation_limit1_draw(force_redraw);
  screen_dashboard_elevation_limit2_draw(force_redraw);
  screen_dashboard_elevation_control_draw(force_redraw);
  screen_dashboard_elevation_pwm_draw(force_redraw);
  screen_dashboard_elevation_current_draw(force_redraw);

  screen_dashboard_azimuth_draw(force_redraw);
  screen_dashboard_azimuth_control_draw(force_redraw);
  screen_dashboard_azimuth_pwm_draw(force_redraw);
  screen_dashboard_azimuth_current_draw(force_redraw);

  screen_dashboard_device_elresolver_draw(force_redraw);
  screen_dashboard_device_azresolver_draw(force_redraw);
  screen_dashboard_device_elmotor_draw(force_redraw);
  screen_dashboard_device_azmotor_draw(force_redraw);
  screen_dashboard_network_draw(force_redraw);
  screen_dashboard_timesync_draw(force_redraw);
  screen_dashboard_draw_estop(force_redraw);

  screen_dashboard_clock_draw();
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
  font_ui1 = gdispOpenFont("UI1");
  font_ui2 = gdispOpenFont("UI2");
  font_dejavusans10 = gdispOpenFont("DejaVuSans10");
  font_dejavusans12 = gdispOpenFont("DejaVuSans12");
  font_dejavusans16 = gdispOpenFont("DejaVuSans16");
  font_dejavusans20 = gdispOpenFont("DejaVuSans20");
  font_dejavusans32 = gdispOpenFont("DejaVuSans32");

  widgets_init();

  screen_dashboard_drawall(true);

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
      screen_dashboard_drawall(false);
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
            screen_dashboard_drawall(true);
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
            screen_dashboard_drawall(true);
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
              screen_dashboard_drawall(true);
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
