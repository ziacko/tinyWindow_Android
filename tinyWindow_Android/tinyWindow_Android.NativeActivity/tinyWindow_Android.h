#ifndef TINYWINDOW_ANDROID_H
#define TINYWINDOW_ANDROID_H

class windowManager
{

	windowManager()
	{

	}

	~windowManager()
	{

	}

	void Initialize()
	{
		GetInstance()->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		eglInitialize(display, 0, 0);
	}

	static inline GLboolean isInitialized()
	{
		return windowManager::instance != nullptr;
	}

	static inline windowManager* AddWindow(const char* name, GLuint colorBits = 8,
		GLuint depthBits, GLuint stencilBits = 8)
	{
		if (GetInstance()->isInitialized())
		{
			if ( name != nullptr )
			{
				tWindow* newWindow = new tWindow(name, colorBits, depthBits, stencilBits);
				GetInstance()->windowList.push_back(newWindow);
				newWindow->iD = GetNumWindows() - 1;
			}
		}
	}

	static inline GLint GetNumWindows()
	{
		if (isInitialized())
		{
			return GetInstance()->windowList.size();
		}
	}

private:

	static inline windowManager* GetInstance( void )
	{
		if (windowManager::instance == nullptr)
		{
			windowManager::instance = new windowManager();
			windowManager::initialized = true;
			return windowManager::instance;
		}

		else
		{
			return windowManager::instance;
		}
	}

	struct tWindow
	{
		tWindow(const char* name, GLuint colorBits = 8, GLuint depthBits = 8, GLuint stencilBits = 8)
		{
			this->name = name;
			this->iD = NULL;
			this->colorBits = colorBits;
			this->depthBits = depthBits;
			this->stencilBits = stencilBits;
			this->initialized = GL_FALSE;
			this->contextCreated = GL_FALSE;
			this->state.angle = 0.0f;

			this->inFocus = GL_FALSE;
			this->attributes = {
				EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
				EGL_DEPTH_SIZE, depthBits,
				EGL_STENCIL_SIZE, stencilBits,
				EGL_RED_SIZE, colorBits,
				EGL_GREEN_SIZE, colorBits,
				EGL_BLUE_SIZE, colorBits,
				EGL_NONE
			};
		}

		~tWindow()
		{

		}

		struct savedState
		{
			GLfloat		angle;
			GLint		x;
			GLint		y;

		};

		const char*		name;
		GLuint			iD;
		GLuint			colorBits;
		GLuint			depthBits;
		GLuint			stencilBits;
		GLboolean		shouldClose;
		GLboolean		inFocus;

		GLboolean		initialized;
		GLboolean		contextCreated;
		GLboolean		isCurrentContext;

		GLuint			currentState;

		//event handling goes here

		EGLContext		context;
		savedState		state;

		EGLint*			attributes;
		GLint			format;
		EGLConfig		config;
		EGLint			numConfigs;

		android_app*		app;
		ASensorManager*		sensorManager;
		const ASensor*		accelerometerSensor;
		ASensorEventQueue*	sensorEventQueue;

		int animating;
	};

	static inline InitializeWindow(tWindow* window)
	{
		eglChooseConfig(GetInstance()->display, &window->attributes, &window->config, 1, &window->numConfigs);

		eglGetConfigAttrib(GetInstance()->display, window->config, EGL_NATIVE_VISUAL_ID, &window->format);

		ANativeWindow_setBuffersGeometry(window->app->window, 0, 0, window->format);

		GetInstance()->surface = eglCreateWindowSurface(GetInstance()->display, window->config, window->app->window, NULL);
		window->context = eglCreateContext(GetInstance()->display, window->config, NULL, NULL);

		eglMakeCurrent(GetInstance()->display, GetInstance()->surface, GetInstance()->surface, window->context);

		eglQuerySurface(GetInstance()->display, GetInstance()->surface, EGL_WIDTH, &GetInstance()->screenResolution[0]);
		eglQuerySurface(GetInstance()->display, GetInstance()->surface, EGL_WIDTH, &GetInstance()->screenResolution[1]);


	}

	static void SwapBuffers()
	{
		eglSwapBuffers(GetInstance()->display, GetInstance()->surface);
	}

	static void HandleInput()
	{

	}



	EGLSurface				surface;
	EGLDisplay				display;
	EGLint					EGLmajor;
	EGLint					EGLminor;
	static GLboolean		initialized;
	static windowManager*	instance;

	std::list< tWindow* >	windowList;
	GLuint					displayResolution[ 2 ];
};

windowManager* windowManager::instance = nullptr;
GLboolean	windowManager::initialized = false;

#endif
