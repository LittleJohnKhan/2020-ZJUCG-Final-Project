#pragma once
#include <Camera.h>
#include <model.h>
#include <scene.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include <common.h>
#include <GameObject.h>
#include <mousepicker.h>

#include <unordered_map>

namespace KooNan
{
	enum class GameMode
	{// 第一级状态
		Title, // 标题页
		Pause, // 暂停页
		Wandering, // 漫步游览模式
		Creating // 创造模式
	};
	enum class CreatingMode
	{
		Placing, // 放置模式
		Viewing // 观察模式
	};
	enum class MouseMode {
		GUIMode, CameraMode
	};
	class GameController
	{
		// 全局状态
	public:
		static MouseMode mouseMode;
		static float deltaTime;
		static float lastFrame;

		static double cursorX, cursorY;

		static Camera mainCamera;
		static Camera oriCreatingCamera;

		static std::unordered_map<std::string, Model*>::iterator curModel;

		static Scene* mainScene;

		// 全局信号：由GUI模块或键鼠输入写入，被其他模块读取
		static string selectedModel;
	public:
		static GameMode gameMode; // 游戏模式
		static GameMode lastGameMode;
		static CreatingMode creatingMode; // 创造模式子模式
		static int sthSelected; // 场景中有物体被拾取
		
		static MousePicker mousePicker;
			// 常量
	public:
		const static unsigned int EDGE_WIDTH = 50;
			// 状态
	private:
		static bool firstMouse; // 是否是第一次点击（用于鼠标移动事件）
		static bool altPressedLast; // 上一次循环是否按下alt键
		static bool midBtnPressedLast; // 上一次循环是否按下鼠标中键
	public:
		static void initGameController(GLFWwindow* window)
		{
			glfwMakeContextCurrent(window);
			glfwSetFramebufferSizeCallback(window, GameController::framebuffer_size_callback);
			glfwSetCursorPosCallback(window, GameController::cursor_callback);
			glfwSetScrollCallback(window, GameController::scroll_callback);
		}
		static void updateGameController(GLFWwindow* window)
		{
			float currentFrame = glfwGetTime();
			GameController::deltaTime = currentFrame - GameController::lastFrame;
			GameController::lastFrame = currentFrame;

			glfwGetCursorPos(window, &cursorX, &cursorY);

			GameController::processInput(window);

			// 创造模式下使用鼠标可以移动相机
			if (gameMode == GameMode::Creating) {
				
				if (cursorX <= EDGE_WIDTH)
					mainCamera.ProcessKeyboard(deltaTime, WEST);
				else if (cursorX >= Common::SCR_WIDTH - EDGE_WIDTH)
					mainCamera.ProcessKeyboard(deltaTime, EAST);
				if(cursorY <= EDGE_WIDTH)
					mainCamera.ProcessKeyboard(deltaTime, NORTH);
				else if(cursorY >= Common::SCR_HEIGHT - EDGE_WIDTH)
					mainCamera.ProcessKeyboard(deltaTime, SOUTH);
			}

			// 检测相机是否低于地形，纠正
			if (mainScene)
			{
				static float border = 0.5f;
				float h = mainScene->getTerrainHeight(mainCamera.Position.x, mainCamera.Position.z);
				if (gameMode == GameMode::Creating && h >= mainCamera.Position.y)
					mainCamera.Position.y = h + border;
				else if(gameMode == GameMode::Wandering)
					mainCamera.Position.y = h + border;
			}
		}
		static void changeGameModeTo(GameMode newmode) {
			lastGameMode = gameMode;
			gameMode = newmode;

			if (gameMode == GameMode::Wandering) {
				altPressedLast = true;
			}
			else if (gameMode == GameMode::Creating) {
				GameController::mainCamera = GameController::oriCreatingCamera;
			}
		}
		static void revertGameMode() {
			changeGameModeTo(lastGameMode);
		}
	private:
		static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
		static void cursor_callback(GLFWwindow* window, double xpos, double ypos);
		static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
		static void processInput(GLFWwindow* window);
		static void updateCursorMode(GLFWwindow* window);

		// 找到光标射线与地形的交点
		static glm::vec3 findFocusInScene();
	private:
		
	};

	// 状态与信号初始化
	MouseMode GameController::mouseMode = MouseMode::GUIMode;
	float GameController::lastFrame = .0f;
	float GameController::deltaTime = .0f;
	double GameController::cursorX = .0;
	double GameController::cursorY = .0;

	Camera GameController::oriCreatingCamera = Camera(0.f, 2.f, 2.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f);
	Camera GameController::mainCamera = GameController::oriCreatingCamera;

	MousePicker GameController::mousePicker = MousePicker(GameController::mainCamera);

