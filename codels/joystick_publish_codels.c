/*
 * Copyright (c) 2016 LAAS/CNRS
 * All rights reserved.
 *
 * Redistribution and use  in source  and binary  forms,  with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   1. Redistributions of  source  code must retain the  above copyright
 *      notice and this list of conditions.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice and  this list of  conditions in the  documentation and/or
 *      other materials provided with the distribution.
 *
 *                                      Anthony Mallet on Thu Apr 14 2016
 */
#include "acjoystick.h"

#include <sys/time.h>

#include <err.h>
#include <fnmatch.h>
#include <signal.h>
#include <stdio.h>

#include "SDL.h"

#include "joystick_c_types.h"

static size_t	strnfilter(char *dst, size_t n, const char *src,
                           const char *accept, char repl);
static void	strnappend(char *dst, size_t n, int index);


/* --- Task publish ----------------------------------------------------- */


/** Codel joystick_init of task publish.
 *
 * Triggered by joystick_start.
 * Yields to joystick_poll.
 * Throws joystick_e_sdl.
 */
genom_event
joystick_init(joystick_ids *ids, genom_context self)
{
  joystick_e_sdl_detail d;

  if (SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_EVENTS)) {
    snprintf(d.what, sizeof(d.what), "%s", SDL_GetError());
    return joystick_e_sdl(&d, self);
  }

  SDL_JoystickEventState(SDL_ENABLE);

  ids->devices._length = 0;

  return joystick_poll;
}


/** Codel joystick_fini of task publish.
 *
 * Triggered by joystick_stop.
 * Yields to joystick_ether.
 * Throws joystick_e_sdl.
 */
genom_event
joystick_fini(sequence_joystick_device_s *devices, genom_context self)
{
  SDL_Quit();
  return joystick_ether;
}


/** Codel joystick_wait_event of task publish.
 *
 * Triggered by joystick_poll.
 * Yields to joystick_poll, joystick_event.
 * Throws joystick_e_sdl.
 */
genom_event
joystick_wait_event(genom_context self)
{
  if (!SDL_WaitEventTimeout(NULL, 500)) return joystick_poll;

  return joystick_event;
}


/** Codel joystick_main of task publish.
 *
 * Triggered by joystick_event.
 * Yields to joystick_poll.
 * Throws joystick_e_sdl.
 */
