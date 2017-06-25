#include "SC_PlugIn.h"
#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>

static InterfaceTable *ft;


struct JackMIDIIn: public Unit
{
  void*                       port_buf;
  jack_nframes_t              next_event_time;
  jack_midi_event_t           next_event;
  jack_nframes_t              event_count;
  jack_nframes_t              event_index;
  jack_midi_event_t           event;
  jack_transport_state_t      transport;
  jack_position_t             position;
  jack_nframes_t              max_event_index;
  jack_port_t*                port;
  bool                        trigger;
  jack_nframes_t              nframes;
};

static void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples);
static void JackMIDIIn_Ctor(JackMIDIIn* unit);

int process(jack_nframes_t nframes, void *arg) {
  //std::cout << "process" << std::endl;
  JackMIDIIn *unit = (JackMIDIIn*) arg;
  unit->nframes = nframes;
  unit->trigger = true;
  return 0;
}

jack_client_t*             client;

PluginLoad(JackMIDIIn)
{
  ft = inTable;
  DefineSimpleUnit(JackMIDIIn);
  if ((client = jack_client_open("SuperCollider JackMIDI", JackNullOption, NULL)) == 0)
  {
    std::cout << "JackMIDIIn: cannot connect to jack server" << std::endl;
  }
}

void JackMIDIIn_Ctor(JackMIDIIn* unit)
{
  jack_set_process_callback(client, process, unit);
  unit->port                = jack_port_register(client, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  if (jack_activate(client) != 0)
  {
    std::cout<<  "JackMIDIIn: cannot activate jack client" << std::endl;
    return;
  }
  jack_position_t position;
  unit->transport           = jack_transport_query( client, &position );
  unit->position            = position;
  unit->port_buf            = jack_port_get_buffer(unit->port, unit->nframes);
  unit->trigger             = true;

  SETCALC(JackMIDIIn_next);
  JackMIDIIn_next(unit, 1);
}

void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples)
{
  int numOutputs = unit->mNumOutputs;
  //std::cout << "next" << std::endl;

  jack_nframes_t           event_count       = unit->event_count;
  jack_nframes_t           event_index       = unit->event_index;
  jack_nframes_t           max_event_index   = unit->max_event_index;
  jack_midi_event_t        event             = unit->event;
  jack_position_t          position          = unit->position;
  
  if (unit->trigger) {
    jack_nframes_t             event_index = 0;
    jack_port_t*               port = unit->port;

    event_count = jack_midi_get_event_count(port);

    if (event_count > 0) {
      jack_midi_event_get(&event, port, 0);
      unit->event = event;
      unit->event_index = 1;
    } else {
      unit->event_index = 0;
    }
    unit->max_event_index = 0;
  }
  
  max_event_index += inNumSamples;
 
  while (event_index < event_count && event_index < max_event_index) {
    std::cout << "Frame " << position.frame << "  Event: " << event_index << " SubFrame#: " << event.time << " \tMessage:\t"
              << (long)event.buffer[0] << "\t" << (long)event.buffer[1]
              << "\t" << (long)event.buffer[2] << std::endl;
    event_index++;
    jack_midi_event_get(&event, unit->port_buf, event_index);
  }

  unit->event=event;
  unit->event_index = event_index;
  unit->max_event_index = max_event_index;
  for (int i = 0; i < inNumSamples; i++) {
    for (int j = 0; j < numOutputs; j++) {
      OUT(j)[i] = 0;
    }
  }
}

