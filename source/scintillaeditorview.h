//
//  scintillaeditorview.h
//  scintilla-example
//
//  Created by Arne Scheffler on 20.01.18.
//

#include "vstgui/lib/ccolor.h"
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

	bool attached (CView* parent) override;
	bool removed (CView* parent) override;
	void setViewSize (const CRect& rect, bool invalid) override;

	/** set current text. (the same as sendMessage (SCI_SETTEXT, 0, text) */
	void setText (UTF8StringPtr text);

	/** send a message to the scintilla backend */
	intptr_t sendMessage (uint32_t message, uintptr_t wParam, intptr_t lParam);

	template<typename T>
	intptr_t sendMessageT (uint32_t message, uintptr_t wParam, T lParam)
	{
		return sendMessage (message, wParam, reinterpret_cast<intptr_t>(lParam));
	}

	void registerListener (IScintillaListener* listener);
	void unregisterListener (IScintillaListener* listener);

	struct Impl;

private:
	void draw (CDrawContext* pContext) override;
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
