#include "SC_PlugIn.h"

static InterfaceTable *ft;


struct JackMIDIIn: public Unit
{
};

static void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples);
static void JackMIDIIn_Ctor(JackMIDIIn* unit);

void JackMIDIIn_Ctor(JackMIDIIn* unit)
{
  SETCALC(JackMIDIIn_next);
  JackMIDIIn_next(unit, 1);
}

void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples)
{
  int numOutputs = unit->mNumOutputs;

  for (int i = 0; i < inNumSamples; i++) {
    for (int j = 0; j < numOutputs; j++) {
      OUT(j)[i] = 0;
    }
  }
}


#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>

jack_client_t*  client;
jack_port_t*    inputPort;
jack_port_t*    outputPort;

int process(jack_nframes_t nframes, void *arg) {
  jack_midi_event_t in_event;
  jack_nframes_t event_index = 0;
  jack_position_t         position;
  jack_transport_state_t  transport;

  // get the port data
  void* port_buf = jack_port_get_buffer( inputPort, nframes);


  // get the transport state of the JACK server
  transport = jack_transport_query( client, &position );

  // input: get number of events, and process them.
  jack_nframes_t event_count = jack_midi_get_event_count(port_buf);
  if(event_count > 0)
  {
    for(int i=0; i<event_count; i++)
    {
      jack_midi_event_get(&in_event, port_buf, i);

      // Using "cout" in the JACK process() callback is NOT realtime, this is
      // used here for simplicity.
      std::cout << "Frame " << position.frame << "  Event: " << i << " SubFrame#: " << in_event.time << " \tMessage:\t"
                << (long)in_event.buffer[0] << "\t" << (long)in_event.buffer[1]
                << "\t" << (long)in_event.buffer[2] << std::endl;
    }
  }

  return 0;
}

PluginLoad(JackMIDIIn)
{
  ft = inTable;
  DefineSimpleUnit(JackMIDIIn);
g
  if ((client = jack_client_open("PrintMidi", JackNullOption, NULL)) == 0)
  {g
    std::cout << "jack server not running?" << std::endl;
  }
  inputPort  = jack_port_register (client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

  jack_set_process_callback (client, process, 0);

}

