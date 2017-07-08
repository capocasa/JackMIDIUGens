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
  for (uint32 i = 0; i < 256; i++) {
    unit->output_buffer[i] = 0;
  }
  for (uint32 i = 0; i < configured_channel_count; i++) {
    unit->configured_channels[i] = IN0(4+i);
    //std::cout << unit->configured_channels[i] << " ";
  }
  //std::cout << "\n";
  for (uint32 i = 0; i < configured_controller_count; i++) {
    unit->configured_controllers[i] = IN0(4+configured_channel_count+i);
    //std::cout << unit->configured_controllers[i] << " ";
  }
  //std::cout << "\n";
  SETCALC(JackMIDIIn_next); 
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

  // I think James McCartney's spirit will haunt me for this one,
  // but I just can't get myself to use nasty macros to avoid a
  // single extra comparison per control period

  bool audiorate = inNumSamples > 1;

  while (event_index < event_count) {
    jack_midi_event_get(&event, jack_midi_port_in_buffer, event_index);
    
    time = event.time - offset;

    if (time >= FULLBUFLENGTH) {
      break;
    }
    
    //std::cout << "event event_index " << event_index << " event_count " << event_count << " time " << event.time << " offset " << offset << " buffer " << (int)event.buffer[0] << " " << (int)event.buffer[1] << " " << (int)event.buffer[2] << " jack_frame_time " << jack_frame_time << " FULLBUFLENGTH " << FULLBUFLENGTH << std::endl;
    
    uint32 event_status = event.buffer[0];
    uint32 event_num = event.buffer[1];
    uint32 event_value = event.buffer[2];

    uint32 event_type = (int) event_status / 16;
    uint32 event_channel = event_status % 16;

    //std::cout << "event_type " << event_type << "event_channel " << channel << "\n";

    // Skip if wrong channel
    int channel_index;
    if (configured_channel_count) {
      for (channel_index = 0; channel_index < configured_channel_count; channel_index++) {
        if (configured_channels[channel_index] == event_channel) {
          break;
        }
      }
      if (channel_index == configured_channel_count) {
        // ugen not configured for this channel, skip event
        event_index++;
        continue;
      }
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
        if (output_buffer[output_buffer_index] == 0) {
          break;
        }
      }
      if (output_buffer_index < notes_channel_count) {
        output_buffer[output_buffer_index] = event_num;
        output_buffer[output_buffer_index+1] = event_value;
      } else {
        // potentially warn
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
      }  else {
        // potentially warn
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
    
    
    }
   

    event_index++;
  
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

