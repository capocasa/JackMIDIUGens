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
  return 0;
}

PluginLoad(JackMIDIIn)
{
  ft = inTable;
  DefineSimpleUnit(JackMIDIIn);

  jack_set_process_callback (client, process, 0);

}

