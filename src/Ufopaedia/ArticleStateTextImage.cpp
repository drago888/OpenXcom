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
#include "ArticleStateTextImage.h"
#include "../Engine/Game.h"
#include "../Engine/Surface.h"
#include "../Engine/Palette.h"
#include "../Mod/Mod.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

	ArticleStateTextImage::ArticleStateTextImage(ArticleDefinitionTextImage *defs, std::shared_ptr<ArticleCommonState> state) : ArticleState(defs->id, std::move(state))
	{
		int bpp = Options::pediaBgResolutionX == Screen::ORIGINAL_WIDTH ? 8 : 32;
		int scaleX = Options::pediaBgResolutionX / Screen::ORIGINAL_WIDTH;
		int scaleY = Options::pediaBgResolutionY / Screen::ORIGINAL_HEIGHT;
		SDL_Color* buttonTextPalette = _game->getMod()->getPalettes().find("PAL_BATTLEPEDIA")->second->getColors();

		// add screen elements
		_txtTitle = new Text(defs->text_width * scaleX, 48 * scaleY, 5 * scaleX, 22 * scaleY, bpp);
		_txtTitle->setScale(scaleX, scaleY);

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
			setStatePalette(_game->getMod()->getPalettes().find("PAL_UFOPAEDIA")->second->getColors()); 
			_cursorColor = Mod::UFOPAEDIA_CURSOR;
		}

		if (bpp == 8)
		{
			_buttonColor = _game->getMod()->getInterface("articleTextImage")->getElement("button")->color;
			_titleColor = _game->getMod()->getInterface("articleTextImage")->getElement("title")->color;
			_textColor1 = _game->getMod()->getInterface("articleTextImage")->getElement("text")->color;
			_textColor2 = _game->getMod()->getInterface("articleTextImage")->getElement("text")->color2;
		}
		else
		{
			_buttonColor = Palette::blockOffset(15) - 1;
			_textColor1 = Palette::blockOffset(14) + 15;
			_textColor2 = Palette::blockOffset(15) + 4;
			_titleColor = Palette::blockOffset(14) + 12;

			// set buttons palette before adding to state
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

		// add other elements
		add(_txtTitle);

		// Set up objects
		if (bpp == 8)
		{
			_game->getMod()->getSurface(defs->image_id)->blitNShade(_bg, 0, 0);
		}
		else
		{
			Surface surf;
			bool use_pal = false;

			// default to true if image_id have . in it eg .spk but is not bigobs.pck_
			if (defs->image_id != "BIGOBS.PCK" && defs->image_id.find(".") != std::string::npos)
			{
				use_pal = true;
			}
			get32Surf("32_" + defs->image_id, defs->image_id, &surf, "PAL_UFOPAEDIA", use_pal)->blitNShade32(_bg, 0, 0);
		}

		if (bpp == 8)
		{
			_btnOk->setColor(_buttonColor);
			_btnPrev->setColor(_buttonColor);
			_btnNext->setColor(_buttonColor);
		}
		else
		{
			_btnOk->setColor(Palette::blockOffset(15) - 1);
			_btnPrev->setColor(Palette::blockOffset(15) - 1);
			_btnNext->setColor(Palette::blockOffset(15) - 1);
		}


		_txtTitle->setColor(_titleColor);
		_txtTitle->setBig();
		_txtTitle->setWordWrap(true);
		_txtTitle->setText(tr(defs->getTitleForPage(_state->current_page)));

		int text_height = _txtTitle->getTextHeight();

		if (defs->rect_text.width == 0)
		{
			_txtInfo = new Text(defs->text_width * scaleX, (176 - text_height) * scaleY, 5 * scaleX, 23 * scaleY + text_height, bpp);
		}
		else
		{
			_txtInfo = new Text(defs->rect_text.width * scaleX, defs->rect_text.height * scaleY, defs->rect_text.x * scaleX, defs->rect_text.y * scaleY, bpp);
		}
		_txtInfo->setScale(scaleX, scaleY);
		add(_txtInfo);

		_txtInfo->setColor(_textColor1);
		_txtInfo->setSecondaryColor(_textColor2);
		_txtInfo->setWordWrap(true);
		_txtInfo->setScrollable(true);
		if (defs->align_bottom)
		{
			_txtInfo->setVerticalAlign(ALIGN_BOTTOM);
		}
		_txtInfo->setText(tr(defs->getTextForPage(_state->current_page)));

		centerAllSurfaces();
	}

	ArticleStateTextImage::~ArticleStateTextImage()
	{}

}
