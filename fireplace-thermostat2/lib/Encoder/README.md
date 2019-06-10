# Encoder Library

Encoder counts pulses from quadrature encoded signals, which are commonly available from rotary knobs, motor or shaft sensors and other position sensors. 

http://www.pjrc.com/teensy/td_libs_Encoder.html

http://www.youtube.com/watch?v=2puhIong-cs

![Encoder Knobs Demo](http://www.pjrc.com/teensy/td_libs_Encoder_1.jpg)

## Particle port

This port of Paul's encoder library uses interrupts on the Particle Photon, Electron and Core.

Carefully read the [Interrupts section of the firmware
docs](https://docs.particle.io/reference/firmware/#interrupts) before
selecting which pins for the encoder signal. **In particular, do not use
D0 or A5 on the Photon as an encoder input.**

## License

Copyright (c) 2011,2013 PJRC.COM, LLC - Paul Stoffregen <paul@pjrc.com>

Modifications for the Particle platform, copyright (c) 2017 Julien Vanier <jvanier@gmail.com>

Released under the MIT license
