#ifndef TINYWINDOW_ANDROID_H
#define TINYWINDOW_ANDROID_H

#include <jni.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>

#include <sys/resource.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>

#include <stdlib.h>
#include <vector>
#include  <stdio.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>


#define TINYWINDOW_OK 1
#define TINYINDOW_ERROR 0


class windowManager
{
public:
	
	struct savedState_t
	{
		GLfloat		angle;
		GLint		x;
		GLint		y;
	};

	windowManager()
	{
		initialized = false;
		SetInternalCallbacks();
		//AndroidAppCreate()
	}

	~windowManager()
	{

	}

	static void Initialize(ANativeActivity* activity, void* savedState, size_t savedStateSize)
	{
		if (!GetInstance()->initialized)
		{
			GetInstance()->activity = activity;
			GetInstance()->savedState = static_cast<savedState_t*>(savedState);
			GetInstance()->savedStateSize = savedStateSize;
		}

		else
		{

		}
	}

	static inline GLboolean isInitialized()
	{
		return windowManager::instance != NULL;
	}

	static inline windowManager* AddWindow(const char* name, GLuint colorBits = 8,
		GLuint depthBits = 12, GLuint stencilBits = 8)
	{
		if (GetInstance()->isInitialized())
		{
			if ( name != NULL )
			{
				window_t* newWindow = new window_t(name, colorBits, depthBits, stencilBits);
				GetInstance()->windowList.push_back(newWindow);
				newWindow->iD = GetNumWindows() - 1;
				return instance;
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

	enum
	{
		LOOPER_ID_MAIN = 1,
		LOOPER_ID_INPUT,
		LOOPER_ID_USER,
	};

	enum
	{
		APP_COMMAND_INPUT_CHANGED,
		APP_COMMAND_INIT_WINDOW,
		APP_COMMAND_TERMINATE_WINDOW,
		APP_COMMAND_WINDOW_RESIZED,
		APP_COMMAND_WINDOW_REDRAW_NEEDED,
		APP_COMMAND_CONTENT_RECT_CHANGED,
		APP_COMMAND_GAINED_FOCUS,
		APP_COMMAND_LOST_FOCUS,
		APP_COMMAND_CONFIG_CHANGED,
		APP_COMMAND_LOW_MEMORY,
		APP_COMMAND_START,
		APP_COMMAND_RESUME,
		APP_COMMAND_SAVE_STATE,
		APP_COMMAND_PAUSE,
		APP_COMMAND_STOP,
		APP_COMMAND_DESTROY
	};

	

	struct androidApp_t; //needed a small forward declaration

	struct androidPollSource_t
	{
		int				iD;
		android_app*	app;
		void(*process)(androidApp_t* app, androidPollSource_t* source);
	};

	struct androidApp_t
	{
		void* userData;

		void (*OnAppCommand)(androidApp_t* app, int command);

		int (*OnInputEvent)(androidApp_t* app, AInputEvent* event);

		ANativeActivity* nativeActivity;

		AConfiguration* config;

		void* savedState;
		size_t savedStateSize;

		ALooper* looper;

		AInputQueue* inputQueue;

		ANativeWindow* window;

		ARect contentRect;

		int activityState;

		int destroyRequested;

		pthread_mutex_t mutex;
		pthread_cond_t condition;

		int messageRead;
		int messageWrite;

		pthread_t	thread;

		android_poll_source commandPollSource;
		android_poll_source inputPollSource;

		int running;
		int stateSaved;
		int destroyed;
		int redrawNeeded;

		AInputQueue* pendingInputQueue;
		ANativeWindow* pendingWindow;
		ARect pendingContentRect;
	};

	struct window_t
	{
		window_t(const char* name, GLuint colorBits = 8, GLuint depthBits = 8, GLuint stencilBits = 8)
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
			/*this->attributes = new EGLint[13]; {
				EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
				EGL_DEPTH_SIZE, depthBits,
				EGL_STENCIL_SIZE, stencilBits,
				EGL_RED_SIZE, colorBits,
				EGL_GREEN_SIZE, colorBits,
				EGL_BLUE_SIZE, colorBits,
				EGL_NONE,
			};*/
		}

		~window_t()
		{

		}



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
		savedState_t		state;

		const EGLint*			attributes;
		GLint			format;
		EGLConfig		config;
		EGLint			numConfigs;

		android_app*		app;
		ASensorManager*		sensorManager;
		const ASensor*		accelerometerSensor;
		ASensorEventQueue*	sensorEventQueue;

		int animating;
	};

	


	static inline windowManager* GetInstance(void)
	{
		if (windowManager::instance == NULL)
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

	static inline void InitializeWindow(window_t* window)
	{
		//eglChooseConfig(GetInstance()->display, &window->attributes, &window->config, 1, &window->numConfigs);

		eglGetConfigAttrib(GetInstance()->display, window->config, EGL_NATIVE_VISUAL_ID, &window->format);

		ANativeWindow_setBuffersGeometry(window->app->window, 0, 0, window->format);

		GetInstance()->surface = eglCreateWindowSurface(GetInstance()->display, window->config, window->app->window, NULL);
		window->context = eglCreateContext(GetInstance()->display, window->config, NULL, NULL);

		eglMakeCurrent(GetInstance()->display, GetInstance()->surface, GetInstance()->surface, window->context);

		//eglQuerySurface(GetInstance()->display, GetInstance()->surface, EGL_WIDTH, &GetInstance()->screenResolution[0]);
		//eglQuerySurface(GetInstance()->display, GetInstance()->surface, EGL_HEIGHT, &GetInstance()->screenResolution[1]);
	}

	static inline void ShutdownWindow(window_t* window)
	{
		if (GetInstance()->display != EGL_NO_DISPLAY)
		{
			//if the context is valid, destroy it
			if (window->context != EGL_NO_CONTEXT)
			{
				eglDestroyContext(GetInstance()->display, window->context);
			}
		}
	}

	static inline void Shutdown()
	{
		//eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		for (int iter = 0; iter < GetInstance()->GetNumWindows(); iter++)
		{
			ShutdownWindow(GetInstance()->windowList[iter]);
		}

		if (GetInstance()->surface != EGL_NO_SURFACE)
		{
			eglDestroySurface(GetInstance()->display, GetInstance()->surface);
			eglTerminate(GetInstance()->display);
		}

		GetInstance()->display = EGL_NO_DISPLAY;
		GetInstance()->surface = EGL_NO_SURFACE;
	}

	static inline void SwapBuffers()
	{
		eglSwapBuffers(GetInstance()->display, GetInstance()->surface);
	}

	static inline GLboolean MakeWindowCurrentContextByName(const char* windowName)
	{
		/*if (GetInstance()->IsInitialized())
		{
			if (DoesExistByName(windowName))
			{
				eglMakeCurrent(GetInstance()->instance, GetInstance()->surface, GetInstance()->surface, );
				return TINYWINDOW_OK;
			}
			return TINYWINDOW_ERROR;
		}*/
		
	}

	static void HandleInput()
	{

	}

	void FreeSavedState(androidApp_t* app)
	{
		//lock the thread
		pthread_mutex_lock(&app->mutex);
		if (app->savedState != NULL)
		{
			free(app->savedState);
			app->savedState = NULL;
			app->savedStateSize = 0;
		}
		pthread_mutex_unlock(&app->mutex);
	}

	int AndroidAppReadCommand(androidApp_t* app)
	{
		int command;
		if (read(app->messageRead, &command, sizeof(command)) == sizeof(command))
		{
			switch (command)
			{
				case APP_COMMAND_SAVE_STATE:
				{
					FreeSavedState(app);
					break;
				}
			}
			return command;
		}
		else
		{

		}
		return -1;
	}

	void AndroidAppPreExecutionCommand(androidApp_t* app, int command)
	{
		switch (command)
		{
			case APP_COMMAND_INPUT_CHANGED:
			{
				pthread_mutex_lock(&app->mutex);
				if (app->inputQueue != NULL)
				{
					AInputQueue_detachLooper(app->inputQueue);
				}
				app->inputQueue = app->pendingInputQueue;
				if (app->inputQueue != NULL)
				{
					AInputQueue_attachLooper(app->inputQueue,
						app->looper, LOOPER_ID_INPUT, NULL,
						&app->inputPollSource);
				}
				pthread_cond_broadcast(&app->condition);
				pthread_mutex_unlock(&app->mutex);
				break;
			}

			case APP_COMMAND_INIT_WINDOW:
			{
				pthread_mutex_lock(&app->mutex);
				app->window = app->pendingWindow;
				pthread_cond_broadcast(&app->condition);
				pthread_mutex_unlock(&app->mutex);
				break;
			}
			case APP_COMMAND_TERMINATE_WINDOW:
			{
				pthread_cond_broadcast(&app->condition);
				break;
			}

			case APP_COMMAND_CONFIG_CHANGED:
			{
				AConfiguration_fromAssetManager(app->config,
					app->nativeActivity->assetManager);
				//print_curre
				break;
			}

			case APP_COMMAND_DESTROY:
			{
				app->destroyRequested = true;
				break;
			}

			case APP_COMMAND_RESUME:
			case APP_COMMAND_START:
			case APP_COMMAND_PAUSE:
			case APP_COMMAND_STOP:
			{
				pthread_mutex_lock(&app->mutex);
				app->activityState = command;
				pthread_cond_broadcast(&app->condition);
				pthread_mutex_unlock(&app->mutex);
				break;
			}
		}
	}

	void AndroidAppPostExecutionCommand(androidApp_t* app, int command)
	{
		switch (command)
		{
			case APP_COMMAND_TERMINATE_WINDOW:
			{
				pthread_mutex_lock(&app->mutex);
				app->window = NULL;
				pthread_cond_broadcast(&app->condition);
				pthread_mutex_unlock(&app->mutex);
				break;
			}

			case APP_COMMAND_SAVE_STATE:
			{
				pthread_mutex_lock(&app->mutex);
				app->stateSaved = 1;
				pthread_cond_broadcast(&app->condition);
				pthread_mutex_unlock(&app->mutex);
				break;
			}

			case APP_COMMAND_RESUME:
			{
				FreeSavedState(app);
				break;
			}
		}
	}

	void AndroidAppDestroy(androidApp_t* app)
	{
		FreeSavedState(app);
		pthread_mutex_lock(&app->mutex);
		if (app->inputQueue != NULL)
		{
			AInputQueue_detachLooper(app->inputQueue);
		}
		AConfiguration_delete(app->config);
		app->destroyed = 1;
		pthread_cond_broadcast(&app->condition);
		pthread_mutex_unlock(&app->mutex);
	}

	void ProcessInput(androidApp_t* app, android_poll_source* pollSource)
	{
		/*AInputEvent* inputEvent = NULL;
		while (AInputQueue_getEvent(app->inputQueue, *inputEvent) >= 0)
		{
			if (AInputQueue_preDispatchEvent(app->inputQueue, inputEvent))
			{
				continue;
			}
			int handled;
			if (app->OnInputEvent != NULL)
			{
				handled = app->OnInputEvent(app, inputEvent);
			}
			AInputQueue_finishEvent(app->inputQueue, inputEvent, handled);
		}*/
	}

	void ProcessCommand(androidApp_t* app, android_poll_source* pollSource)
	{
		int command = AndroidAppReadCommand(app);
		AndroidAppPreExecutionCommand(app, command);
		if (app->OnAppCommand != NULL)
		{
			app->OnAppCommand(app, command);
		}
		AndroidAppPostExecutionCommand(app, command);
	}

	static void* AndroidAppEntry(void* parameter)
	{
		/*androidApp* app = (androidApp*)parameter;

		app->config = AConfiguration_new();
		AConfiguration_fromAssetManager(app->config, app->nativeActivity->assetManager);

		app->commandPollSource.id = LOOPER_ID_MAIN;
		app->commandPollSource.app = app;
		app->commandPollSource.process = ProcessCommand;

		app->inputPollSource.id = LOOPER_ID_INPUT;
		app->inputPollSource.app = app;
		app->inputPollSource.process = ProcessInput;

		ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
		ALooper_addFd(looper, app->messageRead, LOOPER_ID_MAIN, ALOOPER_EVENT_INPUT, NULL,
			&app->commandPollSource);
		app->looper = looper;

		pthread_mutex_lock(&app->mutex);
		app->running = true;
		pthread_cond_broadcast(&app->condition);
		pthread_mutex_unlock(&app->mutex);

		//AndroidMain(app);
		AndroidAppDestroy(app);
		return NULL;*/
	}

	static androidApp_t* AndroidAppCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize)
	{
		androidApp_t* app = new androidApp_t();
		app->nativeActivity = activity;

		pthread_mutex_init(&app->mutex, NULL);
		pthread_cond_init(&app->condition, NULL);

		if (savedState != NULL)
		{
			app->savedState = malloc(savedStateSize);
			app->savedStateSize = savedStateSize;
			memcpy(app->savedState, savedState, savedStateSize);
		}

		int messagePipe[2];
		if (pipe(messagePipe))
		{
			//could not create pipe
			return NULL;
		}

		app->messageRead = messagePipe[0];
		app->messageWrite = messagePipe[1];

		pthread_attr_t attribute;
		pthread_attr_init(&attribute);
		pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_DETACHED);
		pthread_create(&app->thread, &attribute, AndroidAppEntry, app);

		pthread_mutex_lock(&app->mutex);

		while (!app->running)
		{
			pthread_cond_wait(&app->condition, &app->mutex);
		}

		pthread_cond_wait(&app->condition, &app->mutex);

		return app;
	}
	
	static void AndroidAppWriteCommand(androidApp_t* app, int command )
	{
		if (write(app->messageWrite, &command, sizeof(command)) != sizeof(command))
		{
		   //failure writing androidApp, use strerror(errno)
		}
			
	}

	static void AndroidAppSetInput(androidApp_t* app, AInputQueue* inputQueue)
	{
		pthread_mutex_lock(&app->mutex);
		app->pendingInputQueue = inputQueue;
		AndroidAppWriteCommand(app, APP_COMMAND_INPUT_CHANGED);

		while (app->inputQueue != app->pendingInputQueue)
		{
			pthread_cond_wait(&app->condition, &app->mutex);
		}

		pthread_mutex_unlock(&app->mutex);
	}

	static void AndroidAppSetWindow(androidApp_t* app, ANativeWindow* window)
	{
		pthread_mutex_lock(&app->mutex);
		if (app->pendingWindow != NULL)
		{
			AndroidAppWriteCommand(app, APP_COMMAND_TERMINATE_WINDOW);
		}
		
		app->pendingWindow = window;
		if (window != NULL)
		{
			AndroidAppWriteCommand(app, APP_COMMAND_INIT_WINDOW);
		}

		while (app->window != app->pendingWindow)
		{
			pthread_cond_wait(&app->condition, &app->mutex);
		}

		pthread_mutex_unlock(&app->mutex);
	}

	static void AndroidAppSetActivityState(androidApp_t* app, int command)
	{
		pthread_mutex_lock(&app->mutex);
		AndroidAppWriteCommand(app, command);

		while (app->activityState != command)
		{
			pthread_cond_wait(&app->condition, &app->mutex);
		}

		pthread_mutex_unlock(&app->mutex);
	}

	static void OnDestroy(ANativeActivity* activity)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		pthread_mutex_lock(&app->mutex);
		AndroidAppWriteCommand(app, APP_COMMAND_DESTROY);

		while (!app->destroyed)
		{
			pthread_cond_wait(&app->condition, &app->mutex);
		}
		pthread_mutex_unlock(&app->mutex);

		close(app->messageRead);
		close(app->messageWrite);
		pthread_cond_destroy(&app->condition);
		pthread_mutex_destroy(&app->mutex);
		free(app);
	}

