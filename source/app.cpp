// This file is part of VSTGUI. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this
// distribution and at http://github.com/steinbergmedia/vstgui/LICENSE

#include "scintillaeditorview.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/controls/csearchtextedit.h"
#include "vstgui/lib/iviewlistener.h"
#include "vstgui/standalone/include/helpers/appdelegate.h"
#include "vstgui/standalone/include/helpers/preferences.h"
#include "vstgui/standalone/include/helpers/uidesc/customization.h"
#include "vstgui/standalone/include/helpers/value.h"
#include "vstgui/standalone/include/helpers/windowlistener.h"
#include "vstgui/standalone/include/iapplication.h"
#include "vstgui/standalone/include/iuidescwindow.h"
#include "vstgui/uidescription/delegationcontroller.h"

#include "ILexer.h"
#include "SciLexer.h"

using namespace VSTGUI;
using namespace VSTGUI::Standalone;
using namespace VSTGUI::Standalone::Application;

static Command FindCommand = {CommandGroup::Edit, "Find..."};
static Command FindNextCommand = {CommandGroup::Edit, "Find Next"};
static Command FindPreviousCommand = {CommandGroup::Edit, "Find Previous"};
static Command UseSelectionForFindCommand = {CommandGroup::Edit, "Use Selection For Find"};
static Command ZoomInCommand = {"Zoom", "Zoom In"};
static Command ZoomOutCommand = {"Zoom", "Zoom Out"};
static Command ResetZoomCommand = {"Zoom", "Reset Zoom"};

//------------------------------------------------------------------------
class SearchModel : public UIDesc::IModelBinding
{
public:
	static std::shared_ptr<SearchModel>& instance ()
	{
		static auto model = std::make_shared<SearchModel> ();
		return model;
	}

	static constexpr auto WholeWordID = "WholeWord";
	static constexpr auto MatchCaseID = "MatchCase";
	static constexpr auto WordStartID = "WordStart";

	SearchModel ()
	{
		values.emplace_back (Value::makeStepValue (WholeWordID, 2));
		values.emplace_back (Value::makeStepValue (MatchCaseID, 2));
		values.emplace_back (Value::makeStepValue (WordStartID, 2));
	}

	bool matchCase () const { return getStepValue (MatchCaseID) != 0; }
	bool wholeWord () const { return getStepValue (WholeWordID) != 0; }
	bool wordStart () const { return getStepValue (WordStartID) != 0; }

private:
	uint32_t getStepValue (IdStringPtr valueID) const
	{
		auto it = std::find_if (values.begin (), values.end (),
		                        [&] (const auto& other) { return other->getID () == valueID; });
		if (it != values.end ())
		{
			if (auto stepValue = (*it)->dynamicCast<IStepValue> ())
			{
				return stepValue->valueToStep ((*it)->getValue ());
			}
		}
		return 0;
	}

	const ValueList& getValues () const override { return values; }

	ValueList values;
};

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
			if (auto lexer = ScintillaEditorView::createLexer ("cpp"))
			{
				auto keywords =
				    R"(alignas alignof and and_eq asm auto bitand bitor bool break case catch char char16_t char32_t class compl const constexpr const_cast continue decltype default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline int long mutable namespace new noexcept not not_eq nullptr operator or or_eq private protected public register reinterpret_cast return short signed sizeof static static_assert static_cast struct switch template this thread_local throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while xor xor_eq)";
				lexer->WordListSet (0, keywords);
				lexer->PropertySet ("fold", "1");
				lexer->PropertySet ("fold.comment", "1");
				lexer->PropertySet ("lexer.cpp.track.preprocessor", "0");
				editor->setLexer (lexer);

				CColor commentColor;
				auto fontColor = editor->getStaticFontColor ();
				auto backgroundColor = editor->getBackgroundColor ();
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
				editor->setStyleColor (SCE_C_PREPROCESSORCOMMENT, kRedCColor, backgroundColor);

			}
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
		if (command == ZoomInCommand || command == ZoomOutCommand || command == ResetZoomCommand)
			return true;
		if (command == FindCommand || command == Commands::SelectAll)
			return true;
		if (command == UseSelectionForFindCommand)
		{
			auto selection = editor->getSelection ();
			return selection.end != selection.start;
		}
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
		if (command == ZoomInCommand)
		{
			editor->setZoom (editor->getZoom () + 1);
			return true;
		}
		if (command == ZoomOutCommand)
		{
			auto zoom = editor->getZoom ();
			if (zoom > -10)
				editor->setZoom (zoom - 1);
			return true;
		}
		if (command == ResetZoomCommand)
		{
			editor->setZoom (0);
			return true;
		}
		if (command == FindCommand)
		{
			searchField->takeFocus ();
			return true;
		}
		if (command == UseSelectionForFindCommand)
		{
			auto selection = editor->getSelection ();
			auto text = editor->getText (selection);
			searchField->setText (text);
			return true;
		}
		if (command == Commands::SelectAll)
		{
			editor->selectAll ();
			return true;
		}
		return false;
	}

	void doFind (bool next = true)
	{
		uint32_t flags = ScintillaEditorView::ScrollTo | ScintillaEditorView::Wrap;
		if (!next)
			flags |= ScintillaEditorView::Backwards;
		const auto& searchModel = SearchModel::instance ();
		if (searchModel->matchCase ())
			flags |= ScintillaEditorView::MatchCase;
		if (searchModel->wordStart ())
			flags |= ScintillaEditorView::WordStart;
		if (searchModel->wholeWord ())
			flags |= ScintillaEditorView::WholeWord;
		const auto& text = searchField->getText ();
		editor->takeFocus ();
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
		config.modelBinding = SearchModel::instance ();
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
		app.registerCommand (FindCommand, 'f');
		app.registerCommand (FindNextCommand, 'g');
		app.registerCommand (FindPreviousCommand, 'G');
		app.registerCommand (UseSelectionForFindCommand, 'e');
		app.registerCommand (ZoomInCommand, '=');
		app.registerCommand (ZoomOutCommand, '-');
		app.registerCommand (ResetZoomCommand, '0');
	}
	void onClosed (const IWindow& window) override { IApplication::instance ().quit (); }
};

static Init gAppDelegate (std::make_unique<MyApplication> ());
