JackMIDIIn : MultiOutUGen {

  *ar {
    arg polyphony = 5, controls = [], polytouch = true;
    var out;
    polytouch = polytouch.asInt;
    out = this.multiNew('audio', polyphony, controls, polytouch);
    ^(out[..(polyphony*2-1)].reshape(polyphony, 2+polytouch) ++ [out[(2+polytouch*polyphony)..]]);
  }
  *kr {
    ^this.multiNew('control');
  }

  init { arg polyphony, controls, polytouch;
    controls = controls.value.collect { |control|
      switch (control,
        \bend,        14,
        //\cc,          11,
        \touch,       13,
        \polytouch,   10,
        control
      );
    };
    inputs = [polyphony, polytouch, controls.size] ++ controls;
    ^this.initOutputs(2+polytouch*polyphony+controls.size, rate);
  }

}

