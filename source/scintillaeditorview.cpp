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

#include "ScintillaMessages.h"
#include "Scintilla.h"
#include "ILexer.h"
#include "Lexilla.h"
#include "LexillaAccess.h"

//------------------------------------------------------------------------
namespace VSTGUI {
using Message = Scintilla::Message;

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
		sendMessageT (Message::StyleSetFont, i, fontName.data ());
		sendMessageT (Message::StyleSetSizeFractional, i, static_cast<intptr_t> (fontSize));
		sendMessageT (Message::StyleSetBold, i, isBold);
		sendMessageT (Message::StyleSetItalic, i, isItalic);
	}
}

//------------------------------------------------------------------------
SharedPointer<CFontDesc> ScintillaEditorView::getFont () const
{
	return font;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setStaticFontColor (const CColor& color)
{
	if (color == kTransparentCColor)
	{
	}
	else
	{
		auto sc = toScintillaColor (color);
		for (auto index = 0; index <= STYLE_DEFAULT; ++index)
			sendMessageT (Message::StyleSetFore, index, sc);
	}
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getStaticFontColor () const
{
	return fromScintillaColor (sendMessageT (Message::StyleGetFore, STYLE_DEFAULT));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setBackgroundColor (const CColor& color)
{
	auto sc = toScintillaColor (color);
	for (auto index = 0; index <= STYLE_DEFAULT; ++index)
		sendMessageT (Message::StyleSetBack, index, sc);
	platformSetBackgroundColor (color);
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getBackgroundColor () const
{
	return fromScintillaColor (sendMessageT (Message::StyleGetBack, STYLE_DEFAULT));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setSelectionBackgroundColor (const CColor& color)
{
	bool enable = color != kTransparentCColor;
	sendMessageT (Message::SetSelBack, enable, toScintillaColor (color));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setSelectionForegroundColor (const CColor& color)
{
	bool enable = color != kTransparentCColor;
	sendMessageT (Message::SetSelFore, enable, toScintillaColor (color));
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
	return sendMessageT (Message::CanUndo, 0, 0) != 0;
}

//------------------------------------------------------------------------
bool ScintillaEditorView::canRedo () const
{
	return sendMessageT (Message::CanRedo, 0, 0) != 0;
}

//------------------------------------------------------------------------
void ScintillaEditorView::undo ()
{
	sendMessageT (Message::Undo, 0, 0);
}

//------------------------------------------------------------------------
void ScintillaEditorView::redo ()
{
	sendMessageT (Message::Redo, 0, 0);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setText (UTF8StringPtr text)
{
	sendMessageT (Message::SetText, 0, text);
}

//------------------------------------------------------------------------
auto ScintillaEditorView::getSelection () const -> Selection
{
	Selection selection {};
	selection.start = sendMessageT (Message::GetSelectionStart, 0, 0);
	selection.end = sendMessageT (Message::GetSelectionEnd, 0, 0);
	return selection;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setSelection (Selection selection)
{
	sendMessageT (Message::SetSelectionStart, selection.start, 0);
	sendMessageT (Message::SetSelectionEnd, selection.end, 0);
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
		sendMessageT (Message::SetSelectionStart, selection.end);
	sendMessageT (Message::SearchAnchor);

	intptr_t result;
	if (searchFlags & Backwards)
	{
		result = sendMessageT (Message::SearchPrev, sciSearchFlags, searchString);
		if (result < 0 && searchFlags & Wrap)
		{
			auto textLength = sendMessageT (Message::GetTextLength);
			sendMessageT (Message::SetSelectionStart, textLength);
			sendMessageT (Message::SetSelectionEnd, textLength);
			sendMessageT (Message::SearchAnchor);
			result = sendMessageT (Message::SearchPrev, sciSearchFlags, searchString);
		}
	}
	else
	{
		result = sendMessageT (Message::SearchNext, sciSearchFlags, searchString);
		if (result < 0 && searchFlags & Wrap)
		{
			sendMessageT (Message::SetSelectionStart, 0);
			sendMessageT (Message::SetSelectionEnd, 0);
			sendMessageT (Message::SearchAnchor, 0);
			result = sendMessageT (Message::SearchNext, sciSearchFlags, searchString);
		}
	}
	if (result >= 0)
	{
		if (searchFlags & ScrollTo)
		{
			sendMessageT (Message::ScrollCaret, 0, 0);
		}
		return result;
	}
	setSelection (selection);

	return -1;
}

//------------------------------------------------------------------------
bool ScintillaEditorView::replaceSelection (UTF8StringPtr string)
{
	sendMessageT (Message::ReplaceSel, 0, string);
	return true;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setUseTabs (bool state)
{
	sendMessageT (Message::SetUseTabs, state, 0);
}

//------------------------------------------------------------------------
bool ScintillaEditorView::getUseTabs () const
{
	return sendMessageT (Message::GetUseTabs, 0, 0) != 0;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setTabWidth (uint32_t width)
{
	sendMessageT (Message::SetTabWidth, width, 0);
}

//------------------------------------------------------------------------
uint32_t ScintillaEditorView::getTabWidth () const
{
	return static_cast<uint32_t> (sendMessageT (Message::GetTabWidth, 0, 0));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setLexerLanguage (IdStringPtr lang)
{
	if (lexer)
	{
		if (auto lexerName = lexer->GetName ())
		{
			if (std::string_view (lang) == lexerName)
				return;
		}
	}
	lexer = CreateLexer (lang);
	sendMessageT (Message::SetILexer, 0, lexer);
}

//------------------------------------------------------------------------
std::string ScintillaEditorView::getLexerLanguage () const
{
	std::string result;
	auto length = sendMessageT (Message::GetLexerLanguage, 0, 0);
	result.resize (length);
	sendMessageT (Message::GetLexerLanguage, 0, result.data ());
	return result;
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
static std::string kAttrUseTabs = "use-tabs";
static std::string kAttrTabWidth = "tab-width";
static std::string kAttrLexer = "lexer";

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
		// lexer
		attributeNames.push_back (kAttrLexer);
		// margin
		attributeNames.push_back (kAttrEditorMargin);
		// font
		attributeNames.push_back (kAttrEditorFont);
		attributeNames.push_back (UIViewCreator::kAttrFontColor);
		// selection
		attributeNames.push_back (kAttrSelectionBackgroundColor);
		attributeNames.push_back (kAttrSelectionForegroundColor);
		// line-number
		attributeNames.push_back (kAttrLineNumberFontColor);
		attributeNames.push_back (kAttrLineNumberBackgroundColor);
		// other
		attributeNames.push_back (UIViewCreator::kAttrBackgroundColor);
		// tabs
		attributeNames.push_back (kAttrUseTabs);
		attributeNames.push_back (kAttrTabWidth);
		return true;
	}
	AttrType getAttributeType (const std::string& attributeName) const override
	{
		if (attributeName == kAttrLexer)
			return kStringType;
		if (attributeName == kAttrEditorMargin)
			return kPointType;
		if (attributeName == kAttrEditorFont)
			return kFontType;
		if (attributeName == UIViewCreator::kAttrFontColor)
			return kColorType;
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
		if (attributeName == kAttrUseTabs)
			return kBooleanType;
		if (attributeName == kAttrTabWidth)
			return kIntegerType;
		return kUnknownType;
	}
	bool apply (CView* view, const UIAttributes& attr, const IUIDescription* desc) const override
	{
		auto sev = dynamic_cast<ScintillaEditorView*> (view);
		if (!sev)
			return false;
		if (auto lang = attr.getAttributeValue (kAttrLexer))
		{
			sev->setLexerLanguage (lang->data ());
		}
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
			sev->sendMessageT (Message::StyleSetFore, STYLE_LINENUMBER, toScintillaColor (color));
		}
		if (stringToColor (attr.getAttributeValue (kAttrLineNumberBackgroundColor), color, desc))
		{
			sev->sendMessageT (Message::StyleSetBack, STYLE_LINENUMBER, toScintillaColor (color));
		}
		if (auto fontName = attr.getAttributeValue (kAttrEditorFont))
		{
			if (auto font = desc->getFont (fontName->data ()))
			{
				sev->setFont (font);
			}
		}
		if (stringToColor (attr.getAttributeValue (UIViewCreator::kAttrFontColor), color, desc))
		{
			sev->setStaticFontColor (color);
		}
		bool b;
		if (attr.getBooleanAttribute (kAttrUseTabs, b))
		{
			sev->setUseTabs (b);
		}
		int32_t i;
		if (attr.getIntegerAttribute (kAttrTabWidth, i))
		{
			sev->setTabWidth (static_cast<uint32_t> (i));
		}
		CPoint p;
		if (attr.getPointAttribute (kAttrEditorMargin, p))
		{
			sev->sendMessageT (Message::SetMarginLeft, 0, p.x);
			sev->sendMessageT (Message::SetMarginRight, 0, p.y);
		}
		return true;
	}
	bool getAttributeValue (CView* view, const std::string& attName, std::string& stringValue,
	                        const IUIDescription* desc) const override
	{
		auto sev = dynamic_cast<ScintillaEditorView*> (view);
		if (!sev)
			return false;
		if (attName == kAttrLexer)
		{
			stringValue = sev->getLexerLanguage ();
			return true;
		}
		if (attName == kAttrEditorMargin)
		{
			CPoint p;
			p.x = sev->sendMessageT (Message::GetMarginLeft, 0, 0);
			p.y = sev->sendMessageT (Message::GetMarginRight, 0, 0);
			stringValue = UIAttributes::pointToString (p);
			return true;
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
			    fromScintillaColor (sev->sendMessageT (Message::StyleGetFore, STYLE_LINENUMBER, 0));
			return colorToString (color, stringValue, desc);
		}
		if (attName == kAttrLineNumberBackgroundColor)
		{
			auto color =
			    fromScintillaColor (sev->sendMessageT (Message::StyleGetBack, STYLE_LINENUMBER, 0));
			return colorToString (color, stringValue, desc);
		}
		if (attName == UIViewCreator::kAttrBackgroundColor)
		{
			auto color = sev->getBackgroundColor ();
			return colorToString (color, stringValue, desc);
		}
		if (attName == UIViewCreator::kAttrFontColor)
		{
			auto color = sev->getStaticFontColor ();
			return colorToString (color, stringValue, desc);
		}
		if (attName == kAttrUseTabs)
		{
			stringValue = sev->getUseTabs () ? "true" : "false";
			return true;
		}
		if (attName == kAttrTabWidth)
		{
			stringValue = std::to_string (sev->getTabWidth ());
			return true;
		}

		return false;
	}
};
ScintillaEditorViewCreator __gScintillaEditorViewCreator;

//------------------------------------------------------------------------
} // VSTGUI
