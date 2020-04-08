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

#include "ch.h"
#include "hal.h"

#include "gfx.h"

/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palSetLine(LINE_ARD_D13);
    chThdSleepMilliseconds(500);
    palClearLine(LINE_ARD_D13);
    chThdSleepMilliseconds(500);
  }
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

  /*
   * ARD_D13 is programmed as output (board LED).
   */
  palClearLine(LINE_ARD_D13);
  palSetLineMode(LINE_ARD_D13, PAL_MODE_OUTPUT_PUSHPULL);

  /* Initialise screen */
  gfxInit();
  gdispClear(White);

  font_t font;
  font = gdispOpenFont("UI2 Double");
  gdispDrawStringBox(5, 0, 200, 30, "SCOT Controller", font, Black, justifyLeft);
  gdispCloseFont(font);

  font = gdispOpenFont("UI2");
  gdispDrawStringBox(5, 25, 100, 30, "Phil Crump 2020", font, Black, justifyLeft);
  gdispCloseFont(font);

  /*
   * Creates the example thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO+1, Thread1, NULL);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    chThdSleepMilliseconds(500);
  }
}
