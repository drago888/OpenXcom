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
#include "ArticleStateCraftWeapon.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleCraftWeapon.h"
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

	ArticleStateCraftWeapon::ArticleStateCraftWeapon(ArticleDefinitionCraftWeapon *defs, std::shared_ptr<ArticleCommonState> state) : ArticleState(defs->id, std::move(state))
	{
		RuleCraftWeapon *weapon = _game->getMod()->getCraftWeapon(defs->id, true);

		int bpp = Options::pediaBgResolutionX == Screen::ORIGINAL_WIDTH ? 8 : 32;
		int scaleX = Options::pediaBgResolutionX / Screen::ORIGINAL_WIDTH;
		int scaleY = Options::pediaBgResolutionY / Screen::ORIGINAL_HEIGHT;
		SDL_Color* buttonTextPalette = _game->getMod()->getPalettes().find("PAL_BATTLEPEDIA")->second->getColors();
		int titleAddHeight = std::max(32 * (int)(Options::pediaTitleScale - 1) - 10, 0);

		CraftWeaponCategory category = CWC_WEAPON;
		int offset = 0;
		if (weapon->getHidePediaInfo())
		{
			if (weapon->getTractorBeamPower() > 0)
			{
				category = CWC_TRACTOR_BEAM;
				offset = 32; // 2 * 16
			}
			else
			{
				category = CWC_EQUIPMENT;
				offset = 80; // 5 * 16
			}
		}

		// add screen elements
		_txtTitle = new Text(200 * scaleX * Options::pediaTitleScale, 32 * scaleY * Options::pediaTitleScale, 5 * scaleX, 24 * scaleY, bpp);
		_txtTitle->setScale(scaleX * Options::pediaTitleScale, scaleY * Options::pediaTitleScale);

		// Set palette
		if (defs->customPalette && bpp == 8)
		{
			setCustomPalette(_game->getMod()->getSurface(defs->image_id)->getPalette(), Mod::BATTLESCAPE_CURSOR);
		}
		else if (bpp == 8)
		{
			setStandardPalette("PAL_BATTLEPEDIA");
		}
		else
		{
			genPediaPal();
			_cursorColor = Mod::UFOPAEDIA_CURSOR;
		}

		if (bpp == 8)
		{
			_buttonColor = _game->getMod()->getInterface("articleCraftWeapon")->getElement("button")->color;
			_textColor = _game->getMod()->getInterface("articleCraftWeapon")->getElement("text")->color;
			_textColor2 = _game->getMod()->getInterface("articleCraftWeapon")->getElement("text")->color2;
			_listColor1 = _game->getMod()->getInterface("articleCraftWeapon")->getElement("list")->color;
			_listColor2 = _game->getMod()->getInterface("articleCraftWeapon")->getElement("list")->color2;
		}
		else
		{
			_buttonColor = Palette::blockOffset(15) - 1;
			_textColor = Palette::blockOffset(14) + 15;
			_textColor2 = Palette::blockOffset(15) + 4;
			_listColor1 = Palette::blockOffset(14) + 15;
			_listColor2 = Palette::blockOffset(15) + 4;
		}

		ArticleState::initLayout(false);

		// add other elements
		//add(_txtTitle);

		// Set up objects
		if (bpp == 8)
		{
			_game->getMod()->getSurface(defs->image_id)->blitNShade(_bg, 0, 0);
		}
		else
		{
			_game->getMod()->getSurface(getTypeId(defs->image_id), true, Options::pediaBgResolutionX, Options::pediaBgResolutionY)->blitNShade32(_bg, 0, 0);
		}

		_btnOk->setColor(_buttonColor);
		_btnOk->setTextPalette(buttonTextPalette);
		_btnPrev->setColor(_buttonColor);
		_btnPrev->setTextPalette(buttonTextPalette);
		_btnNext->setColor(_buttonColor);
		_btnNext->setTextPalette(buttonTextPalette);
		_btnInfo->setColor(_buttonColor);
		_btnInfo->setVisible(true);
		_btnInfo->setTextPalette(buttonTextPalette);
		add(_bg);
		add(_btnOk);
		add(_btnPrev);
		add(_btnNext);
		add(_btnInfo);

		_txtTitle->setColor(_textColor);
		_txtTitle->setBig();
		_txtTitle->setWordWrap(true);
		_txtTitle->setText(tr(defs->getTitleForPage(_state->current_page)));
		add(_txtTitle);

		_txtInfo = new Text(310 * scaleX, (32 + offset) * scaleY, 5 * scaleX, (160 - offset)*scaleY + titleAddHeight, bpp);
		_txtInfo->setScale(scaleX, scaleY);
		//add(_txtInfo);

		_txtInfo->setColor(_textColor);
		_txtInfo->setSecondaryColor(_textColor2);
		_txtInfo->setWordWrap(true);
		_txtInfo->setText(tr(defs->getTextForPage(_state->current_page)));
		add(_txtInfo);

		
		_lstInfo = new TextList(250 * scaleX *Options::pediaSubTitleScale, (111 - offset) * scaleY * Options::pediaSubTitleScale, 5 * scaleX, 80 * scaleY + titleAddHeight, bpp);
		_lstInfo->setScale(scaleX * Options::pediaSubTitleScale, scaleY * Options::pediaSubTitleScale);
		// 8 bits need to add lstInfo before proceeding
		if (bpp == 8)
		{
			add(_lstInfo);
		}
		_lstInfo->setVisible(category != CWC_EQUIPMENT);

		_lstInfo->setColor(_listColor1);
		_lstInfo->statePalette = _palette;
		_lstInfo->textPalette = _palette;
		_lstInfo->textColor = _textColor;
		_lstInfo->textColor2 = _textColor2;
		_lstInfo->setColumns(2, 180*scaleX, 70*scaleY);
		_lstInfo->setDot(true);
		_lstInfo->setBig();

		if (category == CWC_WEAPON)
		{
			_lstInfo->addRow(2, tr("STR_DAMAGE").c_str(), Unicode::formatNumber(weapon->getDamage()).c_str());
			_lstInfo->setCellColor(0, 1, _listColor2);

			_lstInfo->addRow(2, tr("STR_RANGE").c_str(), tr("STR_KILOMETERS").arg(weapon->getRange()).c_str());
			_lstInfo->setCellColor(1, 1, _listColor2);

			_lstInfo->addRow(2, tr("STR_ACCURACY").c_str(), Unicode::formatPercentage(weapon->getAccuracy()).c_str());
			_lstInfo->setCellColor(2, 1, _listColor2);

			_lstInfo->addRow(2, tr("STR_RE_LOAD_TIME").c_str(), tr("STR_SECONDS").arg(weapon->getStandardReload()).c_str());
			_lstInfo->setCellColor(3, 1, _listColor2);

			_lstInfo->addRow(2, tr("STR_ROUNDS").c_str(), Unicode::formatNumber(weapon->getAmmoMax()).c_str());
			_lstInfo->setCellColor(4, 1, _listColor2);
		}
		else if (category == CWC_TRACTOR_BEAM)
		{
			_lstInfo->addRow(2, tr("STR_TRACTOR_BEAM_POWER").c_str(), Unicode::formatNumber(weapon->getTractorBeamPower()).c_str());
			_lstInfo->setCellColor(0, 1, _listColor2);

			_lstInfo->addRow(2, tr("STR_RANGE").c_str(), tr("STR_KILOMETERS").arg(weapon->getRange()).c_str());
			_lstInfo->setCellColor(1, 1, _listColor2);
		}
		// 32 bits need add after everything done
		if (bpp == 32)
		{
			add(_lstInfo);
		}

		centerAllSurfaces();
	}

	ArticleStateCraftWeapon::~ArticleStateCraftWeapon()
	{}

}
