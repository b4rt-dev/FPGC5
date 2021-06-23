/*
 * ADSR
 * Contains ADSR related functions
 */

// Initialize the ADSR
// by setting initial values for ADSR and display
// range: 0-127
void initADSR()
{
  setADSRattack     (1);
  setADSRdecay      (65);
  setADSRsustain    (0);
  setADSRrelease    (15);
}


// Tell ADSR that the note is pressed
void setGateOn(uint32_t n) 
{
  voices[n].adsr.counter  = 0;
  voices[n].adsr.state    = ADSR_ATTACK;
  voices[n].adsr.pressed  = true;
}


// Tell ADSR that the note is released
void setGateOff(uint32_t n) 
{
  if (!SustainPedal)
  {
    voices[n].adsr.lastOutput   = voices[n].adsr.output;
    voices[n].adsr.counter      = 0;
    voices[n].adsr.state        = ADSR_RELEASE;
    voices[n].adsr.pressed      = false;
  }
  else
  {
    // when the sustain pedal is pressed, we keep the note on sustain, but note that the note is not pressed anymore in case of pedal release
    voices[n].adsr.pressed      = false;
  }
}


// Turn off notes with low volume
void turnOffLowVolumeNotes() 
{
  for (int n = 0; n < MAXVOICES; n++)
  {
    if (voices[n].adsr.state == ADSR_SUSTAIN)
    {
      if (voices[n].adsr.output < 20)
      {
        voices[n].adsr.counter  = 0;
        voices[n].adsr.state    = ADSR_IDLE;
        voices[n].adsr.output   = 0;
        voices[n].adsr.pressed  = false;
      }
    }
  }
}


// Handler when sustain pedal is released
void doSustainPedalRelease() 
{
  for (int n = 0; n < MAXVOICES; n++)
  {
    if (voices[n].adsr.state != ADSR_IDLE && voices[n].adsr.state != ADSR_RELEASE)
    {
      if (voices[n].adsr.pressed == false)
      {
        setGateOff(n);
      }
    }
  }
}


// Sets AmaxCount value based on the given MIDI value (0-127)
// We can use floats here, since this is only calculated when the ADSR settings are changed
// Currently a linear scale, should make this exponential in the future
void setADSRattack(uint32_t MIDIvalue) 
{
  float f = float(SAMPLERATE) * (float(MIDIvalue) * float(ADSR_MAXATTACKTIME) / float(MIDICONTROLMAXVAL));
  AmaxCount = uint32_t(f);
}


// Sets DmaxCount value based on the given MIDI value (0-127)
// We can use floats here, since this is only calculated when the ADSR settings are changed
// Currently a linear scale, should make this exponential in the future
void setADSRdecay(uint32_t MIDIvalue) 
{
  float f = float(SAMPLERATE) * (float(MIDIvalue) * float(ADSR_MAXDECAYTIME) / float(MIDICONTROLMAXVAL));
  DmaxCount = uint32_t(f);
}


// Sets sustain value by mapping the given MIDI value to 0-4095
// We can use floats here, since this is only calculated when the ADSR settings are changed
// Which is the valid range for sustain values
// Currently is a linear function, should make this exponential in the future
void setADSRsustain(uint32_t MIDIvalue)
{
  Sval = MIDIvalue << 5;
}


// Sets RmaxCount value based on the given MIDI value (0-127)
// We can use floats here, since this is only calculated when the ADSR settings are changed
// Currently a linear scale, should make this exponential in the future
void setADSRrelease(uint32_t MIDIvalue) 
{
  float f = float(SAMPLERATE) * (float(MIDIvalue) * float(ADSR_MAXRELEASETIME) / float(MIDICONTROLMAXVAL));
  RmaxCount = uint32_t(f);
}


// Updates the ADSR state, counter and output value for the given voice
// Is called for every sample
void updateADSR(uint32_t n) 
{
  // do different things for different states
  switch (voices[n].adsr.state) 
  {
    case ADSR_ATTACK:
        voices[n].adsr.counter += 1; 
        // if we reached the end of attack
        if (voices[n].adsr.counter >= AmaxCount) 
        {
          voices[n].adsr.state    = ADSR_DECAY;
          voices[n].adsr.counter  = 0;
        }
        else 
        {
          // calculate ADSR table index based on current counter and maxCount
          uint32_t maxCountDiv4096 = AmaxCount >> 12;
          if (maxCountDiv4096 == 0) // prevent division by 0
            maxCountDiv4096 = 1;
          uint32_t index = voices[n].adsr.counter / maxCountDiv4096;
          if (index > 4095)
            index = 4095;
          
          voices[n].adsr.output = getAttack(index);
        }
      break;
    case ADSR_DECAY:
        voices[n].adsr.counter += 1; 
        // if we reached the end of decay
        if ( voices[n].adsr.counter >= DmaxCount) 
        {
          voices[n].adsr.state = ADSR_SUSTAIN;
          voices[n].adsr.counter = 0;
        }
        else 
        {
          // calculate ADSR table index based on current counter and maxCount
          uint32_t maxCountDiv4096 = DmaxCount >> 12;
          if (maxCountDiv4096 == 0) // prevent division by 0
            maxCountDiv4096 = 1;
          uint32_t index = voices[n].adsr.counter / maxCountDiv4096;
          if (index > 4095)
            index = 4095;

          uint32_t v = getDecay(index);
          // scale decay to sustain difference
          v = (v * (4095-Sval));
          v = v >> 12;
          v = v + Sval;
          
          voices[n].adsr.output = v;
        }
      break;
    case ADSR_SUSTAIN:
      voices[n].adsr.output = Sval;
      break;
    case ADSR_RELEASE:
        voices[n].adsr.counter += 1; 
        // if we reached the end of release
        if ( voices[n].adsr.counter >= RmaxCount) 
        {
          voices[n].adsr.counter  = 0;
          voices[n].adsr.state    = ADSR_IDLE;
          voices[n].adsr.output   = 0;
        }
        else 
        {
          // calculate ADSR table index based on current counter and maxCount
          uint32_t maxCountDiv4096 = RmaxCount >> 12;
          if (maxCountDiv4096 == 0)
            maxCountDiv4096 = 1;
          uint32_t index = voices[n].adsr.counter / maxCountDiv4096;
          if (index > 4095)
            index = 4095;
          
          uint32_t v = getDecay(index);

          // scale output height by lastOutput
          v = (v * voices[n].adsr.lastOutput);
          v = v >> 12;
          voices[n].adsr.output = v;
        }
      break;
  }
}
