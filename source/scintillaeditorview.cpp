//
//  scintillaeditorview_uidesc.cpp
//  scintilla-example
//
//  Created by Arne Scheffler on 20.01.18.
//

#include "scintillaeditorview.h"
#include "vstgui/lib/platform/iplatformfont.h"
#include "vstgui/uidescription/detail/uiviewcreatorattributes.h"
#include "vstgui/uidescription/iviewcreator.h"
#include "vstgui/uidescription/uiattributes.h"
#include "vstgui/uidescription/uiviewcreator.h"
#include "vstgui/uidescription/uiviewfactory.h"

#include "Scintilla.h"

//------------------------------------------------------------------------
namespace VSTGUI {

//------------------------------------------------------------------------
void ScintillaEditorView::setFont (const SharedPointer<CFontDesc>& _font)
{
	font = _font;
	if (!font)
		return;
	bool isBold = font->getStyle () & kBoldFace;
	bool isItalic = font->getStyle () & kItalicFace;
	const auto& fontName = font->getName ();
	auto fontSize = font->getSize () * static_cast<CCoord> (SC_FONT_SIZE_MULTIPLIER);
	for (int i = 0; i < 128; i++)
	{
		sendMessageT (SCI_STYLESETFONT, i, fontName.data ());
		sendMessage (SCI_STYLESETSIZEFRACTIONAL, i, fontSize);
		sendMessage (SCI_STYLESETBOLD, i, isBold);
		sendMessage (SCI_STYLESETITALIC, i, isItalic);
	}
}

//------------------------------------------------------------------------
SharedPointer<CFontDesc> ScintillaEditorView::getFont () const
{
	return font;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setBackgroundColor (const CColor& color)
{
	auto sc = toScintillaColor (color);
	for (auto index = 0; index <= STYLE_DEFAULT; ++index)
		sendMessage (SCI_STYLESETBACK, index, sc);
	platformSetBackgroundColor (color);
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getBackgroundColor () const
{
	return fromScintillaColor (sendMessage (SCI_STYLEGETBACK, STYLE_DEFAULT, 0));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setSelectionBackgroundColor (const CColor& color)
{
	bool enable = color != kTransparentCColor;
	sendMessage (SCI_SETSELBACK, enable, toScintillaColor (color));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setSelectionForegroundColor (const CColor& color)
{
	bool enable = color != kTransparentCColor;
	sendMessage (SCI_SETSELFORE, enable, toScintillaColor (color));
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getSelectionBackgroundColor () const
{
	return selectionBackgroundColor;
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getSelectionForegroundColor () const
{
	return selectionForegroundColor;
}

//------------------------------------------------------------------------
bool ScintillaEditorView::canUndo () const
{
	return sendMessage (SCI_CANUNDO, 0, 0) != 0;
}

//------------------------------------------------------------------------
bool ScintillaEditorView::canRedo () const
{
	return sendMessage (SCI_CANREDO, 0, 0) != 0;
}

//------------------------------------------------------------------------
void ScintillaEditorView::undo ()
{
	sendMessage (SCI_UNDO, 0, 0);
}

//------------------------------------------------------------------------
void ScintillaEditorView::redo ()
{
	sendMessage (SCI_REDO, 0, 0);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setText (UTF8StringPtr text)
{
	sendMessageT (SCI_SETTEXT, 0, text);
}

//------------------------------------------------------------------------
auto ScintillaEditorView::getSelection () const -> Selection
{
	Selection selection {};
	selection.start = sendMessage (SCI_GETSELECTIONSTART, 0, 0);
	selection.end = sendMessage (SCI_GETSELECTIONEND, 0, 0);
	return selection;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setSelection (Selection selection)
{
	sendMessage (SCI_SETSELECTIONSTART, selection.start, 0);
	sendMessage (SCI_SETSELECTIONEND, selection.end, 0);
}

//------------------------------------------------------------------------
int64_t ScintillaEditorView::findAndSelect (UTF8StringPtr searchString, uint32_t searchFlags)
{
	int sciSearchFlags = 0;
	if (searchFlags & MatchCase)
		sciSearchFlags |= SCFIND_MATCHCASE;
	if (searchFlags & WholeWord)
		sciSearchFlags |= SCFIND_WHOLEWORD;
	if (searchFlags & WordStart)
		sciSearchFlags |= SCFIND_WORDSTART;

	auto selection = getSelection ();

	if (!(searchFlags & Backwards))
		sendMessage (SCI_SETSELECTIONSTART, selection.end, 0);
	sendMessage (SCI_SEARCHANCHOR, 0, 0);

	intptr_t result;
	if (searchFlags & Backwards)
	{
		result = sendMessageT (SCI_SEARCHPREV, sciSearchFlags, searchString);
		if (result < 0 && searchFlags & Wrap)
		{
			auto textLength = sendMessage (SCI_GETTEXTLENGTH, 0, 0);
			sendMessage (SCI_SETSELECTIONSTART, textLength, 0);
			sendMessage (SCI_SETSELECTIONEND, textLength, 0);
			sendMessage (SCI_SEARCHANCHOR, 0, 0);
			result = sendMessageT (SCI_SEARCHPREV, sciSearchFlags, searchString);
		}
	}
	else
	{
		result = sendMessageT (SCI_SEARCHNEXT, sciSearchFlags, searchString);
		if (result < 0 && searchFlags & Wrap)
		{
			sendMessage (SCI_SETSELECTIONSTART, 0, 0);
			sendMessage (SCI_SETSELECTIONEND, 0, 0);
			sendMessage (SCI_SEARCHANCHOR, 0, 0);
			result = sendMessageT (SCI_SEARCHNEXT, sciSearchFlags, searchString);
		}
	}
	if (result >= 0)
	{
		if (searchFlags & ScrollTo)
		{
			sendMessage (SCI_SCROLLCARET, 0, 0);
		}
		return result;
	}
	setSelection (selection);

	return -1;
}

//------------------------------------------------------------------------
bool ScintillaEditorView::replaceSelection (UTF8StringPtr string)
{
	sendMessageT (SCI_REPLACESEL, 0, string);
	return true;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
using UIViewCreator::stringToColor;
using UIViewCreator::stringToBitmap;
using UIViewCreator::bitmapToString;
using UIViewCreator::colorToString;
using UIViewCreator::pointToString;

static std::string kAttrEditorFont = "editor-font";
static std::string kAttrEditorMargin = "editor-margin";
static std::string kAttrLineNumberFontColor = "line-number-font-color";
static std::string kAttrLineNumberBackgroundColor = "line-number-background-color";
static std::string kAttrSelectionBackgroundColor = "selection-background-color";
static std::string kAttrSelectionForegroundColor = "selection-foreground-color";

//-----------------------------------------------------------------------------
class ScintillaEditorViewCreator : public ViewCreatorAdapter
{
public:
	ScintillaEditorViewCreator () { UIViewFactory::registerViewCreator (*this); }
	IdStringPtr getViewName () const override { return "ScintillaEditorView"; }
	IdStringPtr getBaseViewName () const override { return UIViewCreator::kCView; }
	UTF8StringPtr getDisplayName () const override { return "Scintilla Editor View"; }
	CView* create (const UIAttributes& attributes, const IUIDescription* description) const override
	{
		return new ScintillaEditorView ();
	}
	bool getAttributeNames (std::list<std::string>& attributeNames) const override
	{
		attributeNames.push_back (kAttrEditorMargin);
		// font
		attributeNames.push_back (kAttrEditorFont);
		// selection
		attributeNames.push_back (kAttrSelectionBackgroundColor);
		attributeNames.push_back (kAttrSelectionForegroundColor);
		// line-number
		attributeNames.push_back (kAttrLineNumberFontColor);
		attributeNames.push_back (kAttrLineNumberBackgroundColor);
		// other
		attributeNames.push_back (UIViewCreator::kAttrBackgroundColor);
		return true;
	}
	AttrType getAttributeType (const std::string& attributeName) const override
	{
		if (attributeName == kAttrEditorMargin)
			return kPointType;
		if (attributeName == kAttrEditorFont)
			return kFontType;
		if (attributeName == kAttrSelectionBackgroundColor)
			return kColorType;
		if (attributeName == kAttrSelectionForegroundColor)
			return kColorType;
		if (attributeName == kAttrLineNumberFontColor)
			return kColorType;
		if (attributeName == kAttrLineNumberBackgroundColor)
			return kColorType;
		if (attributeName == UIViewCreator::kAttrBackgroundColor)
			return kColorType;
		return kUnknownType;
	}
	bool apply (CView* view, const UIAttributes& attr, const IUIDescription* desc) const override
	{
		auto sev = dynamic_cast<ScintillaEditorView*> (view);
		if (!sev)
			return false;
		CColor color;
		if (stringToColor (attr.getAttributeValue (UIViewCreator::kAttrBackgroundColor), color,
		                   desc))
		{
			sev->setBackgroundColor (color);
		}
		if (stringToColor (attr.getAttributeValue (kAttrSelectionBackgroundColor), color, desc))
		{
			sev->setSelectionBackgroundColor (color);
		}
		if (stringToColor (attr.getAttributeValue (kAttrSelectionForegroundColor), color, desc))
		{
			sev->setSelectionForegroundColor (color);
		}
		if (stringToColor (attr.getAttributeValue (kAttrLineNumberFontColor), color, desc))
		{
			sev->sendMessage (SCI_STYLESETFORE, STYLE_LINENUMBER, toScintillaColor (color));
		}
		if (stringToColor (attr.getAttributeValue (kAttrLineNumberBackgroundColor), color, desc))
		{
			sev->sendMessage (SCI_STYLESETBACK, STYLE_LINENUMBER, toScintillaColor (color));
		}
		if (auto fontName = attr.getAttributeValue (kAttrEditorFont))
		{
			if (auto font = desc->getFont (fontName->data ()))
			{
				sev->setFont (font);
			}
		}
		CPoint p;
		if (attr.getPointAttribute (kAttrEditorMargin, p))
		{
			sev->sendMessage (SCI_SETMARGINLEFT, 0, p.x);
			sev->sendMessage (SCI_SETMARGINRIGHT, 0, p.y);
		}
		return true;
	}
	bool getAttributeValue (CView* view, const std::string& attName, std::string& stringValue,
	                        const IUIDescription* desc) const override
	{
		auto sev = dynamic_cast<ScintillaEditorView*> (view);
		if (!sev)
			return false;
		if (attName == kAttrEditorMargin)
		{
			CPoint p;
			p.x = sev->sendMessage (SCI_GETMARGINLEFT, 0, 0);
			p.y = sev->sendMessage (SCI_GETMARGINRIGHT, 0, 0);
			return pointToString (p, stringValue);
		}
		if (attName == kAttrEditorFont)
		{
			if (auto name = desc->lookupFontName (sev->getFont ()))
			{
				stringValue = name;
				return true;
			}
			return false;
		}
		if (attName == kAttrSelectionBackgroundColor)
		{
			auto color = sev->getSelectionBackgroundColor ();
			return colorToString (color, stringValue, desc);
		}
		if (attName == kAttrSelectionForegroundColor)
		{
			auto color = sev->getSelectionForegroundColor ();
			return colorToString (color, stringValue, desc);
		}
		if (attName == kAttrLineNumberFontColor)
		{
			auto color =
			    fromScintillaColor (sev->sendMessage (SCI_STYLEGETFORE, STYLE_LINENUMBER, 0));
			return colorToString (color, stringValue, desc);
		}
		if (attName == kAttrLineNumberBackgroundColor)
		{
			auto color =
			    fromScintillaColor (sev->sendMessage (SCI_STYLEGETBACK, STYLE_LINENUMBER, 0));
			return colorToString (color, stringValue, desc);
		}
		if (attName == UIViewCreator::kAttrBackgroundColor)
		{
			auto color = sev->getBackgroundColor ();
			return colorToString (color, stringValue, desc);
		}

		return false;
	}
};
ScintillaEditorViewCreator __gScintillaEditorViewCreator;

//------------------------------------------------------------------------
} // VSTGUI
