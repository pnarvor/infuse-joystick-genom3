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

#include <stdio.h>

#include "joystick_c_types.h"


/* --- Function device_list --------------------------------------------- */

/** Codel joystick_device_list of function device_list.
 *
 * Returns genom_ok.
 * Throws joystick_e_nomem.
 */
genom_event
joystick_device_list(const sequence_joystick_device_s *devices,
                     sequence_string32 *device_list,
                     genom_context self)
{
  int i;

  if (genom_sequence_reserve(device_list, devices->_length))
    return joystick_e_nomem(self);

  device_list->_length = devices->_length;
  for(i = 0; i < devices->_length; i++) {
    snprintf(device_list->_buffer[i], sizeof(device_list->_buffer[i]),
             "%s", devices->_buffer[i].name);
  }

  return genom_ok;
}
