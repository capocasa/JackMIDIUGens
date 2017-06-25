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


PluginLoad(JackMIDIIn)
{
    ft = inTable;
    DefineSimpleUnit(JackMIDIIn);
}

