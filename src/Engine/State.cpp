/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "State.h"
#include <climits>
#include "InteractiveSurface.h"
#include "Game.h"
#include "Screen.h"
#include "Surface.h"
#include "Language.h"
#include "LocalizedText.h"
#include "Palette.h"
#include "../Engine/Sound.h"
#include "../Mod/Mod.h"
#include "../Interface/Window.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Mod/RuleInterface.h"
#include "../Engine/Screen.h"
#include "../Mod/ExtraSprites.h"
#include <utility>

namespace OpenXcom
{

/// Initializes static member
Game* State::_game = 0;

/**
 * Initializes a brand new state with no child elements.
 * By default states are full-screen.
 * @param game Pointer to the core game.
 */
State::State() : _screen(true), _soundPlayed(false), _modal(0), _ruleInterface(0), _ruleInterfaceParent(0), resetScreen(false),
	_resX(Screen::ORIGINAL_WIDTH), _resY(Screen::ORIGINAL_HEIGHT), _bpp(8), _pediaId("")
{
	// initialize palette to all black
	memset(_palette, 0, sizeof(_palette));
	_cursorColor = _game->getCursor()->getColor();
}

/**
 * Deletes all the child elements contained in the state.
 */
State::~State()
{
	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i < _surfaces.end(); ++i)
	{
		delete *i;
	}
}


/**
* Generate the default palette
* Red will have 7 choice (each increment is 1B) - 0x2B, 0x46, 0x61, 0x7C, 0x97, 0xB2, 0xCD
* Green and blue will have 6 choices (each increment is 1B) - 0x3B, 0x56, 0x71, 0x8C, 0xA7, 0xC2
*/
void State::genDefPal()
{
	_palette[0].r = 0, _palette[0].g = 0, _palette[0].b = 0, _palette[0].unused = SDL_ALPHA_OPAQUE;
	_palette[1].r = 0x10, _palette[1].g = 0x20, _palette[1].b = 0x20, _palette[1].unused = SDL_ALPHA_OPAQUE;
	Uint8 r, g, b, i = 2;


	for (r = 0x2B; r < 0xEF; r+=0x1B)
	for (g = 0x3B; g < 0xDF; g+=0x1B)
	for (b = 0x3B; b < 0xDF; b+=0x1B)
			_palette[i].r = r, _palette[i].g = g, _palette[i].b = b, _palette[i++].unused = SDL_ALPHA_OPAQUE;



	_palette[254].r = 0xef, _palette[254].g = 0xdf, _palette[254].b = 0xdf, _palette[254].unused = SDL_ALPHA_OPAQUE;
	_palette[255].r = 0xff, _palette[255].g = 0xff, _palette[255].b = 0xff, _palette[255].unused = SDL_ALPHA_OPAQUE;
}

/**
* Generate the cutscene palette
*/
void State::genCutPal()
{
	setStatePalette(_game->getMod()->getPalettes().find("PAL_UFOPAEDIA")->second->getColors());
}

/**
* Generate the ufopedia articles palette
*/
void State::genPediaPal()
{
	setStatePalette(_game->getMod()->getPalettes().find("PAL_UFOPAEDIA")->second->getColors());
}

/*
* Get the 32 bit surface.
* If not existing, convert the 8 bit surface to 32 bits
* @param id32 the 32 bit id
* @param id8 the 8 bit id
* @param newSurf the new surface if need to convert 8 bit to 32 bits
* @param palName palette name to use if no palette found in 8 bits
* @param usePal use the palette passed in
* @return the 32 bit surface
*/
Surface* State::get32Surf(std::string id32, std::string id8, Surface* newSurf, std::string palName, bool usePal)
{
	Surface* surf;
	try
	{
		surf = _game->getMod()->getSurface(id32, Options::pediaBgResolutionX, Options::pediaBgResolutionY);
	}
	catch (Exception& ex)
	{
		surf = nullptr; // just to ignore any exception when 32 surface does not exists
	}

	if (!surf)
	{
		int scaleX = Options::pediaBgResolutionX / Screen::ORIGINAL_WIDTH;
		int scaleY = Options::pediaBgResolutionY / Screen::ORIGINAL_HEIGHT;
		surf = _game->getMod()->getSurface(id8);
		Palette pal = Palette();
		if (!usePal && surf->getPalette())
		{
			pal.setColors(surf->getPalette(), surf->getSurface()->format->palette->ncolors);
		}
		else
		{
			pal.setColors(_game->getMod()->getPalettes().find(palName)->second->getColors(), 255);
		}
		*newSurf = *surf;
		newSurf->setScale(scaleX, scaleY);
		newSurf->doScale();
		newSurf->convertTo32Bits(newSurf, pal.getColors(), true); // always use the palette passed in
		surf = newSurf;
	}

	return surf;
}

