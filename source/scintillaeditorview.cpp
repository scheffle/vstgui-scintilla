//
//  scintillaeditorview_uidesc.cpp
//  scintilla-example
//
//  Created by Arne Scheffler on 20.01.18.
//

#include "scintillaeditorview.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/platform/iplatformfont.h"
#include "vstgui/uidescription/detail/uiviewcreatorattributes.h"
#include "vstgui/uidescription/iviewcreator.h"
#include "vstgui/uidescription/uiattributes.h"
#include "vstgui/uidescription/uiviewcreator.h"
#include "vstgui/uidescription/uiviewfactory.h"

#include "Scintilla.h"
#include "ScintillaMessages.h"
#include "ScintillaTypes.h"

#include <array>

//------------------------------------------------------------------------
namespace VSTGUI {
using Message = Scintilla::Message;
using Notification = Scintilla::Notification;
using StylesCommon = Scintilla::StylesCommon;
using Element = Scintilla::Element;
using MarkerSymbol = Scintilla::MarkerSymbol;
using MarkerOutline = Scintilla::MarkerOutline;
using AutomaticFold = Scintilla::AutomaticFold;
using WrapVisualFlag = Scintilla::WrapVisualFlag;

//------------------------------------------------------------------------
constexpr AutomaticFold operator| (AutomaticFold a, AutomaticFold b) noexcept
{
	return static_cast<AutomaticFold> (static_cast<int> (a) | static_cast<int> (b));
}

//------------------------------------------------------------------------
constexpr WrapVisualFlag operator| (WrapVisualFlag a, WrapVisualFlag b) noexcept
{
	return static_cast<WrapVisualFlag> (static_cast<int> (a) | static_cast<int> (b));
}

//------------------------------------------------------------------------
constexpr bool operator& (WrapVisualFlag a, WrapVisualFlag b) noexcept
{
	return static_cast<bool> (static_cast<int> (a) & static_cast<int> (b));
}

//------------------------------------------------------------------------
inline WrapVisualFlag& operator|= (WrapVisualFlag& a, WrapVisualFlag b) noexcept
{
	a = a | b;
	return a;
}

//------------------------------------------------------------------------
inline WrapVisualFlag& operator^= (WrapVisualFlag& a, WrapVisualFlag b) noexcept
{
	a = static_cast<WrapVisualFlag> (static_cast<int> (a) & ~static_cast<int> (b));
	return a;
}

//------------------------------------------------------------------------
void ScintillaEditorView::init ()
{
	setWantsFocus (true);
	updateMarginsColumns ();
	registerListener (this);
	sendMessage (Message::SetPhasesDraw, Scintilla::PhasesDraw::Two);
	sendMessage (Message::SetSelectionLayer, Scintilla::Layer::UnderText);
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
	auto fontSize = font->getSize () * static_cast<CCoord> (Scintilla::FontSizeMultiplier);
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
UTF8String ScintillaEditorView::getText (const Range& range)
{
	if (range.start >= range.end)
		return {};
	std::string str;
	str.resize ((range.end - range.start) );
	Sci_TextRange textRange;
	textRange.chrg.cpMin = range.start;
	textRange.chrg.cpMax = range.end;
	textRange.lpstrText = str.data ();
	sendMessage (Message::GetTextRange, 0, &textRange);
	return UTF8String (std::move (str));
}

//------------------------------------------------------------------------
auto ScintillaEditorView::getSelection () const -> Range
{
	Range selection {};
	selection.start = sendMessage (Message::GetSelectionStart);
	selection.end = sendMessage (Message::GetSelectionEnd);
	return selection;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setSelection (Range selection)
{
	sendMessage (Message::SetSelectionStart, selection.start);
	sendMessage (Message::SetSelectionEnd, selection.end);
}

//------------------------------------------------------------------------
int64_t ScintillaEditorView::findAndSelect (UTF8StringPtr searchString, uint32_t searchFlags)
{
	auto sciSearchFlags = Scintilla::FindOption::None;
	if (searchFlags & MatchCase)
		sciSearchFlags |= Scintilla::FindOption::MatchCase;
	if (searchFlags & WholeWord)
		sciSearchFlags |= Scintilla::FindOption::WholeWord;
	if (searchFlags & WordStart)
		sciSearchFlags |= Scintilla::FindOption::WordStart;

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
void ScintillaEditorView::setLineWrap (Scintilla::Wrap mode)
{
	sendMessage (Message::SetWrapMode, mode);
}

//------------------------------------------------------------------------
auto ScintillaEditorView::getLineWrap () const -> Scintilla::Wrap
{
	return static_cast<Scintilla::Wrap> (sendMessage (Message::GetWrapMode));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setLineWrapStartIndent (uint32_t amount)
{
	sendMessage (Message::SetWrapStartIndent, amount);
}

//------------------------------------------------------------------------
uint32_t ScintillaEditorView::getLineWrapStartIndent () const
{
	return sendMessage (Message::GetWrapStartIndent);
}

//------------------------------------------------------------------------
void ScintillaEditorView::setLineWrapIndentMode (Scintilla::WrapIndentMode mode)
{
	sendMessage (Message::SetWrapIndentMode, mode);
}

//------------------------------------------------------------------------
Scintilla::WrapIndentMode ScintillaEditorView::getLineWrapIndentMode () const
{
	return static_cast<Scintilla::WrapIndentMode> (sendMessage (Message::GetWrapIndentMode));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setLineWrapVisualFlags (WrapVisualFlag flags)
{
	sendMessage (Message::SetWrapVisualFlags, flags);
}

//------------------------------------------------------------------------
WrapVisualFlag ScintillaEditorView::getLineWrapVisualFlags () const
{
	return static_cast<WrapVisualFlag> (sendMessage (Message::GetWrapVisualFlags));
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
void ScintillaEditorView::setDefaultFoldDisplayText (UTF8StringPtr text)
{
	sendMessage (Message::SetDefaultFoldDisplayText, 0, text);
}

//------------------------------------------------------------------------
UTF8String ScintillaEditorView::getDefaultFoldDisplayText () const
{
	std::string str;
	auto length = sendMessage (Message::GetDefaultFoldDisplayText);
	if (length > 0)
	{
		str.resize (length);
		sendMessage (Message::GetDefaultFoldDisplayText, length + 1, str.data ());
	}
	return UTF8String (std::move (str));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setFoldDisplayTextStyle (Scintilla::FoldDisplayTextStyle style)
{
	sendMessage (Scintilla::Message::FoldDisplayTextSetStyle, style);
}

//------------------------------------------------------------------------
Scintilla::FoldDisplayTextStyle ScintillaEditorView::getFoldDisplayTextStyle () const
{
	return static_cast<Scintilla::FoldDisplayTextStyle> (
	    sendMessage (Scintilla::Message::FoldDisplayTextGetStyle));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setFoldMarginColorHi (const CColor& color)
{
	foldMarginColorHi = color;
	sendMessage (Message::SetFoldMarginHiColour, true, toScintillaColor (color));
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getFoldMarginColorHi () const
{
	return foldMarginColorHi;
}

//------------------------------------------------------------------------
void ScintillaEditorView::setFoldMarginColor (const CColor& color)
{
	foldMarginColor = color;
	sendMessage (Message::SetFoldMarginColour, true, toScintillaColor (color));
}

//------------------------------------------------------------------------
CColor ScintillaEditorView::getFoldMarginColor () const
{
	return foldMarginColor;
}

//------------------------------------------------------------------------
bool ScintillaEditorView::showLineNumberMargin () const
{
	return (marginsCol & (1 << MarginsCol::LineNumber));
}

//------------------------------------------------------------------------
bool ScintillaEditorView::showFoldMargin () const
{
	return (marginsCol & (1 << MarginsCol::Folding));
}

//------------------------------------------------------------------------
void ScintillaEditorView::setZoom (int32_t zoom)
{
	sendMessage (Message::SetZoom, zoom);
}

//------------------------------------------------------------------------
int32_t ScintillaEditorView::getZoom () const
{
	return static_cast<int32_t> (sendMessage (Message::GetZoom));
}

//------------------------------------------------------------------------
void ScintillaEditorView::updateLineNumberMarginWidth ()
{
	if (showLineNumberMargin ())
	{
		auto lineCount = static_cast<uint32_t> (sendMessage (Message::GetLineCount));
		std::string str = "_99";
		while ((lineCount /= 10) >= 10)
		{
			str += "9";
		}
		auto width = sendMessage (Message::TextWidth, StylesCommon::LineNumber, str.data ());
		sendMessage (Message::SetMarginWidthN, 0, width);
	}
}

//------------------------------------------------------------------------
void ScintillaEditorView::updateMarginsColumns ()
{
	auto lineNumbers = showLineNumberMargin ();
	auto folding = showFoldMargin ();

	auto count = 0;
	if (lineNumbers)
		++count;
	if (folding)
		++count;

	sendMessage (Message::SetMargins, count);
	uint32_t column = 0;
	if (lineNumbers)
	{
		sendMessage (Message::SetMarginTypeN, column, Scintilla::MarginType::Number);
		updateLineNumberMarginWidth ();
		++column;
	}
	if (folding)
	{
		sendMessage (Message::SetMarginTypeN, column, Scintilla::MarginType::Symbol);
		sendMessage (Message::SetMarginMaskN, column, Scintilla::MaskFolders);
		sendMessage (Message::SetMarginWidthN, column, 16 + getZoom ());
		sendMessage (Message::SetMarginSensitiveN, column, 1);

		sendMessage (Message::MarkerDefine, MarkerOutline::Folder, MarkerSymbol::Arrow);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderOpen, MarkerSymbol::ArrowDown);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderEnd, MarkerSymbol::Empty);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderMidTail, MarkerSymbol::Empty);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderOpenMid, MarkerSymbol::Empty);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderSub, MarkerSymbol::Empty);
		sendMessage (Message::MarkerDefine, MarkerOutline::FolderTail, MarkerSymbol::Empty);

		sendMessage (Message::SetAutomaticFold,
		             AutomaticFold::Show | AutomaticFold::Change | AutomaticFold::Click);
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
void ScintillaEditorView::onScintillaNotification (SCNotification* notification)
{
	assert (notification);
	switch (static_cast<Notification> (notification->nmhdr.code))
	{
		case Notification::Modified:
		{
			if (notification->linesAdded != 0)
				updateLineNumberMarginWidth ();
			break;
		}
		case Notification::Zoom:
		{
			updateMarginsColumns ();
			break;
		}
		case Notification::FocusIn:
		{
			if (auto frame = getFrame ())
			{
				if (frame->getFocusView () != this)
				{
					frame->setFocusView (this);
				}
			}
			break;
		}
		case Notification::FocusOut:
		{
			if (auto frame = getFrame ())
			{
				if (frame->getFocusView () == this)
				{
					frame->setFocusView (nullptr);
				}
			}
			break;
		}
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
static std::string kAttrFoldMarginColor = "fold-margin-color";
static std::string kAttrFoldMarginColorHi = "fold-margin-color-hi";
static std::string kAttrFoldDisplayTextStyle = "fold-display-text-style";
static std::string kAttrDefaultFoldDisplayText = "default-fold-display-text";
static std::string kAttrUseTabs = "use-tabs";
static std::string kAttrTabWidth = "tab-width";
static std::string kAttrLineWrapMode = "line-wrap-mode";
static std::string kAttrLineWrapIndentMode = "line-wrap-indent-mode";
static std::string kAttrLineWrapStartIndent = "line-wrap-start-indent";
static std::string kAttrLineWrapVisualStart = "line-wrap-visual-start";
static std::string kAttrLineWrapVisualEnd = "line-wrap-visual-end";
static std::string kAttrLineWrapVisualMargin = "line-wrap-visual-margin";

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
		// line wrap
		attributeNames.push_back (kAttrLineWrapMode);
		attributeNames.push_back (kAttrLineWrapVisualStart);
		attributeNames.push_back (kAttrLineWrapVisualEnd);
		attributeNames.push_back (kAttrLineWrapVisualMargin);
		attributeNames.push_back (kAttrLineWrapIndentMode);
		attributeNames.push_back (kAttrLineWrapStartIndent);
		// folding
		attributeNames.push_back (kAttrShowFolding);
		attributeNames.push_back (kAttrFoldMarginColor);
		attributeNames.push_back (kAttrFoldMarginColorHi);
		attributeNames.push_back (kAttrFoldDisplayTextStyle);
		attributeNames.push_back (kAttrDefaultFoldDisplayText);
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
		if (attributeName == kAttrLineWrapMode)
			return kListType;
		if (attributeName == kAttrLineWrapIndentMode)
			return kListType;
		if (attributeName == kAttrLineWrapStartIndent)
			return kIntegerType;
		if (attributeName == kAttrLineWrapVisualStart)
			return kBooleanType;
		if (attributeName == kAttrLineWrapVisualEnd)
			return kBooleanType;
		if (attributeName == kAttrLineWrapVisualMargin)
			return kBooleanType;
		if (attributeName == kAttrShowFolding)
			return kBooleanType;
		if (attributeName == kAttrFoldMarginColor)
			return kColorType;
		if (attributeName == kAttrFoldMarginColorHi)
			return kColorType;
		if (attributeName == kAttrFoldDisplayTextStyle)
			return kListType;
		if (attributeName == kAttrDefaultFoldDisplayText)
			return kStringType;
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
		if (stringToColor (attr.getAttributeValue (kAttrFoldMarginColor), color, desc))
		{
			sev->setFoldMarginColor (color);
		}
		if (stringToColor (attr.getAttributeValue (kAttrFoldMarginColorHi), color, desc))
		{
			sev->setFoldMarginColorHi (color);
		}
		if (auto style = attr.getAttributeValue (kAttrFoldDisplayTextStyle))
		{
			for (auto index = 0; index < getFoldDisplayTextStyles ().size (); ++index)
			{
				if (*style == getFoldDisplayTextStyles ()[index])
				{
					sev->setFoldDisplayTextStyle (
					    static_cast<Scintilla::FoldDisplayTextStyle> (index));
					break;
				}
			}
		}
		if (auto str = attr.getAttributeValue (kAttrDefaultFoldDisplayText))
		{
			sev->setDefaultFoldDisplayText (str->data ());
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
		if (auto mode = attr.getAttributeValue (kAttrLineWrapMode))
		{
			for (auto index = 0; index < getLineWrapModes ().size (); ++index)
			{
				if (*mode == getLineWrapModes ()[index])
				{
					sev->setLineWrap (static_cast<Scintilla::Wrap> (index));
					break;
				}
			}
		}
		if (auto mode = attr.getAttributeValue (kAttrLineWrapIndentMode))
		{
			for (auto index = 0; index < getLineWrapIndentModes ().size (); ++index)
			{
				if (*mode == getLineWrapIndentModes ()[index])
				{
					sev->setLineWrapIndentMode (static_cast<Scintilla::WrapIndentMode> (index));
					break;
				}
			}
		}
		if (attr.getIntegerAttribute (kAttrLineWrapStartIndent, i))
		{
			sev->setLineWrapStartIndent (static_cast<uint32_t> (i));
		}
		auto visualFlags = sev->getLineWrapVisualFlags ();
		if (attr.getBooleanAttribute (kAttrLineWrapVisualStart, b))
		{
			if (b)
				visualFlags |= WrapVisualFlag::Start;
			else
				visualFlags ^= WrapVisualFlag::Start;
		}
		if (attr.getBooleanAttribute (kAttrLineWrapVisualEnd, b))
		{
			if (b)
				visualFlags |= WrapVisualFlag::End;
			else
				visualFlags ^= WrapVisualFlag::End;
		}
		if (attr.getBooleanAttribute (kAttrLineWrapVisualMargin, b))
		{
			if (b)
				visualFlags |= WrapVisualFlag::Margin;
			else
				visualFlags ^= WrapVisualFlag::Margin;
		}
		sev->setLineWrapVisualFlags (visualFlags);
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
		if (attName == kAttrFoldMarginColor)
		{
			auto color = sev->getFoldMarginColor ();
			return colorToString (color, stringValue, desc);
		}
		if (attName == kAttrFoldMarginColorHi)
		{
			auto color = sev->getFoldMarginColorHi ();
			return colorToString (color, stringValue, desc);
		}
		if (attName == kAttrFoldDisplayTextStyle)
		{
			auto index = static_cast<size_t> (sev->getFoldDisplayTextStyle ());
			stringValue = getFoldDisplayTextStyles ()[index];
			return true;
		}
		if (attName == kAttrDefaultFoldDisplayText)
		{
			stringValue = sev->getDefaultFoldDisplayText ();
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
			stringValue = UIAttributes::integerToString (sev->getTabWidth ());
			return true;
		}
		if (attName == kAttrLineWrapMode)
		{
			auto index = static_cast<size_t> (sev->getLineWrap ());
			stringValue = getLineWrapModes ()[index];
			return true;
		}
		if (attName == kAttrLineWrapIndentMode)
		{
			auto index = static_cast<size_t> (sev->getLineWrapIndentMode ());
			stringValue = getLineWrapIndentModes ()[index];
			return true;
		}
		if (attName == kAttrLineWrapStartIndent)
		{
			stringValue = UIAttributes::integerToString (sev->getLineWrapStartIndent ());
			return true;
		}
		if (attName == kAttrLineWrapVisualStart)
		{
			stringValue =
			    (sev->getLineWrapVisualFlags () & WrapVisualFlag::Start) ? "true" : "false";
			return true;
		}
		if (attName == kAttrLineWrapVisualEnd)
		{
			stringValue = (sev->getLineWrapVisualFlags () & WrapVisualFlag::End) ? "true" : "false";
			return true;
		}
		if (attName == kAttrLineWrapVisualMargin)
		{
			stringValue =
			    (sev->getLineWrapVisualFlags () & WrapVisualFlag::Margin) ? "true" : "false";
			return true;
		}

		return false;
	}
	using LineWrapModes = std::array<string, 4>;
	static LineWrapModes& getLineWrapModes ()
	{
		static LineWrapModes modes = {"None", "Word", "Char", "White Space"};
		return modes;
	}
	using LineWrapIndentModes = std::array<string, 4>;
	static LineWrapIndentModes& getLineWrapIndentModes ()
	{
		static LineWrapIndentModes modes = {"Fixed", "Same", "Indent", "Deep Indent"};
		return modes;
	}
	using FoldDisplayTextStyles = std::array<string, 3>;
	static FoldDisplayTextStyles& getFoldDisplayTextStyles ()
	{
		static FoldDisplayTextStyles styles = {"None", "Standard", "Boxed"};
		return styles;
	}
	bool getPossibleListValues (const string& attributeName,
	                            ConstStringPtrList& values) const override
	{
		if (attributeName == kAttrLineWrapMode)
		{
			for (auto& m : getLineWrapModes ())
				values.emplace_back (&m);
			return true;
		}
		if (attributeName == kAttrLineWrapIndentMode)
		{
			for (auto& m : getLineWrapIndentModes ())
				values.emplace_back (&m);
			return true;
		}
		if (attributeName == kAttrFoldDisplayTextStyle)
		{
			for (auto& m : getFoldDisplayTextStyles ())
				values.emplace_back (&m);
			return true;
		}
		return false;
	}
};
ScintillaEditorViewCreator __gScintillaEditorViewCreator;

//------------------------------------------------------------------------
} // VSTGUI
