JackMIDIIn : MultiOutUGen {
  *ar {
    ^this.multiNew('audio', 2);
  }
  *kr {
    ^this.multiNew('control');
  }

  init { arg argNumChannels ... theInputs;
    inputs = theInputs;
    ^this.initOutputs(argNumChannels, rate);
  }
  argNamesInputsOffset { ^2 }

}

