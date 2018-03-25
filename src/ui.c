#include <genesis.h>
#include <stdbool.h>
#include <synth.h>
#include <channel.h>
#include <ui.h>
#include <ui_display.h>

#define SELECTION_COUNT FM_PARAMETER_COUNT + (OPERATOR_PARAMETER_COUNT * OPERATOR_COUNT)

typedef void DebouncedFunc(u16 joyState, u8 selection);

static bool modifyValue(u16 joyState, u16 *value);
static void debounce(DebouncedFunc func, u16 joyState, u8 selection);
static void checkPlayNoteButton(u16 joyState);
static void checkChannelSwitch(u16 joyState, u8 selection);
static void checkSelectionChangeButtons(u16 joyState, u8 selection);
static void checkValueChangeButtons(u16 joyState, u8 selection);
static void updateGlobalParameter(u16 joyState, u16 index);
static void updateOpParameter(u16 joyState, u16 index);
static void updateFmParameter(u16 joyState, u16 index);

static u8 currentSelection = 0;
static Channel *currentChannel;

void ui_init(void)
{
    currentChannel = synth_channel(0);
    display_draw(currentChannel, currentSelection);
}

void ui_checkInput(void)
{
    u16 joyState = JOY_readJoypad(JOY_1);
    checkPlayNoteButton(joyState);
    debounce(checkSelectionChangeButtons, joyState, currentSelection);
    debounce(checkValueChangeButtons, joyState, currentSelection);
    checkChannelSwitch(joyState, currentSelection);
    display_updateUiIfRequired(currentChannel, currentSelection);
}

static void checkChannelSwitch(u16 joyState, u8 selection)
{
    if (joyState & BUTTON_START)
    {
        u8 chanNum = (currentChannel->number) + 1;
        if (chanNum == CHANNEL_COUNT)
        {
            chanNum = 0;
        }
        currentChannel = synth_channel(chanNum);
        display_requestUiUpdate();
    }
}

static void checkPlayNoteButton(u16 joyState)
{
    static bool playing = false;
    if (joyState & BUTTON_A)
    {
        if (!playing)
        {
            channel_playNote(currentChannel);
        }
        playing = true;
    }
    else
    {
        playing = false;
        channel_stopNote(currentChannel);
    }
}

static void checkSelectionChangeButtons(u16 joyState, u8 selection)
{
    if (joyState & BUTTON_DOWN)
    {
        selection += 1;
    }
    else if (joyState & BUTTON_UP)
    {
        selection -= 1;
    }
    else
    {
        return;
    }
    if (selection == (u8)-1)
    {
        selection = SELECTION_COUNT - 1;
    }
    if (selection > SELECTION_COUNT - 1)
    {
        selection = 0;
    }
    currentSelection = selection;
    display_requestUiUpdate();
}

static void checkValueChangeButtons(u16 joyState, u8 index)
{
    if (index < GLOBAL_PARAMETER_COUNT)
    {
        updateGlobalParameter(joyState, index);
        return;
    }
    index -= GLOBAL_PARAMETER_COUNT;
    if (index < FM_PARAMETER_COUNT)
    {
        updateFmParameter(joyState, index);
        return;
    }
    index -= FM_PARAMETER_COUNT;
    updateOpParameter(joyState, index);
}

static void updateGlobalParameter(u16 joyState, u16 index)
{
    u16 value = synth_globalParameterValue(index);
    if (!modifyValue(joyState, &value))
    {
        return;
    }
    synth_setGlobalParameterValue(index, value);
    display_requestUiUpdate();
}

static void updateFmParameter(u16 joyState, u16 index)
{
    u16 value = channel_parameterValue(currentChannel, index);
    if (!modifyValue(joyState, &value))
    {
        return;
    }
    channel_setParameterValue(currentChannel, index, value);
    display_requestUiUpdate();
}

static void updateOpParameter(u16 joyState, u16 index)
{
    Operator *op = channel_operator(currentChannel, index / OPERATOR_PARAMETER_COUNT);
    OpParameters opParameter = index % OPERATOR_PARAMETER_COUNT;
    u16 value = operator_parameterValue(op, opParameter);
    if (!modifyValue(joyState, &value))
    {
        return;
    }
    operator_setParameterValue(op, opParameter, value);
    display_requestUiUpdate();
}

static bool modifyValue(u16 joyState, u16 *value)
{
    if (joyState & BUTTON_RIGHT)
    {
        (*value)++;
    }
    else if (joyState & BUTTON_LEFT)
    {
        (*value)--;
    }
    else
    {
        return false;
    }
    return true;
}

static void debounce(DebouncedFunc func, u16 joyState, u8 selection)
{
    const u8 REPEAT_RATE = 2;
    static u16 counter;
    static u16 lastJoyState;
    if (lastJoyState == joyState)
    {
        counter++;
        if (counter > REPEAT_RATE)
        {
            counter = 0;
        }
    }
    else
    {
        counter = 0;
        lastJoyState = joyState;
    }
    if (counter == 0)
    {
        func(joyState, selection);
    }
}
