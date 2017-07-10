
JackMIDIUGens
=============

Server-side sample-accurate MIDI for SuperCollider via Jack.

Build
-----

Clone sc3-plugins 

    cd $MY_SOURCE_DIR
    git clone https://github.com/supercollider/sc3-plugins

Clone JackMIDIUGens:

    cd $MY_SOURCE_DIR
    git clone https://github.com/carlocapocasa/jackmidiugens.git JackMIDIUGens.git

Place JackMIDIUGens into your sc3-plugins without confusing git

    cd $MY_SOURCE_DIR/sc3-plugins/source
    ln -s ../../JackMIDIUGens

Include in sc3-plugins build script

Edit `$MY_SOURCE_DIR/sc3-plugins/source/CMakeLists.txt` and insert a line `JackMIDIUGens` right before `JoshUGens`

Build

    mkdir $MY_SOURCE_DIR/sc3-plugins/build
    cd $MY_SOURCE_DIR/sc3-plugins/build
    
    # adapt to your system with the sc3-plugins readme
    cmake .. -DSUPERNOVA=on 
    make JackMIDIUGens JackMIDIUgens_supernova

Install

    sudo ln -s $MY_SOURCE_DIR/sc3-plugins/build/source/JackMIDIUGens.so /usr/lib/SuperCollider/plugins
    sudo ln -s $MY_SOURCE_DIR/sc3-plugins/build/source/JackMIDIUGens_supernova.so /usr/lib/SuperCollider/plugins 

Update
------

    cd $MY_SOURCE_DIR/JackMIDIUGens/build
    git pull
    make JackMIDIUGens JackMIDIUgens_supernova

    # restart server

Usage
-----

Please see the help file sc/help/JackMIDIIn.sc for instructions and examples

License
-------
Copyright (c) 2017 Carlo Capocasa. Licensed under the GNU General Public License 2.0 or later.

