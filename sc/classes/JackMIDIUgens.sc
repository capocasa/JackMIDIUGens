JackMIDIIn : MultiOutUGen {

  *ar {
    arg polyphony = 5, channels, controls, polytouch = true;
    ^this.jackMIDINew('audio', polyphony, channels, controls, polytouch);
  }
  *kr {
    arg polyphony = 5, channels, controls, polytouch = true;
    ^this.jackMIDINew('control', polyphony, channels, controls, polytouch);
  }

  *jackMIDINew {
    arg rate, polyphony = 5, channels, controls, polytouch = true;
    var out, note, channel_controls;
    polytouch = polytouch.asInt;
    out = this.multiNew(rate, polyphony, channels, controls, polytouch);
    note = out[..(2+polytouch*polyphony-1)].reshape(polyphony, 2+polytouch);
    if (controls.value.size>0) {
       channel_controls = out[(2+polytouch*polyphony)..];
      ^[note, channel_controls];
    };
    ^note;
  }


  init { arg polyphony, channels, controls, polytouch;
    controls = controls.value.collect { |control|
      switch (control,
        \bend,        14,
        //\cc,          11,
        \touch,       13,
        \polytouch,   10,
        control
      );
    };
    channels = channels.value.asArray;
    inputs = [polyphony, channels.size,controls.size, polytouch] ++ channels ++ controls;
    ^this.initOutputs(2+polytouch*polyphony+controls.size, rate);
  }

}

