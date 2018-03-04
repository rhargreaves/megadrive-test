#include <genesis.h>
#include <stdbool.h>
#include <util.h>

typedef void _changeValueFunc();
typedef void _debouncedFunc(u16 joyState);

static void updateAlgorithmAndFeedback(void);
static void updateGlobalLFO(void);
static void updateStereoAndLFO(void);
static void updateFreqAndOctave(void);
static void updateNote(void);
static void debounce(_debouncedFunc func, u16 joyState, u8 rate);
static void YM2612_setFrequency(u16 freq, u8 octave);
static void YM2612_setAlgorithm(u8 algorithm, u8 feedback);
static void YM2612_setGlobalLFO(u8 enable, u8 freq);
static void YM2612_setStereoAndLFO(u8 stereo, u8 ams, u8 fms);
static void checkPlayNoteButton(u16 joyState);
static void checkSelectionChangeButtons(u16 joyState);
static void checkValueChangeButtons(u16 joyState);
static void printValue(const char* header, u16 minSize, u32 value, u16 row);
static void printText(const char* header, u16 minSize, const char* value, u16 row);
static void playFmNote(void);
static void stopFmNote(void);

typedef struct {
    const char name[10];
    const u16 minSize;
    u16 value;
    const u16 maxValue;
    const u8 step;
    const void (*onUpdate)();
} FmParameter;

static FmParameter fmParameters[] = {
    {
        "G.LFO On ", 1, 1, 1, 1, updateGlobalLFO
    },
    {
        "G.LFO Frq", 1, 3, 7, 1, updateGlobalLFO
    },
    {
        "Note     ", 2, 1, 11, 1, updateNote
    },
    {
        "Freq Num ", 4, 653, 2047, 4, updateFreqAndOctave
    },
    {
        "Octave   ", 1, 4, 7, 1, updateFreqAndOctave
    },
    {
        "Algorithm", 1, 0, 7, 1, updateAlgorithmAndFeedback
    },
    {
        "Feedback ", 1, 0, 7, 1, updateAlgorithmAndFeedback
    },
    {
        "LFO AMS  ", 1, 0, 7, 1, updateStereoAndLFO
    },
    {
        "LFO FMS  ", 1, 0, 7, 1, updateStereoAndLFO
    },
    {
        "Stereo   ", 1, 3, 3, 1, updateStereoAndLFO
    }
};

static const char notes_text[][3] = {"B ", "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#"};
static const u16 notes_freq[] = {617, 653, 692, 733, 777, 823, 872, 924, 979, 1037, 1099, 1164};

#define MAX_PARAMETERS sizeof(fmParameters) / sizeof(FmParameter)

enum FmParameters {
    PARAMETER_G_LFO_ON,
    PARAMETER_G_LFO_FREQ,
    PARAMETER_NOTE,
    PARAMETER_FREQ,
    PARAMETER_OCTAVE,
    PARAMETER_ALGORITHM,
    PARAMETER_FEEDBACK,
    PARAMETER_LFO_AMS,
    PARAMETER_LFO_FMS,
    PARAMETER_STEREO
};

static u8 selection = 0;

void updateNote(void)
{
    u16 note_index = fmParameters[PARAMETER_NOTE].value;
    u16 note_freq = notes_freq[note_index];
    fmParameters[PARAMETER_FREQ].value = note_freq;
    fmParameters[PARAMETER_FREQ].onUpdate();
}

void updateStereoAndLFO(void)
{
    YM2612_setStereoAndLFO(
        fmParameters[PARAMETER_STEREO].value,
        fmParameters[PARAMETER_LFO_AMS].value,
        fmParameters[PARAMETER_LFO_FMS].value
    );
}

void updateFreqAndOctave(void)
{
    YM2612_setFrequency(
        fmParameters[PARAMETER_FREQ].value,
        fmParameters[PARAMETER_OCTAVE].value);
}

void updateGlobalLFO(void)
{
    YM2612_setGlobalLFO(
        fmParameters[PARAMETER_G_LFO_ON].value,
        fmParameters[PARAMETER_G_LFO_FREQ].value
    );
}

void updateAlgorithmAndFeedback(void)
{
	YM2612_setAlgorithm(
        fmParameters[PARAMETER_ALGORITHM].value,
        fmParameters[PARAMETER_FEEDBACK].value);
}

void player_init(void)
{
}

void player_checkInput(void)
{
    u16 joyState = JOY_readJoypad(JOY_1);
    checkPlayNoteButton(joyState);
    debounce(checkSelectionChangeButtons, joyState, 8);
    debounce(checkValueChangeButtons, joyState, 6);

    for(int index = 0; index<MAX_PARAMETERS; index++)
    {
        FmParameter p = fmParameters[index];
        VDP_setTextPalette(selection == index ? PAL3 : PAL0);
        if(index == PARAMETER_NOTE)
        {
            char* note_text = notes_text[fmParameters[PARAMETER_NOTE].value];
            printText(p.name, p.minSize, note_text, index + 3);
        }
        else
        {
            printValue(p.name, p.minSize, p.value, index + 3);
        }
    }
    VDP_setTextPalette(PAL0);
}

static void printText(const char* header, u16 minSize, const char* value, u16 row)
{
    char text[50];
    strcpy(text, header);
    strcat(text, " ");
    strcat(text, value);
    VDP_drawText(text, 0, row);
}

static void printValue(const char* header, u16 minSize, u32 value, u16 row)
{
    char text[50];
    char str[5];
    uintToStr(value, str, minSize);
    strcpy(text, header);
    strcat(text, " ");
    strcat(text, str);
    VDP_drawText(text, 0, row);
}

