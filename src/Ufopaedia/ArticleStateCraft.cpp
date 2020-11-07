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

#include <sstream>
#include "ArticleStateCraft.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleCraft.h"
#include "../Engine/Game.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Unicode.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Engine/Screen.h"

namespace OpenXcom
{

	ArticleStateCraft::ArticleStateCraft(ArticleDefinitionCraft *defs, std::shared_ptr<ArticleCommonState> state) : ArticleState(defs->id, std::move(state))
	{
		RuleCraft *craft = _game->getMod()->getCraft(defs->id, true);

		int bpp = Options::pediaBgResolutionX == Screen::ORIGINAL_WIDTH ? 8 : 32;
		int scaleX = Options::pediaBgResolutionX / Screen::ORIGINAL_WIDTH;
		int scaleY = Options::pediaBgResolutionY / Screen::ORIGINAL_HEIGHT;
		SDL_Color* buttonTextPalette = _game->getMod()->getPalettes().find("PAL_BATTLEPEDIA")->second->getColors();


		// Set palette
		if (defs->customPalette && bpp == 8)
		{
			setCustomPalette(_game->getMod()->getSurface(defs->image_id)->getPalette(), Mod::UFOPAEDIA_CURSOR);
		}
		else if (bpp == 8)
		{
			setStandardPalette("PAL_UFOPAEDIA");
		}
		else
		{
			genPediaPal();
			_cursorColor = Mod::UFOPAEDIA_CURSOR;
		}

		// set buttons palette before adding to state
		_btnOk->statePalette = _palette;
		_btnOk->setTextPalette(buttonTextPalette);
		_btnPrev->statePalette = _palette;
		_btnPrev->setTextPalette(buttonTextPalette);
		_btnNext->statePalette = _palette;
		_btnNext->setTextPalette(buttonTextPalette);
		_btnInfo->statePalette = _palette;
		_btnInfo->setTextPalette(buttonTextPalette);
		ArticleState::initLayout();


		// Set up objects
		if (Options::pediaBgResolutionX == Screen::ORIGINAL_WIDTH)
		{
			_game->getMod()->getSurface(defs->image_id)->blitNShade(_bg, 0, 0);
		}
		else
		{
			_game->getMod()->getSurface(getTypeId(defs->image_id, bpp), Options::pediaBgResolutionX, Options::pediaBgResolutionY)->blitNShade32(_bg, 0, 0, 3);
		}
		_btnOk->setColor(Palette::blockOffset(15)-1);
		_btnPrev->setColor(Palette::blockOffset(15)-1);
		_btnNext->setColor(Palette::blockOffset(15)-1);
		_btnInfo->setColor(Palette::blockOffset(15)-1);
		_btnInfo->setVisible(_game->getMod()->getShowPediaInfoButton());

		// add screen elements
		_txtTitle = new Text(210 * scaleX, 32 * scaleY, 5 * scaleX, 24 * scaleY, bpp);
		_txtTitle->setScale(scaleX, scaleY);
		add(_txtTitle);
		_txtTitle->setColor(Palette::blockOffset(14)+15);
		_txtTitle->setBig();
		_txtTitle->setWordWrap(true);
		_txtTitle->setText(tr(defs->getTitleForPage(_state->current_page)));


		_txtInfo = new Text(defs->rect_text.width * scaleX, defs->rect_text.height * scaleY, defs->rect_text.x * scaleX, defs->rect_text.y * scaleY, bpp);
		_txtInfo->setScale(scaleX, scaleY);
		add(_txtInfo);
		_txtInfo->setColor(Palette::blockOffset(14)+15);
		_txtInfo->setSecondaryColor(Palette::blockOffset(15) + 4);
		_txtInfo->setWordWrap(true);
		_txtInfo->setText(tr(defs->getTextForPage(_state->current_page)));

		_txtStats = new Text(defs->rect_stats.width * scaleX, defs->rect_stats.height * scaleY, defs->rect_stats.x * scaleX, defs->rect_stats.y * scaleY, bpp);
		_txtStats->setScale(scaleX, scaleY);
		add(_txtStats);
		_txtStats->setColor(Palette::blockOffset(14)+15);
		_txtStats->setSecondaryColor(Palette::blockOffset(15)+4);

		std::ostringstream ss;
		ss << tr("STR_MAXIMUM_SPEED_UC").arg(Unicode::formatNumber(craft->getMaxSpeed())) << '\n';
		ss << tr("STR_ACCELERATION").arg(craft->getAcceleration()) << '\n';
		int range;
		switch (_game->getMod()->getPediaReplaceCraftFuelWithRangeType())
		{
			// Both max range alone and average range get rounded
			case 0:
			case 2:
				range = craft->calculateRange(_game->getMod()->getPediaReplaceCraftFuelWithRangeType());
				if (range == -1)
				{
					ss << tr("STR_MAXIMUM_RANGE").arg(tr("STR_INFINITE_RANGE")) << '\n';
					break;
				}

				// Round the answer to
				if (range < 100)
				{
					// don't round if it's small!
				}
				else if (range < 1000)
				{
					// nearest 10 nautical miles
					range += 10 / 2;
					range -= range % 10;
				}
				else
				{
					// nearest 100 nautical miles
					range += 100 / 2;
					range -= range % 100;
				}

				ss << tr("STR_MAXIMUM_RANGE").arg(Unicode::formatNumber(range)) << '\n';
				break;
			// Min-maxxers can fret over exact numbers
			case 1:
				if (craft->calculateRange(0) == -1)
				{
					ss << tr("STR_MAXIMUM_RANGE").arg(tr("STR_INFINITE_RANGE")) << '\n';
					break;
				}

				ss << tr("STR_MINIMUM_RANGE").arg(Unicode::formatNumber(craft->calculateRange(1))) << '\n';
				ss << tr("STR_MAXIMUM_RANGE").arg(Unicode::formatNumber(craft->calculateRange(0))) << '\n';
				break;
			default :
				ss << tr("STR_FUEL_CAPACITY").arg(Unicode::formatNumber(craft->getMaxFuel())) << '\n';
				break;
		}
		ss << tr("STR_WEAPON_PODS").arg(craft->getWeapons()) << '\n';
		ss << tr("STR_DAMAGE_CAPACITY_UC").arg(Unicode::formatNumber(craft->getMaxDamage())) << '\n';
		ss << tr("STR_CARGO_SPACE").arg(craft->getSoldiers()) << '\n';
		if (craft->getPilots() > 0)
		{
			ss << tr("STR_COCKPIT_CAPACITY").arg(craft->getPilots()) << '\n';
		}
		ss << tr("STR_HWP_CAPACITY").arg(craft->getVehicles());
		_txtStats->setText(ss.str());

		centerAllSurfaces();
	}

	ArticleStateCraft::~ArticleStateCraft()
	{}

}