/*
* Get the 32 bit sprites type id
* @param id the 8 bit sprite type id
* @return the typeid to use
*/
std::string State::getTypeId(std::string id, int bpp)
{
	if (bpp != 8 && _game->getMod()->getExtraSprites().find(id)->second.size() > 0)
	{
		ExtraSprites* sprite = _game->getMod()->getExtraSprites().find(id)->second[0];

		std::string first_sprite = sprite->getSprites() && sprite->getSprites()->size() > 0 ? sprite->getSprites()->begin()->second : "";

		if (OpenXcom::in32BitsFolder(first_sprite))
		{
			return "32_" + id;
		}
	}

	return id;
}

/**
 * Set interface data from the ruleset, also sets the palette for the state.
 * @param category Name of the interface set.
 * @param alterPal Should we swap out the backpal colors?
 * @param battleGame Should we use battlescape palette? (this only applies to options screens)
 */
void State::setInterface(const std::string& category, bool alterPal, SavedBattleGame *battleGame)
{
	int backPal = -1;
	std::string pal = "PAL_GEOSCAPE";

	_ruleInterface = _game->getMod()->getInterface(category);
	if (_ruleInterface)
	{
		_ruleInterfaceParent = _game->getMod()->getInterface(_ruleInterface->getParent());
		pal = _ruleInterface->getPalette();
		Element *element = _ruleInterface->getElement("palette");
		if (_ruleInterfaceParent)
		{
			if (!element)
			{
				element = _ruleInterfaceParent->getElement("palette");
			}
			if (pal.empty())
			{
				pal = _ruleInterfaceParent->getPalette();
			}
		}
		if (element)
		{
			int color = alterPal ? element->color2 : element->color;
			if (color != INT_MAX)
			{
				backPal = color;
			}
		}
	}
	if (battleGame)
	{
		battleGame->setPaletteByDepth(this);
	}
	else if (pal.empty())
	{
		pal = "PAL_GEOSCAPE";
		setStandardPalette(pal, backPal);
	}
	else
	{
		setStandardPalette(pal, backPal);
	}
}

/**
 * Set window background from the ruleset.
 * @param window Window handle.
 * @param s ID of the interface ruleset entry.
 */
void State::setWindowBackground(Window *window, const std::string &s)
{
	auto bgImageName = _game->getMod()->getInterface(s)->getBackgroundImage();
	auto bgImage = _game->getMod()->getSurface(bgImageName);
	window->setBackground(bgImage);
}

/**
 * Adds a new child surface for the state to take care of,
 * giving it the game's display palette. Once associated,
 * the state handles all of the surface's behaviour
 * and management automatically.
 * @param surface Child surface.
 * @note Since visible elements can overlap one another,
 * they have to be added in ascending Z-Order to be blitted
 * correctly onto the screen.
 */
void State::add(Surface *surface)
{
	// Set palette
	surface->setPalette(_palette);
	surface->statePalette = _palette;


	// Set default text resources
	if (_game->getLanguage() && _game->getMod())
		surface->initText(_game->getMod()->getFont("FONT_BIG",true,(int)surface->getScaleX(),(int)surface->getScaleY()),
			              _game->getMod()->getFont("FONT_SMALL",true,(int)surface->getScaleX(),(int)surface->getScaleY()),
			              _game->getLanguage());

	_surfaces.push_back(surface);
}

