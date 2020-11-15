#include <iostream>
#include <math.h>
#include "audio_out.h"


using namespace std;



double w(double hertz)
{
	return hertz * 2.0 * PI;
}

double oscillator(double hertz, double time, int type) {

	switch (type) {

	case 0: //Sine wave
		return sin(w(hertz) * time);

	case 1: //Square wave
		return sin(w(hertz) * time) > 0.0 ? 0.5 : -0.5;

	case 2: //Triangle wave
		return 2 / PI * asin(sin(w(hertz) * time));

	case 3: //Wobbly square wave
	{
		double out = 0.0;
		for (double n = 1.0; n < 60.0; n++)
			out += (sin(n * w(hertz) * time)) / n;
		return out * (2.0 / PI);
	}

	case 4: //Perfect square wave
		return (2.0 / PI) * (hertz * PI * fmod(time, 1.0 / hertz) - (PI / 2.0));


	case 5: //White Noise
		return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;

	default: return 0;
	}
}

struct ADSR  //Envelope Generator
{
	double attack;
	double decay;
	double release;
	
	double sustainLevel;
	double sustain_startLevel;

	double trigger_startTime;
	double trigger_stopTime;
	bool noteOn;

	ADSR()
	{
		attack = 0.2;
		decay = 0.1;
		sustain_startLevel = 1.0;
		sustainLevel = 0.8;
		release = 0.5;
		
		trigger_startTime = 0.0;
		trigger_stopTime = 0.0;
		noteOn = false;
	}

	double getAmplitude(double time)
	{
		double damplitude = 0.0;
		double gateTime = time - trigger_startTime;

		if (noteOn)   //Attack,Decay,Sustain portion
		{	//attack time
			if (gateTime <= attack)
				damplitude = (gateTime / attack) * sustainLevel;
			//decay time
			if (gateTime > attack && gateTime <= (attack + decay))
				damplitude = ((gateTime - attack) / decay) * (sustainLevel - sustain_startLevel) + sustain_startLevel;
			//sustain level
			if (gateTime > (attack + decay))
			{
				damplitude = sustainLevel;
			}

		}
		else   //Release portion
		{
			damplitude = (time - trigger_stopTime) / release * (0.0 - sustainLevel) + sustainLevel;
		}
		
		if (damplitude <= 0.0001)
		{
			damplitude = 0.0;
		}

		return damplitude;
	}

	void NoteOn(double timeOn)
	{
		trigger_startTime = timeOn;
		noteOn = true;
	}
	void NoteOff(double timeOff)
	{
		trigger_stopTime = timeOff;
		noteOn = false;
	}
};

//Global Synthesizer variables
atomic<double> freq = 0.0;
double OctvFreq = 110.0;
double octvScale = pow(2.0, 1.0 / 12.0);
ADSR envelope;

//Syntheziser Function
double meknoise(double time)
{
	double out = envelope.getAmplitude(time) *
		(
			+ oscillator(freq, time, 2)
			+ oscillator(freq, time, 1)
		
		);

	return 0.5 * out;
}


int main()
{
	//Retrieve Audio hardware // Header file courtesy of onelonecoder
	vector<wstring> devices = olcNoiseMaker<short>::Enumerate();
	//Display hardware
	for (auto d : devices) wcout << "Found Output Device:" << d << endl;
	//Create audio engine
	olcNoiseMaker<short>sound(devices[0], 44100, 1, 8, 512);
	//Send sound to audio out
	sound.SetUserFunction(meknoise);

	int currentKey = -1;
	bool keypress = false;
	
	//Play on Computer Keyboard
	while(1)
	{
		keypress = false;
		for (int k = 0; k < 16; k++)
		{
			if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbeL\xbe"[k])) & 0x8000) 
			{
				if (currentKey != k)
				{
					freq = OctvFreq * pow(octvScale, k);
					envelope.NoteOn(sound.GetTime());
					wcout << "\rNote On: " << sound.GetTime() << "s" << freq;
					currentKey = k;
				}
				keypress = true;
			}
		}
		if (!keypress)
		{
			if (currentKey != -1)
			{
				wcout << "\rNote Off: " << sound.GetTime() << "s" << freq;
				envelope.NoteOff(sound.GetTime());
				currentKey = -1;
			}
			
		}		
	}

	return 0;
}