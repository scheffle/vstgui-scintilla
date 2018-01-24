//
//  scintillaeditorview_mac.mm
//  scintilla-example
//
//  Created by Arne Scheffler on 20.01.18.
//

#include "Scintilla.h"
#include "scintillaeditorview.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/dispatchlist.h"
#include "vstgui/lib/iscalefactorchangedlistener.h"
#include "vstgui/lib/platform/platform_win32.h"

extern void* hInstance;

//------------------------------------------------------------------------
namespace VSTGUI {

static const WCHAR* gWindowClassName = L"VSTGUIScintillaInvisibleWindowClass";

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
	HMODULE scintillaModule;
	WNDCLASSEX windowClass {};

	Globals ()
	{
		scintillaModule = LoadLibraryA ("SciLexer.dll");

		windowClass.cbSize = sizeof (WNDCLASSEX);
		windowClass.style = CS_DBLCLKS;
		windowClass.lpfnWndProc = DefWindowProc;
		windowClass.hInstance = static_cast<HINSTANCE> (hInstance);
		windowClass.hCursor = LoadCursor (static_cast<HINSTANCE> (hInstance), IDC_ARROW);
		windowClass.hbrBackground = nullptr;
		windowClass.lpszClassName = gWindowClassName;

		RegisterClassEx (&windowClass);
	}
	~Globals () noexcept
	{
		UnregisterClass (gWindowClassName, static_cast<HINSTANCE> (hInstance));
		FreeLibrary (scintillaModule);
	}
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
	HWND invisibleWindow;
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

	impl->invisibleWindow =
	    CreateWindowEx (WS_EX_APPWINDOW, gWindowClassName, nullptr, 0, 0, 0, 100, 100, nullptr,
	                    nullptr, static_cast<HINSTANCE> (hInstance), nullptr);

	impl->control = CreateWindowExA (
	    0, "Scintilla", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN, 0, 0, 100, 100,
	    impl->invisibleWindow, nullptr, static_cast<HINSTANCE> (hInstance), nullptr);

	if (!impl->control)
		return;

	impl->directFn = reinterpret_cast<Impl::DirectFunc> (
	    SendMessage (impl->control, SCI_GETDIRECTFUNCTION, 0, 0));
	impl->directPtr =
	    reinterpret_cast<void*> (SendMessage (impl->control, SCI_GETDIRECTPOINTER, 0, 0));

	sendMessage (SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITERETAIN, 0);

	impl->scaleFactorChangeListener.func = [this] (auto, auto) {
		setViewSize (getViewSize (), false);
	};
}

//------------------------------------------------------------------------
ScintillaEditorView::~ScintillaEditorView () noexcept
{
	if (!impl)
		return;
	if (impl->control)
		DestroyWindow (impl->control);
	if (impl->invisibleWindow)
		DestroyWindow (impl->invisibleWindow);
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
		setViewSize (getViewSize (), false);
		SetParent (impl->control, win32Frame->getHWND ());
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
	SetParent (impl->control, impl->invisibleWindow);
	return CView::removed (parent);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setViewSize (const CRect& rect, bool invalid)
{
	if (isAttached () && impl)
	{
		auto r = translateToGlobal (rect);
		SetWindowPos (impl->control, nullptr, static_cast<int> (r.left), static_cast<int> (r.top),
		              static_cast<int> (r.getWidth ()), static_cast<int> (r.getHeight ()),
		              SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
	}
	CView::setViewSize (rect, false);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setMouseEnabled (bool enable)
{
	CView::setMouseEnabled (enable);
}

//------------------------------------------------------------------------
void ScintillaEditorView::platformSetBackgroundColor (const CColor& color)
{
}

//------------------------------------------------------------------------
intptr_t ScintillaEditorView::sendMessage (uint32_t message, uintptr_t wParam,
                                           intptr_t lParam) const
{
	return impl ? impl->directFn (impl->directPtr, message, wParam, lParam) : 0;
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
