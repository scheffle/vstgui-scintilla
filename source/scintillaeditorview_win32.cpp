// This file is part of VSTGUI. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this
// distribution and at http://github.com/steinbergmedia/vstgui/LICENSE

#include "Scintilla.h"
#include "scintillaeditorview.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/dispatchlist.h"
#include "vstgui/lib/iscalefactorchangedlistener.h"
#include "vstgui/lib/platform/platform_win32.h"
#include "vstgui/lib/platform/win32/win32factory.h"

#include <cassert>

//------------------------------------------------------------------------
namespace VSTGUI {

//------------------------------------------------------------------------
struct Globals
{
	static Globals& instance ()
	{
		static Globals g;
		return g;
	}

	bool available () const { return scintillaModule != nullptr; }

private:
	HMODULE scintillaModule {nullptr};

	Globals ()
	{
		auto hInstance = getPlatformFactory ().asWin32Factory ()->getInstance ();
		std::string path;
		path.resize (MAX_PATH);
		auto size = GetModuleFileNameA (hInstance, path.data (), static_cast<DWORD> (path.size ()));
		path.resize (size);
		auto it = path.find_last_of ("\\");
		if (it != std::string::npos)
		{
			path.resize (it);
			path += "\\Scintilla.dll";
			scintillaModule = LoadLibraryA (path.data ());
		}
	}
	~Globals () noexcept { FreeLibrary (scintillaModule); }
};

//------------------------------------------------------------------------
struct HWNDWrapper final
{
	using WindowProcFunc =
	    std::function<LONG_PTR (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)>;

	HWNDWrapper (HINSTANCE instance) : instance (instance) {}

	~HWNDWrapper () noexcept
	{
		if (window)
		{
			SetWindowLongPtr (window, GWLP_USERDATA, (__int3264) (LONG_PTR) nullptr);
			DestroyWindow (window);
		}
		destroyWindowClass ();
	}

	bool create (const TCHAR* title, const CRect& frame, HWND parent, DWORD exStyle = 0,
	             DWORD style = WS_CHILD)
	{
		if (!initWindowClass ())
			return false;
		window = CreateWindowEx (
		    exStyle, MAKEINTATOM (windowClassAtom), title, style, static_cast<int> (frame.left),
		    static_cast<int> (frame.top), static_cast<int> (frame.getWidth ()),
		    static_cast<int> (frame.getHeight ()), parent, nullptr, instance, nullptr);
		if (!window)
		{
			auto error = GetLastError ();
			printf ("%d", error);
			return false;
		}
		SetWindowLongPtr (window, GWLP_USERDATA, (__int3264) (LONG_PTR)this);
		return true;
	}

	void setWindowProc (WindowProcFunc&& func) { windowProc = std::move (func); }

	void setSize (const CRect& r)
	{
		if (!window)
			return;
		SetWindowPos (window, HWND_TOP, static_cast<int> (r.left), static_cast<int> (r.top),
		              static_cast<int> (r.getWidth ()), static_cast<int> (r.getHeight ()),
		              SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_DEFERERASE);
	}

	void show (bool state) { ShowWindow (window, state ? SW_SHOW : SW_HIDE); }
	void setEnabled (bool state) { EnableWindow (window, state); }

	HWND getHWND () const { return window; }

private:
	bool initWindowClass ()
	{
		assert (instance != nullptr);

		if (windowClassAtom != 0)
			return true;

		std::wstring windowClassName;
		windowClassName = L"VSTGUI Scintilla Window ";
		windowClassName += std::to_wstring (reinterpret_cast<uint64_t> (this));

		WNDCLASS windowClass;
		windowClass.style = CS_GLOBALCLASS; //|CS_OWNDC; // add Private-DC constant

		windowClass.lpfnWndProc = WindowProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = instance;
		windowClass.hIcon = 0;

		windowClass.hCursor = LoadCursor (NULL, IDC_ARROW);
		windowClass.hbrBackground = 0;

		windowClass.lpszMenuName = 0;
		windowClass.lpszClassName = windowClassName.data ();
		windowClassAtom = RegisterClass (&windowClass);
		return windowClassAtom != 0;
	}

	void destroyWindowClass ()
	{
		if (windowClassAtom == 0)
			return;
		UnregisterClass (MAKEINTATOM (windowClassAtom), instance);
		windowClassAtom = 0;
	}

	static LONG_PTR WINAPI WindowProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		auto instance = reinterpret_cast<HWNDWrapper*> (GetWindowLongPtr (hwnd, GWLP_USERDATA));
		if (instance && instance->windowProc)
			return instance->windowProc (hwnd, message, wParam, lParam);
		return DefWindowProc (hwnd, message, wParam, lParam);
	}

	WindowProcFunc windowProc;
	HWND window {nullptr};
	HINSTANCE instance {nullptr};
	ATOM windowClassAtom {0};
};

//------------------------------------------------------------------------
struct ScaleFactorChangeListener : IScaleFactorChangedListener
{
	using CallbackFunc = std::function<void (CFrame*, double)>;

	CallbackFunc func;
	void onScaleFactorChanged (CFrame* frame, double newScaleFactor) override
	{
		if (func)
			func (frame, newScaleFactor);
	}
};

