#include "SC_PlugIn.h"
#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>

static InterfaceTable *ft;


struct JackMIDIIn: public Unit
{
  uint32              event_count;
};

static void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples);
static void JackMIDIIn_Ctor(JackMIDIIn* unit);

jack_client_t*  client;
jack_port_t*    inputPort;
jack_port_t*    outputPort;

int process(jack_nframes_t nframes, void *arg) {
  jack_midi_event_t event;
  void* port_buf = jack_port_get_buffer( inputPort, nframes);
  JackMIDIIn *unit = (JackMIDIIn*) arg;
  jack_nframes_t event_count = jack_midi_get_event_count(port_buf);

  if (event_count > 0) {
    jack_midi_event_get(&event, port_buf, 0);
    
    for (int i = 0; i < unit->event_count; i++) {
      jack_midi_event_get(&event, port_buf, i);
      //std::cout << "process " << i << " " << event.time << std::endl;
    }
  }
  std::cout << "process" << std::endl;

  return 0;
}

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

  inputPort  = jack_port_register (client, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  jack_set_process_callback (client, process, unit);
  if (jack_activate(client) != 0)
  {
    std::cout<<  "JackMIDIIn: cannot activate jack client" << std::endl;
    return;
  }

  // ar ctor 48000   2.08333e-05   64   750    0.00133333    48000    64 
  // kr ctor   750   0.00133333     1   750    0.00133333    48000    64 
  //std::cout << "ctor " << SAMPLERATE << " " << SAMPLEDUR << " " << BUFLENGTH << " " << BUFRATE << " " << BUFDUR << " " << FULLRATE << " " << FULLBUFLENGTH << " " << std::endl;   

  unit->event_count = 0;
  
  SETCALC(JackMIDIIn_next);
  JackMIDIIn_next(unit, 1);
}

void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples)
{
  unit->event_count++;

  std::cout << "next " << unit->event_count << std::endl;
  int numOutputs = unit->mNumOutputs;
  for (int i = 0; i < inNumSamples; i++) {
    for (int j = 0; j < numOutputs; j++) {
      OUT(j)[i] = 0;
    }
  }
}

