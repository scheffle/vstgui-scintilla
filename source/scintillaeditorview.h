//
//  scintillaeditorview.h
//  scintilla-example
//
//  Created by Arne Scheffler on 20.01.18.
//

#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/cview.h"
#include <memory>

struct SCNotification; // forward

namespace Scintilla { class ILexer5; };

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
	ScintillaEditorView ();
	~ScintillaEditorView () noexcept;

	/** set current text */
	void setText (UTF8StringPtr text);
	/** get current text */
	UTF8String getText () const;

	/** set font for all styles.
	 *	@param font font
	 */
	void setFont (const SharedPointer<CFontDesc>& font);
	/** get the font for all styles
	 *	@return font previously set with setFont
	 */
	SharedPointer<CFontDesc> getFont () const;

	void setStaticFontColor (const CColor& color);
	CColor getStaticFontColor () const;

	void setBackgroundColor (const CColor& color);
	CColor getBackgroundColor () const;

	void setCaretColor (const CColor& color);
	CColor getCaretColor () const;

	// ------------------------------------
	// Selection
	struct Selection
	{
		int64_t start;
		int64_t end;
	};

	Selection getSelection () const;
	void setSelection (Selection selection);
	bool replaceSelection (UTF8StringPtr string);

	void setSelectionBackgroundColor (const CColor& color);
	CColor getSelectionBackgroundColor () const;
	void setSelectionForegroundColor (const CColor& color);
	CColor getSelectionForegroundColor () const;

	void setInactiveSelectionBackgroundColor (const CColor& color);
	CColor getInactiveSelectionBackgroundColor () const;
	void setInactiveSelectionForegroundColor (const CColor& color);
	CColor getInactiveSelectionForegroundColor () const;

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
	bool canUndo () const;
	bool canRedo () const;
	void undo ();
	void redo ();

	// ------------------------------------
	// Tabs/Indentation
	void setUseTabs (bool state);
	bool getUseTabs () const;
	void setTabWidth (uint32_t width);
	uint32_t getTabWidth () const;

	// ------------------------------------
	// Line Numbers
	void setLineNumbersVisible (bool state);
	bool getLineNumbersVisible () const;
	void setLineNumberForegroundColor (const CColor& color);
	CColor getLineNumberForegroundColor () const;
	void setLineNumberBackgroundColor (const CColor& color);
	CColor getLineNumberBackgroundColor () const;

	// ------------------------------------
	// Folding
	void setFoldingVisible (bool state);
	bool getFoldingVisible () const;

#if 0 // not possible with scintilla 5.1
	void setFoldingForegroundColor (const CColor& color);
	CColor getFoldingForegroundColor () const;
	void setFoldingBackgroundColor (const CColor& color);
	CColor getFoldingBackgroundColor () const;
#endif

	// ------------------------------------
	// Lexer
	void setLexer (Scintilla::ILexer5* lexer);
	Scintilla::ILexer5* getLexer () const;

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
	void setMouseEnabled (bool bEnable = true) override;
	void looseFocus () override;
	void takeFocus () override;

	struct Impl;

private:
	void init ();

	void onScintillaNotification (SCNotification* notification) override;
	void draw (CDrawContext* pContext) override;
	void platformSetBackgroundColor (const CColor& color);
	void updateMarginsColumns ();

	SharedPointer<CFontDesc> font;
	Scintilla::ILexer5* lexer {nullptr};

	enum MarginsCol
	{
		LineNumber = 0,
		Folding
	};

	uint32_t marginsCol {0};
	std::unique_ptr<Impl> impl;
};

//------------------------------------------------------------------------
/** a helper to convert from CColor to a color scintilla understands */
inline intptr_t toScintillaColor (const CColor& c)
{
	int32_t red = static_cast<int32_t> (c.red);
	int32_t green = static_cast<int32_t> (c.green);
	int32_t blue = static_cast<int32_t> (c.blue);
	int32_t alpha = static_cast<int32_t> (c.alpha);
	return (alpha << 24) + (blue << 16) + (green << 8) + red;
}

//------------------------------------------------------------------------
inline CColor fromScintillaColor (intptr_t v)
{
	int32_t alpha = (v & 0xFF000000) >> 24;
	int32_t blue = (v & 0x00FF0000) >> 16;
	int32_t green = (v & 0x0000FF00) >> 8;
	int32_t red = v & 0x000000FF;
	return CColor (static_cast<uint8_t> (red), static_cast<uint8_t> (green),
	               static_cast<uint8_t> (blue), static_cast<uint8_t> (alpha));
}

//------------------------------------------------------------------------
} // VSTGUI
