#include "SC_PlugIn.h"
#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>

static InterfaceTable *ft;


struct JackMIDIIn: public Unit
{
  void*                       jack_midi_port_in_buffer;
  jack_nframes_t              jack_frame_time;
  jack_nframes_t              i;
  jack_nframes_t              n;
  jack_nframes_t              offset;
  float                       ob;
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
  unit->ob = 0.0;
  SETCALC(JackMIDIIn_next); 
}


void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples)
{
  int numOutputs = unit->mNumOutputs;
  //std::cout << "next" << std::endl;

  jack_nframes_t     jack_frame_time = jack_last_frame_time(jack_client);

  void*              jack_midi_port_in_buffer;
  jack_nframes_t     offset;
  jack_nframes_t     i;
  jack_nframes_t     n;

  if (unit->jack_frame_time != jack_frame_time) {
    jack_midi_port_in_buffer = jack_port_get_buffer(jack_midi_port_in, jack_nframes);
  
    //std::cout << "new frame time " << jack_frame_time << std::endl;
 
    jack_midi_event_t event;
    i = 0;
    n = jack_midi_get_event_count(jack_midi_port_in_buffer);
    offset = 0;
 
  } else {
    jack_midi_port_in_buffer = unit->jack_midi_port_in_buffer;
    i = unit->i;
    n = unit->n;
    offset = unit->offset;
  }

  //std::cout << "cycle" << std::endl;
  
  jack_midi_event_t event;
  jack_nframes_t time;
  
  jack_nframes_t last_time = 0;
    
  float ob = unit->ob;

  while (i < n) {
    jack_midi_event_get(&event, jack_midi_port_in_buffer, i);
    
    time = event.time - offset;

    if (time >= FULLBUFLENGTH) {
      break;
    }
    
    //std::cout << "event " << i << " " << n << " " << event.time << " " << offset << " " << time << " " << jack_frame_time << " " << FULLBUFLENGTH << std::endl;

    uint32 type = event.buffer[0];
    uint32 note = event.buffer[1];
    uint32 value = event.buffer[2];
   
    for (jack_nframes_t j = last_time; j < time; j++) {
      OUT(0)[j] = ob;
    }

    ob = (float)note;
    
    i++;
  
    last_time = time;
  }

  for(jack_nframes_t j = last_time; j < FULLBUFLENGTH; j++) {
    OUT(0)[j] = ob;
  }

  offset += FULLBUFLENGTH;
  
  unit->jack_frame_time = jack_frame_time;
  unit->offset = offset;
  unit->i = i;
  unit->n = n;
  unit->jack_midi_port_in_buffer = jack_midi_port_in_buffer;

}

