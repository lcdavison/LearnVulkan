#pragma once

#include "Common.hpp"

#include <array>

namespace Input
{
    enum class KeyStates : uint8
    {
        Up,
        Down,
    };

    struct InputState
    {
        std::array<KeyStates, 256u> PreviousKeyboardState = {};
        std::array<KeyStates, 256u> CurrentKeyboardState = {};

        float MouseSensitivity = { 8.0f };

        int32 PreviousMouseX = {};
        int32 PreviousMouseY = {};

        int32 CurrentMouseX = {};
        int32 CurrentMouseY = {};
    };

    extern void UpdateInputState(uint32 const DPI);

    extern InputState State;

    inline bool const IsKeyPushed(uint8 const VirtualKeyCode)
    {
        return State.PreviousKeyboardState [VirtualKeyCode] == KeyStates::Up && State.CurrentKeyboardState [VirtualKeyCode] == KeyStates::Down;
    }

    inline bool const IsKeyReleased(uint8 const VirtualKeyCode)
    {
        return State.PreviousKeyboardState [VirtualKeyCode] == KeyStates::Down && State.CurrentKeyboardState [VirtualKeyCode] == KeyStates::Up;
    }

    inline bool const IsKeyUp(uint8 const VirtualKeyCode)
    {
        return State.CurrentKeyboardState [VirtualKeyCode] == KeyStates::Up;
    }

    inline bool const IsKeyDown(uint8 const VirtualKeyCode)
    {
        return State.CurrentKeyboardState [VirtualKeyCode] == KeyStates::Down;
    }
}