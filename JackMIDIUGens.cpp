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
  uint32                      num_controllers;
  uint32                      controllers[256];
  uint32                      ob[256];
  uint32                      polyphony;
  bool                        polytouch;
  uint32                      num_chan;
  uint32                      chan[16];
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
  uint32 num_chan = IN0(1);
  unit->num_chan = num_chan;
  uint32 num_controllers = IN0(2);
  unit->num_controllers = num_controllers;
  unit->polytouch = IN0(3);
  for (uint32 i = 0; i < 256; i++) {
    unit->ob[i] = 0;
  }
  for (uint32 i = 0; i < num_chan; i++) {
    unit->chan[i] = IN0(4+i);
    std::cout << unit->chan[i] << " ";
  }
  std::cout << "\n";
  for (uint32 i = 0; i < num_controllers; i++) {
    unit->controllers[i] = IN0(4+num_chan+i);
    std::cout << unit->controllers[i] << " ";
  }
  std::cout << "\n";
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
    
    //std::cout << "numOutputs " << numOutputs << std::endl;
  
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
    
  uint32* ob = unit->ob;

  uint32 polyphony = unit->polyphony;
  uint32 num_controllers = unit->num_controllers;
  uint32* controllers = unit->controllers;

  uint32 polytouch = unit->polytouch;
  uint32 width = 2 + polytouch;
  uint32 fullwidth = width * polyphony;

  uint32* obc = ob + fullwidth;

  uint32* chan = unit->chan;
  uint32 num_chan = unit->num_chan;

  while (i < n) {
    jack_midi_event_get(&event, jack_midi_port_in_buffer, i);
    
    time = event.time - offset;

    if (time >= FULLBUFLENGTH) {
      break;
    }
    
    //std::cout << "event i " << i << " n " << n << " time " << event.time << " offset " << offset << " buffer " << (int)event.buffer[0] << " " << (int)event.buffer[1] << " " << (int)event.buffer[2] << " jack_frame_time " << jack_frame_time << " FULLBUFLENGTH " << FULLBUFLENGTH << std::endl;
    
    uint32 typechan = event.buffer[0];
    uint32 note = event.buffer[1];
    uint32 value = event.buffer[2];

    uint32 type = (int) typechan / 16;
    uint32 channel = typechan % 16;

    //std::cout << "type " << type << "channel " << channel << "\n";

    int ch;
    if (num_chan) {
      for (ch = 0; ch < num_chan; ch++) {
        if (chan[ch] == channel) {
          break;
        }
      }
      if (ch == num_chan) {
        // ugen not configured for this channel, skip event
        i++;
        continue;
      }
    }

    for (jack_nframes_t j = last_time; j < time; j++) {
      for (int k = 0; k < numOutputs; k++) {
        OUT(k)[j] = (float)ob[k];
      }
    }

    uint32 oo;
    switch(type) {
    
    // noteon 
    case 9:

      // find empty output 
      for (oo = 0; oo < fullwidth; oo += width) {
        if (ob[oo] == 0) {
          break;
        }
      }
      if (oo < numOutputs) {
        ob[oo] = note;
        ob[oo+1] = value;
      } else {
        // potentially warn
      }
      break;
    
    // noteff 
    case 8:
      
      // find playing note
      for (oo = 0; oo < fullwidth; oo += width) {
        if (ob[oo] == note) {
          break;
        }
      }
      if (oo < numOutputs) {
        ob[oo] = 0;
        ob[oo+1] = 0;
        if (polytouch) {
          ob[oo+2] = 0;
        }
      }  else {
        // potentially warn
      }
      break;
    
    // pitch bend
    case 14:
      
      //std::cout << "bend " << note << " " << value << std::endl;
      
      for (int j = 0; j < num_controllers; j++) {
        if (controllers[j] == 14) {
          obc[j] = (float)(note + 128*value);
        }
      }
      
      break;

    // controller
    case 11:
     
      //std::cout << "controller " << note << " " << value << std::endl;

      for (int j = 0; j < num_controllers; j++) {
        if (controllers[j] == note) {
          obc[j] = (float)value;
        }
      }

      break;
    
    // polytouch
    case 10:
      
      //std::cout << "polytouch " << note << " " << value << std::endl;
      if (polytouch) {
        // find playing note
        for (oo = 0; oo < fullwidth; oo += width) {
          if (ob[oo] == note) {
            break;
          }
        }
        if (oo < numOutputs) {
          ob[oo+2] = value;
        }  else {
          // potentially warn
        }
      }

      break;
    
    // touch
    case 13:
      
      //std::cout << "touch " << note << " " << value << std::endl;
      
      for (int j = 0; j < num_controllers; j++) {
        if (controllers[j] == 13) {
          obc[j] = (float)note;
        }
      }

      break;
    
    
    }
   

    i++;
  
    last_time = time;
  }

  for(jack_nframes_t j = last_time; j < FULLBUFLENGTH; j++) {
    for (int k = 0; k < numOutputs; k++) {
      OUT(k)[j] = (float)ob[k];
    }
  }

  offset += FULLBUFLENGTH;
  
  unit->jack_frame_time = jack_frame_time;
  unit->offset = offset;
  unit->i = i;
  unit->n = n;
  unit->jack_midi_port_in_buffer = jack_midi_port_in_buffer;

}

