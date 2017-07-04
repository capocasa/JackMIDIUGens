#include "SC_PlugIn.h"
#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>

static InterfaceTable *ft;


struct JackMIDIIn: public Unit
{
  uint32 jack_frame;
  void* port_buf;
  jack_nframes_t              jack_frame_time;
  jack_nframes_t              i;
  jack_nframes_t              n;
  jack_nframes_t              offset;
};

static void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples);
static void JackMIDIIn_Ctor(JackMIDIIn* unit);
jack_client_t* client = NULL; 
jack_port_t* port = NULL;
jack_nframes_t nframes = 0;

int jack_buffer_size(jack_nframes_t nframes_new, void *arg) {
  nframes = nframes_new;
}

void jack_init() {
  if ((client = jack_client_open("SuperCollider JackMIDI", JackUseExactName, NULL)) == 0)
  {
    //std::cout << "JackMIDIIn: cannot connect to jack server" << std::endl;
    return;
  }
  port = jack_port_register (client, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  //jack_set_process_callback (client, process, unit);
  if (jack_activate(client) != 0)
  {
    //std::cout<<  "JackMIDIIn: cannot activate jack client" << std::endl;
    return;
  }
  nframes = jack_get_buffer_size(client);
  jack_set_buffer_size_callback(client, jack_buffer_size, 0);
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
  SETCALC(JackMIDIIn_next); 
}


void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples)
{
  int numOutputs = unit->mNumOutputs;
  //std::cout << "next" << std::endl;

  jack_nframes_t jack_frame_time = jack_last_frame_time(client);

  void* port_buf;
  jack_nframes_t offset;
  jack_nframes_t i;
  jack_nframes_t n;
 
  if (unit->jack_frame_time != jack_frame_time) {
    port_buf = jack_port_get_buffer( port, nframes);
  
    //std::cout << "new frame time " << jack_frame_time << std::endl;
 
    jack_midi_event_t event;
    i = 0;
    n = jack_midi_get_event_count(port_buf);
    offset = 0;
 
  } else {
    port_buf = unit->port_buf;
    i = unit->i;
    n = unit->n;
    offset = unit->offset;
  }

  //std::cout << "cycle" << std::endl;
  
  jack_midi_event_t event;
  jack_nframes_t time; 
  while (i < n) {
    jack_midi_event_get(&event, port_buf, i);
    
    time = event.time - offset;

    if (time >= FULLBUFLENGTH) {
      break;
    }
    
    //std::cout << "event " << i << " " << n << " " << event.time << " " << offset << " " << time << " " << jack_frame_time << " " << FULLBUFLENGTH << std::endl;
  
    OUT(0)[time] = (float)event.buffer[0];

    i++;
  }
 
  offset += FULLBUFLENGTH;
  
  unit->jack_frame_time = jack_frame_time;
  unit->offset = offset;
  unit->i = i;
  unit->n = n;
  unit->port_buf = port_buf;

}

