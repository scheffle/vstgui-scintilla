// This file is part of VSTGUI. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this
// distribution and at http://github.com/steinbergmedia/vstgui/LICENSE

#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/cview.h"
#include <memory>

struct SCNotification; // forward

namespace Scintilla {
class ILexer5;
enum class Wrap;
enum class WrapIndentMode;
enum class WrapVisualFlag;
enum class FoldDisplayTextStyle;
}

//------------------------------------------------------------------------
namespace VSTGUI {

//------------------------------------------------------------------------
class IScintillaListener
{
public:
	virtual void onScintillaNotification (SCNotification* notification) = 0;

	virtual ~IScintillaListener () noexcept = default;
};

//------------------------------------------------------------------------
class ScintillaEditorView : public CView, public IScintillaListener
{
public:
	struct Range
	{
		int64_t start;
		int64_t end;
	};

	ScintillaEditorView ();
	~ScintillaEditorView () noexcept override;

	/** set current text */
	void setText (UTF8StringPtr text);
	/** get current text */
	[[nodiscard]] UTF8String getText () const;
	/** get part of the text  */
	[[nodiscard]] UTF8String getText (const Range& range);

	/** set font for all styles.
	 *	@param font font
	 */
	void setFont (const SharedPointer<CFontDesc>& font);
	/** get the font for all styles
	 *	@return font previously set with setFont
	 */
	[[nodiscard]] SharedPointer<CFontDesc> getFont () const;

	void setStaticFontColor (const CColor& color);
	[[nodiscard]] CColor getStaticFontColor () const;

	void setBackgroundColor (const CColor& color);
	[[nodiscard]] CColor getBackgroundColor () const;

	void setCaretColor (const CColor& color);
	[[nodiscard]] CColor getCaretColor () const;

	// ------------------------------------
	// Selection
	[[nodiscard]] Range getSelection () const;
	void setSelection (Range selection);
	void selectAll ();

	[[maybe_unused]] bool replaceSelection (UTF8StringPtr string);

	void setSelectionBackgroundColor (const CColor& color);
	[[nodiscard]] CColor getSelectionBackgroundColor () const;
	void setSelectionForegroundColor (const CColor& color);
	[[nodiscard]] CColor getSelectionForegroundColor () const;

	void setInactiveSelectionBackgroundColor (const CColor& color);
	[[nodiscard]] CColor getInactiveSelectionBackgroundColor () const;
	void setInactiveSelectionForegroundColor (const CColor& color);
	[[nodiscard]] CColor getInactiveSelectionForegroundColor () const;

	// ------------------------------------
	// Search
	enum SearchFlags
	{
		MatchCase = 1 << 0,
		WholeWord = 1 << 1,
		WordStart = 1 << 2,
		ScrollTo = 1 << 3,
		Wrap = 1 << 4,
		Backwards = 1 << 5
	};
	/** find a string and select it.
	 *	@param searchString string to find
	 *	@param searchFlags flags see SearchFlags
	 *	@return -1 if not found otherwise index of searchString
	 */
	int64_t findAndSelect (UTF8StringPtr searchString, uint32_t searchFlags);

	// ------------------------------------
	// Undo/Redo
	[[nodiscard]] bool canUndo () const;
	[[nodiscard]] bool canRedo () const;
	void undo ();
	void redo ();

	// ------------------------------------
	// Tabs/Indentation
	void setUseTabs (bool state);
	[[nodiscard]] bool getUseTabs () const;
	void setTabWidth (uint32_t width);
	[[nodiscard]] uint32_t getTabWidth () const;

	// ------------------------------------
	// Line Numbers
	void setLineNumbersVisible (bool state);
	[[nodiscard]] bool getLineNumbersVisible () const;
	void setLineNumberForegroundColor (const CColor& color);
	[[nodiscard]] CColor getLineNumberForegroundColor () const;
	void setLineNumberBackgroundColor (const CColor& color);
	[[nodiscard]] CColor getLineNumberBackgroundColor () const;

	// ------------------------------------
	// Folding
	void setFoldingVisible (bool state);
	[[nodiscard]] bool getFoldingVisible () const;
	void setDefaultFoldDisplayText (UTF8StringPtr text);
	[[nodiscard]] UTF8String getDefaultFoldDisplayText () const;
	void setFoldDisplayTextStyle (Scintilla::FoldDisplayTextStyle style);
	[[nodiscard]] Scintilla::FoldDisplayTextStyle getFoldDisplayTextStyle () const;

	void setFoldMarginColorHi (const CColor& color);
	[[nodiscard]] CColor getFoldMarginColorHi () const;
	void setFoldMarginColor (const CColor& color);
	[[nodiscard]] CColor getFoldMarginColor () const;

