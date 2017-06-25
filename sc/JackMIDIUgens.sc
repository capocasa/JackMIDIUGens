JackMIDIIn : UGen {
  *ar {
    ^this.multiNew('audio');
  }
  *kr {
    ^this.multiNew('control');
  }
}

