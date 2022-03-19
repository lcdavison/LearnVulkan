#pragma once

#include <Windows.h>

#include <cstdint>

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

		inline static std::uint32_t kApplicationVersionNo = 0u;

		inline static const std::uint32_t kDefaultWindowWidth = 1280u;
		inline static const std::uint32_t kDefaultWindowHeight = 720u;

		HWND WindowHandle_;

	};
}
