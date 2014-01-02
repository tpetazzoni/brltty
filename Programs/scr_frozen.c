/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2014 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "scr.h"
#include "scr_frozen.h"

static ScreenDescription screenDescription;
static ScreenCharacter *screenCharacters;

static int
construct_FrozenScreen (BaseScreen *source) {
  describeBaseScreen(source, &screenDescription);
  if ((screenCharacters = calloc(screenDescription.rows*screenDescription.cols, sizeof(*screenCharacters)))) {
    const ScreenBox box = {
      .left=0, .width=screenDescription.cols,
      .top=0, .height=screenDescription.rows
    };
    if (source->readCharacters(&box, screenCharacters)) {
      return 1;
    }
    free(screenCharacters);
    screenCharacters = NULL;
  } else {
    logMallocError();
  }
  return 0;
}

static void
destruct_FrozenScreen (void) {
  if (screenCharacters) {
    free(screenCharacters);
    screenCharacters = NULL;
  }
}

static void
describe_FrozenScreen (ScreenDescription *description) {
  *description = screenDescription;
}

static int
readCharacters_FrozenScreen (const ScreenBox *box, ScreenCharacter *buffer) {
  if (validateScreenBox(box, screenDescription.cols, screenDescription.rows)) {
    int row;
    for (row=0; row<box->height; row++) {
      memcpy(&buffer[row * box->width],
             &screenCharacters[((box->top + row) * screenDescription.cols) + box->left],
             box->width * sizeof(*screenCharacters));
    }
    return 1;
  }
  return 0;
}

static int
currentVirtualTerminal_FrozenScreen (void) {
  return screenDescription.number;
}

void
initializeFrozenScreen (FrozenScreen *frozen) {
  initializeBaseScreen(&frozen->base);
  frozen->base.describe = describe_FrozenScreen;
  frozen->base.readCharacters = readCharacters_FrozenScreen;
  frozen->base.currentVirtualTerminal = currentVirtualTerminal_FrozenScreen;
  frozen->construct = construct_FrozenScreen;
  frozen->destruct = destruct_FrozenScreen;
  screenCharacters = NULL;
}
