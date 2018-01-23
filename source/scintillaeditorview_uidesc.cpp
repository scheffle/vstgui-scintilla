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
using UIViewCreator::stringToColor;
using UIViewCreator::stringToBitmap;
using UIViewCreator::bitmapToString;
using UIViewCreator::colorToString;
using UIViewCreator::pointToString;

static std::string kAttrEditorFont = "editor-font";
static std::string kAttrEditorMargin = "editor-margin";
static std::string kAttrLineNumberFontColor = "line-number-font-color";
static std::string kAttrLineNumberBackgroundColor = "line-number-background-color";

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
		if (stringToColor (attr.getAttributeValue (kAttrLineNumberFontColor), color, desc))
		{
			sev->sendMessage (SCI_STYLESETFORE, STYLE_LINENUMBER, toScintillaColor (color));
		}
		if (stringToColor (attr.getAttributeValue (kAttrLineNumberBackgroundColor), color, desc))
		{
			sev->sendMessage (SCI_STYLESETBACK, STYLE_LINENUMBER, toScintillaColor (color));
		}
		if (stringToColor (attr.getAttributeValue (UIViewCreator::kAttrBackgroundColor), color,
		                   desc))
		{
			for (auto index = 0; index <= STYLE_DEFAULT; ++index)
				sev->sendMessage (SCI_STYLESETBACK, index, toScintillaColor (color));
		}
		if (auto fontName = attr.getAttributeValue (kAttrEditorFont))
		{
			if (auto font = desc->getFont (fontName->data ()))
			{
				sev->sendMessageT (SCI_STYLESETFONT, STYLE_DEFAULT, font->getName ().data ());
				sev->sendMessage (SCI_STYLESETSIZEFRACTIONAL, STYLE_DEFAULT,
				                  font->getSize () * static_cast<CCoord> (SC_FONT_SIZE_MULTIPLIER));
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
			auto fontNameStrLength = sev->sendMessage (SCI_STYLEGETFONT, STYLE_DEFAULT, 0);
			if (fontNameStrLength <= 0)
				return false;
			auto fontName = std::make_unique<char[]> (fontNameStrLength + 1);
			sev->sendMessageT (SCI_STYLEGETFONT, STYLE_DEFAULT, fontName.get ());
			auto fontSize = static_cast<CCoord> (
			                    sev->sendMessage (SCI_STYLEGETSIZEFRACTIONAL, STYLE_DEFAULT, 0)) /
			                static_cast<CCoord> (SC_FONT_SIZE_MULTIPLIER);
			CFontDesc font (fontName.get (), fontSize);
			if (auto name = desc->lookupFontName (&font))
			{
				stringValue = name;
				return true;
			}
			return false;
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
			auto color = fromScintillaColor (sev->sendMessage (SCI_STYLEGETBACK, STYLE_DEFAULT, 0));
			return colorToString (color, stringValue, desc);
		}

		return false;
	}
};
ScintillaEditorViewCreator __gScintillaEditorViewCreator;

//------------------------------------------------------------------------
} // VSTGUI
