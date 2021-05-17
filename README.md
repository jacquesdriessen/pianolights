# Piano MIDI Lights
Networked MIDI Piano Lights

### The Idea
- Pretty simple. Use your IOS device with a MIDI app, stream that over the network to strip of lights that will guide you through what keys to press to "play that piece of music".

### Not for the faint of heart.
- A project I did a long time ago, and requires some electronics and programming skills, would have been really neat to market this but since it's been lying on the shelf for the past 5 years decided to publish as is.
- Also (probably) requires (networked) MIDI / (some) music knowledge and a bit of perseverance.
- However, some of my family members managed to succesfully use this whilst getting started with learning how to play the piano.
- Last but not least. Not intended to replace piano lessons, it will teach you all the wrong things but's a quick way to learn how much fun it can be. In short - tested this with myself and a family member(both of us had piano lessons for years) and more than anything else confused us. For the kids it was amazing way to quickly learn how to play certain music they liked without being proficient enought to read sheet music that difficult.
- Although Piano Keys I learned through this don't have a fully standardizes size, they usually are pretty similar and a standard 144 / meter strip by sheer coincidence seemed to fit (2 leds / key) plus would fit right between where the piano keys "disappear" into the piano. This may or may not work for your particular set up.

### Instructions

- You will need an ESP8266 (ESP-01 will do) and a 144 leds / meter led strip WS2812 (enough for 72 keys).
- You will need an old version of the NeoPixelBus Library (the ones that the Arduino IDE offers are too different / new, I made this back in 2016). Grab it here (https://github.com/Makuna/NeoPixelBus/tree/cad11b9dc7aa8bdb82f62e51976dcdb2c69b8501)
- The current AppleMidi library also won't work, use this older one instead https://github.com/ftrias/Arduino-AppleMidi-Library
- In the arduino IDE / find the esp8266_mdns library (mrdunk)
- Build the electronics, the lights need ground / 5V and quite a bit of amps (you might want a fuse just in case and make sure your 5V power supply can provide it), the data line needs to be connected to a free ESP port (see below), ESP needs 3.3V (e.g. you will want a 5V -> 3.3V buck converter I presume)
- Update the pin for the "data line", I used pin 2, but any other pin would work as well
`NeoPixelBus strip = NeoPixelBus(pixelCount, 2, NEO_GRB);`
- It will start up with an access point called "piano", connect to it and configure your wifi parameters. Strip will use colours to guide you through (starts green, whilst connecting it will one by one make them blue, red is failed connection, that's where you have the access point, and once connected it's rainbow)
- The difficult bit is getting it to work. It might be possible to do this on non Apple software but that wasn't tested. Also be aware I cannot guarantee this will work for your setup and the software I used costs money - I recommend trying to find something that doesn't cost money first before you decide to purchase anything.
- IOS, I happened to own an (awesome) app called Piano 3D by Massive Technologies Inc. which supports networked MIDI.
- MACOS, I happened to own logic pro (if I remember correctedly garage band didn't support this), you can use the Audio MIDI Setup to connect to the ESP.
- Anyway, the moment the connection is made - the strip goes dark and then you can send midi data to it.
- Not 100% sure anymore where the middle C is, video I think was for an earlier version
- `#define MIDDLE_C 60` in the code defines the middle C, change as you desire (the video has a different setting and you probably want to have it more around the middle of the strip e.g. 36), this setting was so it would allow you to start the strip on the "far left of the piano", which is more convenient.

### Youtube Demo:

[![](https://i9.ytimg.com/vi/O6-8lZxrovw/mq1.jpg?sqp=CMC1iYUG&rs=AOn4CLDmJ3HO2YKeyzMRsXPxY2hD2UDLTw)](https://www.youtube.com/watch?v=O6-8lZxrovw)
