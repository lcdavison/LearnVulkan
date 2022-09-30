#include "Input/InputManager.hpp"

#include "Common.hpp"

#include <Windows.h>

#include <array>

Input::InputState Input::State = {};

void Input::UpdateInputState(uint32 const /*DPI*/)
{
    std::copy(Input::State.CurrentKeyboardState.cbegin(), Input::State.CurrentKeyboardState.cend(), Input::State.PreviousKeyboardState.begin());

    Input::State.PreviousMouseX = Input::State.CurrentMouseX;
    Input::State.PreviousMouseY = Input::State.CurrentMouseY;
}