/**
 * As above, except this adds a surface based on an
 * interface element defined in the ruleset.
 * @note that this function REQUIRES the ruleset to have been loaded prior to use.
 * @param surface Child surface.
 * @param id the ID of the element defined in the ruleset, if any.
 * @param category the category of elements this interface is associated with.
 * @param parent the surface to base the coordinates of this element off.
 * @note if no parent is defined the element will not be moved.
 */
void State::add(Surface *surface, const std::string &id, const std::string &category, Surface *parent)
{
	// Set palette
	surface->setPalette(_palette);

	// this only works if we're dealing with a battlescape button
	BattlescapeButton *bsbtn = dynamic_cast<BattlescapeButton*>(surface);

	if (_game->getMod()->getInterface(category))
	{
		Element *element = _game->getMod()->getInterface(category)->getElement(id);
		if (element)
		{
			if (parent && element->w != INT_MAX && element->h != INT_MAX)
			{
				surface->setWidth(element->w);
				surface->setHeight(element->h);
			}

			if (parent && element->x != INT_MAX && element->y != INT_MAX)
			{
				surface->setX(parent->getX() + element->x);
				surface->setY(parent->getY() + element->y);
			}

			auto inter = dynamic_cast<InteractiveSurface*>(surface);
			if (inter)
			{
				inter->setTFTDMode(element->TFTDMode);
			}

			if (element->color != INT_MAX)
			{
				surface->setColor(element->color);
			}
			if (element->color2 != INT_MAX)
			{
				surface->setSecondaryColor(element->color2);
			}
			if (element->border != INT_MAX)
			{
				surface->setBorderColor(element->border);
			}
		}
	}

	if (bsbtn)
	{
		// this will initialize the graphics and settings of the battlescape button.
		bsbtn->copy(parent);
		bsbtn->initSurfaces();
	}

	// Set default text resources
	if (_game->getLanguage() && _game->getMod())
		surface->initText(_game->getMod()->getFont("FONT_BIG"), _game->getMod()->getFont("FONT_SMALL"), _game->getLanguage());

	_surfaces.push_back(surface);
}

/**
 * Returns whether this is a full-screen state.
 * This is used to optimize the state machine since full-screen
 * states automatically cover the whole screen, (whether they
 * actually use it all or not) so states behind them can be
 * safely ignored since they'd be covered up.
 * @return True if it's a screen, False otherwise.
 */
bool State::isScreen() const
{
	return _screen;
}

/**
 * Toggles the full-screen flag. Used by windows to
 * keep the previous screen in display while the window
 * is still "popping up".
 */
void State::toggleScreen()
{
	_screen = !_screen;
}

/**
 * Initializes the state and its child elements. This is
 * used for settings that have to be reset every time the
 * state is returned to focus (eg. palettes), so can't
 * just be put in the constructor (remember there's a stack
 * of states, so they can be created once while being
 * repeatedly switched back into focus).
 */
void State::init()
{
	_game->getScreen()->setPalette(_palette);
	_game->getCursor()->setPalette(_palette);
	_game->getCursor()->statePalette = _palette;
	_game->getCursor()->setColor(_cursorColor);
	_game->getCursor()->draw();
	_game->getFpsCounter()->setPalette(_palette);
	_game->getFpsCounter()->statePalette = _palette;
	_game->getFpsCounter()->setColor(_cursorColor);
	_game->getFpsCounter()->draw();

	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		Window* window = dynamic_cast<Window*>(*i);
		if (window)
		{
			window->invalidate();
		}
	}
	if (_ruleInterface != 0 && !_ruleInterface->getMusic().empty())
	{
		_game->getMod()->playMusic(_ruleInterface->getMusic());
	}
	if (_ruleInterface != 0 && _ruleInterface->getSound() > -1)
	{
		if (!_soundPlayed)
		{
			_game->getMod()->getSound("GEO.CAT", _ruleInterface->getSound())->play();
			_soundPlayed = true;
		}
	}
}

/**
 * Runs any code the state needs to keep updating every
 * game cycle, like timers and other real-time elements.
 */
void State::think()
{
	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
		(*i)->think();
}

