/*
 * Copyright 2014-2017 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui/MainMenu.h"

#include <iomanip>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "audio/Audio.h"
#include "core/Application.h"
#include "core/Config.h"
#include "core/Core.h"
#include "core/Localisation.h"
#include "core/SaveGame.h"
#include "core/Version.h"

#include "gui/Menu.h"
#include "gui/MenuWidgets.h"
#include "gui/MenuPublic.h"
#include "gui/Hud.h"
#include "gui/Text.h"
#include "gui/TextManager.h"
#include "gui/menu/MenuFader.h"
#include "gui/widget/CheckboxWidget.h"
#include "gui/widget/CycleTextWidget.h"
#include "gui/widget/PanelWidget.h"
#include "gui/widget/SliderWidget.h"
#include "graphics/Draw.h"
#include "graphics/Math.h"
#include "graphics/Renderer.h"
#include "graphics/font/Font.h"
#include "graphics/data/TextureContainer.h"
#include "input/Input.h"
#include "input/Keyboard.h"
#include "scene/GameSound.h"
#include "window/RenderWindow.h"

CWindowMenu * pWindowMenu = NULL;
TextWidget * pDeleteConfirm = NULL;
TextWidget * pLoadConfirm = NULL;
TextWidget * pDeleteButton = NULL;
TextWidget * pMenuElementApply = NULL;

extern MainMenu *mainMenu;

class NewQuestMenuPage : public MenuPage {
	
public:
	
	NewQuestMenuPage(const Vec2f & pos, const Vec2f & size)
		: MenuPage(pos, size, NEW_QUEST)
	{}
	
	~NewQuestMenuPage() { }
	
	void init() {
		
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_confirm");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText);
			txt->SetCheckOff();
			addCenter(txt, true);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_main_newquest_confirm");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText);
			txt->SetCheckOff();
			addCenter(txt, true);
		}
		
		{
			std::string szMenuText = getLocalised("system_yes");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(m_size.x - 40, 380));
			txt->clicked = boost::bind(ARXMenu_NewQuest);
			add(txt);
		}
		
		{
			std::string szMenuText = getLocalised("system_no");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(10, 380));
			txt->m_targetMenu = MAIN;
			txt->SetShortCut(Keyboard::Key_Escape);
			add(txt);
		}
	}
	
};

class ChooseLoadOrSaveMenuPage : public MenuPage {
	
public:
	
	ChooseLoadOrSaveMenuPage(const Vec2f & pos, const Vec2f & size)
		: MenuPage(pos, size, EDIT_QUEST)
	{}
	
	~ChooseLoadOrSaveMenuPage() { }
	
	void init() {
		
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_load");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f_ZERO);
			txt->clicked = boost::bind(&ChooseLoadOrSaveMenuPage::onClickLoad, this);
			txt->m_targetMenu = EDIT_QUEST_LOAD;
			txt->m_savegame = SavegameHandle();
			addCenter(txt, true);
		}
		
		{
			std::string szMenuText = getLocalised( "system_menus_main_editquest_save");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f_ZERO);
			txt->m_targetMenu = EDIT_QUEST_SAVE;
			
			if(!g_canResumeGame) {
				txt->SetCheckOff();
				txt->lColor = Color(127, 127, 127);
			}
			addCenter(txt, true);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(20, 380), Vec2f(16, 16), "graph/interface/menus/back");
			cb->m_targetMenu = MAIN;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
private:
	
	void onClickLoad() {
		if(pWindowMenu)
		for(size_t i = 0; i < pWindowMenu->m_pages.size(); i++) {
			MenuPage * page = pWindowMenu->m_pages[i];
			
			if(page->eMenuState == EDIT_QUEST_LOAD) {
				page->m_savegame = m_savegame;
				
				for(size_t j = 0; j < page->m_children.m_widgets.size(); j++) {
					Widget * widget = page->m_children.m_widgets[j];
					
					if(widget->m_id == BUTTON_MENUEDITQUEST_LOAD) {
						((TextWidget *)widget)->bSelected = false;
					}
				}
			}
		}
	}
	
};

class LoadMenuPage : public MenuPage {
	
public:
	
	LoadMenuPage(const Vec2f & pos, const Vec2f & size)
		: MenuPage(pos, size, EDIT_QUEST_LOAD)
	{}
	
	~LoadMenuPage() { }
	
	void init() {
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f_ZERO, Vec2f(48, 48), "graph/interface/icons/menu_main_load");
			cb->SetCheckOff();
			addCenter(cb, true);
		}
		
		std::string quicksaveName = getLocalised("system_menus_main_quickloadsave", "Quicksave");
		
		// TODO make this list scrollable
		// TODO align the date part to the right!
		
		{
		size_t quicksaveNum = 0;
		
		// Show quicksaves.
		for(size_t i = 0; i < savegames.size(); i++) {
			const SaveGame & save = savegames[i];
			
			if(!save.quicksave) {
				continue;
			}
			
			std::ostringstream text;
			text << quicksaveName << ' ' << ++quicksaveNum << "   " << save.time;
			
			TextWidget * txt = new TextWidget(BUTTON_MENUEDITQUEST_LOAD, hFontControls, text.str(), Vec2f(20, 0));
			txt->m_savegame = SavegameHandle(i);
			addCenter(txt);
		}
		
		// Show regular saves.
		for(size_t i = 0; i < savegames.size(); i++) {
			const SaveGame & save = savegames[i];
			
			if(save.quicksave) {
				continue;
			}
			
			std::string text = save.name +  "   " + save.time;
			
			TextWidget * txt = new TextWidget(BUTTON_MENUEDITQUEST_LOAD, hFontControls, text, Vec2f(20, 0));
			txt->m_savegame = SavegameHandle(i);
			addCenter(txt);
		}
		
		{
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontControls, std::string(), Vec2f(20, 0));
			txt->m_targetMenu = EDIT_QUEST_SAVE_CONFIRM;
			txt->SetCheckOff();
			txt->m_savegame = SavegameHandle();
			addCenter(txt);
		}
		
		// Delete button
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_delete");
			szMenuText += "   ";
			TextWidget * txt = new TextWidget(BUTTON_MENUEDITQUEST_DELETE_CONFIRM, hFontMenu, szMenuText, Vec2f_ZERO);
			txt->m_targetMenu = EDIT_QUEST_LOAD;
			txt->SetPos(Vec2f(RATIO_X(m_size.x-10)-txt->m_rect.width(), RATIO_Y(54)));
			txt->SetCheckOff();
			txt->lOldColor = txt->lColor;
			txt->lColor = Color::grayb(127);
			add(txt);
			pDeleteConfirm = txt;
		}
		
		// Load button
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_load");
			szMenuText += "   ";
			TextWidget * txt = new TextWidget(BUTTON_MENUEDITQUEST_LOAD_CONFIRM, hFontMenu, szMenuText, Vec2f_ZERO);
			txt->m_targetMenu = MAIN;
			txt->SetPos(Vec2f(RATIO_X(m_size.x-10)-txt->m_rect.width(), RATIO_Y(380) + RATIO_Y(40)));
			txt->SetCheckOff();
			txt->lOldColor = txt->lColor;
			txt->lColor = Color::grayb(127);
			add(txt);
			pLoadConfirm = txt;
		}
		
		// Back button
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(20, 420), Vec2f(16, 16), "graph/interface/menus/back");
			cb->clicked = boost::bind(&LoadMenuPage::onClickBack, this);
			cb->m_targetMenu = EDIT_QUEST;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
		}
	}
	
private:
	
	void onClickBack() {
		pLoadConfirm->SetCheckOff();
		pLoadConfirm->lColor = Color::grayb(127);
		pDeleteConfirm->SetCheckOff();
		pDeleteConfirm->lColor = Color::grayb(127);
	}
	
};

class SaveMenuPage : public MenuPage {
	
public:
	
	SaveMenuPage(const Vec2f & pos, const Vec2f & size)
		: MenuPage(pos, size, EDIT_QUEST_SAVE)
	{}
	
	~SaveMenuPage() { }
	
	void init() {
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(10, 0), Vec2f(48, 48), "graph/interface/icons/menu_main_save");
			cb->SetCheckOff();
			addCenter(cb, true);
		}
		
		std::string quicksaveName = getLocalised("system_menus_main_quickloadsave", "Quicksave");
		size_t quicksaveNum = 0;
		
		// Show quicksaves.
		for(size_t i = 0; i < savegames.size(); i++) {
			const SaveGame & save = savegames[i];
			
			if(!save.quicksave) {
				continue;
			}
			
			std::ostringstream text;
			text << quicksaveName << ' ' << ++quicksaveNum << "   " << save.time;
			
			TextWidget * txt = new TextWidget(BUTTON_MENUEDITQUEST_SAVEINFO, hFontControls, text.str(), Vec2f(20, 0));
			txt->m_targetMenu = EDIT_QUEST_SAVE_CONFIRM;
			txt->setColor(Color::grayb(127));
			txt->SetCheckOff();
			txt->m_savegame = SavegameHandle(i);
			addCenter(txt);
		}
		
		// Show regular saves.
		for(size_t i = 0; i < savegames.size(); i++) {
			const SaveGame & save = savegames[i];
			
			if(save.quicksave) {
				continue;
			}
			
			std::string text = save.name +  "   " + save.time;
			
			TextWidget * txt = new TextWidget(BUTTON_MENUEDITQUEST_SAVEINFO, hFontControls, text, Vec2f(20, 0));
			txt->m_targetMenu = EDIT_QUEST_SAVE_CONFIRM;
			txt->m_savegame = SavegameHandle(i);
			addCenter(txt);
		}
		
		for(size_t i = savegames.size(); i <= 15; i++) {
			
			std::ostringstream text;
			text << '-' << std::setfill('0') << std::setw(4) << i << '-';
			
			TextWidget * txt = new TextWidget(BUTTON_MENUEDITQUEST_SAVEINFO, hFontControls, text.str(), Vec2f(20, 0));
			txt->m_targetMenu = EDIT_QUEST_SAVE_CONFIRM;
			txt->m_savegame = SavegameHandle();
			addCenter(txt);
		}
	
		{
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontControls, std::string(), Vec2f(20, 0));
			txt->m_targetMenu = EDIT_QUEST_SAVE_CONFIRM;
			txt->m_savegame = SavegameHandle();
			txt->SetCheckOff();
			addCenter(txt);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(20, 420), Vec2f(16, 16), "graph/interface/menus/back");
			cb->m_targetMenu = EDIT_QUEST;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
};

class SaveConfirmMenuPage : public MenuPage {
	
public:
	
	SaveConfirmMenuPage(const Vec2f & pos, const Vec2f & size)
		: MenuPage(pos, size, EDIT_QUEST_SAVE_CONFIRM)
	{}
	
	~SaveConfirmMenuPage() { }
	
	void init() {
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f_ZERO, Vec2f(48, 48), "graph/interface/icons/menu_main_save");
			cb->SetCheckOff();
			addCenter(cb, true);
		}
		
		{
			std::string szMenuText = getLocalised("system_menu_editquest_newsavegame", "---");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->m_savegame = SavegameHandle();
			txt->eState=EDIT;
			txt->ePlace=CENTER;
			addCenter(txt, true);
		}
		
		// Delete button
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_delete");
			TextWidget * txt = new TextWidget(BUTTON_MENUEDITQUEST_DELETE, hFontMenu, szMenuText, Vec2f_ZERO);
			txt->m_targetMenu = EDIT_QUEST_SAVE;
			txt->SetPos(Vec2f(RATIO_X(m_size.x-10)-txt->m_rect.width(), RATIO_Y(5)));
			txt->lOldColor = txt->lColor;
			add(txt);
			pDeleteButton = txt;
		}
		
		// Save button
		{
			std::string szMenuText = getLocalised("system_menus_main_editquest_save");
			TextWidget * txt = new TextWidget(BUTTON_MENUEDITQUEST_SAVE, hFontMenu, szMenuText, Vec2f_ZERO);
			txt->m_targetMenu = MAIN;
			txt->SetPos(Vec2f(RATIO_X(m_size.x-10)-txt->m_rect.width(), RATIO_Y(380)));
			add(txt);
		}
		
		// Back button
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(20, 380), Vec2f(16, 16), "graph/interface/menus/back");
			cb->m_targetMenu = EDIT_QUEST_SAVE;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
};

int newWidth;
int newHeight;
bool newFullscreen;

class OptionsMenuPage : public MenuPage {
	
public:
	
	OptionsMenuPage(const Vec2f & pos, const Vec2f & size)
		: MenuPage(pos, size, OPTIONS)
	{}
	
	~OptionsMenuPage() { }
	
	void init() {
		
		{
			std::string szMenuText = getLocalised("system_menus_options_video");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f_ZERO);
			txt->clicked = boost::bind(&OptionsMenuPage::onClickedVideo, this);
			txt->m_targetMenu = OPTIONS_VIDEO;
			addCenter(txt, true);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_interface",
			                                      "Interface settings");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f_ZERO);
			txt->m_targetMenu = OPTIONS_INTERFACE;
			addCenter(txt, true);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_audio");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f_ZERO);
			txt->m_targetMenu = OPTIONS_AUDIO;
			addCenter(txt, true);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_input");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f_ZERO);
			txt->m_targetMenu = OPTIONS_INPUT;
			addCenter(txt, true);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(20, 380), Vec2f(16, 16), "graph/interface/menus/back");
			cb->m_targetMenu = MAIN;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
private:
	
	void onClickedVideo() {
		newWidth = config.video.resolution.x;
		newHeight = config.video.resolution.y;
		newFullscreen = config.video.fullscreen;
	}
	
};

// TODO remove this
const std::string AUTO_RESOLUTION_STRING = "Desktop";

class VideoOptionsMenuPage : public MenuPage {
	
public:
	
	VideoOptionsMenuPage(const Vec2f & pos, const Vec2f & size)
		: MenuPage(pos, size, OPTIONS_VIDEO)
		, m_minimizeOnFocusLostCheckbox(NULL)
	{
		fullscreenCheckbox = NULL;
		pMenuSliderResol = NULL;
	}
	
	~VideoOptionsMenuPage() { }
	
	CheckboxWidget * fullscreenCheckbox;
	CycleTextWidget * pMenuSliderResol;
	CheckboxWidget * m_minimizeOnFocusLostCheckbox;
	
	void init() {
		
		// Renderer selection
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_video_renderer", "Renderer");
			szMenuText += "  ";
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			CycleTextWidget * slider = new CycleTextWidget;
			slider->valueChanged = boost::bind(&VideoOptionsMenuPage::onChangedRenderer, this, _1, _2);
			
			{
				TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, "Auto-Select", Vec2f_ZERO);
				slider->AddText(txt);
				slider->selectLast();
			}
			
			{
				TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, "OpenGL", Vec2f_ZERO);
				slider->AddText(txt);
				if(config.video.renderer == "OpenGL") {
					slider->selectLast();
				}
			}
			
			float fRatio    = (RATIO_X(m_size.x-9) - slider->m_rect.width());
			slider->Move(Vec2f(fRatio, 0));
			panel->AddElement(slider);
			addCenter(panel);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_videos_full_screen");
			if(szMenuText.empty()) {
				// TODO once we ship our own amendmends to the loc files a cleaner
				// fix would be to just define system_menus_options_videos_full_screen
				// for the german version there
				szMenuText = getLocalised("system_menus_options_video_full_screen");
			}
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&VideoOptionsMenuPage::onChangedFullscreen, this, _1);
			cb->iState = config.video.fullscreen ? 1 : 0;
			addCenter(cb);
			fullscreenCheckbox = cb;
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_video_resolution");
			szMenuText += "  ";
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			pMenuSliderResol = new CycleTextWidget;
			pMenuSliderResol->valueChanged = boost::bind(&VideoOptionsMenuPage::onChangedResolution, this, _1, _2);
			
			pMenuSliderResol->setEnabled(config.video.fullscreen);
			
			const RenderWindow::DisplayModes & modes = mainApp->getWindow()->getDisplayModes();
			for(size_t i = 0; i != modes.size(); ++i) {
				
				const DisplayMode & mode = modes[i];
				
				// find the aspect ratio
				unsigned a = mode.resolution.x;
				unsigned b = mode.resolution.y;
				while(b != 0) {
					unsigned t = a % b;
					a = b, b = t;
				}
				Vec2i aspect = mode.resolution / Vec2i(a);
				
				std::stringstream ss;
				ss << mode;
				
				if(aspect.x < 100 && aspect.y < 100) {
					if(aspect == Vec2i(8, 5)) {
						aspect = Vec2i(16, 10);
					}
					ss << " (" << aspect.x << ':' << aspect.y << ')';
				}
				
				pMenuSliderResol->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, ss.str()));
				
				if(mode.resolution == config.video.resolution) {
					pMenuSliderResol->selectLast();
				}
			}
			
			pMenuSliderResol->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, AUTO_RESOLUTION_STRING));
			
			if(config.video.resolution == Vec2i_ZERO) {
				pMenuSliderResol->selectLast();
			}
		
			float fRatio    = (RATIO_X(m_size.x-9) - pMenuSliderResol->m_rect.width()); 
		
			pMenuSliderResol->Move(Vec2f(fRatio, 0));
			
			panel->AddElement(pMenuSliderResol);
			addCenter(panel);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_videos_minimize_on_focus_lost",
			                                      "Minimize on focus loss");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&VideoOptionsMenuPage::onChangedMinimizeOnFocusLost, this, _1);
			setMinimizeOnFocusLostState(cb, config.video.fullscreen);
			addCenter(cb);
			m_minimizeOnFocusLostCheckbox = cb;
		}
		
		{
			// Add spacing
			addCenter(new TextWidget(BUTTON_INVALID, hFontMenu, std::string(), Vec2f(20, 0)));
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_detail");
			szMenuText += " ";
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			
			CycleTextWidget * cb = new CycleTextWidget;
			cb->valueChanged = boost::bind(&VideoOptionsMenuPage::onChangedQuality, this, _1, _2);
			szMenuText = getLocalised("system_menus_options_video_texture_low");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			szMenuText = getLocalised("system_menus_options_video_texture_med");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			szMenuText = getLocalised("system_menus_options_video_texture_high");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			cb->setValue(config.video.levelOfDetail);
			
			cb->Move(Vec2f(RATIO_X(m_size.x-9) - cb->m_rect.width(), 0));
			panel->AddElement(cb);
			
			addCenter(panel);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_video_brouillard");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			SliderWidget * sld = new SliderWidget(Vec2f(200, 0));
			sld->valueChanged = boost::bind(&VideoOptionsMenuPage::onChangedFogDistance, this, _1);
			sld->setValue(config.video.fogDistance);
			panel->AddElement(sld);
			addCenter(panel);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_video_antialiasing", "antialiasing");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&VideoOptionsMenuPage::onChangedAntialiasing, this, _1);
			cb->iState = config.video.antialiasing ? 1 : 0;
			addCenter(cb);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_video_vsync", "VSync");
			szMenuText += " ";
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			
			CycleTextWidget * cb = new CycleTextWidget;
			cb->valueChanged = boost::bind(&VideoOptionsMenuPage::onChangedVSync, this, _1, _2);
			szMenuText = getLocalised("system_menus_options_video_vsync_off", "Off");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			szMenuText = getLocalised("system_menus_options_video_vsync_on", "On");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			szMenuText = getLocalised("system_menus_options_video_vsync_auto", "Adaptive");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			cb->setValue(config.video.vsync < 0 ? 2 : config.video.vsync);
			
			cb->Move(Vec2f(RATIO_X(m_size.x-9) - cb->m_rect.width(), 0));
			panel->AddElement(cb);
			
			addCenter(panel);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_video_texture_filter_anisotropic",
			                                      "Anisotropic filtering");
			szMenuText += " ";
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			
			CycleTextWidget * cb = new CycleTextWidget;
			cb->valueChanged = boost::bind(&VideoOptionsMenuPage::onChangedMaxAnisotropy, this, _1, _2);
			szMenuText = getLocalised("system_menus_options_video_filter_anisotropic_off", "Off");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			int maxAnisotropy = int(GRenderer->getMaxSupportedAnisotropy());
			int selected = 0;
			if(maxAnisotropy > 1) {
				int i = 1;
				std::ostringstream oss;
				for(int anisotropy = 2; ; anisotropy *= 2, i++) {
					if(anisotropy > maxAnisotropy) {
						anisotropy = maxAnisotropy;
					}
					oss.str(std::string());
					oss << 'x' << anisotropy;
					cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, oss.str()));
					if(config.video.maxAnisotropicFiltering == anisotropy) {
						selected = i;
					}
					if(config.video.maxAnisotropicFiltering > anisotropy
					   && config.video.maxAnisotropicFiltering < anisotropy * 2
					   && config.video.maxAnisotropicFiltering <= maxAnisotropy) {
						oss.str(std::string());
						oss << 'x' << config.video.maxAnisotropicFiltering;
						cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, oss.str()));
						selected = ++i;
					}
					if(anisotropy == maxAnisotropy) {
						i++;
						break;
					}
				}
				szMenuText = getLocalised("system_menus_options_video_filter_anisotropic_max", "Unlimited");
				cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
				if(config.video.maxAnisotropicFiltering > maxAnisotropy) {
					selected = i;
				}
			}
			cb->setValue(selected);
			cb->Move(Vec2f(RATIO_X(m_size.x-9) - cb->m_rect.width(), 0));
			if(maxAnisotropy <= 1) {
				cb->setEnabled(false);
			}
			panel->AddElement(cb);
			
			addCenter(panel);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_video_apply");
			szMenuText += "   ";
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(240, 0));
			txt->clicked = boost::bind(&VideoOptionsMenuPage::onClickedApply, this);
			txt->SetPos(Vec2f(RATIO_X(m_size.x-10)-txt->m_rect.width(), RATIO_Y(380) + RATIO_Y(40)));
			txt->SetCheckOff();
			add(txt);
			pMenuElementApply = txt;
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(20, 420), Vec2f(16, 16), "graph/interface/menus/back");
			cb->clicked = boost::bind(&VideoOptionsMenuPage::onClickedBack, this);
			cb->m_targetMenu = OPTIONS;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
private:
	
	void setMinimizeOnFocusLostState(CheckboxWidget * cb, bool fullscreen) {
		
		if(!fullscreen) {
			cb->iState = 0;
			cb->SetCheckOff();
			return;
		}
		
		Window::MinimizeSetting minimize = mainApp->getWindow()->willMinimizeOnFocusLost();
		cb->iState = (minimize == Window::Enabled || minimize == Window::AlwaysEnabled) ? 1 : 0;
		if(minimize != Window::AlwaysDisabled && minimize != Window::AlwaysEnabled) {
			cb->SetCheckOn();
		} else {
			cb->SetCheckOff();
		}
		
	}
	
	void onChangedRenderer(int pos, const std::string & str) {
		ARX_UNUSED(str);
		
		switch(pos) {
			case 0:  config.video.renderer = "auto"; break;
			case 1:  config.video.renderer = "OpenGL";  break;
			default: config.video.renderer = "auto"; break;
		}
	}
	
	void onChangedFullscreen(int state) {
		newFullscreen = ((state)?true:false);
		
		if(pMenuSliderResol) {
			pMenuSliderResol->setEnabled(newFullscreen);
			setMinimizeOnFocusLostState(m_minimizeOnFocusLostCheckbox, newFullscreen);
		}
	}
	
	void onChangedResolution(int pos, const std::string & str) {
		ARX_UNUSED(pos);
		
		if(str == AUTO_RESOLUTION_STRING) {
			newWidth = newHeight = 0;
		} else {
			std::stringstream ss(str);
			int iX = config.video.resolution.x;
			int iY = config.video.resolution.y;
			char tmp;
			ss >> iX >> tmp >> iY;
			newWidth = iX;
			newHeight = iY;
		}
	}
	
	void onChangedMinimizeOnFocusLost(int state) {
		config.window.minimizeOnFocusLost = state ? true : false;
		mainApp->getWindow()->setMinimizeOnFocusLost(config.window.minimizeOnFocusLost);
	}
	
	void onChangedQuality(int pos, const std::string & str) {
		ARX_UNUSED(str);
		
		ARXMenu_Options_Video_SetDetailsQuality(pos);
	}
	
	void onChangedFogDistance(int value) {
		ARXMenu_Options_Video_SetFogDistance(value);
	}
	
	void onChangedAntialiasing(int state) {
		config.video.antialiasing = state ? true : false;
	}
	
	void onChangedVSync(int pos, const std::string & str) {
		ARX_UNUSED(str);
		config.video.vsync = pos > 1 ? -1 : pos;
		mainApp->getWindow()->setVSync(config.video.vsync);
	}
	
	void onChangedMaxAnisotropy(int pos, const std::string & str) {
		ARX_UNUSED(str);
		
		int anisotropy = 1;
		if(pos > 0) {
			if(!str.empty() && str[0] == 'x') {
				std::stringstream ss(str.substr(1));
				ss >> anisotropy;
			} else {
				anisotropy = 9001;
			}
		}
		
		config.video.maxAnisotropicFiltering = anisotropy;
		GRenderer->setMaxAnisotropy(anisotropy);
	}
	
	void onClickedBack() {
		if(pMenuSliderResol && pMenuSliderResol->getOldValue() >= 0) {
			pMenuSliderResol->setValue(pMenuSliderResol->getOldValue());
			pMenuSliderResol->setOldValue(-1);
			newWidth=config.video.resolution.x;
			newHeight=config.video.resolution.y;
		}
		
		if(fullscreenCheckbox && fullscreenCheckbox->iOldState >= 0) {
			fullscreenCheckbox->iState = fullscreenCheckbox->iOldState;
			fullscreenCheckbox->iOldState = -1;
			newFullscreen = config.video.fullscreen;
		}
	}
	
	void onClickedApply() {
		if(newWidth != config.video.resolution.x
		   || newHeight!=config.video.resolution.y
		   || newFullscreen != config.video.fullscreen
		) {
			ARXMenu_Private_Options_Video_SetResolution(newFullscreen, newWidth, newHeight);
			pMenuSliderResol->setOldValue(-1);
			fullscreenCheckbox->iOldState = -1;
		}
		mainMenu->bReInitAll=true;
	}
};

class InterfaceOptionsMenuPage : public MenuPage {
	
public:
	
	InterfaceOptionsMenuPage(const Vec2f & pos, const Vec2f & size)
		: MenuPage(pos, size, OPTIONS_INTERFACE)
	{ }
	
	~InterfaceOptionsMenuPage() { }
	
	void init() {
		
		{
			std::string szMenuText = getLocalised("system_menus_options_video_crosshair",
			                         "Cross hair cursor");
			szMenuText = getLocalised("system_menus_options_interface_crosshair", szMenuText);
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&InterfaceOptionsMenuPage::onChangedCrosshair, this, _1);
			cb->iState = config.interface.showCrosshair ? 1 : 0;
			addCenter(cb);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_interface_limit_speech_width",
			                         "Limit speech text width");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&InterfaceOptionsMenuPage::onChangedSpeechWidth, this, _1);
			cb->iState = config.interface.limitSpeechWidth ? 1 : 0;
			addCenter(cb);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_interface_cinematic_widescreen_mode",
			                                      "Cinematics mode");
			szMenuText += " ";
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			
			CycleTextWidget * cb = new CycleTextWidget;
			cb->valueChanged = boost::bind(&InterfaceOptionsMenuPage::onChangedCinematicMode, this, _1, _2);
			szMenuText = getLocalised("system_menus_options_interface_letterbox", "Letterbox");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			szMenuText = getLocalised("system_menus_options_interface_hard_edges", "Hard Edges");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			szMenuText = getLocalised("system_menus_options_interface_fade_edges", "Fade Edges");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			cb->setValue(config.interface.cinematicWidescreenMode);
			
			cb->Move(Vec2f(RATIO_X(m_size.x-9) - cb->m_rect.width(), 0));
			panel->AddElement(cb);
			
			addCenter(panel);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_interface_hud_scale",
			                                      "HUD size");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			SliderWidget * sld = new SliderWidget(Vec2f(200, 0));
			sld->valueChanged = boost::bind(&InterfaceOptionsMenuPage::onChangedHudScale, this, _1);
			sld->setValue(config.interface.hudScale * 10.f);
			panel->AddElement(sld);
			addCenter(panel);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_interface_hud_scale_integer",
			                                      "Round HUD scale factor");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&InterfaceOptionsMenuPage::onChangedHudScaleInteger, this, _1);
			cb->iState = config.interface.hudScaleInteger ? 1 : 0;
			addCenter(cb);
		}

		{
			std::string szMenuText = getLocalised("system_menus_options_interface_scale_cursor_with_hud",
												  "Scale cursor with HUD");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&InterfaceOptionsMenuPage::onChangedScaleCursorWithHud, this, _1);
			cb->iState = config.interface.scaleCursorWithHud ? 1 : 0;
			addCenter(cb);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_interface_hud_scale_filter",
			                                      "HUD scale filter");
			szMenuText += " ";
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			
			CycleTextWidget * cb = new CycleTextWidget;
			cb->valueChanged = boost::bind(&InterfaceOptionsMenuPage::onChangedHudScaleFilter, this, _1, _2);
			szMenuText = getLocalised("system_menus_options_video_filter_nearest", "Nearest");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			szMenuText = getLocalised("system_menus_options_video_filter_bilinear", "Bilinear");
			cb->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText));
			cb->setValue(config.interface.hudScaleFilter);
			
			cb->Move(Vec2f(RATIO_X(m_size.x-9) - cb->m_rect.width(), 0));
			panel->AddElement(cb);
			
			addCenter(panel);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(20, 420), Vec2f(16, 16), "graph/interface/menus/back");
			cb->m_targetMenu = OPTIONS;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
private:
	
	void onChangedCrosshair(int state) {
		config.interface.showCrosshair = state ? true : false;
	}
	
	void onChangedSpeechWidth(int state) {
		config.interface.limitSpeechWidth = state ? true : false;
	}
	
	void onChangedCinematicMode(int pos, const std::string & str) {
		ARX_UNUSED(str);
		
		config.interface.cinematicWidescreenMode = CinematicWidescreenMode(pos);
	}
	
	void onChangedHudScale(int state) {
		config.interface.hudScale = float(state) * 0.1f;
		g_hudRoot.recalcScale();
	}
	
	void onChangedHudScaleInteger(int state) {
		config.interface.hudScaleInteger = state ? true : false;
		g_hudRoot.recalcScale();
	}

	void onChangedScaleCursorWithHud(int state) {
		config.interface.scaleCursorWithHud = state ? true : false;
	}
	
	void onChangedHudScaleFilter(int pos, const std::string & str) {
		ARX_UNUSED(str);
		
		config.interface.hudScaleFilter = UIScaleFilter(pos);
	}
	
};


class AudioOptionsMenuPage : public MenuPage {
	
public:
	
	AudioOptionsMenuPage(const Vec2f & pos, const Vec2f & size)
		: MenuPage(pos, size, OPTIONS_AUDIO)
	{}
	
	~AudioOptionsMenuPage() { }
	
	void init() {
		
		// Audio backend selection
		{
			
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_audio_device", "Device");
			szMenuText += "  ";
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			CycleTextWidget * slider = new CycleTextWidget;
			slider->valueChanged = boost::bind(&AudioOptionsMenuPage::onChangedDevice, this, _1, _2);
			
			float maxwidth = RATIO_X(m_size.x - 28) - txt->m_rect.width() - slider->m_rect.width();
			
			slider->AddText(new TextWidget(BUTTON_INVALID, hFontControls, "Default"));
			slider->selectLast();
			
			BOOST_FOREACH(const std::string & device, audio::getDevices()) {
				TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontControls, device);
				if(txt->m_rect.width() > maxwidth) {
					txt->m_rect.right = txt->m_rect.left + maxwidth;
				}
				slider->AddText(txt);
				if(config.audio.device == device) {
					slider->selectLast();
				}
			}
			
			float fRatio    = (RATIO_X(m_size.x-9) - slider->m_rect.width());
			slider->Move(Vec2f(fRatio, 0));
			panel->AddElement(slider);
			addCenter(panel);
			
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_audio_master_volume");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			SliderWidget * sld = new SliderWidget(Vec2f(200, 0));
			sld->valueChanged = boost::bind(&AudioOptionsMenuPage::onChangedMasterVolume, this, _1);
			sld->setValue((int)config.audio.volume); // TODO use float sliders
			panel->AddElement(sld);
			addCenter(panel);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_audio_effects_volume");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			SliderWidget * sld = new SliderWidget(Vec2f(200, 0));
			sld->valueChanged = boost::bind(&AudioOptionsMenuPage::onChangedEffectsVolume, this, _1);
			sld->setValue((int)config.audio.sfxVolume);
			panel->AddElement(sld);
			addCenter(panel);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_audio_speech_volume");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			SliderWidget * sld = new SliderWidget(Vec2f(200, 0));
			sld->valueChanged = boost::bind(&AudioOptionsMenuPage::onChangedSpeechVolume, this, _1);
			sld->setValue((int)config.audio.speechVolume);
			panel->AddElement(sld);
			addCenter(panel);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_audio_ambiance_volume");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			SliderWidget * sld = new SliderWidget(Vec2f(200, 0));
			sld->valueChanged = boost::bind(&AudioOptionsMenuPage::onChangedAmbianceVolume, this, _1);
			sld->setValue((int)config.audio.ambianceVolume);
			panel->AddElement(sld);
			addCenter(panel);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_audio_mute_on_focus_lost", "Mute when not focused");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&AudioOptionsMenuPage::onChangedMuteOnFocusLost, this, _1);
			cb->iState = config.audio.muteOnFocusLost ? 1 : 0;
			addCenter(cb);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_audio_eax", "EAX");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&AudioOptionsMenuPage::onChangedEax, this, _1);
			if(audio::isReverbSupported()) {
				cb->iState = config.audio.eax ? 1 : 0;
			} else {
				cb->SetCheckOff();
			}
			addCenter(cb);
		}
		
		audio::HRTFStatus hrtf = audio::getHRTFStatus();
		if(hrtf != audio::HRTFUnavailable) {
			
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_audio_hrtf", "Virtual surround");
			szMenuText += "  ";
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			
			Widget * value;
			if(hrtf == audio::HRTFRequired) {
				value = new TextWidget(BUTTON_INVALID, hFontMenu, "Enabled");
				value->SetCheckOff();
			} else if(hrtf == audio::HRTFForbidden) {
				value = new TextWidget(BUTTON_INVALID, hFontMenu, "Disabled");
				value->SetCheckOff();
			} else {
				CycleTextWidget * slider = new CycleTextWidget;
				slider->valueChanged = boost::bind(&AudioOptionsMenuPage::onChangedHRTF, this, _1, _2);
				slider->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, "Disabled"));
				if(config.audio.hrtf == audio::HRTFDisable) {
					slider->selectLast();
				}
				slider->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, "Automatic"));
				if(config.audio.hrtf == audio::HRTFDefault) {
					slider->selectLast();
				}
				slider->AddText(new TextWidget(BUTTON_INVALID, hFontMenu, "Enabled"));
				if(config.audio.hrtf == audio::HRTFEnable) {
					slider->selectLast();
				}
				value = slider;
			}
			float fRatio    = (RATIO_X(m_size.x-9) - value->m_rect.width());
			value->Move(Vec2f(fRatio, 0));
			panel->AddElement(value);
			
			addCenter(panel);
			
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(20, 380), Vec2f(16, 16), "graph/interface/menus/back");
			cb->m_targetMenu = OPTIONS;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
private:
	
	void onChangedDevice(int pos, const std::string & str) {
		if(pos == 0) {
			ARXMenu_Options_Audio_SetDevice("auto");
		} else {
			ARXMenu_Options_Audio_SetDevice(str);
		}
	}
	
	void onChangedMasterVolume(int value) {
		ARXMenu_Options_Audio_SetMasterVolume(value);
	}
	
	void onChangedEffectsVolume(int value) {
		ARXMenu_Options_Audio_SetSfxVolume(value);
	}
	
	void onChangedSpeechVolume(int value) {
		ARXMenu_Options_Audio_SetSpeechVolume(value);
	}
	
	void onChangedAmbianceVolume(int value) {
		ARXMenu_Options_Audio_SetAmbianceVolume(value);
	}
	
	void onChangedEax(int state) {
		config.audio.eax = (state != 0);
		ARX_SOUND_SetReverb(config.audio.eax);
	}
	
	void onChangedHRTF(int pos, const std::string & str) {
		ARX_UNUSED(str);
		switch(pos) {
			case 0: config.audio.hrtf = audio::HRTFDisable; break;
			case 1: config.audio.hrtf = audio::HRTFDefault; break;
			case 2: config.audio.hrtf = audio::HRTFEnable; break;
			default: ARX_DEAD_CODE();
		}
		audio::setHRTFEnabled(config.audio.hrtf);
	}
	
	void onChangedMuteOnFocusLost(int state) {
		config.audio.muteOnFocusLost = (state != 0);
		if(!mainApp->getWindow()->hasFocus()) {
			ARXMenu_Options_Audio_SetMuted(config.audio.muteOnFocusLost);
		}
	}
	
};

class InputOptionsMenuPage : public MenuPage {
	
public:
	
	InputOptionsMenuPage(const Vec2f & pos, const Vec2f & size)
		: MenuPage(pos, size, OPTIONS_INPUT)
	{}
	
	~InputOptionsMenuPage() { }
	
	void init() {
		
		{
			std::string szMenuText = getLocalised("system_menus_options_input_customize_controls");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->m_targetMenu = OPTIONS_INPUT_CUSTOMIZE_KEYS_1;
			addCenter(txt);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_input_invert_mouse");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&InputOptionsMenuPage::onChangedInvertMouse, this, _1);
			cb->iState = config.input.invertMouse ? 1 : 0;
			addCenter(cb);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_auto_ready_weapon");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&InputOptionsMenuPage::onChangedAutoReadyWeapon, this, _1);
			cb->iState = config.input.autoReadyWeapon ? 1 : 0;
			addCenter(cb);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_input_mouse_look_toggle");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&InputOptionsMenuPage::onChangedToggleMouselook, this, _1);
			cb->iState = config.input.mouseLookToggle ? 1 : 0;
			addCenter(cb);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_input_mouse_sensitivity");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			SliderWidget * sld = new SliderWidget(Vec2f(200, 0));
			sld->valueChanged = boost::bind(&InputOptionsMenuPage::onChangedMouseSensitivity, this, _1);
			sld->setValue(config.input.mouseSensitivity);
			panel->AddElement(sld);
			addCenter(panel);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_input_mouse_acceleration", "Mouse acceleration");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			SliderWidget * sld = new SliderWidget(Vec2f(200, 0));
			sld->valueChanged = boost::bind(&InputOptionsMenuPage::onChangedMouseAcceleration, this, _1);
			sld->setValue(config.input.mouseAcceleration);
			panel->AddElement(sld);
			addCenter(panel);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_options_raw_mouse_input", "Raw mouse input");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&InputOptionsMenuPage::onChangedRawMouseInput, this, _1);
			cb->iState = config.input.rawMouseInput ? 1 : 0;
			addCenter(cb);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_autodescription", "auto_description");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&InputOptionsMenuPage::onChangedAutoDescription, this, _1);
			cb->iState = config.input.autoDescription ? 1 : 0;
			addCenter(cb);
		}
		
		{
			PanelWidget * panel = new PanelWidget;
			std::string szMenuText = getLocalised("system_menus_options_misc_quicksave_slots", "Quicksave slots");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			txt->SetCheckOff();
			panel->AddElement(txt);
			SliderWidget * sld = new SliderWidget(Vec2f(200, 0));
			sld->setMinimum(1);
			sld->valueChanged = boost::bind(&InputOptionsMenuPage::onChangedQuicksaveSlots, this, _1);
			sld->setValue(config.misc.quicksaveSlots);
			panel->AddElement(sld);
			addCenter(panel);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_border_turning", "Border turning");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&InputOptionsMenuPage::onChangedBorderTurning, this, _1);
			cb->iState = config.input.borderTurning ? 1 : 0;
			addCenter(cb);
		}
		
		{
			std::string szMenuText = getLocalised("system_menus_alt_rune_recognition", "Alternate rune recognition");
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, szMenuText, Vec2f(20, 0));
			CheckboxWidget * cb = new CheckboxWidget(txt);
			cb->stateChanged = boost::bind(&InputOptionsMenuPage::onChangedAltRuneRecognition, this, _1);
			cb->iState = config.input.useAltRuneRecognition ? 1 : 0;
			addCenter(cb);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(20, 380), Vec2f(16, 16), "graph/interface/menus/back");
			cb->m_targetMenu = OPTIONS;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
	}
	
private:
	
	void onChangedInvertMouse(int state) {
		config.input.invertMouse = (state) ? true : false;
		GInput->setInvertMouseY(config.input.invertMouse);
	}
	
	void onChangedAutoReadyWeapon(int state) {
		config.input.autoReadyWeapon = (state) ? true : false;
	}
	
	void onChangedToggleMouselook(int state) {
		config.input.mouseLookToggle = (state) ? true : false;
	}
	
	void onChangedMouseSensitivity(int value) {
		config.input.mouseSensitivity = glm::clamp(value, 0, 10);
		GInput->setMouseSensitivity(config.input.mouseSensitivity);
	}
	
	void onChangedMouseAcceleration(int value) {
		config.input.mouseAcceleration = glm::clamp(value, 0, 10);
		GInput->setMouseAcceleration(config.input.mouseAcceleration);
	}
	
	void onChangedRawMouseInput(int state) {
		config.input.rawMouseInput = (state) ? true : false;
		GInput->setRawMouseInput(config.input.rawMouseInput);
	}
	
	void onChangedAutoDescription(int state) {
		config.input.autoDescription = (state) ? true : false;
	}
	
	void onChangedQuicksaveSlots(int value) {
		config.misc.quicksaveSlots = value;
	}
	
	void onChangedBorderTurning(int value) {
		config.input.borderTurning = (value) ? true : false;
	}
	
	void onChangedAltRuneRecognition(int value) {
		config.input.useAltRuneRecognition = (value) ? true : false;
	}
	
};


class ControlOptionsPage : public MenuPage {
	
public:
	
	ControlOptionsPage(const Vec2f & pos, const Vec2f & size, MENUSTATE state)
		: MenuPage(pos, size, state)
	{}
	
protected:
	
	void addControlRow(long & y, ControlAction controlAction,
	                             const std::string & a,
	                             const char * defaultText = "?",
	                             const char * specialSuffix = "") {
		
		PanelWidget * panel = new PanelWidget;
		
		{
		std::string szMenuText = getLocalised(a, defaultText);
		szMenuText += specialSuffix;
		TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontControls, szMenuText, Vec2f(20, 0));
		txt->SetCheckOff();
		panel->AddElement(txt);
		}
		
		{
		TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontControls, "---", Vec2f(150, 0));
		
		txt->m_isKeybind = true;
		txt->m_keybindAction = controlAction;
		txt->m_keybindIndex = 0;
		
		txt->eState=GETTOUCH;
		panel->AddElement(txt);
		}
		
		{
		TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontControls, "---", Vec2f(245, 0));
		
		txt->m_isKeybind = true;
		txt->m_keybindAction = controlAction;
		txt->m_keybindIndex = 1;
		
		txt->eState=GETTOUCH;
		panel->AddElement(txt);
		}
		
		panel->Move(Vec2f(0, y));
		add(panel);
		y += panel->m_rect.height() + RATIO_Y(3.f);
	}
	
};


class ControlOptionsMenuPage1 : public ControlOptionsPage {
	
public:
	
	ControlOptionsMenuPage1(const Vec2f & pos, const Vec2f & size)
		: ControlOptionsPage(pos, size, OPTIONS_INPUT_CUSTOMIZE_KEYS_1)
	{}
	
	~ControlOptionsMenuPage1() { }
	
	void init() {
		
		long y = static_cast<long>(RATIO_Y(8.f));
		
		addControlRow(y, CONTROLS_CUST_USE,          "system_menus_options_input_customize_controls_mouselook");
		
		addControlRow(y, CONTROLS_CUST_ACTION,       "system_menus_options_input_customize_controls_action_combine");
		addControlRow(y, CONTROLS_CUST_JUMP,         "system_menus_options_input_customize_controls_jump");
		addControlRow(y, CONTROLS_CUST_MAGICMODE,    "system_menus_options_input_customize_controls_magic_mode");
		addControlRow(y, CONTROLS_CUST_STEALTHMODE,  "system_menus_options_input_customize_controls_stealth_mode");
		addControlRow(y, CONTROLS_CUST_WALKFORWARD,  "system_menus_options_input_customize_controls_walk_forward");
		addControlRow(y, CONTROLS_CUST_WALKBACKWARD, "system_menus_options_input_customize_controls_walk_backward");
		addControlRow(y, CONTROLS_CUST_STRAFELEFT,   "system_menus_options_input_customize_controls_strafe_left");
		addControlRow(y, CONTROLS_CUST_STRAFERIGHT,  "system_menus_options_input_customize_controls_strafe_right");
		addControlRow(y, CONTROLS_CUST_LEANLEFT,     "system_menus_options_input_customize_controls_lean_left");
		addControlRow(y, CONTROLS_CUST_LEANRIGHT,    "system_menus_options_input_customize_controls_lean_right");
		addControlRow(y, CONTROLS_CUST_CROUCH,       "system_menus_options_input_customize_controls_crouch");
		addControlRow(y, CONTROLS_CUST_CROUCHTOGGLE, "system_menus_options_input_customize_controls_crouch_toggle");
		
		addControlRow(y, CONTROLS_CUST_STRAFE,       "system_menus_options_input_customize_controls_strafe");
		addControlRow(y, CONTROLS_CUST_CENTERVIEW,   "system_menus_options_input_customize_controls_center_view");
		addControlRow(y, CONTROLS_CUST_FREELOOK,     "system_menus_options_input_customize_controls_freelook");
		
		addControlRow(y, CONTROLS_CUST_TURNLEFT,     "system_menus_options_input_customize_controls_turn_left");
		addControlRow(y, CONTROLS_CUST_TURNRIGHT,    "system_menus_options_input_customize_controls_turn_right");
		addControlRow(y, CONTROLS_CUST_LOOKUP,       "system_menus_options_input_customize_controls_look_up");
		addControlRow(y, CONTROLS_CUST_LOOKDOWN,     "system_menus_options_input_customize_controls_look_down");
		
		addControlRow(y, CONTROLS_CUST_MINIMAP,      "system_menus_options_input_customize_controls_bookmap", "?", "2");

		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(20, 380), Vec2f(16, 16), "graph/interface/menus/back");
			cb->m_targetMenu = OPTIONS_INPUT;
			cb->SetShortCut(Keyboard::Key_Escape);
			cb->clicked = boost::bind(&ControlOptionsMenuPage1::onClickedBack, this);
			add(cb);
		}
		
		{
			std::string szMenuText = getLocalised( "system_menus_options_input_customize_default" );
			TextWidget * txt = new TextWidget(BUTTON_MENUOPTIONS_CONTROLS_CUST_DEFAULT, hFontMenu, szMenuText);
			txt->SetPos(Vec2f((RATIO_X(m_size.x) - txt->m_rect.width())*0.5f, RATIO_Y(380)));
			add(txt);
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(280, 380), Vec2f(16, 16), "graph/interface/menus/next");
			cb->m_targetMenu = OPTIONS_INPUT_CUSTOMIZE_KEYS_2;
			add(cb);
		}
	
		ReInitActionKey();
	}
	
private:
	
	void onClickedBack(){
		config.save();
	}
	
};

class ControlOptionsMenuPage2 : public ControlOptionsPage {
	
public:
	
	ControlOptionsMenuPage2(const Vec2f & pos, const Vec2f & size)
		: ControlOptionsPage(pos, size, OPTIONS_INPUT_CUSTOMIZE_KEYS_2)
	{}
	
	~ControlOptionsMenuPage2() { }
	
	void init() {
		
		long y = static_cast<long>(RATIO_Y(8.f));
		
		addControlRow(y, CONTROLS_CUST_INVENTORY,         "system_menus_options_input_customize_controls_inventory");
		addControlRow(y, CONTROLS_CUST_BOOK,              "system_menus_options_input_customize_controls_book");
		addControlRow(y, CONTROLS_CUST_BOOKCHARSHEET,     "system_menus_options_input_customize_controls_bookcharsheet");
		addControlRow(y, CONTROLS_CUST_BOOKMAP,           "system_menus_options_input_customize_controls_bookmap");
		addControlRow(y, CONTROLS_CUST_BOOKSPELL,         "system_menus_options_input_customize_controls_bookspell");
		addControlRow(y, CONTROLS_CUST_BOOKQUEST,         "system_menus_options_input_customize_controls_bookquest");
		addControlRow(y, CONTROLS_CUST_DRINKPOTIONLIFE,   "system_menus_options_input_customize_controls_drink_potion_life");
		addControlRow(y, CONTROLS_CUST_DRINKPOTIONMANA,   "system_menus_options_input_customize_controls_drink_potion_mana");
		addControlRow(y, CONTROLS_CUST_DRINKPOTIONCURE,   "system_menus_options_input_customize_controls_drink_potion_cure", "Antidote potion");
		addControlRow(y, CONTROLS_CUST_TORCH,             "system_menus_options_input_customize_controls_torch");
		
		addControlRow(y, CONTROLS_CUST_CANCELCURSPELL,    "system_menus_options_input_customize_controls_cancelcurrentspell");
		addControlRow(y, CONTROLS_CUST_PRECAST1,          "system_menus_options_input_customize_controls_precast1");
		addControlRow(y, CONTROLS_CUST_PRECAST2,          "system_menus_options_input_customize_controls_precast2");
		addControlRow(y, CONTROLS_CUST_PRECAST3,          "system_menus_options_input_customize_controls_precast3");
		
		addControlRow(y, CONTROLS_CUST_WEAPON,            "system_menus_options_input_customize_controls_weapon");
		addControlRow(y, CONTROLS_CUST_UNEQUIPWEAPON,     "system_menus_options_input_customize_controls_unequipweapon");
		
		addControlRow(y, CONTROLS_CUST_PREVIOUS,          "system_menus_options_input_customize_controls_previous");
		addControlRow(y, CONTROLS_CUST_NEXT,              "system_menus_options_input_customize_controls_next");
		
		addControlRow(y, CONTROLS_CUST_QUICKLOAD,         "system_menus_options_input_customize_controls_quickload");
		addControlRow(y, CONTROLS_CUST_QUICKSAVE,         "system_menus_options_input_customize_controls_quicksave");
		
		addControlRow(y, CONTROLS_CUST_TOGGLE_FULLSCREEN, "system_menus_options_input_customize_controls_toggle_fullscreen", "Toggle fullscreen");
		
		if(config.input.allowConsole) {
			addControlRow(y, CONTROLS_CUST_CONSOLE, "system_menus_options_input_customize_controls_console", "Script console");
		}
		
		{
			ButtonWidget * cb = new ButtonWidget(Vec2f(20, 380), Vec2f(16, 16), "graph/interface/menus/back");
			cb->m_targetMenu = OPTIONS_INPUT_CUSTOMIZE_KEYS_1;
			cb->SetShortCut(Keyboard::Key_Escape);
			add(cb);
		}
		
		{
			std::string szMenuText = getLocalised( "system_menus_options_input_customize_default" );
			TextWidget * txt = new TextWidget(BUTTON_MENUOPTIONS_CONTROLS_CUST_DEFAULT, hFontMenu, szMenuText);
			txt->SetPos(Vec2f((RATIO_X(m_size.x) - txt->m_rect.width())*0.5f, RATIO_Y(380)));
			add(txt);
		}
		
		ReInitActionKey();
	}
	
};

class QuitConfirmMenuPage : public MenuPage {
	
public:
	
	QuitConfirmMenuPage(const Vec2f & pos, const Vec2f & size)
		: MenuPage(pos, size, QUIT)
	{}
	
	~QuitConfirmMenuPage() { }
	
	void init() {
		
		{
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, getLocalised("system_menus_main_quit"));
			txt->SetCheckOff();
			addCenter(txt, true);
		}
		
		{
			TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMenu, getLocalised("system_menus_main_editquest_confirm"));
			txt->SetCheckOff();
			addCenter(txt, true);
		}
		
		{
			TextWidget * yes = new TextWidget(BUTTON_INVALID, hFontMenu, getLocalised("system_yes"), Vec2f(m_size.x - 40, 380));
			yes->clicked = boost::bind(&QuitConfirmMenuPage::onClickedYes, this);
			add(yes);
		}
		
		{
			TextWidget * no = new TextWidget(BUTTON_INVALID, hFontMenu, getLocalised("system_no"), Vec2f(10, 380));
			no->m_targetMenu = MAIN;
			no->SetShortCut(Keyboard::Key_Escape);
			add(no);
		}
	}
	
private:
	
	void onClickedYes() {
		MenuFader_start(true, true, AMCM_OFF);
	}
};





void MainMenuLeftCreate(MENUSTATE eMenuState)
{
	mainMenu->eOldMenuState=eMenuState;
	
	delete pWindowMenu, pWindowMenu = NULL;
	
	Vec2f windowMenuPos = Vec2f(20, 25);
	Vec2f windowMenuSize = Vec2f(321, 430);
	
	pWindowMenu = new CWindowMenu(windowMenuPos, windowMenuSize);
	
	Vec2f offset = Vec2f_ZERO;
	Vec2f size = windowMenuSize - offset;
	
	pWindowMenu->m_currentPageId = eMenuState;
	
	{
		NewQuestMenuPage * page = new NewQuestMenuPage(offset, size);
		page->init();
		pWindowMenu->add(page);
	}

	{
	ChooseLoadOrSaveMenuPage * page = new ChooseLoadOrSaveMenuPage(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	LoadMenuPage * page = new LoadMenuPage(offset + Vec2f(0, -40), size);
	page->m_savegame = SavegameHandle();
	page->m_rowSpacing = 5;
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	SaveMenuPage * page = new SaveMenuPage(offset + Vec2f(0, -40), size);
	page->m_rowSpacing = 5;
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	SaveConfirmMenuPage * page = new SaveConfirmMenuPage(offset, size);
	page->m_savegame = SavegameHandle();
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	OptionsMenuPage * page = new OptionsMenuPage(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	VideoOptionsMenuPage * page = new VideoOptionsMenuPage(offset + Vec2f(0, -35), size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	InterfaceOptionsMenuPage * page = new InterfaceOptionsMenuPage(offset + Vec2f(0, -35), size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	AudioOptionsMenuPage * page = new AudioOptionsMenuPage(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	InputOptionsMenuPage * page = new InputOptionsMenuPage(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	ControlOptionsMenuPage1 * page = new ControlOptionsMenuPage1(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	ControlOptionsMenuPage2 * page = new ControlOptionsMenuPage2(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
	
	{
	QuitConfirmMenuPage * page = new QuitConfirmMenuPage(offset, size);
	page->init();
	pWindowMenu->add(page);
	}
}



MainMenu::MainMenu()
	: bReInitAll(false)
	, eOldMenuState(NOP)
	, eOldMenuWindowState(NOP)
	, m_selected(NULL)
	, m_background(NULL)
	, m_widgets(new WidgetContainer())
	, m_resumeGame(NULL)
{}

MainMenu::~MainMenu() {
	delete m_widgets;
	delete m_background;
}

void MainMenu::init()
{
	m_background = TextureContainer::LoadUI("graph/interface/menus/menu_main_background", TextureContainer::NoColorKey);

	Vec2f pos = Vec2f(370, 100);
	int yOffset = 50;
	
	{
	std::string szMenuText = getLocalised("system_menus_main_resumegame");
	TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, pos);
	txt->clicked = boost::bind(&MainMenu::onClickedResumeGame, this);
	txt->m_targetMenu = RESUME_GAME;
	m_widgets->add(txt);
	m_resumeGame = txt;
	}
	pos.y += yOffset;
	{
	std::string szMenuText = getLocalised("system_menus_main_newquest");
	TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, pos);
	txt->clicked = boost::bind(&MainMenu::onClickedNewQuest, this);
	txt->m_targetMenu = NEW_QUEST;
	m_widgets->add(txt);
	}
	pos.y += yOffset;
	{
	std::string szMenuText = getLocalised("system_menus_main_editquest");
	TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, pos);
	txt->m_targetMenu = EDIT_QUEST;
	m_widgets->add(txt);
	}
	pos.y += yOffset;
	{
	std::string szMenuText = getLocalised("system_menus_main_options");
	TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, pos);
	txt->m_targetMenu = OPTIONS;
	m_widgets->add(txt);
	}
	pos.y += yOffset;
	{
	std::string szMenuText = getLocalised("system_menus_main_credits");
	TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, pos);
	txt->clicked = boost::bind(&MainMenu::onClickedCredits, this);
	txt->m_targetMenu = CREDITS;
	m_widgets->add(txt);
	}
	pos.y += yOffset;
	{
	std::string szMenuText = getLocalised("system_menus_main_quit");
	TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontMainMenu, szMenuText, pos);
	txt->m_targetMenu = QUIT;
	m_widgets->add(txt);
	}
	pos.y += yOffset;
	
	std::string version = arx_name + " " + arx_version;
	if(!arx_release_codename.empty()) {
		version += " \"";
		version += arx_release_codename;
		version += "\"";
	}

	float verPosX = g_size.right - 20 * g_sizeRatio.x - hFontControls->getTextSize(version).width();
	TextWidget * txt = new TextWidget(BUTTON_INVALID, hFontControls, version, Vec2f(verPosX / g_sizeRatio.x, 80));
	
	txt->SetCheckOff();
	txt->lColor=Color(127,127,127);
	m_widgets->add(txt);
}

void MainMenu::onClickedResumeGame(){
	pTextManage->Clear();
	if(!g_canResumeGame) {
		ARX_QuickLoad();
	} else {
		ARXMenu_ResumeGame();
	}
}

void MainMenu::onClickedNewQuest() {
	if(!g_canResumeGame) {
		ARXMenu_NewQuest();
	}
}

void MainMenu::onClickedCredits() {
	MenuFader_start(true, true, AMCM_CREDITS);
}


MENUSTATE MainMenu::Update() {
	
	if(m_resumeGame) {
		if(g_canResumeGame) {
			m_resumeGame->SetCheckOn();
			m_resumeGame->lColor = Color(232, 204, 142);
		} else if(savegames.size() == 0) {
			m_resumeGame->SetCheckOff();
			m_resumeGame->lColor = Color(127, 127, 127);
		}
	}
	
	m_selected = m_widgets->getAtPos(Vec2f(GInput->getMousePosition()));
	
	if(m_selected && GInput->getMouseButton(Mouse::Button_0)) {
		m_selected->OnMouseClick();
		return m_selected->m_targetMenu;
	}
	
	return NOP;
}

// TODO remove this
extern bool bNoMenu;

void MainMenu::Render() {

	if(bNoMenu)
		return;

	if(m_background) {
		UseRenderState state(render2D().noBlend());
		EERIEDrawBitmap(Rectf(Vec2f(0, 0), g_size.width(), g_size.height()), 0.999f, m_background, Color::white);
	}
	
	{Widget * w; BOOST_FOREACH(w, m_widgets->m_widgets) {
		w->Update();
		w->Render();
	}}

	//HIGHLIGHT
	if(m_selected) {
		m_selected->RenderMouseOver();
	}

	//DEBUG ZONE
	GRenderer->ResetTexture(0);
	m_widgets->drawDebug();
}
