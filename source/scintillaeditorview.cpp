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
#include "ScintillaTypes.h"
#include "ScintillaCall.h"
#include "Scintilla.h"
#include "ILexer.h"
#include "Lexilla.h"
#include "LexillaAccess.h"

//------------------------------------------------------------------------
namespace VSTGUI {
using Message = Scintilla::Message;
using StylesCommon = Scintilla::StylesCommon;
using Element = Scintilla::Element;

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
		sendMessage (Message::StyleSetFont, i, fontName.data ());
		sendMessage (Message::StyleSetSizeFractional, i, static_cast<intptr_t> (fontSize));
		sendMessage (Message::StyleSetBold, i, isBold);
		sendMessage (Message::StyleSetItalic, i, isItalic);
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
			sendMessage (Message::StyleSetFore, index, sc);
	}
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getStaticFontColor () const
{
	return fromScintillaColor (sendMessage (Message::StyleGetFore, StylesCommon::Default));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setBackgroundColor (const CColor& color)
{
	auto sc = toScintillaColor (color);
	for (auto index = 0; index <= STYLE_DEFAULT; ++index)
		sendMessage (Message::StyleSetBack, index, sc);
	platformSetBackgroundColor (color);
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getBackgroundColor () const
{
	return fromScintillaColor (sendMessage (Message::StyleGetBack, StylesCommon::Default));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setSelectionBackgroundColor (const CColor& color)
{
	bool enable = color != kTransparentCColor;
	sendMessage (Message::SetElementColour, Element::SelectionBack, toScintillaColor (color));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setSelectionForegroundColor (const CColor& color)
{
	bool enable = color != kTransparentCColor;
	sendMessage (Message::SetElementColour, Element::SelectionText, toScintillaColor (color));
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
void ScintillaEditorView::setCaretColor (const CColor& color)
{
	sendMessage (Message::SetElementColour, Element::Caret, toScintillaColor (color));
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getCaretColor () const
{
	return fromScintillaColor (sendMessage (Message::GetElementColour, Element::Caret));
}

//------------------------------------------------------------------------
bool ScintillaEditorView::canUndo () const
{
	return sendMessage (Message::CanUndo) != 0;
}

//------------------------------------------------------------------------
bool ScintillaEditorView::canRedo () const
{
	return sendMessage (Message::CanRedo) != 0;
}

//------------------------------------------------------------------------
void ScintillaEditorView::undo ()
{
	sendMessage (Message::Undo);
}

//------------------------------------------------------------------------
void ScintillaEditorView::redo ()
{
	sendMessage (Message::Redo);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setText (UTF8StringPtr text)
{
	sendMessage (Message::SetText, 0, text);
}

//------------------------------------------------------------------------
UTF8String ScintillaEditorView::getText () const
{
	std::string str;
	auto length = sendMessage (Message::GetTextLength);
	if (length > 0)
	{
		str.resize (length);
		sendMessage (Message::GetText, length+1, str.data ());
	}
	return UTF8String (std::move (str));
}

//------------------------------------------------------------------------
auto ScintillaEditorView::getSelection () const -> Selection
{
	Selection selection {};
	selection.start = sendMessage (Message::GetSelectionStart);
	selection.end = sendMessage (Message::GetSelectionEnd);
	return selection;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setSelection (Selection selection)
{
	sendMessage (Message::SetSelectionStart, selection.start);
	sendMessage (Message::SetSelectionEnd, selection.end);
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
		sendMessage (Message::SetSelectionStart, selection.end);
	sendMessage (Message::SearchAnchor);

	intptr_t result;
	if (searchFlags & Backwards)
	{
		result = sendMessage (Message::SearchPrev, sciSearchFlags, searchString);
		if (result < 0 && searchFlags & Wrap)
		{
			auto textLength = sendMessage (Message::GetTextLength);
			sendMessage (Message::SetSelectionStart, textLength);
			sendMessage (Message::SetSelectionEnd, textLength);
			sendMessage (Message::SearchAnchor);
			result = sendMessage (Message::SearchPrev, sciSearchFlags, searchString);
		}
	}
	else
	{
		result = sendMessage (Message::SearchNext, sciSearchFlags, searchString);
		if (result < 0 && searchFlags & Wrap)
		{
			sendMessage (Message::SetSelectionStart);
			sendMessage (Message::SetSelectionEnd);
			sendMessage (Message::SearchAnchor);
			result = sendMessage (Message::SearchNext, sciSearchFlags, searchString);
		}
	}
	if (result >= 0)
	{
		if (searchFlags & ScrollTo)
		{
			sendMessage (Message::ScrollCaret);
		}
		return result;
	}
	setSelection (selection);

	return -1;
}

//------------------------------------------------------------------------
bool ScintillaEditorView::replaceSelection (UTF8StringPtr string)
{
	sendMessage (Message::ReplaceSel, 0, string);
	return true;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setUseTabs (bool state)
{
	sendMessage (Message::SetUseTabs, state);
}

//------------------------------------------------------------------------
bool ScintillaEditorView::getUseTabs () const
{
	return sendMessage (Message::GetUseTabs) != 0;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setTabWidth (uint32_t width)
{
	sendMessage (Message::SetTabWidth, width);
}

//------------------------------------------------------------------------
uint32_t ScintillaEditorView::getTabWidth () const
{
	return static_cast<uint32_t> (sendMessage (Message::GetTabWidth));
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
	sendMessage (Message::SetILexer, 0, lexer);
}

//------------------------------------------------------------------------
std::string ScintillaEditorView::getLexerLanguage () const
{
	std::string result;
	auto length = sendMessage (Message::GetLexerLanguage);
	result.resize (length);
	sendMessage (Message::GetLexerLanguage, 0, result.data ());
	return result;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setLineNumbersVisible (bool state)
{
	sendMessage (Message::SetMargins, state ? 1 : 0);
	if (state)
	{
		sendMessage (Message::SetMarginTypeN, 0, SC_MARGIN_NUMBER);
		auto width = sendMessage (Message::TextWidth, StylesCommon::LineNumber, "_99999");
		sendMessage (Message::SetMarginWidthN, 0, width);
	}
	else
	{
		sendMessage (Message::SetMarginWidthN, 0, 0);
		// the following is a workaround so that the margins are removed immediately on macOS
		auto size = getViewSize ();
		size.right--;
		setViewSize (size, false);
		size.right++;
		setViewSize (size, false);
	}
}

//------------------------------------------------------------------------
bool ScintillaEditorView::getLineNumbersVisible () const
{
	return sendMessage (Message::GetMargins) != 0;
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
static std::string kAttrShowLineNumbers = "show-line-numbers";
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
		attributeNames.push_back (kAttrShowLineNumbers);
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
		if (attributeName == kAttrShowLineNumbers)
			return kBooleanType;
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
			sev->sendMessage (Message::StyleSetFore, StylesCommon::LineNumber, toScintillaColor (color));
		}
		if (stringToColor (attr.getAttributeValue (kAttrLineNumberBackgroundColor), color, desc))
		{
			sev->sendMessage (Message::StyleSetBack, StylesCommon::LineNumber, toScintillaColor (color));
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
			sev->setCaretColor (color);
		}
		bool b;
		if (attr.getBooleanAttribute (kAttrUseTabs, b))
		{
			sev->setUseTabs (b);
		}
		if (attr.getBooleanAttribute(kAttrShowLineNumbers, b))
		{
			sev->setLineNumbersVisible (b);
		}
		int32_t i;
		if (attr.getIntegerAttribute (kAttrTabWidth, i))
		{
			sev->setTabWidth (static_cast<uint32_t> (i));
		}
		CPoint p;
		if (attr.getPointAttribute (kAttrEditorMargin, p))
		{
			sev->sendMessage (Message::SetMarginLeft, 0, p.x);
			sev->sendMessage (Message::SetMarginRight, 0, p.y);
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
			p.x = sev->sendMessage (Message::GetMarginLeft);
			p.y = sev->sendMessage (Message::GetMarginRight);
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
			    fromScintillaColor (sev->sendMessage (Message::StyleGetFore, StylesCommon::LineNumber));
			return colorToString (color, stringValue, desc);
		}
		if (attName == kAttrLineNumberBackgroundColor)
		{
			auto color =
			    fromScintillaColor (sev->sendMessage (Message::StyleGetBack, StylesCommon::LineNumber));
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
		if (attName == kAttrShowLineNumbers)
		{
			stringValue = sev->getLineNumbersVisible () ? "true" : "false";
			return true;
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