//------------------------------------------------------------------------
struct ScintillaEditorView::Impl
{
	using DirectFunc = int (__cdecl*) (void*, UINT, WPARAM, LPARAM);

	DispatchList<IScintillaListener*> listeners;
	HWND control;
	std::unique_ptr<HWNDWrapper> window;
	std::unique_ptr<HWNDWrapper> invisibleWindow;
	DirectFunc directFn;
	void* directPtr;
	ScaleFactorChangeListener scaleFactorChangeListener;
};

//------------------------------------------------------------------------
ScintillaEditorView::ScintillaEditorView () : CView (CRect (0, 0, 0, 0))
{
	if (!Globals::instance ().available ())
		return;

	impl = std::make_unique<Impl> ();

	auto hInstance = getPlatformFactory ().asWin32Factory ()->getInstance ();
	impl->invisibleWindow = std::make_unique<HWNDWrapper> (hInstance);
	if (!impl->invisibleWindow->create (L"VSTGUI Scintilla Invisible", {0., 0., 100., 100.},
	                                    nullptr, WS_EX_APPWINDOW, 0))
	{
		impl = nullptr;
		return;
	}
	impl->window = std::make_unique<HWNDWrapper> (hInstance);

	impl->control = CreateWindowExA (
	    0, "Scintilla", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN, 0, 0, 100, 100,
	    impl->invisibleWindow->getHWND (), nullptr, hInstance, nullptr);

	if (!impl->control)
	{
		impl = nullptr;
		return;
	}

	impl->directFn = reinterpret_cast<Impl::DirectFunc> (
	    SendMessage (impl->control, SCI_GETDIRECTFUNCTION, 0, 0));
	impl->directPtr =
	    reinterpret_cast<void*> (SendMessage (impl->control, SCI_GETDIRECTPOINTER, 0, 0));

	sendMessage (SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITERETAIN, 0);

	impl->scaleFactorChangeListener.func = [this] (auto, auto) {
		updateMarginsColumns ();
		setViewSize (getViewSize (), false);
	};

	impl->window->setWindowProc ([this] (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
		if (message == WM_NOTIFY)
		{
			auto notification = reinterpret_cast<SCNotification*> (lParam);
			impl->listeners.forEach (
			    [&] (auto& listener) { listener->onScintillaNotification (notification); });
		}
		return DefWindowProc (hwnd, message, wParam, lParam);
	});

	init ();
}

//------------------------------------------------------------------------
ScintillaEditorView::~ScintillaEditorView () noexcept
{
	if (!impl)
		return;
	if (impl->control)
		DestroyWindow (impl->control);
}

//------------------------------------------------------------------------
void ScintillaEditorView::draw (CDrawContext* pContext)
{
	setDirty (false);
}

//------------------------------------------------------------------------
bool ScintillaEditorView::attached (CView* parent)
{
	if (!impl)
		return false;

	auto parentFrame = parent->getFrame ();
	if (!parentFrame)
		return false;
	auto win32Frame = dynamic_cast<IWin32PlatformFrame*> (parentFrame->getPlatformFrame ());
	if (!win32Frame)
		return false;
	if (CView::attached (parent))
	{
		if (!impl->window->create (L"VSTGUI Scintilla Wrapper", getViewSize (),
		                           win32Frame->getHWND (), 0,
		                           WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE))
			return false;
		setViewSize (getViewSize (), false);
		SetParent (impl->control, impl->window->getHWND ());
		getFrame ()->registerScaleFactorChangedListeneer (&impl->scaleFactorChangeListener);
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool ScintillaEditorView::removed (CView* parent)
{
	if (!impl)
		return false;

	getFrame ()->unregisterScaleFactorChangedListeneer (&impl->scaleFactorChangeListener);
	SetParent (impl->control, impl->invisibleWindow->getHWND ());
	impl->window = nullptr;
	return CView::removed (parent);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setViewSize (const CRect& rect, bool invalid)
{
	if (isAttached () && impl)
	{
		auto r = translateToGlobal (rect);
		impl->window->setSize (r);
		SetWindowPos (impl->control, nullptr, 0, 0, static_cast<int> (r.getWidth ()),
		              static_cast<int> (r.getHeight ()),
		              SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
	}
	CView::setViewSize (rect, false);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setMouseEnabled (bool enable)
{
	CView::setMouseEnabled (enable);
	if (impl)
		EnableWindow (impl->control, enable);
}

//------------------------------------------------------------------------
void ScintillaEditorView::platformSetBackgroundColor (const CColor& color)
{
}

//------------------------------------------------------------------------
intptr_t ScintillaEditorView::sendMessage (uint32_t message, uintptr_t wParam,
                                           intptr_t lParam) const
{
	return (impl && impl->directFn) ? impl->directFn (impl->directPtr, message, wParam, lParam) : 0;
}

//------------------------------------------------------------------------
void ScintillaEditorView::registerListener (IScintillaListener* listener)
{
	impl->listeners.add (listener);
}

//------------------------------------------------------------------------
void ScintillaEditorView::unregisterListener (IScintillaListener* listener)
{
	impl->listeners.remove (listener);
}

//------------------------------------------------------------------------
} // VSTGUI
