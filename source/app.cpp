// This file is part of VSTGUI. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this
// distribution and at http://github.com/steinbergmedia/vstgui/LICENSE

#include "scintillaeditorview.h"
#include "vstgui/lib/controls/csearchtextedit.h"
#include "vstgui/lib/iviewlistener.h"
#include "vstgui/standalone/include/helpers/appdelegate.h"
#include "vstgui/standalone/include/helpers/uidesc/customization.h"
#include "vstgui/standalone/include/helpers/preferences.h"
#include "vstgui/standalone/include/helpers/windowlistener.h"
#include "vstgui/standalone/include/iapplication.h"
#include "vstgui/standalone/include/iuidescwindow.h"
#include "vstgui/uidescription/delegationcontroller.h"

#include "Scintilla.h"
#include "SciLexer.h"
#include "ILexer.h"

using namespace VSTGUI;
using namespace VSTGUI::Standalone;
using namespace VSTGUI::Standalone::Application;

static Command FindNextCommand = {CommandGroup::Edit, "Find Next"};
static Command FindPreviousCommand = {CommandGroup::Edit, "Find Previous"};

//------------------------------------------------------------------------
class EditorController : public DelegationController,
                         public ICommandHandler,
                         public ViewListenerAdapter
{
public:
	EditorController (IController* parent) : DelegationController (parent) {}

	CView* verifyView (CView* view, const UIAttributes& attributes,
	                   const IUIDescription* description) override
	{
		if (auto ed = dynamic_cast<ScintillaEditorView*> (view))
		{
			editor = ed;
			editor->registerViewListener (this);

			if (auto lexer = editor->getLexer())
			{
				auto keywords =
				    R"(alignas alignof and and_eq asm auto bitand bitor bool break case catch char char16_t char32_t class compl const constexpr const_cast continue decltype default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline int long mutable namespace new noexcept not not_eq nullptr operator or or_eq private protected public register reinterpret_cast return short signed sizeof static static_assert static_cast struct switch template this thread_local throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while xor xor_eq)";
				lexer->WordListSet (0, keywords);
			}

			CColor commentColor;
			auto backgroundColor = editor->getBackgroundColor ();
			auto fontColor = editor->getStaticFontColor ();
			if (backgroundColor.getLightness () > fontColor.getLightness ())
				commentColor = backgroundColor;
			else
				commentColor = fontColor;
			double h, s, l;
			commentColor.toHSL (h, s, l);
			l *= 0.5;
			commentColor.fromHSL (h, s, l);
			editor->setStyleColor (SCE_C_COMMENT, commentColor);
			editor->setStyleColor (SCE_C_COMMENTLINE, commentColor);
			editor->setStyleColor (SCE_C_COMMENTDOC, commentColor);
			editor->setStyleFontWeight (SCE_C_WORD, 900);

			Preferences prefs;
			if (auto value = prefs.get ("EditorText"))
			{
				editor->setText (*value);
			}
		}
		else if (auto sf = dynamic_cast<CSearchTextEdit*> (view))
		{
			searchField = sf;
			searchField->setListener (this);
		}
		return controller->verifyView (view, attributes, description);
	}

	void valueChanged (CControl* control) override
	{
		if (control == searchField)
		{
			doFind ();
		}
	}

	void viewWillDelete (CView* view) override
	{
		if (view == editor)
		{
			auto text = editor->getText ();
			Preferences prefs;
			prefs.set ("EditorText", text);
			editor = nullptr;
		}
		view->unregisterViewListener (this);
	}

	bool canHandleCommand (const Command& command) override
	{
		if (command == FindNextCommand || command == FindPreviousCommand)
			return !searchField->getText ().empty ();
		if (command == Commands::Undo)
			return editor->canUndo ();
		if (command == Commands::Redo)
			return editor->canRedo ();
		return false;
	}
	bool handleCommand (const Command& command) override
	{
		if (command == FindNextCommand)
		{
			doFind ();
			return true;
		}
		if (command == FindPreviousCommand)
		{
			doFind (false);
			return true;
		}
		if (command == Commands::Undo)
		{
			editor->undo ();
			return true;
		}
		if (command == Commands::Redo)
		{
			editor->redo ();
			return true;
		}
		return false;
	}

	void doFind (bool next = true)
	{
		uint32_t flags = ScintillaEditorView::ScrollTo | ScintillaEditorView::Wrap;
		if (!next)
			flags |= ScintillaEditorView::Backwards;
		const auto& text = searchField->getText ();
		editor->findAndSelect (text.data (), flags);
	}
	
private:
	ScintillaEditorView* editor {nullptr};
	CSearchTextEdit* searchField {nullptr};
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
		auto& app = IApplication::instance ();
		app.registerCommand (FindNextCommand, 'g');
		app.registerCommand (FindPreviousCommand, 'G');
	}
	void onClosed (const IWindow& window) override { IApplication::instance ().quit (); }
};

static Init gAppDelegate (std::make_unique<MyApplication> ());
