JackMIDIIn : MultiOutUGen {

  *ar {
    arg polyphony = 5, channels, controls, polytouch = true, notes;
    ^this.jackMIDINew('audio', polyphony, channels, controls, polytouch, notes);
  }
  *kr {
    arg polyphony = 5, channels, controls, polytouch = true, notes;
    ^this.jackMIDINew('control', polyphony, channels, controls, polytouch, notes);
  }

  *jackMIDINew {
    arg rate, polyphony = 5, channels, controls, polytouch = true, notes;
    var out, note, channel_controls;
    polytouch = polytouch.asInt;
    out = this.multiNew(rate, polyphony, channels, controls, polytouch, notes);
    if (polyphony == 1) {
      ^out;
    };
    note = out[..(2+polytouch*polyphony-1)].reshape(polyphony, 2+polytouch);
    if (controls.value.size>0) {
      channel_controls = out[(2+polytouch*polyphony)..];
      if (polyphony == 0) {
        ^channel_controls;
      };
      ^[note, channel_controls];
    };
    ^note;
  }


  init { arg polyphony, channels, controls, polytouch, notes;
    controls = controls.value.collect { |control|
      // arbitrarily encode controller names as midi type + 1000 to avoid collisions
      switch (control,
        \bend,        1014,
        //\cc,        1011,
        \touch,       1013,
        //\polytouch,   1010,
        control
      );
    };
    channels = channels.value.asArray;
    notes = notes.value.asArray;
    inputs = [polyphony, channels.size,controls.size, polytouch, notes.size] ++ channels ++ controls ++ notes;
    ^this.initOutputs(2+polytouch*polyphony+controls.size, rate);
  }

}

