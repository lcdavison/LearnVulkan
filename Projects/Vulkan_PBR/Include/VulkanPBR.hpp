#pragma once

#include <Windows.h>

#include "CommonTypes.hpp"

namespace PBR
{
	class VulkanApplication
	{
		VulkanApplication() = default;

	public:
		inline static VulkanApplication& Get()
		{
			static VulkanApplication Instance = VulkanApplication();
			return Instance;
		}

		bool Initialise();
		bool Run();

	private:
		static LRESULT CALLBACK WindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

	private:
		inline static const TCHAR* kWindowClassName = TEXT("Vulkan-PBR");
		inline static const TCHAR* kWindowName = TEXT("PBR Window");

		inline static uint32 kApplicationVersionNo = 0u;

		inline static const uint32 kDefaultWindowWidth = 1280u;
		inline static const uint32 kDefaultWindowHeight = 720u;

		HWND WindowHandle_;

	};
}
