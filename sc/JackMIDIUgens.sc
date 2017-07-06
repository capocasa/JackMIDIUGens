JackMIDIIn : MultiOutUGen {
  
  var
    polyphony
  ;
  
  *ar {
    arg polyphony = 5;
    ^this.multiNew('audio', polyphony).reshape(polyphony, 2);
  }
  *kr {
    ^this.multiNew('control');
  }

  init { arg argPolyphony ... theInputs;
    polyphony = argPolyphony;
    inputs = theInputs;
    ^this.initOutputs(2*polyphony, rate);
  }
  argNamesInputsOffset { ^2 }

}

