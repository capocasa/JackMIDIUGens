/*
    JackMIDIUGens Jack MIDI for SuperCollider
    Copyright (c) 2017 Carlo Capocasa. All rights reserved.
    https://capocasa.net

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "SC_PlugIn.h"
#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>

// MIDI event types

#define EVENT_NOTEOFF      8
#define EVENT_NOTEON       9
#define EVENT_POLYTOUCH    10
#define EVENT_CONTROLLER   11
#define EVENT_TOUCH        13
#define EVENT_PITCHBEND    14

// Integers to represent configurable controllers
// Arbitrary, must be the same as in the sclang class file

#define CONTROLLER_PITCHBEND  1014
#define CONTROLLER_TOUCH      1013


static InterfaceTable *ft;


struct JackMIDIIn: public Unit
{
  void*                       jack_midi_port_in_buffer;
  jack_nframes_t              jack_frame_time;
  jack_nframes_t              event_index;
  jack_nframes_t              event_count;
  jack_nframes_t              offset;
  uint32                      configured_controller_count;
  uint32                      configured_controllers[256];
  uint32                      output_buffer[256];
  uint32                      polyphony;
  bool                        polytouch;
  uint32                      configured_channel_count;
  uint32                      configured_channels[16];
  uint32                      configured_note_count;
  uint32                      configured_notes[128];
};

static void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples);
static void JackMIDIIn_Ctor(JackMIDIIn* unit);
jack_client_t* jack_client = NULL; 
jack_port_t* jack_midi_port_in = NULL;
jack_nframes_t jack_nframes = 0;

int jack_buffer_size(jack_nframes_t nframes, void *arg) {
  jack_nframes = nframes;
}

void jack_init() {
  if ((jack_client = jack_client_open("JackMIDIUGens", JackNullOption, NULL)) == 0)
  {
    //std::cout << "JackMIDIIn: cannot connect to jack server" << std::endl;
    return;
  }
  jack_midi_port_in = jack_port_register (jack_client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  //jack_set_process_callback (client, process, unit);
  if (jack_activate(jack_client) != 0)
  {
    //std::cout<<  "JackMIDIIn: cannot activate jack client" << std::endl;
    return;
  }
  jack_nframes = jack_get_buffer_size(jack_client);
  jack_set_buffer_size_callback(jack_client, jack_buffer_size, 0);
}

PluginLoad(JackMIDIIn)
{
  ft = inTable;
  DefineSimpleUnit(JackMIDIIn);
  
  jack_init();
}

void JackMIDIIn_Ctor(JackMIDIIn* unit)
{
  unit->jack_frame_time = 0;
  unit->polyphony = IN0(0);
  uint32 configured_channel_count = IN0(1);
  unit->configured_channel_count = configured_channel_count;
  uint32 configured_controller_count = IN0(2);
  unit->configured_controller_count = configured_controller_count;
  unit->polytouch = IN0(3);
  uint32 configured_note_count = IN0(4);
  unit->configured_note_count = configured_note_count;

  for (uint32 i = 0; i < 256; i++) {
    unit->output_buffer[i] = 0;
  }
  
  uint32 offset_in = 5;
  for (uint32 i = 0; i < configured_channel_count; i++) {
    unit->configured_channels[i] = IN0(offset_in++);
    //std::cout << unit->configured_channels[i] << " ";
  }
  //std::cout << "\n";
  for (uint32 i = 0; i < configured_controller_count; i++) {
    unit->configured_controllers[i] = IN0(offset_in++);
    //std::cout << unit->configured_controllers[i] << " ";
  }
  //std::cout << "\n";
  for (uint32 i = 0; i < configured_note_count; i++) {
    unit->configured_notes[i] = IN0(offset_in++);
    //std::cout << unit->configured_notes[i] << " ";
  }
  //std::cout << "\n";
  SETCALC(JackMIDIIn_next); 
}

inline bool is_configured_channel(uint32* configured_channels, uint32 configured_channel_count, uint32 event_channel) {
  if (configured_channel_count == 0) {
    return true;
  }
  for (int i = 0; i < configured_channel_count; i++) {
    if (configured_channels[i] == event_channel) {
      return true;
    }
  }
  return false;
}

inline bool is_configured_note(uint32* configured_notes, uint32 configured_note_count, uint32 event_note) {
  if (configured_note_count == 0) {
    return true;
  }
  for (int i = 0; i < configured_note_count; i++) {
    if (configured_notes[i] == event_note) {
      return true;
    }
  }
  return false;
}

void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples)
{
  int numOutputs = unit->mNumOutputs;
  //std::cout << "next" << std::endl;

  jack_nframes_t     jack_frame_time = jack_last_frame_time(jack_client);

  void*              jack_midi_port_in_buffer;
  jack_nframes_t     offset;
  jack_nframes_t     event_index;
  jack_nframes_t     event_count;

  if (unit->jack_frame_time != jack_frame_time) {
    jack_midi_port_in_buffer = jack_port_get_buffer(jack_midi_port_in, jack_nframes);
    
    //std::cout << "numOutputs " << numOutputs << std::endl;
  
    //std::cout << "new frame time " << jack_frame_time << std::endl;
 
    jack_midi_event_t event;
    event_index = 0;
    event_count = jack_midi_get_event_count(jack_midi_port_in_buffer);
    offset = 0;
 
  } else {
    //std::cout << "WARNING recycling" << std::endl;
    jack_midi_port_in_buffer = unit->jack_midi_port_in_buffer;
    event_index = unit->event_index;
    event_count = unit->event_count;
    offset = unit->offset;
  }

  //std::cout << "cycle" << std::endl;
  
  jack_midi_event_t event;
  jack_nframes_t time;
  
  jack_nframes_t last_time = 0;
    
  uint32* output_buffer = unit->output_buffer;

  uint32 polyphony = unit->polyphony;
  uint32 configured_controller_count = unit->configured_controller_count;
  uint32* configured_controllers = unit->configured_controllers;

  uint32 polytouch = unit->polytouch;
  uint32 note_channel_count = 2 + polytouch;
  uint32 notes_channel_count = note_channel_count * polyphony;

  uint32* output_buffer_channel_controllers = output_buffer + notes_channel_count;

  uint32* configured_channels = unit->configured_channels;
  uint32 configured_channel_count = unit->configured_channel_count;
  
  uint32* configured_notes = unit->configured_notes;
  uint32 configured_note_count = unit->configured_note_count;

  // I think James McCartney's spirit will haunt me for this one,
  // but I just can't get myself to use nasty macros to avoid a
  // single extra comparison per control period

  bool audiorate = inNumSamples > 1;

  for ( ; event_index < event_count; event_index++) {
    jack_midi_event_get(&event, jack_midi_port_in_buffer, event_index);
    
    time = event.time - offset;

    if (time >= FULLBUFLENGTH) {
      std::cout << "JackMIDIUGens: Warning, time too early, dropped note" << std::endl;
      break;
    }
    
    //std::cout << "event event_index " << event_index << " event_count " << event_count << " time " << event.time << " offset " << offset << " buffer " << (int)event.buffer[0] << " " << (int)event.buffer[1] << " " << (int)event.buffer[2] << " jack_frame_time " << jack_frame_time << " FULLBUFLENGTH " << FULLBUFLENGTH << std::endl;
    
    uint32 event_status = event.buffer[0];
    uint32 event_num = event.buffer[1];
    uint32 event_value = event.buffer[2];

    uint32 event_type = (int) event_status / 16;
    uint32 event_channel = event_status % 16;
    

    //std::cout << "EVENT: event_type " << event_type << " event_channel " << event_channel << " event_num " << event_num << std::endl;

    if ( ! is_configured_channel(configured_channels, configured_channel_count, event_channel)) {
      //std::cout << "nochan note " << event_value << " channel " << event_channel << std::endl;
      continue;
    }
    
    switch(event_type) {
    case EVENT_NOTEON:
    case EVENT_NOTEOFF:
      if ( ! is_configured_note(configured_notes, configured_note_count, event_num)) {
        // std::cout << "nonote note " << event_value << " channel " << event_channel << std::endl;
        continue;
      }
    default:
      break;
    }

    // Output up until just before this event
    if (audiorate) {
      for (jack_nframes_t i = last_time; i < time; i++) {
        for (int j = 0; j < numOutputs; j++) {
          OUT(j)[i] = (float)output_buffer[j];
        }
      }
    } else {
      // no intermittent output for control rate
    }

    uint32 output_buffer_index;
    switch(event_type) {
    
    case EVENT_NOTEON:

      // find empty output 
      for (output_buffer_index = 0; output_buffer_index < notes_channel_count; output_buffer_index += note_channel_count) {
        if (event_num == 0 || output_buffer[output_buffer_index] == 0) {
          break;
        }
      }
      if (output_buffer_index < notes_channel_count) {
        output_buffer[output_buffer_index] = event_num;
        output_buffer[output_buffer_index+1] = event_value;
        //std::cout << "NOTE ON: note " << event_num << " on channel " << event_channel << std::endl;
      } else {
        // potentially warn
        std::cout << "JackMIDIUGens: Warning, dropped noteOn event " << event_num << " on channel " << event_channel << std::endl;
      }
      break;
    
    case EVENT_NOTEOFF:
      
      // find playing note
      for (output_buffer_index = 0; output_buffer_index < notes_channel_count; output_buffer_index += note_channel_count) {
        if (output_buffer[output_buffer_index] == event_num) {
          break;
        }
      }
      if (output_buffer_index < notes_channel_count) {
        output_buffer[output_buffer_index] = 0;
        output_buffer[output_buffer_index+1] = 0;
        if (polytouch) {
          output_buffer[output_buffer_index+2] = 0;
        }
        //std::cout << "NOTE OFF: note " << event_num << " on channel " << event_channel << std::endl;
      }  else {
        // potentially warn
        //std::cout << "JackMIDIUGens: Warning, dropped noteOff event " << event_num << " on channel " << event_channel << std::endl;
      }
      break;
    
    case EVENT_PITCHBEND:
      
      //std::cout << "bend " << event_num << " " << event_value << std::endl;
      
      for (int i = 0; i < configured_controller_count; i++) {
        if (configured_controllers[i] == CONTROLLER_PITCHBEND) {
          output_buffer_channel_controllers[i] = (float)(event_num + 128*event_value);
        }
      }
      
      break;

    case EVENT_CONTROLLER:
     
      //std::cout << "controller " << event_num << " " << event_value << std::endl;

      for (int i = 0; i < configured_controller_count; i++) {
        if (configured_controllers[i] == event_num) {
          output_buffer_channel_controllers[i] = (float)event_value;
        }
      }

      break;
    
    case EVENT_POLYTOUCH:
      
      //std::cout << "polytouch " << event_num << " " << event_value << std::endl;
      if (polytouch) {
        // find playing note
        for (output_buffer_index = 0; output_buffer_index < notes_channel_count; output_buffer_index += note_channel_count) {
          if (output_buffer[output_buffer_index] == event_num) {
            break;
          }
        }
        if (output_buffer_index < notes_channel_count) {
          output_buffer[output_buffer_index+2] = event_value;
        }  else {
          // potentially warn
        }
      }

      break;
    
    case EVENT_TOUCH:
      
      //std::cout << "touch " << event_num << " " << event_value << std::endl;
      
      for (int i = 0; i < configured_controller_count; i++) {
        if (configured_controllers[i] == CONTROLLER_TOUCH) {
          output_buffer_channel_controllers[i] = (float)event_num;
        }
      }

      break;
   
    default:
      // ignore other types
      break;
    }
  
    last_time = time;
  }

  if (audiorate) {
    // Output this event and to completion of cycle
    for(jack_nframes_t i = last_time; i < FULLBUFLENGTH; i++) {
      for (int j = 0; j < numOutputs; j++) {
        OUT(j)[i] = (float)output_buffer[j];
      }
    }
  } else {
    // Output this event
    for (int j = 0; j < numOutputs; j++) {
      OUT0(j) = (float)output_buffer[j];
    }
  }

  offset += FULLBUFLENGTH;
  
  unit->jack_frame_time = jack_frame_time;
  unit->offset = offset;
  unit->event_index = event_index;
  unit->event_count = event_count;
  unit->jack_midi_port_in_buffer = jack_midi_port_in_buffer;

}