	static void OnStart(ANativeActivity* activity)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		AndroidAppSetActivityState(app, APP_COMMAND_START);

	}

	static void OnResume(ANativeActivity* activity)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		AndroidAppSetActivityState(app, APP_COMMAND_RESUME);
	}

	static void* OnSaveInstanceState(ANativeActivity* activity, size_t* outLength)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		void* state = NULL;

		pthread_mutex_lock(&app->mutex);
		app->stateSaved = 0;
		AndroidAppWriteCommand(app, APP_COMMAND_SAVE_STATE);

		while (!app->stateSaved)
		{
			pthread_cond_wait(&app->condition, &app->mutex);
		}

		if (app->stateSaved != NULL)
		{
			state = app->savedState;
			*outLength = app->savedStateSize;
			app->savedState = NULL;
			app->savedStateSize = 0;
		}

		pthread_mutex_unlock(&app->mutex);

		return state;
	}

	static void OnPause(ANativeActivity* activity)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		AndroidAppSetActivityState(app, APP_COMMAND_PAUSE);
	}

	static void OnStop(ANativeActivity* activity)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		AndroidAppSetActivityState(app, APP_COMMAND_STOP);
	}

	static void OnConfigurationChanged(ANativeActivity* activity)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		AndroidAppWriteCommand(app, APP_COMMAND_CONFIG_CHANGED);
	}

	static void OnLowMemory(ANativeActivity* activity)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		AndroidAppWriteCommand(app, APP_COMMAND_LOW_MEMORY);
	}

	static void OnWindowFocusedChanged(ANativeActivity* activity, int focused)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		AndroidAppWriteCommand(app, focused ? APP_COMMAND_GAINED_FOCUS : APP_COMMAND_LOST_FOCUS);
	}

	static void OnNativeWindowCreated(ANativeActivity* activity, ANativeWindow* nativeWindow)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		AndroidAppSetWindow(app, nativeWindow);
	}

	static void OnNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* nativeWindow)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		AndroidAppSetWindow(app, NULL);
	}

	static void OnInputQueueCreated(ANativeActivity* activity, AInputQueue* queue)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		AndroidAppSetInput(app, queue);
	}

	static void OnInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue)
	{
		androidApp_t* app = static_cast<androidApp_t*>(activity->instance);
		AndroidAppSetInput(app, NULL);
	}

	void SetInternalCallbacks()
	{
		GetInstance()->activity->callbacks->onDestroy = &windowManager::OnDestroy;
		GetInstance()->activity->callbacks->onStart = &windowManager::OnStart;
		GetInstance()->activity->callbacks->onResume = &windowManager::OnResume;
		GetInstance()->activity->callbacks->onSaveInstanceState = &windowManager::OnSaveInstanceState;
		GetInstance()->activity->callbacks->onPause = &windowManager::OnPause;
		GetInstance()->activity->callbacks->onStop = &windowManager::OnStop;
		GetInstance()->activity->callbacks->onConfigurationChanged = &windowManager::OnConfigurationChanged;
		GetInstance()->activity->callbacks->onLowMemory = &windowManager::OnLowMemory;
		GetInstance()->activity->callbacks->onWindowFocusChanged = &windowManager::OnWindowFocusedChanged;
		GetInstance()->activity->callbacks->onNativeWindowCreated = &windowManager::OnNativeWindowCreated;
		GetInstance()->activity->callbacks->onNativeWindowDestroyed = &windowManager::OnNativeWindowDestroyed;
		GetInstance()->activity->callbacks->onInputQueueCreated = &windowManager::OnInputQueueCreated;
		GetInstance()->activity->callbacks->onInputQueueDestroyed = &windowManager::OnInputQueueDestroyed;
	}

	EGLSurface				surface;
	EGLDisplay				display;
	EGLint					EGLmajor;
	EGLint					EGLminor;
	static GLboolean		initialized;
	static windowManager*	instance;
	ANativeActivity*		activity;
	savedState_t*			savedState;
	size_t					savedStateSize;

	std::vector< window_t* >	windowList;
	GLuint					displayResolution[ 2 ];
	
};

void ANativeActivity_onCreate(ANativeActivity* activity,
	void* savedState, size_t savedStateSize)
{
	windowManager::Initialize(activity, savedState, savedStateSize);
}

extern int main();

windowManager* windowManager::instance = NULL;
GLboolean	windowManager::initialized = false;

#endif
