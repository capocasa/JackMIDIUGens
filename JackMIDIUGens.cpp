#include "SC_PlugIn.h"
#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <thread>

static InterfaceTable *ft;


struct JackMIDIIn: public Unit
{
  uint32 jack_frame;
  void* port_buf;
  jack_nframes_t              nframes;
  jack_nframes_t              next_event_time;
  jack_midi_event_t           next_event;
  jack_nframes_t              time_n;
  jack_nframes_t              count;
  jack_nframes_t              event_i;
  jack_midi_event_t           event;
  jack_transport_state_t      transport;
  jack_position_t             position;
  jack_client_t*              client;
  jack_port_t*                port;
};

static void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples);
static void JackMIDIIn_Ctor(JackMIDIIn* unit);

int process(jack_nframes_t nframes, void *arg) {
  /*
  jack_midi_event_t event;
  jack_nframes_t event_i = 0;
  jack_position_t         position;

  void* port_buf = jack_port_get_buffer( port, nframes);

  JackMIDIIn *unit = (JackMIDIIn*) arg;
  
  unit->port_buf = port_buf;
  unit->jack_frame = 0;
  unit->count = jack_midi_get_count(port_buf);
  unit->transport = jack_transport_query( client, &position );

  if (unit->count > 0) {
    jack_midi_event_get(&event, port_buf, 0);
    unit->event = event;
    unit->event_i = 0;
  
for (int i = 0; i < unit->count; i++) { jack_midi_event_get(&event, port_buf, i); std::cout << "process " << i << " " << event.time << std::endl;  }
  
  }
  unit->event_i = 0;
  unit->time_n = 0;
*/
  return 0;
}

PluginLoad(JackMIDIIn)
{
  ft = inTable;
  DefineSimpleUnit(JackMIDIIn);
}

jack_client_t* client = NULL; 
jack_port_t* port = NULL;
jack_nframes_t nframes = 0;

int jack_buffer_size(jack_nframes_t nframes_new, void *arg) {
  nframes = nframes_new;
}

void jack_init() {
  if ((client = jack_client_open("SuperCollider JackMIDI", JackNullOption, NULL)) == 0)
  {
    std::cout << "JackMIDIIn: cannot connect to jack server" << std::endl;
  }
  port = jack_port_register (client, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  //jack_set_process_callback (client, process, unit);
  if (jack_activate(client) != 0)
  {
    std::cout<<  "JackMIDIIn: cannot activate jack client" << std::endl;
    return;
  }
  nframes = jack_get_buffer_size(client);
  jack_set_buffer_size_callback(client, jack_buffer_size, 0);
}

static void con() __attribute__((constructor));
void con() {
  std::thread jack_init_thread(jack_init);
  jack_init_thread.join();
}


void JackMIDIIn_Ctor(JackMIDIIn* unit)
{

  //unit->client = unit->mWorld->hw->mAudioDriver;
  //uint32 mw = unit->mWorld->hw->mMaxWireBufs;
  

  /*
  
  
  jack_midi_event_t event;
  void* port_buf = jack_port_get_buffer( port, 2048);
  jack_midi_event_get(&event, port_buf, 0);

  unit->client = client;
  unit->port = port;
  
  unit->event_i = 0;
  unit->time_n = 0;

  unit->event = event;
  // ar ctor 48000   2.08333e-05   64   750    0.00133333    48000    64 
  // kr ctor   750   0.00133333     1   750    0.00133333    48000    64 
  //std::cout << "ctor " << SAMPLERATE << " " << SAMPLEDUR << " " << BUFLENGTH << " " << BUFRATE << " " << BUFDUR << " " << FULLRATE << " " << FULLBUFLENGTH << " " << std::endl;   
  
  
  SETCALC(JackMIDIIn_next);
  JackMIDIIn_next(unit, 1);
}

void JackMIDIIn_next(JackMIDIIn *unit, int inNumSamples)
{
  int numOutputs = unit->mNumOutputs;
  //std::cout << "next" << std::endl;

  jack_client_t* client = unit->client;
  jack_port_t* port = unit->port;
  void* port_buf = jack_port_get_buffer( port, 2048);

  jack_nframes_t count = unit->count;
  jack_nframes_t event_i = unit->event_i;
  jack_midi_event_t event = unit->event;
  jack_nframes_t time = event.time;
  jack_nframes_t time_n = unit->time_n;
  jack_nframes_t time_0 = time_n;
  time_n += FULLBUFLENGTH;

  if (event_i == 0) {
    //std::cout << "recycle" << std::endl;
    count = jack_midi_get_event_count(port_buf);
    if (count > 0) {
      jack_midi_event_get(&event, port_buf, 0);
      time = event.time;
    } else {
      time = 0;
    }
  }
  
  for (int i = 0; i < inNumSamples; i++) {
    OUT(0)[i] = 0;
  }

  while (event_i < count && time < time_n) {
    //std::cout << "nextt " << event_i << " " << time_n << " " << event.time << std::endl;
    
    std::cout << "nex " << time << " " << time_0 << " " << (time - time_0) << " " << time_n << " " << event_i << " " << count << std::endl;

    //OUT(0)[event_i - time_0] = event.buffer[0];
    //OUT(0)[time - time_0] = 0.5;
    
    event_i++;
    if (event_i >= count) {
      jack_midi_event_get(&event, port_buf, event_i);
      time = event.time;
    }
  }


  if (time_n >= 2048) {
    time_n = 0;
    event_i = 0;
  }

/*
  while (event_i < count && event_i < time_n) {
    //std::cout << "Frame " << position.frame << "  Event: " << event_i << " SubFrame#: " << event.time << " \tMessage:\t"
    //          << (long)event.buffer[0] << "\t" << (long)event.buffer[1]
    //          << "\t" << (long)event.buffer[2] << std::endl;
    std::cout << "next " << event_i << " " << event.time << std::endl;
    event_i++;
    jack_midi_event_get(&event, port_buf, event_i);
  }

  unit->event=event;
  unit->event_i = event_i;
*/

  unit->event_i = event_i;
  unit->time_n = time_n;
  unit->count = count;
  unit->event = event;
}

