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
class ScintillaEditorView : public CView
{
public:
	ScintillaEditorView ();
	~ScintillaEditorView () noexcept;

	void setFont (const SharedPointer<CFontDesc>& font);
	SharedPointer<CFontDesc> getFont () const;

	/** set current text. (the same as sendMessage (SCI_SETTEXT, 0, text) */
	void setText (UTF8StringPtr text);

	struct Selection
	{
		int64_t start;
		int64_t end;
	};

	Selection getSelection () const;
	void setSelection (Selection selection);
	bool replaceSelection (UTF8StringPtr string);

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

	bool canUndo () const;
	bool canRedo () const;
	void undo ();
	void redo ();

	/** send a message to the scintilla backend */
	intptr_t sendMessage (uint32_t message, uintptr_t wParam, intptr_t lParam) const;

	template <typename T>
	intptr_t sendMessageT (uint32_t message, uintptr_t wParam, T lParam) const
	{
		return sendMessage (message, wParam, reinterpret_cast<intptr_t> (lParam));
	}

	void registerListener (IScintillaListener* listener);
	void unregisterListener (IScintillaListener* listener);

	// overwrites
	bool attached (CView* parent) override;
	bool removed (CView* parent) override;
	void setViewSize (const CRect& rect, bool invalid) override;
	void setMouseEnabled (bool bEnable = true) override;

	struct Impl;

private:
	void draw (CDrawContext* pContext) override;

	SharedPointer<CFontDesc> font;
	std::unique_ptr<Impl> impl;
};

//------------------------------------------------------------------------
/** a helper to convert from CColor to a color scintilla understands */
inline intptr_t toScintillaColor (const CColor& c)
{
	int32_t red = static_cast<int32_t> (c.red);
	int32_t green = static_cast<int32_t> (c.green);
	int32_t blue = static_cast<int32_t> (c.blue);
	return (blue << 16) + (green << 8) + red;
}

//------------------------------------------------------------------------
inline CColor fromScintillaColor (intptr_t v)
{
	int32_t blue = (v & 0x00FF0000) >> 16;
	int32_t green = (v & 0x0000FF00) >> 8;
	int32_t red = v & 0x000000FF;
	return CColor (static_cast<uint8_t> (red), static_cast<uint8_t> (green),
	               static_cast<uint8_t> (blue));
}

//------------------------------------------------------------------------
} // VSTGUI
