//
//  scintillaeditorview_uidesc.cpp
//  scintilla-example
//
//  Created by Arne Scheffler on 20.01.18.
//

#include "scintillaeditorview.h"
#include "vstgui/uidescription/detail/uiviewcreatorattributes.h"
#include "vstgui/uidescription/iviewcreator.h"
#include "vstgui/uidescription/uiattributes.h"
#include "vstgui/uidescription/uiviewcreator.h"
#include "vstgui/uidescription/uiviewfactory.h"

//------------------------------------------------------------------------
namespace VSTGUI {
using UIViewCreator::stringToColor;
using UIViewCreator::stringToBitmap;
using UIViewCreator::bitmapToString;
using UIViewCreator::colorToString;

//-----------------------------------------------------------------------------
class ScintillaEditorViewCreator : public ViewCreatorAdapter
{
public:
	ScintillaEditorViewCreator () { UIViewFactory::registerViewCreator (*this); }
	IdStringPtr getViewName () const override { return "ScintillaEditorView"; }
	IdStringPtr getBaseViewName () const override { return UIViewCreator::kCControl; }
	UTF8StringPtr getDisplayName () const override { return "Scintilla Editor View"; }
	CView* create (const UIAttributes& attributes, const IUIDescription* description) const override
	{
		return new ScintillaEditorView ();
	}
};
ScintillaEditorViewCreator __gScintillaEditorViewCreator;

//------------------------------------------------------------------------
} // VSTGUI