/**
 * Takes care of any events from the core game engine,
 * and passes them on to its InteractiveSurface child elements.
 * @param action Pointer to an action.
 */
void State::handle(Action *action)
{
	if (!_modal)
	{
		for (std::vector<Surface*>::reverse_iterator i = _surfaces.rbegin(); i != _surfaces.rend(); ++i)
		{
			InteractiveSurface* j = dynamic_cast<InteractiveSurface*>(*i);
			if (j != 0)
				j->handle(action, this);
		}
	}
	else
	{
		_modal->handle(action, this);
	}
}

/**
 * Blits all the visible Surface child elements onto the
 * display screen, by order of addition.
 */
void State::blit()
{
	if (resetScreen)
	{
		resetScreen = !resetScreen;

		_game->getScreen()->resetDisplay(true, false, _resX, _resY, _bpp);
	}
	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
		(*i)->blit(_game->getScreen()->getSurface());
}

/**
 * Hides all the Surface child elements on display.
 */
void State::hideAll()
{
	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
		(*i)->setHidden(true);
}

/**
 * Shows all the hidden Surface child elements.
 */
void State::showAll()
{
	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
		(*i)->setHidden(false);
}

/**
 * Resets the status of all the Surface child elements,
 * like unpressing buttons.
 */
void State::resetAll()
{
	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		InteractiveSurface *s = dynamic_cast<InteractiveSurface*>(*i);
		if (s != 0)
		{
			s->unpress(this);
			//s->setFocus(false);
		}
	}
}

/**
 * Get the localized text for dictionary key @a id.
 * This function forwards the call to Language::getString(const std::string &).
 * @param id The dictionary key to search for.
 * @return The localized text.
 */
LocalizedText State::tr(const std::string &id) const
{
	return _game->getLanguage()->getString(id);
}

/**
* Get the localized text from dictionary.
* This function forwards the call to Language::getString(const std::string &).
* @param id The (prefix of) dictionary key to search for.
* @param alt Used to construct the (suffix of) dictionary key to search for.
* @return The localized text.
*/
LocalizedText State::trAlt(const std::string &id, int alt) const
{
	std::ostringstream ss;
	ss << id;
	// alt = 0 is the original, alt > 0 are the alternatives
	if (alt > 0)
	{
		ss << "_" << alt;
	}
	return _game->getLanguage()->getString(ss.str());
}

/**
 * Get a modifiable copy of the localized text for dictionary key @a id.
 * This function forwards the call to Language::getString(const std::string &, unsigned).
 * @param id The dictionary key to search for.
 * @param n The number to use for the proper version.
 * @return The localized text.
 */
LocalizedText State::tr(const std::string &id, unsigned n) const
{
	return _game->getLanguage()->getString(id, n);
}

/**
 * Get the localized text for dictionary key @a id.
 * This function forwards the call to Language::getString(const std::string &, SoldierGender).
 * @param id The dictionary key to search for.
 * @param gender Current soldier gender.
 * @return The localized text.
 */
LocalizedText State::tr(const std::string &id, SoldierGender gender) const
{
	return _game->getLanguage()->getString(id, gender);
}

/**
 * centers all the surfaces on the screen.
 */
void State::centerAllSurfaces()
{
	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		(*i)->setX((*i)->getX() + _game->getScreen()->getDX());
		(*i)->setY((*i)->getY() + _game->getScreen()->getDY());
	}
}

/**
 * drop all the surfaces by half the screen height
 */
void State::lowerAllSurfaces()
{
	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		(*i)->setY((*i)->getY() + _game->getScreen()->getDY() / 2);
	}
}

/**
 * switch all the colours to something a little more battlescape appropriate.
 */
void State::applyBattlescapeTheme(const std::string& category)
{
	Element * element = _game->getMod()->getInterface("mainMenu")->getElement("battlescapeTheme");
	std::string altBg = _game->getMod()->getInterface(category)->getAltBackgroundImage();
	if (altBg.empty())
	{
		altBg = "TAC00.SCR";
	}
	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		(*i)->setColor(element->color);
		(*i)->setHighContrast(true);
		Window* window = dynamic_cast<Window*>(*i);
		if (window)
		{
			window->setBackground(_game->getMod()->getSurface(altBg));
		}
		TextList* list = dynamic_cast<TextList*>(*i);
		if (list)
		{
			list->setArrowColor(element->border);
		}
		ComboBox *combo = dynamic_cast<ComboBox*>(*i);
		if (combo)
		{
			combo->setArrowColor(element->border);
		}
	}
}

