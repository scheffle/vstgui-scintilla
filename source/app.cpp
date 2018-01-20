// This file is part of VSTGUI. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this
// distribution and at http://github.com/steinbergmedia/vstgui/LICENSE

#include "scintillaeditorview.h"
#include "vstgui/standalone/include/helpers/appdelegate.h"
#include "vstgui/standalone/include/helpers/uidesc/customization.h"
#include "vstgui/standalone/include/helpers/windowlistener.h"
#include "vstgui/standalone/include/iapplication.h"
#include "vstgui/standalone/include/iuidescwindow.h"
#include "vstgui/uidescription/delegationcontroller.h"

#include "Scintilla.h"
#include "SciLexer.h"

using namespace VSTGUI;
using namespace VSTGUI::Standalone;
using namespace VSTGUI::Standalone::Application;

//------------------------------------------------------------------------
class EditorController : public DelegationController
{
public:
	EditorController (IController* parent) : DelegationController (parent) {}

	CView* verifyView (CView* view, const UIAttributes& attributes, const IUIDescription* description) override
	{
		if (auto ed = dynamic_cast<ScintillaEditorView*>(view))
		{
			editor = ed;
			editor->sendMessage (SCI_SETLEXER, SCLEX_CPP, 0);
			editor->sendMessage(SCI_STYLESETFORE, STYLE_LINENUMBER, ScintillaEditorView::colorFromCColor(kBlackCColor));
			editor->sendMessage(SCI_STYLESETBACK, STYLE_LINENUMBER, ScintillaEditorView::colorFromCColor(kGreyCColor));
			editor->sendMessage(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
			editor->sendMessage(SCI_SETMARGINWIDTHN, 0, 35);
			editor->sendMessage(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<intptr_t>("Menlo"));
			editor->sendMessage(SCI_STYLESETSIZE, STYLE_DEFAULT, 14);
 			// [mEditor setGeneralProperty: SCI_SETLEXER parameter: SCLEX_CPP value: 0];
//			[mEditor setColorProperty:SCI_STYLESETFORE
//			                parameter:STYLE_LINENUMBER
//			                 fromHTML:@"#F0F0F0"];
//			[mEditor setColorProperty:SCI_STYLESETBACK
//			                parameter:STYLE_LINENUMBER
//			                 fromHTML:@"#808080"];
//
//			[mEditor setGeneralProperty:SCI_SETMARGINTYPEN parameter:0 value:SC_MARGIN_NUMBER];
//			[mEditor setGeneralProperty:SCI_SETMARGINWIDTHN parameter:0 value:35];
		}
		return controller->verifyView (view, attributes, description);
	}

private:
	ScintillaEditorView* editor {nullptr};
};

//------------------------------------------------------------------------
class MyApplication : public DelegateAdapter, public WindowListenerAdapter
{
public:
	MyApplication () : DelegateAdapter ({"scintilla-example", "1.0.0", "vstgui.examples.scintilla"})
	{
	}

	void finishLaunching () override
	{
		auto customization = UIDesc::Customization::make ();
		customization->addCreateViewControllerFunc (
		    "EditorController", [] (const auto& name, auto parent, const auto uiDesc) {
			    return new EditorController (parent);
		    });

		UIDesc::Config config;
		config.uiDescFileName = "Editor.uidesc";
		config.viewName = "Editor";
		config.windowConfig.title = "Editor";
		config.windowConfig.autoSaveFrameName = "EditorWindow";
		config.windowConfig.style.border ().close ().size ().centered ();
		config.customization = customization;
		if (auto window = UIDesc::makeWindow (config))
		{
			window->show ();
			window->registerWindowListener (this);
		}
		else
		{
			IApplication::instance ().quit ();
		}
	}
	void onClosed (const IWindow& window) override { IApplication::instance ().quit (); }
};

static Init gAppDelegate (std::make_unique<MyApplication> ());
