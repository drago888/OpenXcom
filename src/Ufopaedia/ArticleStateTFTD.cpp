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

#include "../Mod/ArticleDefinition.h"
#include "ArticleStateTFTD.h"
#include "../Engine/Game.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/LocalizedText.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleInterface.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

namespace OpenXcom
{

	ArticleStateTFTD::ArticleStateTFTD(ArticleDefinitionTFTD *defs, std::shared_ptr<ArticleCommonState> state) : ArticleState(defs->id, std::move(state))
	{
		int bpp = Options::pediaBgResolutionX == Screen::ORIGINAL_WIDTH ? 8 : 32;
		int scaleX = Options::pediaBgResolutionX / Screen::ORIGINAL_WIDTH;
		int scaleY = Options::pediaBgResolutionY / Screen::ORIGINAL_HEIGHT;
		SDL_Color* buttonTextPalette = _game->getMod()->getPalettes().find("PAL_BATTLEPEDIA")->second->getColors();

		RuleInterface *ruleInterface;
		switch (defs->getType())
		{
			case UFOPAEDIA_TYPE_TFTD:
				ruleInterface = _game->getMod()->getInterface("articleTFTD");
				break;
			case UFOPAEDIA_TYPE_TFTD_CRAFT:
				ruleInterface = _game->getMod()->getInterface("articleCraftTFTD");
				break;
			case UFOPAEDIA_TYPE_TFTD_CRAFT_WEAPON:
				ruleInterface = _game->getMod()->getInterface("articleCraftWeaponTFTD");
				break;
			case UFOPAEDIA_TYPE_TFTD_VEHICLE:
				ruleInterface = _game->getMod()->getInterface("articleVehicleTFTD");
				break;
			case UFOPAEDIA_TYPE_TFTD_ITEM:
				ruleInterface = _game->getMod()->getInterface("articleItemTFTD");
				break;
			case UFOPAEDIA_TYPE_TFTD_ARMOR:
				ruleInterface = _game->getMod()->getInterface("articleArmorTFTD");
				break;
			case UFOPAEDIA_TYPE_TFTD_BASE_FACILITY:
				ruleInterface = _game->getMod()->getInterface("articleBaseFacilityTFTD");
				break;
			case UFOPAEDIA_TYPE_TFTD_USO:
				ruleInterface = _game->getMod()->getInterface("articleUsoTFTD");
				break;
			default:
				ruleInterface = _game->getMod()->getInterface("articleTFTD");
				break;
		}



		if (bpp == 8)
		{
			// Set palette
			if (defs->customPalette)
			{
				if (ruleInterface->getPalette() == "PAL_GEOSCAPE")
					_cursorColor = Mod::GEOSCAPE_CURSOR;
				else if (ruleInterface->getPalette() == "PAL_BASESCAPE")
					_cursorColor = Mod::BASESCAPE_CURSOR;
				else if (ruleInterface->getPalette() == "PAL_UFOPAEDIA")
					_cursorColor = Mod::UFOPAEDIA_CURSOR;
				else if (ruleInterface->getPalette() == "PAL_GRAPHS")
					_cursorColor = Mod::GRAPHS_CURSOR;
				else
					_cursorColor = Mod::BATTLESCAPE_CURSOR;

				setCustomPalette(_game->getMod()->getSurface(defs->image_id)->getPalette(), _cursorColor);
			}
			else
			{
				setStandardPalette(ruleInterface->getPalette());
			}
		}
		else
		{
			genPediaPal();
			_cursorColor = Mod::UFOPAEDIA_CURSOR;
		}

		_buttonColor = ruleInterface->getElement("button")->color;
		_textColor = ruleInterface->getElement("text")->color;
		_textColor2 = ruleInterface->getElement("text")->color2;
		_listColor1 = ruleInterface->getElement("list")->color;
		_listColor2 = ruleInterface->getElement("list")->color2;
		_arrowColor = _listColor2;
		if (ruleInterface->getElement("arrow"))
		{
			_arrowColor = ruleInterface->getElement("arrow")->color;
		}
		if (ruleInterface->getElement("ammoColor"))
		{
			_ammoColor = ruleInterface->getElement("ammoColor")->color;
		}

		_btnInfo->setX(183 * scaleX);
		_btnInfo->setY(179 * scaleY);
		_btnInfo->setHeight(10 * scaleY);
		_btnInfo->setWidth(40 * scaleX);
		_btnInfo->setColor(_buttonColor);
		_btnOk->setX(227 * scaleX);
		_btnOk->setY(179 * scaleY);
		_btnOk->setHeight(10 * scaleY);
		_btnOk->setWidth(23 * scaleX);
		_btnOk->setColor(_buttonColor);
		_btnPrev->setX(254 * scaleX);
		_btnPrev->setY(179 * scaleY);
		_btnPrev->setHeight(10 * scaleY);
		_btnPrev->setWidth(23 * scaleX);
		_btnPrev->setColor(_buttonColor);
		_btnNext->setX(281 * scaleX);
		_btnNext->setY(179 * scaleY);
		_btnNext->setHeight(10 * scaleY);
		_btnNext->setWidth(23 * scaleX);
		_btnNext->setColor(_buttonColor);

		if (bpp != 8) {
			_btnOk->statePalette = _palette;
			_btnOk->setTextPalette(buttonTextPalette);
			_btnPrev->statePalette = _palette;
			_btnPrev->setTextPalette(buttonTextPalette);
			_btnNext->statePalette = _palette;
			_btnNext->setTextPalette(buttonTextPalette);
			_btnInfo->statePalette = _palette;
			_btnInfo->setTextPalette(buttonTextPalette);
		}
		ArticleState::initLayout();

		// Step 1: background image
		if (!defs->customPalette)
		{
			if (bpp == 8) {
				_game->getMod()->getSurface(ruleInterface->getBackgroundImage())->blitNShade(_bg, 0, 0);
			}
			else {
				Surface surf;
				get32Surf("32_" + ruleInterface->getBackgroundImage(), ruleInterface->getBackgroundImage(), &surf, "PAL_BATTLEPEDIA")->blitNShade32(_bg, 0, 0, -30); //brighten the background
			}
		}

		// Step 2: article image (optional)
		Surface *image = _game->getMod()->getSurface(defs->image_id, false);
		if (image)
		{
			if (bpp == 8) {
				image->blitNShade(_bg, 0, 0);
			}
			else {
				Surface surf;
				get32Surf("32_" + defs->image_id, defs->image_id, &surf, "PAL_BATTLEPEDIA")->blitNShade32(_bg, 0, 0, -30); //brighten the background
			}

		}

		// Step 3: info button image
		Surface *button = _game->getMod()->getSurface(ruleInterface->getBackgroundImage() + "-InfoButton", false);
		if (!defs->customPalette && button && _game->getMod()->getShowPediaInfoButton())
		{
			switch (defs->getType())
			{
				case UFOPAEDIA_TYPE_TFTD_ITEM:
				case UFOPAEDIA_TYPE_TFTD_ARMOR:
				case UFOPAEDIA_TYPE_TFTD_BASE_FACILITY:
				case UFOPAEDIA_TYPE_TFTD_CRAFT:
				case UFOPAEDIA_TYPE_TFTD_CRAFT_WEAPON:
				case UFOPAEDIA_TYPE_TFTD_USO:
					if (bpp == 8) {
						button->blitNShade(_bg, 0, 0);
					}
					else {
						Surface surf;
						get32Surf("32_" + ruleInterface->getBackgroundImage() + "-InfoButton", ruleInterface->getBackgroundImage() + "-InfoButton", &surf, "PAL_BATTLEPEDIA")->blitNShade32(_bg, 0, 0, -30); //brighten the background
					}

					break;
				default:
					break;
			}
		}

		_txtInfo = new Text(defs->text_width * scaleX, 150 * scaleY, (320 - defs->text_width) * scaleX, 34 * scaleY, bpp);
		_txtInfo->setScale(scaleX, scaleY);
		add(_txtInfo);
		_txtInfo->setColor(_textColor);
		_txtInfo->setSecondaryColor(_textColor2);
		_txtInfo->setWordWrap(true);
		_txtInfo->setText(tr(defs->getTextForPage(_state->current_page)));

		_txtTitle = new Text(284 * scaleX, 16 * scaleY, 36 * scaleX, 14 * scaleX, bpp);
		_txtTitle->setScale(scaleX, scaleY);
		add(_txtTitle);
		_txtTitle->setColor(_textColor);
		_txtTitle->setBig();
		_txtTitle->setWordWrap(true);
		_txtTitle->setAlign(ALIGN_CENTER);
		_txtTitle->setText(tr(defs->getTitleForPage(_state->current_page)));


		// all of the above are common to the TFTD articles.

		if (defs->getType() == UFOPAEDIA_TYPE_TFTD)
		{
			// this command is contained in all the subtypes of this article,
			// and probably shouldn't run until all surfaces are added.
			// in the case of a simple image/title/text article,
			// we're done adding surfaces for now.
			centerAllSurfaces();
		}

	}

	ArticleStateTFTD::~ArticleStateTFTD()
	{}

}
