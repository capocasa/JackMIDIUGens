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


examples::

// Simple sine
(
  {
    JackMIDIIn.ar.collect {|c|
      var note,vel;
      #note,vel=c;
      SinOsc.ar(note.midicps,0,(vel/128).lag);
    }.sum;
  }.play;
)

// Polytouch pulse
(
  {
    JackMIDIIn.ar(polytouch:true).collect {|c|
      var note,vel,touch;
      #note,vel,touch=c;
      Pulse.ar(note.midicps,(touch/128).lag,(vel/128).lag);
    }.sum;
  }.play;
)

// Channel 1, with pitch bend and a control interface fader mapped to channel 1
(
  {
    var notes,controls,bend,cc1;
    #notes,controls = JackMIDIIn.ar(5,nil,`[\bend,1], false);
    #bend,cc1=controls;
    notes.collect {|c|
      var note,vel;
      #note,vel=c;
      Pan2.ar(SinOsc.ar(note.midicps*(bend/8192).lag,0,(vel/128).lag),(cc1/64-1).lag);
    }.sum;
  }.play;
)


discussion::

Processing MIDI data on the server side is a more natural fit, because by nature MIDI is a realtime data stream like any other. While sacrificing the ability to create synths on the fly, it eliminates all sources of timing inaccuracy except the hardware interface itself. This brings SuperCollider MIDI accuracy on par with most DAWs available, and probably surpasses them.

By eliminating the need to program responders or keep track of parameters, it also provides a much more convenient interface for synth programming, helping with the overall goal of letting users focus on their creativity.

On linux, a hardware interfacing program is required. It is highly recommended to use jamrouter for the best possible results.

code:: jamrouter -M generic -D /dev/midi1 -o JackMIDIUGens:midi_in


