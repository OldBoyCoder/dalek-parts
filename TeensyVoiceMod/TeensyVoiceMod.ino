#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2sInput;           //xy=263.1999969482422,307.1999969482422
AudioAnalyzePeak         peakVoice;          //xy=465.1999969482422,174.1999969482422
AudioSynthWaveformSine   sineGenerate;          //xy=496.20001220703125,430.20001220703125
AudioEffectDelay         delayPreventFeedback;         //xy=497.1999969482422,296.1999969482422
AudioEffectFreeverb      freeverbVoice;      //xy=705.1999969482422,189.1999969482422
AudioPlayMemory          playGun;       //xy=737.2000122070312,388.20001220703125
AudioPlayMemory          playAmbient;       //xy=853.1999969482422,528.1999969482422
AudioEffectMultiply      multiplyCropToSine;      //xy=903.2000122070312,267.20001220703125
AudioAnalyzePeak         peakGun;          //xy=919.2000122070312,423.20001220703125
AudioMixer4              mixerEnd;         //xy=1125.199951171875,341.20001220703125
AudioOutputI2S           i2sOutput;           //xy=1338.199951171875,336.20001220703125
AudioConnection          patchCord1(i2sInput, 0, peakVoice, 0);
AudioConnection          patchCord2(i2sInput, 0, delayPreventFeedback, 0);
AudioConnection          patchCord3(sineGenerate, 0, multiplyCropToSine, 1);
AudioConnection          patchCord4(delayPreventFeedback, 0, freeverbVoice, 0);
AudioConnection          patchCord5(freeverbVoice, 0, multiplyCropToSine, 0);
AudioConnection          patchCord6(playGun, 0, mixerEnd, 1);
AudioConnection          patchCord7(playGun, peakGun);
AudioConnection          patchCord8(playAmbient, 0, mixerEnd, 2);
AudioConnection          patchCord9(multiplyCropToSine, 0, mixerEnd, 0);
AudioConnection          patchCord10(mixerEnd, 0, i2sOutput, 0);
AudioConnection          patchCord11(mixerEnd, 0, i2sOutput, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=200.1999969482422,104.19999694824219
// GUItool: end automatically generated code

#include <Bounce.h>       // Library used to DeBounce the trigger button
#include "GunSound.h"     // Include files contiaining the gun sound
#include "AudioSampleGenesis_ambient1.h"     // Include files contiaining the gun sound
#include "AudioSampleGenesis_ambient2.h"     // Include files contiaining the gun sound
#include "AudioSampleGenesis_ambient3.h"     // Include files contiaining the gun sound
#include "AudioSampleGenesis_ambient4.h"     // Include files contiaining the gun sound
#include "AudioSampleGenesis_ambient5.h"     // Include files contiaining the gun sound
#include "AudioSampleGenesis_ambient6.h"     // Include files contiaining the gun sound
#define LineLevel 31      // 13->31 = 3.16->1.16Vpp
#define MicGain 15        // 0->63 (dB)
#define VoiceVolume 1.0   // 0->1.0 Off to full volume
#define GunVolume 0.6     // 0->1.0, If gun and voice add to more than 1.0 clipping can occur
#define ModFrequency 30.0 // Modulation frequency
#define DelayLine 20      // Delay line length (msec)

// Setup pin 1 with a 250msec debounce for the gun trigger
Bounce button1 = Bounce(1,250);

elapsedMillis NoiseGateTime = 0;  // Holds the amount of time the gate has been open
boolean NoiseGateOpen = false;    // True when the gate is open
float NoiseGateLength = 125.0;    // Time to close the gate after voice is no longer detected (msec)

void setup() {
  pinMode(1,INPUT_PULLUP);        // Gun trigger is on pin 1
  pinMode(3,OUTPUT);              // Dome light output is on pin 3
  pinMode(4,OUTPUT);              // Gun effect output is on pin 4

  AudioMemory(128);               // Memory allocated to the Audio Library

  sgtl5000_1.enable();                     // Activate the I2S communication with the Audio Shield
  sgtl5000_1.lineOutLevel(LineLevel);      // Set the output Vpp level
  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC); // Configure for microphone input
  sgtl5000_1.micGain(MicGain);             // Set the microphone pre-amp gain

  // Setup the LFO (Low Frequency Oscillator)
  sineGenerate.amplitude(1.0);
  sineGenerate.frequency(ModFrequency);
  sineGenerate.phase(0.0);

  // Delay line helps prevent feedback
  delayPreventFeedback.delay(0,DelayLine);
  delayPreventFeedback.disable(1);
  delayPreventFeedback.disable(2);
  delayPreventFeedback.disable(3);
  delayPreventFeedback.disable(4);
  delayPreventFeedback.disable(5);
  delayPreventFeedback.disable(6);
  delayPreventFeedback.disable(7);

  // Mix the voice and gun sounds before the final data stream is sent back to the Audio Adapter
  mixerEnd.gain(0,VoiceVolume);
  mixerEnd.gain(1,GunVolume);
  mixerEnd.gain(2,0.0);
  mixerEnd.gain(3,0.0);
}


void loop() {
    
  // Check for a trigger event and play the sound if triggered
  button1.update();
  if (button1.fallingEdge()) playGun.play(GunSound);
  playAmbient.play(AudioSampleGenesis_ambient1);
  // Handle the gun effect
  if (playGun.isPlaying() && !digitalRead(4)) digitalWrite(4,HIGH); // Turn ON
  else if (digitalRead(4)) digitalWrite(4,LOW); // Turn OFF

  // Handle the noise gate
  if (peakVoice.available()) {
    if (peakVoice.read() > 0.2) {  // While voice is detected
      NoiseGateTime = 0;       // keep resetting the noise gate
      if (!NoiseGateOpen) {    // Open the gate only if it is closed
        sineGenerate.phase(0.0);      // Always open the gate with sine wave
        sineGenerate.amplitude(1.0);  // phase to zero for a soft open
        NoiseGateOpen = true;
        digitalWrite(3,HIGH);  // Turn on the dome light
      }
    } else { // If the min gate open time expires close the gate
      if (NoiseGateTime > NoiseGateLength && NoiseGateOpen) {
        sineGenerate.amplitude(0.0);
        NoiseGateOpen = false;
        digitalWrite(3,LOW);   // Turn off the dome light
      }
    }
  }
}
