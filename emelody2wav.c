/*
   Simple eMelody monotonic ringtone to .WAV file
   Colin Wilcox, Wireless Edge, 2001
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define NUM_CHANNELS 1
#define DURATION_MS 300 // Each note lasts 300 ms
#define MAX_NOTES 512
#define AMPLITUDE 30000
#define PI 3.14159265

typedef struct {
    char note;
    int octave;
} Tone;

// Map notes to frequencies (basic equal temperament)
double get_frequency(char note, int octave) 
{
    int n;
    switch (note)
    {
        case 'c': n = 0;
		  break;
        case 'd': n = 2;
		  break;
        case 'e': n = 4; 
		  break;
        case 'f': n = 5;
  		  break;
        case 'g': n = 7;
		  break;
        case 'a': n = 9;
		  break;
        case 'b': n = 11;
		  break;
        case 'p': return 0.0; // pause
        default: return 0.0;
    }

    // MIDI note number for C4 = 60, A4 = 69
    int midi_number = 12 * (octave + 1) + n;
    return 440.0 * pow(2.0, (midi_number - 69) / 12.0);
}

// Write WAV header
void write_wav_header(FILE *f, int num_samples) 
{
    int byte_rate = SAMPLE_RATE * NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    int block_align = NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    int data_chunk_size = num_samples * NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    int chunk_size = 36 + data_chunk_size;

    fwrite("RIFF", 1, 4, f);
    fwrite(&chunk_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);

    // fmt subchunk
    fwrite("fmt ", 1, 4, f);
    int subchunk1_size = 16;
    short audio_format = 1;
    fwrite(&subchunk1_size, 4, 1, f);
    fwrite(&audio_format, 2, 1, f);
    fwrite(&NUM_CHANNELS, 2, 1, f);
    fwrite(&SAMPLE_RATE, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&BITS_PER_SAMPLE, 2, 1, f);

    // data subchunk
    fwrite("data", 1, 4, f);
    fwrite(&data_chunk_size, 4, 1, f);
}

// Generate sine wave for a frequency
void generate_sine_wave(FILE *f, double freq, int duration_ms) 
{
    int num_samples = SAMPLE_RATE * duration_ms / 1000;

    for (int i = 0; i < num_samples; i++)
    {
        double sample;
        if (freq == 0.0)
        {
            sample = 0.0; // silence
        } else
        {
            double t = (double)i / SAMPLE_RATE;
            sample = AMPLITUDE * sin(2 * PI * freq * t);
        }

        short s = (short)sample;
        fwrite(&s, sizeof(short), 1, f);
    }
}

// Extract melody line from eMelody text
const char* extract_melody_line(const char *emelody)
{
    const char *start = strstr(emelody, "MELODY:");
    if (!start) {
	 return NULL;
    }

    start += strlen("MELODY:");
    while (*start == ' ' || *start == '\n')
    {
       start++;
    }

    return start;
}

// Parse melody and synthesize
void parse_and_generate(const char *melody, FILE *fout, int *total_samples) 
{
    int octave = 4;
    *total_samples = 0;

    for (int i = 0; melody[i] != '\0'; i++) 
    {
        char ch = melody[i];
        if (ch == '>')
        {
            octave++;
        }
        else if (ch == '<')
        {
            octave--;
        } 
        else if ((ch >= 'a' && ch <= 'g') || ch == 'p')
        {
            double freq = get_frequency(ch, octave);
            generate_sine_wave(fout, freq, DURATION_MS);
            *total_samples += SAMPLE_RATE * DURATION_MS / 1000;
        }
        // Ignore other characters (volume, tempo for now)
    }
}

int main(int argc, char *argv[]) {
  
    if argc != 2 {
	fprintf(stderr, "Error: No melody file supplied.\n");
	return -1
    }


    const char *melody = extract_melody_line(argv[1]);
    if (!melody) 
    {
        fprintf(stderr, "Error: Could not find MELODY line.\n");
        return -2;
    }

    FILE *fout = fopen("output.wav", "wb");
    if (!fout)
    {
        perror("fopen");
        return -3;
    }

    // Reserve space for header
    fseek(fout, 44, SEEK_SET);

    int total_samples = 0;
    parse_and_generate(melody, fout, &total_samples);

    // Go back and write the header now
    fseek(fout, 0, SEEK_SET);
    write_wav_header(fout, total_samples);

    fclose(fout);
    printf("WAV file created: output.wav\n");
    return 0;
}

// end of file
