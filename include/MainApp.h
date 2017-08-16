#pragma once

#include "GameContext.h"

class FreeCamera;
class InputManager;
class SceneManager;
class Window;

class MainApp final
{
public:
	MainApp();
	~MainApp();

	void Initialize();
	void UpdateAndRender();
	void Stop();
	
private:
	enum class RendererID
	{
		VULKAN,
		D3D,
		GL,

		_LAST_ELEMENT
	};

	void Destroy();

	void CycleRenderer();
	void InitializeWindowAndRenderer();
	void DestroyWindowAndRenderer();
	
	std::string RenderIDToString(RendererID rendererID) const;

	size_t m_RendererCount;

	Window* m_Window;
	SceneManager* m_SceneManager;
	GameContext m_GameContext;
	FreeCamera* m_DefaultCamera;

	glm::vec3 m_ClearColor;
	bool m_VSyncEnabled;

	RendererID m_RendererIndex;

	bool m_Running;

	MainApp(const MainApp&) = delete;
	MainApp& operator=(const MainApp&) = delete;
};