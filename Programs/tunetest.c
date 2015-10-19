/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2015 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* tunetest.c - Test program for the tune playing library
 */

#include "prologue.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "log.h"
#include "options.h"
#include "tune.h"
#include "tune_utils.h"
#include "notes.h"
#include "parse.h"
#include "prefs.h"

static char *opt_tuneDevice;
static char *opt_outputVolume;

#ifdef HAVE_MIDI_SUPPORT
static char *opt_midiInstrument;
#endif /* HAVE_MIDI_SUPPORT */

BEGIN_OPTION_TABLE(programOptions)
  { .letter = 'v',
    .word = "volume",
    .argument = "loudness",
    .setting.string = &opt_outputVolume,
    .description = "Output volume (percentage)."
  },

  { .letter = 'd',
    .word = "device",
    .argument = "device",
    .setting.string = &opt_tuneDevice,
    .description = "Name of tune device."
  },

#ifdef HAVE_PCM_SUPPORT
  { .letter = 'p',
    .word = "pcm-device",
    .flags = OPT_Hidden,
    .argument = "device",
    .setting.string = &opt_pcmDevice,
    .description = "Device specifier for soundcard digital audio."
  },
#endif /* HAVE_PCM_SUPPORT */

#ifdef HAVE_MIDI_SUPPORT
  { .letter = 'm',
    .word = "midi-device",
    .flags = OPT_Hidden,
    .argument = "device",
    .setting.string = &opt_midiDevice,
    .description = "Device specifier for the Musical Instrument Digital Interface."
  },

  { .letter = 'i',
    .word = "instrument",
    .argument = "instrument",
    .setting.string = &opt_midiInstrument,
    .description = "Name of MIDI instrument."
  },
#endif /* HAVE_MIDI_SUPPORT */
END_OPTION_TABLE

static int
parseTone (const char *operand, int *note, int *duration) {
  const size_t operandSize = strlen(operand) + 1;
  char noteOperand[operandSize];
  char durationOperand[operandSize];

  int noteValue = NOTE_MIDDLE_C;
  int durationValue = 255;

  {
    const char *delimiter = strchr(operand, '/');

    if (delimiter) {
      const size_t length = delimiter - operand;

      memcpy(noteOperand, operand, length);
      noteOperand[length] = 0;

      strcpy(durationOperand, delimiter+1);
    } else {
      strcpy(noteOperand, operand);
      *durationOperand = 0;
    }
  }

  if (*noteOperand) {
    const char *c = noteOperand;

    static const char letters[] = "cdefgab";
    const char *letter = strchr(letters, *c);

    if (letter) {
      static const unsigned char offsets[] = {0, 2, 4, 5, 7, 9, 11};

      noteValue += offsets[letter - letters];
      c += 1;

      if (*c) {
        static const int minimum = -1;
        static const int maximum = 9;
        int octave = 4;

        if (!validateInteger(&octave, c, &minimum, &maximum)) {
          logMessage(LOG_ERR, "invalid octave: %s", c);
          return 0;
        }

        noteValue += (octave - 4) * NOTES_PER_OCTAVE;
      }
    } else {
      const int minimum = getLowestNote();
      const int maximum = getHighestNote();

      if (!validateInteger(&noteValue, c, &minimum, &maximum)) {
        logMessage(LOG_ERR, "invalid note: %s", noteOperand);
        return 0;
      }
    }
  }

  if ((noteValue < 0) || (noteValue > getHighestNote())) {
    logMessage(LOG_ERR, "note out of range: %s", noteOperand);
    return 0;
  }

  if (*durationOperand) {
    static const int minimum = 1;
    int maximum = durationValue;

    if (!validateInteger(&durationValue, durationOperand, &minimum, &maximum)) {
      logMessage(LOG_ERR, "invalid duration: %s", durationOperand);
      return 0;
    }
  }

  *note = noteValue;
  *duration = durationValue;
  return 1;
}

int
main (int argc, char *argv[]) {
  {
    static const OptionsDescriptor descriptor = {
      OPTION_TABLE(programOptions),
      .applicationName = "tunetest",
      .argumentsSummary = "[note][/[duration]] ..."
    };
    PROCESS_OPTIONS(descriptor, argc, argv);
  }

  resetPreferences();
  if (!setTuneDevice(opt_tuneDevice)) return PROG_EXIT_SYNTAX;
  if (!setTuneVolume(opt_outputVolume)) return PROG_EXIT_SYNTAX;

#ifdef HAVE_MIDI_SUPPORT
  if (!setTuneInstrument(opt_midiInstrument)) return PROG_EXIT_SYNTAX;
#endif /* HAVE_MIDI_SUPPORT */

  if (!argc) {
    logMessage(LOG_ERR, "missing tune");
    return PROG_EXIT_SYNTAX;
  }

  {
    NoteElement elements[argc + 1];

    {
      NoteElement *element = elements;

      while (argc) {
        int note;
        int duration;

        if (!parseTone(*argv, &note, &duration)) {
          return PROG_EXIT_SYNTAX;
        }

        {
          NoteElement tone = NOTE_PLAY(duration, note);
          *(element++) = tone;
        }

        argv += 1;
        argc -= 1;
      }

      {
        NoteElement tone = NOTE_STOP();
        *element = tone;
      }
    }

    if (!selectTuneDevice()) return PROG_EXIT_SEMANTIC;
    tunePlayNotes(elements);
    tuneSynchronize();
  }

  return PROG_EXIT_SUCCESS;
}