genom_event
joystick_main(sequence_joystick_device_s *devices,
              const joystick_device *device, genom_context self)
{
  char dirty[devices->_length];
  or_joystick_state *state;
  joystick_device_s *dev;
  struct timeval tv;
  SDL_Event ev;
  int i;

  gettimeofday(&tv, NULL);
  for(i = 0; i < sizeof(dirty); i++)
    dirty[i] = 0;

  while(SDL_PollEvent(&ev)) {
    switch(ev.type) {
      case SDL_JOYAXISMOTION: {
        for(i = 0, dev = devices->_buffer; i < devices->_length; i++, dev++)
          if (dev->id == ev.jaxis.which) break;
        if (!dev) break;
        state = device->data(dev->name, self);
        if (!state) break;
        if (ev.jaxis.axis >= state->axes._length) break;

        state->ts.sec = tv.tv_sec;
        state->ts.nsec = 1000 * tv.tv_usec;
        state->axes._buffer[ev.jaxis.axis] = ev.jaxis.value;
        if (i < sizeof(dirty)) dirty[i] = 1;
        break;
      }

      case SDL_JOYHATMOTION: {
        short ud, lr;

        for(i = 0, dev = devices->_buffer; i < devices->_length; i++, dev++)
          if (dev->id == ev.jhat.which) break;
        if (!dev) break;
        state = device->data(dev->name, self);
        if (!state) break;
        if (ev.jhat.hat >= dev->hats) {
          warnx("hat index out of range: %d (max %d)", ev.jhat.hat, dev->hats);
          break;
        }

        state->ts.sec = tv.tv_sec;
        state->ts.nsec = 1000 * tv.tv_usec;
        switch (ev.jhat.value) {
          case SDL_HAT_LEFTUP: case SDL_HAT_UP: case SDL_HAT_RIGHTUP:
            ud = -32768;
            break;

          case SDL_HAT_LEFTDOWN: case SDL_HAT_DOWN: case SDL_HAT_RIGHTDOWN:
            ud = 32767;
            break;

          default: ud = 0; break;
        }
        switch (ev.jhat.value) {
          case SDL_HAT_LEFTUP: case SDL_HAT_LEFT: case SDL_HAT_LEFTDOWN:
            lr = -32768;
            break;

          case SDL_HAT_RIGHTUP: case SDL_HAT_RIGHT: case SDL_HAT_RIGHTDOWN:
            lr = 32767;
            break;

          default: lr = 0; break;
        }
        state->axes._buffer[2*ev.jhat.hat + dev->axes] = ud;
        state->axes._buffer[2*ev.jhat.hat+1 + dev->axes] = lr;
        if (i < sizeof(dirty)) dirty[i] = 1;
        break;
      }

      case SDL_JOYBUTTONDOWN:
      case SDL_JOYBUTTONUP: {
        for(i = 0, dev = devices->_buffer; i < devices->_length; i++, dev++)
          if (dev->id == ev.jbutton.which) break;
        if (!dev) break;
        state = device->data(dev->name, self);
        if (!state) break;
        if (ev.jbutton.button >= state->buttons._length) break;

        state->ts.sec = tv.tv_sec;
        state->ts.nsec = 1000 * tv.tv_usec;
        state->buttons._buffer[ev.jbutton.button] =
          ev.jbutton.state == SDL_PRESSED;
        if (i < sizeof(dirty)) dirty[i] = 1;
        break;
      }

      case SDL_JOYDEVICEADDED: {
        SDL_Joystick *j;
        int index, exists;
        char name[32];
        genom_event e;

        j = SDL_JoystickOpen(ev.jdevice.which);
        if (!j) {
          warnx("adding new device: %s", SDL_GetError());
          break;
        }

        genom_sequence_reserve(devices, devices->_length + 1);
        dev = &devices->_buffer[devices->_length];

        dev->id = SDL_JoystickInstanceID(j);
        strnfilter(name, sizeof(name), SDL_JoystickName(j),
                   "[a-zA-Z0-9_]", '_');
    shortened:
        index = 0;
        do {
          strcpy(dev->name, name);
          if (index) strnappend(dev->name, sizeof(dev->name), index);

          for(exists = 0, i = 0; i < devices->_length; i++)
            if (!strcmp(dev->name, devices->_buffer[i].name)) {
              exists = 1;
              index++;
              break;
            }
        } while(exists);

        e = device->open(dev->name, self);
        if (e) {
          if (e == genom_syserr_id) {
            const genom_syserr_detail *d = self->raised(NULL, self);
            if (d && d->code == ENAMETOOLONG) {
              /* shorten the name */
              char *cut = strrchr(name, '_');
              if (cut) {
                *cut = 0;
                goto shortened;
              } else if (name[0] && name[1]) {
                name[strlen(name)-1] = 0;
                goto shortened;
              }
            }
          }

          SDL_JoystickClose(j);
          warnx("cannot add new device `%s'", dev->name);
          break;
        }

        state = device->data(dev->name, self);
        state->ts.sec = tv.tv_sec;
        state->ts.nsec = 1000 * tv.tv_usec;

        dev->buttons = SDL_JoystickNumButtons(j);
        state->buttons._length = dev->buttons;
        if (state->buttons._length > state->buttons._maximum)
          state->buttons._length = state->buttons._maximum;
        for(i = 0; i < state->buttons._length; i++)
          state->buttons._buffer[i] = false;

        dev->axes = SDL_JoystickNumAxes(j);
        dev->hats = SDL_JoystickNumHats(j);
        state->axes._length = dev->axes + 2*dev->hats;
        if (state->axes._length > state->axes._maximum)
          state->axes._length = state->axes._maximum;
        for(i = 0; i < state->axes._length; i++)
          state->axes._buffer[i] = 0;

        devices->_length++;
        warnx("added new device: %s", dev->name);
        break;
      }

      case SDL_JOYDEVICEREMOVED: {
        for(i = 0, dev = devices->_buffer; i < devices->_length; i++, dev++)
          if (dev->id == ev.jdevice.which) break;
        if (!dev) break;

        device->close(dev->name, self);
        warnx("removed device: %s", dev->name);

        memmove(dev, dev + 1, (devices->_length - i - 1) * sizeof(*dev));
        memmove(dirty + i, dirty + i + 1, devices->_length - i - 1);
        devices->_length--;
        break;
      }

      default:
        warnx("unexpected event 0x%04x", ev.type);
    }
  }

  for(i = 0; i < sizeof(dirty); i++)
    if (dirty[i]) device->write(devices->_buffer[i].name, self);
  for(i = sizeof(dirty); i < devices->_length; i++)
    device->write(devices->_buffer[i].name, self);

  return joystick_poll;
}


/* --- Activity rename -------------------------------------------------- */

/** Codel joystick_rename of activity rename.
 *
 * Triggered by joystick_start.
 * Yields to joystick_ether.
 * Throws joystick_e_sdl, joystick_e_noent.
 */
genom_event
joystick_rename(const char name[32], const char newname[32],
                sequence_joystick_device_s *devices,
                const joystick_device *device, genom_context self)
{
  char devname[32];
  genom_event e;
  int i;

  for(i = 0; i < devices->_length; i++) {
    if (!strcmp(name, devices->_buffer[i].name)) {
      strnfilter(devname, sizeof(devname), newname, "[a-zA-Z0-9_]", '_');
      e = device->open(devname, self);
      if (e) return e;

      memcpy(device->data(devname, self),
             device->data(devices->_buffer[i].name, self),
             sizeof(or_joystick_state));
      device->write(devname, self);

      device->close(devices->_buffer[i].name, self);
      strcpy(devices->_buffer[i].name, devname);
      return joystick_ether;
    }
  }

  return joystick_e_noent(self);
}


/* --- strnfilter ---------------------------------------------------------- */

static size_t
strnfilter(char *dst, size_t n, const char *src, const char *accept, char repl)
{
  char x[2];

  x[1] = '\0';
  while(--n && *src) {
    *x = *src++;
    if (fnmatch(accept, x, 0))
      *dst++ = repl;
    else
      *dst++ = *x;
  }
  *dst = '\0';
  return n;
}


/* --- strnappend ---------------------------------------------------------- */

static void
strnappend(char *dst, size_t n, int index)
{
  char buf[16];
  size_t a, l;

  a = snprintf(buf, sizeof(buf), "_%d", index);
  l = strlen(dst);
  if (l + a > n - 1) l = n - a - 1;

  strcpy(dst + l, buf);
}
