class:: JackMIDIIn
summary:: Server side Jack MIDI input class
categories::  UGens>IO


Description::
Uses Jack MIDI to provide a server side polyphonic sample accurate MIDI stream in audio or control rates.

classmethods::

method:: kr
method:: ar

argument::polyphony

The maximum number of simultaneous notes.

argument::chan
The MIDI channel or channels to respond to. Use an integer to provide a single channel, an array to provide several channels, or nil to respond to all channels.

::

argument::controls
An array of channel control values to be output, in addition to notes.

\bend
\touch
integer values (mapped to the corresponding cc value)

argument::polytouch

Whether to output polytouch values for each note. Default true.

returns:

An array of multichannel outputs of the size polyphony. The first output is the note value, the second is the velocity, and an optional third is the polytouch value. They are output as long as the note is held and can be used as gates. The first unused output is used for the next note.

code::[[note,velocity],[note,velocity],[note,velocity],[note,velocity],[note,velocity]]

If controls are defined, the note array is added to an additional array as the first item. The second item is an array of the control outputs.

code::[[[note,velocity],[note,velocity],[note,velocity],[note,velocity],[note,velocity]],[control1,control2]]

Those are a lot of arrays, but they are fairly easy to use thanks to the SuperCollider language's capabilities.

If the polyphony is 0, only the controllers are output as a single array.

code::[control1,control2]

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

// Channel 1, with pitch bend and a control interface fader mapped to control 1
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

// use two control interface faders to sweep and q a filter without notes
(
  {
    var cc1, cc11;
    #cc1, cc11 = JackMIDIIn.ar(0,nil,`[1,11]);
    RLPF.ar(Saw.ar(55),(cc1/128).lag*4500,(cc11/128).lag);
  }.play
)


discussion::

Processing MIDI data on the server side is a more natural fit, because by nature MIDI is a realtime data stream like any other. While sacrificing the ability to create synths on the fly, it eliminates all sources of timing inaccuracy except the hardware interface itself. This brings SuperCollider MIDI accuracy on par with most DAWs available, and probably surpasses them.

By eliminating the need to program responders or keep track of parameters, it also provides a much more convenient interface for synth programming, helping with the overall goal of letting users focus on their creativity.

On linux, a hardware interfacing program is required. It is highly recommended to use jamrouter for the best possible results.

code:: jamrouter -M generic -D /dev/midi1 -o JackMIDIUGens:midi_in


