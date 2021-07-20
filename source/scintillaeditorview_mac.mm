//
//  scintillaeditorview_mac.mm
//  scintilla-example
//
//  Created by Arne Scheffler on 20.01.18.
//

#import "scintillaeditorview.h"
#import "vstgui/lib/cframe.h"
#import "vstgui/lib/dispatchlist.h"
#import "vstgui/lib/platform/platform_macos.h"
#import <Scintilla/ScintillaView.h>

//------------------------------------------------------------------------
@interface VSTGUI_ScintillaView_Delegate : NSObject <ScintillaNotificationProtocol>
@property (readwrite, nonatomic) VSTGUI::ScintillaEditorView::Impl* impl;
@end

//------------------------------------------------------------------------
namespace VSTGUI {

//------------------------------------------------------------------------
namespace {

//------------------------------------------------------------------------
void releaseObject (NSObject* obj)
{
#if !__has_feature(objc_arc)
	[obj release];
#endif
}

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
struct ScintillaEditorView::Impl
{
	ScintillaView* view {nil};
	VSTGUI_ScintillaView_Delegate* delegate {nil};
	DispatchList<IScintillaListener*> listeners;
};

//------------------------------------------------------------------------
ScintillaEditorView::ScintillaEditorView () : CView (CRect (0, 0, 0, 0))
{
	impl = std::make_unique<Impl> ();
	impl->delegate = [VSTGUI_ScintillaView_Delegate new];
	impl->delegate.impl = impl.get ();
	impl->view = [[ScintillaView alloc] initWithFrame:NSMakeRect (0, 0, 10, 10)];
	impl->view.delegate = impl->delegate;
	init ();
}

//------------------------------------------------------------------------
ScintillaEditorView::~ScintillaEditorView () noexcept
{
	if (impl->view)
		impl->view.delegate = nil;
	releaseObject (impl->view);
	releaseObject (impl->delegate);
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
	if (CView::attached (parent))
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
	return CView::removed (parent);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setViewSize (const CRect& rect, bool invalid)
{
	if (isAttached ())
	{
		CPoint p;
		localToFrame (p);
		NSRect frameRect;
		frameRect.origin.x = rect.left + p.x;
		frameRect.origin.y = rect.top + p.y;
		frameRect.size.width = rect.getWidth ();
		frameRect.size.height = rect.getHeight ();
		impl->view.frame = frameRect;
	}
	CView::setViewSize (rect, false);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setMouseEnabled (bool enable)
{
	[impl->view setEditable:enable];
	CView::setMouseEnabled (enable);
}

//------------------------------------------------------------------------
void ScintillaEditorView::platformSetBackgroundColor (const CColor& color)
{
	NSColor* nsColor = [NSColor colorWithRed:color.red / 255.
	                                   green:color.green / 255.
	                                    blue:color.blue / 255.
	                                   alpha:color.alpha / 255.];
	impl->view.scrollView.contentView.backgroundColor = nsColor;
}

//------------------------------------------------------------------------
intptr_t ScintillaEditorView::sendMessage (uint32_t message, uintptr_t wParam,
                                           intptr_t lParam) const
{
	return [impl->view message:message wParam:wParam lParam:lParam];
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

//------------------------------------------------------------------------
@implementation VSTGUI_ScintillaView_Delegate

//------------------------------------------------------------------------
- (void)notification:(SCNotification*)notification
{
	self.impl->listeners.forEach (
	    [&] (auto& listener) { listener->onScintillaNotification (notification); });
}

@end
