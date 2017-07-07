class:: JackMIDIIn
summary:: Server side Jack MIDI input class
categories::  UGens>IO


Description::
Uses Jack MIDI to provide a server side polyphonic sample accurate MIDI stream in audio or control rates.

classmethods::

method:: kr
method:: ar

argument::polyphony

The maximum number of simultaneous notes. Each note gets its own output, to be connected to its own UGens.

argument::chan
The MIDI channel or channels to respond to. Use an integer to provide a single channel, an array to provide several channels, or nil to respond to all channels.

::

argument::controls
An array of channel control values to output, in addition to notes.

\bend
\touch
integer values (mapped to the corresponding cc value)

argument::polytouch

Whether to output polytouch values for each note.

returns:

An array with two items, the first for the notes, velocities and (optionally) polytouch, the second for the per-channel control values.

With no polytouch or channel controls and the default polyphony of 5, the output is:

code::[[[note,velocity],[note,velocity],[note,velocity],[note,velocity],[note,velocity]],[]]

With a polyphony of 3, enabled polytouch and cc1 and bend controls specified, the output is

code::[[[note,velocity,polytouch],[note,velocity,polytouch],[note,velocity,polytouch]],[cc1,velocity]]


Examples::

// Simple sine
(
  JackMIDIIn.ar.collect {|c|
    var note,vel;
    #note,vel=c;
    SinOsc.ar(note.midicps,0,(vel/128).lag);
  }.sum;
)

// Polytouch pulse
(
  JackMIDIIn.ar(polytouch:true).collect {|c|
    var note,vel,touch;
    #note,vel,touch=c;
    Pulse.ar(note.midicps,(touch/128).lag,(vel/128).lag);
  }.sum;
)

// Channel 1, with pitch bend and a control interface fader mapped to channel 1
(
  var notes,controls;
  #notes,controls = JackMIDIIn.ar(5,1,[\bend,\cc1]).collect {|c|
    var note,vel,bend,touch;
    #note,vel=c;
    #bend,touch=controls;
    Pulse.ar(note.midicps*(bend/16384)+0.5),(touch/128).lag,(vel/128).lag);
  }.sum;
)

discussion::

JackMIDIUGens implement MIDI connectivity on the server side, because a different set of trade-offs was desired than was previously possible.

Client side MIDI is more flexible and can start and stop synths, and works well enough in practice, but offers no realtime guarantees, which can be a problem for more percussive instruments- and nonpercussive instruments as well, when one considers that the hardware interface, system MIDI, OSC sending and receiving as well as scclient code itself each can introduce unpredictable amouns of jitter and latency. With server side MIDI, using Jack as an interface, MIDI timing can theoretically be the same as the audio rate.

With client side MIDI, a new synth is commonly generated from a synthdef for each noteOn message, saving resources when the synth is not playing. With client side MIDI, synths are kept running even when they are not played. This trades some loss of efficiency but introduces reliability. When playing a complex piece, the CPU cycles are sure to be sufficient no matter how much input is produced in the heat of the moment, because they are already being used.

An existing drawback is complexity of the signal paths, which is handled fairly well by SuperCollider's excellent looping.

It is recommended, when on Linux, to use Jack MIDI combined with jamrouter, which handles MIDI timing processing.

code:: jamrouter -M generic -D /dev/midi1