static void checkPlayNoteButton(u16 joyState)
{
    static bool playing = false;
    if(joyState & BUTTON_A)
    {
        if(!playing)
        {
            playFmNote();
        }
        playing = true;
    }
    else
    {
        playing = false;
        stopFmNote();
    }
}

static void checkSelectionChangeButtons(u16 joyState)
{
    if(joyState & BUTTON_DOWN)
    {
        selection += 1;
    }
    else if(joyState & BUTTON_UP)
    {
        selection -= 1;
    }
    else
    {
        return;
    }
    if(selection == (u8)-1) {
        selection = MAX_PARAMETERS - 1;
    }
    if(selection > MAX_PARAMETERS - 1) {
        selection = 0;
    }
}

static void checkValueChangeButtons(u16 joyState)
{
    FmParameter* parameter = &fmParameters[selection];
    if(joyState & BUTTON_RIGHT)
    {
        parameter->value += parameter->step;
    }
    else if(joyState & BUTTON_LEFT)
    {
        parameter->value -= parameter->step;
    }
    else
    {
        return;
    }
    if(parameter->value == (u16)-1) {
        parameter->value = parameter->maxValue;
    }
    if(parameter->value > parameter->maxValue) {
        parameter->value = 0;
    }
    parameter->onUpdate();
}

static void debounce(_debouncedFunc func, u16 joyState, u8 rate)
{
    static u16 counter;
    static u16 lastJoyState;
    if(lastJoyState == joyState)
    {
        counter++;
        if(counter > rate)
        {
            counter = 0;
        }
    }
    else
    {
        counter = 0;
        lastJoyState = joyState;
    }
    if(counter == 0)
    {
        func(joyState);
    }
}

static void playFmNote(void)
{
    YM2612_setGlobalLFO(
        fmParameters[PARAMETER_G_LFO_ON].value,
        fmParameters[PARAMETER_G_LFO_FREQ].value
    );
	YM2612_writeReg(0, 0x27, 0);    // Ch 3 Normal
	YM2612_writeReg(0, 0x28, 0);    // All channels off
	YM2612_writeReg(0, 0x28, 1);
	YM2612_writeReg(0, 0x28, 2);
	YM2612_writeReg(0, 0x28, 4);
	YM2612_writeReg(0, 0x28, 5);
	YM2612_writeReg(0, 0x28, 6);
	YM2612_writeReg(0, 0x30, 0x71); // DT1/MUL
	YM2612_writeReg(0, 0x34, 0x0D);
	YM2612_writeReg(0, 0x38, 0x33);
	YM2612_writeReg(0, 0x3C, 0x01);
	YM2612_writeReg(0, 0x40, 0x23); // Total Level
	YM2612_writeReg(0, 0x44, 0x2D);
	YM2612_writeReg(0, 0x48, 0x26);
	YM2612_writeReg(0, 0x4C, 0x00);
	YM2612_writeReg(0, 0x50, 0x5F); // RS/AR
	YM2612_writeReg(0, 0x54, 0x99);
	YM2612_writeReg(0, 0x58, 0x5F);
	YM2612_writeReg(0, 0x5C, 0x99);
	YM2612_writeReg(0, 0x60, 5);   // AM/D1R
	YM2612_writeReg(0, 0x64, 5);
	YM2612_writeReg(0, 0x68, 5);
	YM2612_writeReg(0, 0x6C, 7);
	YM2612_writeReg(0, 0x70, 2);   // D2R
	YM2612_writeReg(0, 0x74, 2);
	YM2612_writeReg(0, 0x78, 2);
	YM2612_writeReg(0, 0x7C, 2);
	YM2612_writeReg(0, 0x80, 0x11);    // D1L/RR
	YM2612_writeReg(0, 0x84, 0x11);
	YM2612_writeReg(0, 0x88, 0x11);
	YM2612_writeReg(0, 0x8C, 0xA6);
	YM2612_writeReg(0, 0x90, 0);   // Proprietary
	YM2612_writeReg(0, 0x94, 0);
	YM2612_writeReg(0, 0x98, 0);
	YM2612_writeReg(0, 0x9C, 0);
	YM2612_setAlgorithm(
        fmParameters[PARAMETER_ALGORITHM].value,
        fmParameters[PARAMETER_FEEDBACK].value);
    YM2612_setStereoAndLFO(
        fmParameters[PARAMETER_STEREO].value,
        fmParameters[PARAMETER_LFO_AMS].value,
        fmParameters[PARAMETER_LFO_FMS].value
    );
    YM2612_writeReg(0, 0x28, 0x00); // Key off
    YM2612_setFrequency(
        fmParameters[PARAMETER_FREQ].value,
        fmParameters[PARAMETER_OCTAVE].value);
	YM2612_writeReg(0, 0x28, 0xF0); // Key On
}

static void YM2612_setStereoAndLFO(u8 stereo, u8 ams, u8 fms)
{
    YM2612_writeReg(0, 0xB4, (stereo << 6) + (ams << 3) + fms);
}

static void YM2612_setGlobalLFO(u8 enable, u8 freq)
{
    YM2612_writeReg(0, 0x22, (enable << 3) + freq);
}

static void YM2612_setFrequency(u16 freq, u8 octave)
{
	YM2612_writeReg(0, 0xA4, (freq >> 8) + (octave << 3));
	YM2612_writeReg(0, 0xA0, freq);
}

static void YM2612_setAlgorithm(u8 algorithm, u8 feedback)
{
	YM2612_writeReg(0, 0xB0, algorithm + (feedback << 3));
}

static void stopFmNote(void)
{
    YM2612_writeReg(0, 0x28, 0x00); // Key Off
}