/**
 * redraw all the text-type surfaces.
 */
void State::redrawText()
{
	for (std::vector<Surface*>::iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		Text* text = dynamic_cast<Text*>(*i);
		TextButton* button = dynamic_cast<TextButton*>(*i);
		TextEdit* edit = dynamic_cast<TextEdit*>(*i);
		TextList* list = dynamic_cast<TextList*>(*i);
		if (text || button || edit || list)
		{
			(*i)->draw();
		}
	}
}

/**
 * Changes the current modal surface. If a surface is modal,
 * then only that surface can receive events. This is used
 * when an element needs to take priority over everything else,
 * eg. focus.
 * @param surface Pointer to modal surface, NULL for no modal.
 */
void State::setModal(InteractiveSurface *surface)
{
	_modal = surface;
}

/**
 * Replaces a certain amount of colors in the state's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void State::setStatePalette(const SDL_Color *colors, int firstcolor, int ncolors)
{
	if (colors)
	{
		memcpy(_palette + firstcolor, colors, ncolors * sizeof(SDL_Color));
	}
}

/**
 * Set palette for helper surfaces like cursor or fps counter.
 */
void State::setModPalette()
{
	{
		_game->getCursor()->setPalette(_palette);
		_game->getCursor()->statePalette = _palette;
		_game->getCursor()->draw();
		_game->getFpsCounter()->setPalette(_palette);
		_game->getFpsCounter()->statePalette = _palette;
		_game->getFpsCounter()->draw();
	}
}

/**
 * Loads palettes from the game resources into the state.
 * @param palette String ID of the palette to load.
 * @param backpals BACKPALS.DAT offset to use.
 */
void State::setStandardPalette(const std::string &palette, int backpals)
{
	setStatePalette(_game->getMod()->getPalette(palette)->getColors(), 0, 256);
	if (palette == "PAL_GEOSCAPE")
	{
		_cursorColor = Mod::GEOSCAPE_CURSOR;
	}
	else if (palette == "PAL_BASESCAPE")
	{
		_cursorColor = Mod::BASESCAPE_CURSOR;
	}
	else if (palette == "PAL_UFOPAEDIA")
	{
		_cursorColor = Mod::UFOPAEDIA_CURSOR;
	}
	else if (palette == "PAL_GRAPHS")
	{
		_cursorColor = Mod::GRAPHS_CURSOR;
	}
	else
	{
		_cursorColor = Mod::BATTLESCAPE_CURSOR;
	}
	if (backpals != -1)
		setStatePalette(_game->getMod()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(backpals)), Palette::backPos, 16);
	setModPalette(); // delay actual update to the end
}

/**
* Loads palettes from the given resources into the state.
* @param colors Pointer to the set of colors.
* @param cursorColor Cursor color to use.
*/
void State::setCustomPalette(SDL_Color *colors, int cursorColor)
{
	setStatePalette(colors, 0, 256);
	_cursorColor = cursorColor;
	setModPalette(); // delay actual update to the end
}

/**
 * Returns the state's 8bpp palette.
 * @return Pointer to the palette's colors.
 */
SDL_Color *State::getPalette()
{
	return _palette;
}

/**
 * Each state will probably need its own resize handling,
 * so this space intentionally left blank
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void State::resize(int &dX, int &dY)
{
	recenter(dX, dY);
}

/**
 * Re-orients all the surfaces in the state.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void State::recenter(int dX, int dY)
{
	for (std::vector<Surface*>::const_iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		(*i)->setX((*i)->getX() + dX / 2);
		(*i)->setY((*i)->getY() + dY / 2);
	}
}

void State::setGamePtr(Game* game)
{
	_game = game;
}

}
