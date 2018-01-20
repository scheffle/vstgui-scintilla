//
//  ScintillaView.h
//  scintilla-example
//
//  Created by Arne Scheffler on 20.01.18.
//

#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/ccolor.h"
#include <memory>

//------------------------------------------------------------------------
namespace VSTGUI {

//------------------------------------------------------------------------
class ScintillaEditorView : public CControl
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

	static intptr_t colorFromCColor (const CColor& c)
	{
		int32_t red = static_cast<int32_t> (c.red);
		int32_t green = static_cast<int32_t> (c.green);
		int32_t blue = static_cast<int32_t> (c.blue);
		return (blue << 16) + (green << 8) + red;
	}

	CLASS_METHODS_NOCOPY (ScintillaEditorView, CControl)
private:
	void draw (CDrawContext* pContext) override;
	struct Impl;
	std::unique_ptr<Impl> impl;
};

//------------------------------------------------------------------------
} // VSTGUI