	GameMode GameController::gameMode = GameMode::Creating;
	GameMode GameController::lastGameMode = GameMode::Title;
	CreatingMode GameController::creatingMode = CreatingMode::Placing;
	int GameController::sthSelected = 0;

	bool GameController::firstMouse = true;
	bool GameController::altPressedLast = false;
	bool GameController::midBtnPressedLast = false;

	Scene* GameController::mainScene = NULL;

	string GameController::selectedModel = "";

	// 函数定义
	void GameController::framebuffer_size_callback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
		Common::SCR_HEIGHT = height;
		Common::SCR_WIDTH = width;
	}

	void GameController::cursor_callback(GLFWwindow* window, double xpos, double ypos)
	{
		if (mouseMode == MouseMode::GUIMode) return;

		static float lastX = Common::SCR_WIDTH / 2.0f;
		static float lastY = Common::SCR_HEIGHT / 2.0f;
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}
		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;

		// 避免光标不显示时点到GUI上的按钮，保持光标位置在屏幕中心
		lastX = Common::SCR_WIDTH / 2.0f;
		lastY = Common::SCR_HEIGHT / 2.0f;
		glfwSetCursorPos(window, lastX, lastY);

		if(gameMode == GameMode::Wandering) {
			mainCamera.ProcessMouseMovement(xoffset, yoffset);
		}
		else if (gameMode == GameMode::Creating) {
			if (creatingMode == CreatingMode::Placing)
			{
				static float viewDist = 5.0f;
				mainCamera.ProcessMouseMovement(xoffset, yoffset, mainCamera.Position + viewDist * mainCamera.Front);
			}
		}
	}

	void GameController::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{
		mainCamera.ProcessMouseScroll(gameMode == GameMode::Wandering ? FOVY_CHANGE : HEIGHT_CHANGE, yoffset);
	}

	void GameController::processInput(GLFWwindow* window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		if (gameMode == GameMode::Creating)
		{
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				mainCamera.ProcessKeyboard(deltaTime, NORTH);
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				mainCamera.ProcessKeyboard(deltaTime, SOUTH);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				mainCamera.ProcessKeyboard(deltaTime, WEST);
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				mainCamera.ProcessKeyboard(deltaTime, EAST);

			bool midBtnPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
			if (midBtnPressed != midBtnPressedLast)
			{
				midBtnPressedLast = midBtnPressed;
				updateCursorMode(window);
			}

			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
			{
				glm::vec3 t = findFocusInScene();
				if (t != mainCamera.Position && selectedModel != "")
				{
					glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), t);
					GameObject* p1 = new GameObject(selectedModel.c_str(), modelMat);
					selectedModel = "";
				}
				
			}
		}
		else if (gameMode == GameMode::Wandering)
		{
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				mainCamera.ProcessKeyboard(deltaTime, FORWARD);
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				mainCamera.ProcessKeyboard(deltaTime, BACKWARD);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				mainCamera.ProcessKeyboard(deltaTime, LEFT);
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				mainCamera.ProcessKeyboard(deltaTime, RIGHT);

			bool altPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
			if (altPressed != altPressedLast)
			{
				altPressedLast = altPressed;
				updateCursorMode(window);
			}
		}
	}

	void GameController::updateCursorMode(GLFWwindow* window)
	{
		if(gameMode == GameMode::Wandering)
			if (altPressedLast)
			{
				mouseMode = MouseMode::GUIMode;
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				GameController::firstMouse = true;
			}
			else
			{
				mouseMode = MouseMode::CameraMode;
				glfwSetCursorPos(window, Common::SCR_WIDTH / 2.0f, Common::SCR_HEIGHT / 2.0f);
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
		else if (gameMode == GameMode::Creating)
			if (midBtnPressedLast)
			{
				mouseMode = MouseMode::CameraMode;
				glfwSetCursorPos(window, Common::SCR_WIDTH / 2.0f, Common::SCR_HEIGHT / 2.0f);
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else
			{
				mouseMode = MouseMode::GUIMode;
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				GameController::firstMouse = true;
			}
	}
	glm::vec3 GameController::findFocusInScene()
	{
		if(mainScene == NULL)
			return mainCamera.Position;

		// 防止向上放置，防止getTerrainHeight的断言异常
		if(mainCamera.Front.y >= 0)
			return mainCamera.Position;

		mousePicker.update(cursorX, cursorY);
		glm::vec3 d3 = mousePicker.getCurrentRay();

		static int maxStep = 100;
		static float stepWise = 2.0f;
		int step;
		glm::vec3 curPosition = glm::vec3(mainCamera.Position);
		for (step = 0; step < maxStep; step++)
		{
			if (curPosition.y <= mainScene->getTerrainHeight(curPosition.x,curPosition.z))
				break;
			curPosition += stepWise * d3;
		}
		if(step == maxStep)
			return mainCamera.Position;
		return curPosition - stepWise * d3;
	}
}