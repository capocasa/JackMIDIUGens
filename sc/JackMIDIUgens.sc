JackMIDIIn : MultiOutUGen {

  *ar {
    arg polyphony = 5, controls = [];
    var out = this.multiNew('audio', polyphony, controls);
    ^(out[..(polyphony*2-1)].reshape(polyphony, 2) ++ [out[(polyphony*2)..]]);
  }
  *kr {
    ^this.multiNew('control');
  }

  init { arg polyphony, controls;
    controls = controls.value.collect { |control|
      switch (control,
        \bend,        224,
        //\cc,          176,
        \touch,       208,
        \polytouch,   160,
        control
      );
    };
    inputs = [polyphony, controls.size] ++ controls;
    ^this.initOutputs(2*polyphony+controls.size, rate);
  }

}

