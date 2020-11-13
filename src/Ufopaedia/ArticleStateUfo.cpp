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
#include "ArticleStateUfo.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleUfo.h"
#include "../Engine/Game.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Unicode.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

	ArticleStateUfo::ArticleStateUfo(ArticleDefinitionUfo *defs, std::shared_ptr<ArticleCommonState> state) : ArticleState(defs->id, std::move(state))
	{
		RuleUfo *ufo = _game->getMod()->getUfo(defs->id, true);

		int bpp = Options::pediaBgResolutionX == Screen::ORIGINAL_WIDTH ? 8 : 32;
		int scaleX = Options::pediaBgResolutionX / Screen::ORIGINAL_WIDTH;
		int scaleY = Options::pediaBgResolutionY / Screen::ORIGINAL_HEIGHT;
		SDL_Color* buttonTextPalette = _game->getMod()->getPalettes().find("PAL_BATTLEPEDIA")->second->getColors();

		// Set palette
		if (bpp == 8)
		{
			setStandardPalette("PAL_GEOSCAPE");
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
		if (bpp == 8)
		{
			_btnOk->setColor(Palette::blockOffset(8) + 5);
			_btnPrev->setColor(Palette::blockOffset(8) + 5);
			_btnNext->setColor(Palette::blockOffset(8) + 5);
			_btnInfo->setColor(Palette::blockOffset(8) + 5);
		}
		else
		{
			_btnOk->setColor(Palette::blockOffset(15) - 1);
			_btnPrev->setColor(Palette::blockOffset(15) - 1);
			_btnNext->setColor(Palette::blockOffset(15) - 1);
			_btnInfo->setColor(Palette::blockOffset(15) - 1);
		}
		_btnInfo->setVisible(_game->getMod()->getShowPediaInfoButton());

		// Set up objects
		if (bpp == 8)
		{
			_game->getMod()->getSurface("BACK11.SCR")->blitNShade(_bg, 0, 0);
		}
		else
		{
			Surface surf;
			// this background does not exists thus force this palette
			get32Surf("32_BACK11.SCR", "BACK11.SCR", &surf, "PAL_GEOSCAPE", true)->blitNShade32(_bg, 0, 0);
		}

		// add screen elements
		_txtTitle = new Text(155 * scaleX, 32 * scaleY, 5 * scaleX, 24 * scaleY, bpp);
		_txtTitle->setScale(scaleX, scaleY);
		add(_txtTitle);
		if (bpp == 8)
		{
			_txtTitle->setColor(Palette::blockOffset(8) + 5);
		}
		else
		{
			_txtTitle->setColor(Palette::blockOffset(4) + 12);
		}
		_txtTitle->setBig();
		_txtTitle->setWordWrap(true);
		_txtTitle->setText(tr(defs->getTitleForPage(_state->current_page)));

		_image = new Surface(160 * scaleX, 52 * scaleY, 160 * scaleX, 6 * scaleY, bpp);
		add(_image);

		RuleInterface *dogfightInterface = _game->getMod()->getInterface("dogfight");

		auto crop = _game->getMod()->getSurface("INTERWIN.DAT")->getCrop();
		crop.setX(0);
		crop.setY(0);
		crop.getCrop()->x = 0;
		crop.getCrop()->y = 0;
		crop.getCrop()->w = _image->getWidth();
		crop.getCrop()->h = _image->getHeight();
		_image->drawRect(crop.getCrop(), 15);
		if (bpp == 8)
		{
			crop.blit(_image);
		}
		else
		{
			crop.blit32(_image);
		}

		Surface cropSurf;

		if (ufo->getModSprite().empty())
		{
			crop.getCrop()->y = (dogfightInterface->getElement("previewMid")->y + dogfightInterface->getElement("previewMid")->h * ufo->getSprite()) * scaleY;
			crop.getCrop()->h = dogfightInterface->getElement("previewMid")->h * scaleY;
		}
		else
		{
			if (bpp == 8)
			{
				crop = _game->getMod()->getSurface(ufo->getModSprite())->getCrop();
			}
			else
			{
				Surface* sprite = _game->getMod()->getSurface(ufo->getModSprite());
				cropSurf = *sprite;
				cropSurf.setScale(scaleX, scaleY);
				cropSurf.doScale();
				cropSurf.convertTo32Bits(&cropSurf, _game->getMod()->getPalettes().find("PAL_GEOSCAPE")->second->getColors());
				crop = cropSurf.getCrop();
			}
		}
		crop.setX(0);
		crop.setY(0);
		if (bpp == 8)
		{
			crop.blit(_image);
		}
		else
		{
			crop.blit32(_image);
		}

		_txtInfo = new Text(300 * scaleX, 50 * scaleY, 10 * scaleX, 140 * scaleY, bpp);
		_txtInfo->setScale(scaleX, scaleY);
		add(_txtInfo);
		if (bpp == 8)
		{
			_txtInfo->setColor(Palette::blockOffset(8) + 5);
		}
		else
		{
			_txtInfo->setColor(Palette::blockOffset(4) + 8);
		}
		_txtInfo->setSecondaryColor(Palette::blockOffset(8) + 10);
		_txtInfo->setWordWrap(true);
		_txtInfo->setText(tr(defs->getTextForPage(_state->current_page)));

		_lstInfo = new TextList(310 * scaleX, 64 * scaleY, 10 * scaleX, 68 * scaleY, bpp);
		_lstInfo->setScale(scaleX, scaleY);
		add(_lstInfo);
		_lstInfo->statePalette = _palette;
		_lstInfo->textPalette = _palette;
		_lstInfo->textColor = Palette::blockOffset(14) + 15;
		_lstInfo->textColor2 = Palette::blockOffset(15) + 4;

		centerAllSurfaces();

		_lstInfo->setColor(Palette::blockOffset(8)+5);
		_lstInfo->setColumns(2, 200 * scaleX, 110 * scaleY);
//		_lstInfo->setCondensed(true);
		_lstInfo->setBig();
		_lstInfo->setDot(true);

		_lstInfo->addRow(2, tr("STR_DAMAGE_CAPACITY").c_str(), Unicode::formatNumber(ufo->getStats().damageMax).c_str());

		_lstInfo->addRow(2, tr("STR_WEAPON_POWER").c_str(), Unicode::formatNumber(ufo->getWeaponPower()).c_str());

		_lstInfo->addRow(2, tr("STR_WEAPON_RANGE").c_str(), tr("STR_KILOMETERS").arg(ufo->getWeaponRange()).c_str());

		_lstInfo->addRow(2, tr("STR_MAXIMUM_SPEED").c_str(), tr("STR_KNOTS").arg(Unicode::formatNumber(ufo->getStats().speedMax)).c_str());
	}

	ArticleStateUfo::~ArticleStateUfo()
	{}

}
