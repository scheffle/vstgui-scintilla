//
//  ScintillaView.cpp
//  scintilla-example
//
//  Created by Arne Scheffler on 20.01.18.
//

#import "scintillaeditorview.h"
#import "vstgui/lib/cframe.h"
#import "vstgui/lib/platform/platform_macos.h"
#import <Scintilla/ScintillaView.h>

//------------------------------------------------------------------------
namespace VSTGUI {

//------------------------------------------------------------------------
namespace {

//------------------------------------------------------------------------
void releaseObject (NSObject* obj)
{
	// we can check for ARC here and just do nothing
	[obj release];
}

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
struct ScintillaEditorView::Impl
{
	ScintillaView* view {nil};
};

//------------------------------------------------------------------------
ScintillaEditorView::ScintillaEditorView () : CControl (CRect (0, 0, 0, 0))
{
	impl = std::make_unique<Impl> ();
	impl->view = [[ScintillaView alloc] initWithFrame:NSMakeRect (0, 0, 10, 10)];
}

//------------------------------------------------------------------------
ScintillaEditorView::~ScintillaEditorView () noexcept
{
	releaseObject (impl->view);
}

//------------------------------------------------------------------------
void ScintillaEditorView::draw (CDrawContext* pContext)
{
	setDirty (false);
}

//------------------------------------------------------------------------
bool ScintillaEditorView::attached (CView* parent)
{
	auto parentFrame = parent->getFrame ();
	if (!parentFrame)
		return false;
	auto cocoaFrame = dynamic_cast<ICocoaPlatformFrame*> (parentFrame->getPlatformFrame ());
	if (!cocoaFrame)
		return false;
	if (CControl::attached (parent))
	{
		setViewSize (getViewSize (), false);
		[cocoaFrame->getNSView () addSubview:impl->view];
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool ScintillaEditorView::removed (CView* parent)
{
	[impl->view removeFromSuperview];
	return CControl::removed (parent);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setViewSize (const CRect& rect, bool invalid)
{
	CPoint p;
	localToFrame (p);
	NSRect frameRect;
	frameRect.origin.x = rect.left + p.x;
	frameRect.origin.y = rect.top + p.y;
	frameRect.size.width = getWidth ();
	frameRect.size.height = getHeight ();
	impl->view.frame = frameRect;
	CControl::setViewSize (rect, false);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setText (UTF8StringPtr text)
{
	sendMessage (SCI_SETTEXT, 0, reinterpret_cast<intptr_t> (text));
	valueChanged ();
}

//------------------------------------------------------------------------
intptr_t ScintillaEditorView::sendMessage (uint32_t message, uintptr_t wParam, intptr_t lParam)
{
	return [impl->view message:message wParam:wParam lParam:lParam];
}

//------------------------------------------------------------------------
} // VSTGUI
