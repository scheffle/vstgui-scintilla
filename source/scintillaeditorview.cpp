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

#include "ILexer.h"
#include "Scintilla.h"
#include "ScintillaMessages.h"
#include "ScintillaTypes.h"

//------------------------------------------------------------------------
namespace VSTGUI {
using Message = Scintilla::Message;
using Notification = Scintilla::Notification;
using StylesCommon = Scintilla::StylesCommon;
using Element = Scintilla::Element;
using MarkerSymbol = Scintilla::MarkerSymbol;
using MarkerOutline = Scintilla::MarkerOutline;

//------------------------------------------------------------------------
void ScintillaEditorView::init ()
{
	setWantsFocus (true);
	updateMarginsColumns ();
	registerListener (this);
	sendMessage (Message::SetPhasesDraw, SC_PHASES_TWO);
	sendMessage (Message::SetSelectionLayer, SC_LAYER_UNDER_TEXT);
}

//------------------------------------------------------------------------
void ScintillaEditorView::looseFocus ()
{
	sendMessage (Message::SetFocus, 0);
}

//------------------------------------------------------------------------
void ScintillaEditorView::takeFocus ()
{
	sendMessage (Message::SetFocus, 1);
	sendMessage (Message::GrabFocus);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setStyleColor (uint32_t index, const CColor& textColor,
                                         const CColor& backColor)
{
	sendMessage (Message::StyleSetFore, index, toScintillaColor (textColor));
	if (backColor != kTransparentCColor)
		sendMessage (Message::StyleSetBack, index, toScintillaColor (backColor));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setStyleFontWeight (uint32_t index, uint32_t weight)
{
	sendMessage (Message::StyleSetWeight, index, weight);
}

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
	sendMessage (Message::SetElementColour, Element::SelectionBack, toScintillaColor (color));
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getSelectionBackgroundColor () const
{
	return fromScintillaColor (sendMessage (Message::GetElementColour, Element::SelectionBack));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setSelectionForegroundColor (const CColor& color)
{
	sendMessage (Message::SetElementColour, Element::SelectionText, toScintillaColor (color));
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getSelectionForegroundColor () const
{
	return fromScintillaColor (sendMessage (Message::GetElementColour, Element::SelectionText));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setInactiveSelectionBackgroundColor (const CColor& color)
{
	sendMessage (Message::SetElementColour, Element::SelectionInactiveBack,
	             toScintillaColor (color));
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getInactiveSelectionBackgroundColor () const
{
	return fromScintillaColor (
	    sendMessage (Message::GetElementColour, Element::SelectionInactiveBack));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setInactiveSelectionForegroundColor (const CColor& color)
{
	sendMessage (Message::SetElementColour, Element::SelectionInactiveText,
	             toScintillaColor (color));
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getInactiveSelectionForegroundColor () const
{
	return fromScintillaColor (
	    sendMessage (Message::GetElementColour, Element::SelectionInactiveText));
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
	sendMessage (Message::EmptyUndoBuffer);
}

//------------------------------------------------------------------------
UTF8String ScintillaEditorView::getText () const
{
	std::string str;
	auto length = sendMessage (Message::GetTextLength);
	if (length > 0)
	{
		str.resize (length);
		sendMessage (Message::GetText, length + 1, str.data ());
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
void ScintillaEditorView::setLexer (Scintilla::ILexer5* inLexer)
{
	lexer = inLexer;
	sendMessage (Message::SetILexer, 0, lexer);
}

//------------------------------------------------------------------------
Scintilla::ILexer5* ScintillaEditorView::getLexer () const
{
	return lexer;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setLineNumbersVisible (bool state)
{
	auto currentState = (marginsCol & (1 << MarginsCol::LineNumber));
	if (currentState == state)
		return;
	if (state)
		marginsCol |= (1 << MarginsCol::LineNumber);
	else
		marginsCol &= ~(1 << MarginsCol::LineNumber);

	updateMarginsColumns ();
}

//------------------------------------------------------------------------
bool ScintillaEditorView::getLineNumbersVisible () const
{
	return (marginsCol & (1 << MarginsCol::LineNumber));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setLineNumberForegroundColor (const CColor& color)
{
	sendMessage (Message::StyleSetFore, StylesCommon::LineNumber, toScintillaColor (color));
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getLineNumberForegroundColor () const
{
	return fromScintillaColor (sendMessage (Message::StyleGetFore, StylesCommon::LineNumber));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setLineNumberBackgroundColor (const CColor& color)
{
	sendMessage (Message::StyleSetBack, StylesCommon::LineNumber, toScintillaColor (color));
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getLineNumberBackgroundColor () const
{
	return fromScintillaColor (sendMessage (Message::StyleGetBack, StylesCommon::LineNumber));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setFoldingVisible (bool state)
{
	auto currentState = (marginsCol & (1 << MarginsCol::Folding));
	if (currentState == state)
		return;
	if (state)
	{
		marginsCol |= (1 << MarginsCol::Folding);
	}
	else
	{
		marginsCol &= ~(1 << MarginsCol::Folding);
	}

	updateMarginsColumns ();
}

//------------------------------------------------------------------------
bool ScintillaEditorView::getFoldingVisible () const
{
	return (marginsCol & (1 << MarginsCol::Folding));
}

//------------------------------------------------------------------------
void ScintillaEditorView::updateMarginsColumns ()
{
	auto lineNumbers = (marginsCol & (1 << MarginsCol::LineNumber));
	auto folding = (marginsCol & (1 << MarginsCol::Folding));

	auto count = 0;
	if (lineNumbers)
		++count;
	if (folding)
		++count;

	sendMessage (Message::SetMargins, count);
	uint32_t column = 0;
	if (lineNumbers)
	{
		sendMessage (Message::SetMarginTypeN, column, SC_MARGIN_NUMBER);
		auto width = sendMessage (Message::TextWidth, StylesCommon::LineNumber, "_99999");
		sendMessage (Message::SetMarginWidthN, column, width);
		++column;
	}
	if (folding)
	{
		sendMessage (Message::SetMarginTypeN, column, SC_MARGIN_SYMBOL);
		sendMessage (Message::SetMarginMaskN, column, SC_MASK_FOLDERS);
		sendMessage (Message::SetMarginWidthN, column, 20);
		sendMessage (Message::SetMarginSensitiveN, column, 1);

		sendMessage (Message::MarkerDefine, MarkerOutline::Folder, MarkerSymbol::Arrow);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderOpen, MarkerSymbol::ArrowDown);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderEnd, MarkerSymbol::Empty);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderMidTail, MarkerSymbol::Empty);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderOpenMid, MarkerSymbol::Empty);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderSub, MarkerSymbol::Empty);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderTail, MarkerSymbol::Empty);

		sendMessage (Message::SetAutomaticFold,
		             SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CHANGE | SC_AUTOMATICFOLD_CLICK);

		if (lexer)
		{
			lexer->PropertySet ("fold", "1");
			lexer->PropertySet ("fold.comment", "1");
		}
	}
	if (count == 0)
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
int32_t ScintillaEditorView::getFoldingIndex () const
{
	int32_t index = -1;
	if (getFoldingVisible ())
		index = 0;
	if (getLineNumbersVisible ())
		++index;
	return index;
}

//------------------------------------------------------------------------
void ScintillaEditorView::onScintillaNotification (SCNotification* notification)
{
	assert (notification);
	switch (static_cast<Notification> (notification->nmhdr.code))
	{
		default: break;
	}
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
static std::string kAttrInactiveSelectionBackgroundColor = "selection-inactive-background-color";
static std::string kAttrInactiveSelectionForegroundColor = "selection-inactive-foreground-color";
static std::string kAttrShowLineNumbers = "show-line-numbers";
static std::string kAttrShowFolding = "show-folding";
static std::string kAttrUseTabs = "use-tabs";
static std::string kAttrTabWidth = "tab-width";

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
		// margin
		attributeNames.push_back (kAttrEditorMargin);
		// font
		attributeNames.push_back (kAttrEditorFont);
		attributeNames.push_back (UIViewCreator::kAttrFontColor);
		// selection
		attributeNames.push_back (kAttrSelectionBackgroundColor);
		attributeNames.push_back (kAttrSelectionForegroundColor);
		attributeNames.push_back (kAttrInactiveSelectionBackgroundColor);
		attributeNames.push_back (kAttrInactiveSelectionForegroundColor);
		// folding
		attributeNames.push_back (kAttrShowFolding);
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
		if (attributeName == kAttrInactiveSelectionBackgroundColor)
			return kColorType;
		if (attributeName == kAttrInactiveSelectionForegroundColor)
			return kColorType;
		if (attributeName == kAttrShowFolding)
			return kBooleanType;
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
		if (stringToColor (attr.getAttributeValue (kAttrInactiveSelectionBackgroundColor), color,
		                   desc))
		{
			sev->setInactiveSelectionBackgroundColor (color);
		}
		if (stringToColor (attr.getAttributeValue (kAttrInactiveSelectionForegroundColor), color,
		                   desc))
		{
			sev->setInactiveSelectionForegroundColor (color);
		}
		if (stringToColor (attr.getAttributeValue (kAttrLineNumberFontColor), color, desc))
		{
			sev->setLineNumberForegroundColor (color);
		}
		if (stringToColor (attr.getAttributeValue (kAttrLineNumberBackgroundColor), color, desc))
		{
			sev->setLineNumberBackgroundColor (color);
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
		if (attr.getBooleanAttribute (kAttrShowLineNumbers, b))
		{
			sev->setLineNumbersVisible (b);
		}
		if (attr.getBooleanAttribute (kAttrShowFolding, b))
		{
			sev->setFoldingVisible (b);
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
		if (attName == kAttrInactiveSelectionBackgroundColor)
		{
			auto color = sev->getInactiveSelectionBackgroundColor ();
			return colorToString (color, stringValue, desc);
		}
		if (attName == kAttrInactiveSelectionForegroundColor)
		{
			auto color = sev->getInactiveSelectionForegroundColor ();
			return colorToString (color, stringValue, desc);
		}
		if (attName == kAttrLineNumberFontColor)
		{
			auto color = sev->getLineNumberForegroundColor ();
			return colorToString (color, stringValue, desc);
		}
		if (attName == kAttrLineNumberBackgroundColor)
		{
			auto color = sev->getLineNumberBackgroundColor ();
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
		if (attName == kAttrShowFolding)
		{
			stringValue = sev->getFoldingVisible () ? "true" : "false";
			return true;
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