	// ------------------------------------
	// Line Wrap
	void setLineWrap (Scintilla::Wrap mode);
	[[nodiscard]] Scintilla::Wrap getLineWrap () const;
	void setLineWrapStartIndent (uint32_t amount);
	[[nodiscard]] uint32_t getLineWrapStartIndent () const;
	void setLineWrapIndentMode (Scintilla::WrapIndentMode mode);
	[[nodiscard]] Scintilla::WrapIndentMode getLineWrapIndentMode () const;
	void setLineWrapVisualFlags (Scintilla::WrapVisualFlag flags);
	[[nodiscard]] Scintilla::WrapVisualFlag getLineWrapVisualFlags () const;

	// ------------------------------------
	// Zoom
	void setZoom (int32_t zoom);
	[[nodiscard]] int32_t getZoom () const;

	// ------------------------------------
	// Lexer
	void setLexer (Scintilla::ILexer5* lexer);
	[[nodiscard]] Scintilla::ILexer5* getLexer () const;

	static Scintilla::ILexer5* createLexer (const char* name);

	/** set style color */
	void setStyleColor (uint32_t index, const CColor& textColor,
	                    const CColor& backColor = kTransparentCColor);
	/** set style font weight [1..999] where normal=400, semibold=600, bold=700 */
	void setStyleFontWeight (uint32_t index, uint32_t weight);

	// ------------------------------------
	// Low-level
	/** send a message to the scintilla backend */
	intptr_t sendMessage (uint32_t message, uintptr_t wParam, intptr_t lParam) const;

	template <typename MT, typename WPARAMT = uintptr_t, typename LPARAMT = intptr_t>
	intptr_t sendMessage (MT message, WPARAMT wParam = 0, LPARAMT lParam = 0) const
	{
		if
			constexpr (std::is_arithmetic_v<LPARAMT> || std::is_enum_v<LPARAMT>)
			{
				return sendMessage (static_cast<uint32_t> (message),
				                    static_cast<uintptr_t> (wParam),
				                    static_cast<intptr_t> (lParam));
			}
		else
			return sendMessage (static_cast<uint32_t> (message), static_cast<uintptr_t> (wParam),
			                    reinterpret_cast<intptr_t> (lParam));
	}

	void registerListener (IScintillaListener* listener);
	void unregisterListener (IScintillaListener* listener);

	// ------------------------------------
	bool attached (CView* parent) override;
	bool removed (CView* parent) override;
	void setViewSize (const CRect& rect, bool invalid) override;
	void setMouseEnabled (bool bEnable) override;
	void looseFocus () override;
	void takeFocus () override;

	struct Impl;

private:
	void init ();

	void onScintillaNotification (SCNotification* notification) override;
	void draw (CDrawContext* pContext) override;
	void platformSetBackgroundColor (const CColor& color);
	[[nodiscard]] bool showLineNumberMargin () const;
	[[nodiscard]] bool showFoldMargin () const;

	void updateMarginsColumns ();
	void updateLineNumberMarginWidth ();

	enum MarginsCol
	{
		LineNumber = 0,
		Folding
	};

	SharedPointer<CFontDesc> font;
	Scintilla::ILexer5* lexer {nullptr};
	uint32_t marginsCol {0};
	CColor foldMarginColorHi {kBlackCColor};
	CColor foldMarginColor {kWhiteCColor};

	std::unique_ptr<Impl> impl;
};

//------------------------------------------------------------------------
/** a helper to convert from CColor to a color scintilla understands */
inline intptr_t toScintillaColor (const CColor& c)
{
	auto red = static_cast<int32_t> (c.red);
	auto green = static_cast<int32_t> (c.green);
	auto blue = static_cast<int32_t> (c.blue);
	auto alpha = static_cast<int32_t> (c.alpha);
	return (alpha << 24) + (blue << 16) + (green << 8) + red;
}

//------------------------------------------------------------------------
inline CColor fromScintillaColor (intptr_t v)
{
	auto alpha = static_cast<int32_t> (v & 0xFF000000) >> 24;
	auto blue = static_cast<int32_t> (v & 0x00FF0000) >> 16;
	auto green = static_cast<int32_t> (v & 0x0000FF00) >> 8;
	auto red = static_cast<int32_t> (v & 0x000000FF);
	return {static_cast<uint8_t> (red), static_cast<uint8_t> (green), static_cast<uint8_t> (blue),
	        static_cast<uint8_t> (alpha)};
}

//------------------------------------------------------------------------
} // VSTGUI
