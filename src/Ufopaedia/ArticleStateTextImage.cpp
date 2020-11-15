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
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/LocalizedText.h"
#include "../Mod/Mod.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

namespace OpenXcom
{

	ArticleStateTextImage::ArticleStateTextImage(ArticleDefinitionTextImage *defs, std::shared_ptr<ArticleCommonState> state) : ArticleState(defs->id, std::move(state))
	{
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
		if (bpp == 8)
		{
			_btnOk->setColor(Palette::blockOffset(5) + 3);
			_btnPrev->setColor(Palette::blockOffset(5) + 3);
			_btnNext->setColor(Palette::blockOffset(5) + 3);
		}
		else
		{
			_btnOk->setColor(Palette::blockOffset(15) - 1);
			_btnPrev->setColor(Palette::blockOffset(15) - 1);
			_btnNext->setColor(Palette::blockOffset(15) - 1);
		}

		// add screen elements
		_txtTitle = new Text(defs->text_width * scaleX, 48 * scaleY, 5 * scaleX, 22 * scaleY, bpp);
		_txtTitle->setScale(scaleX, scaleY);
		add(_txtTitle);
		_txtTitle->setColor(Palette::blockOffset(15) + 4);
		_txtTitle->setBig();
		_txtTitle->setWordWrap(true);
		_txtTitle->setText(tr(defs->getTitleForPage(_state->current_page)));

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





		int text_height = _txtTitle->getTextHeight();

		if (defs->rect_text.width == 0)
		{
			_txtInfo = new Text(defs->text_width * scaleX, 162 * scaleY, 5 * scaleX, 23 * scaleY + text_height, bpp);
		}
		else
		{
			_txtInfo = new Text(defs->rect_text.width * scaleX, defs->rect_text.height * scaleY, defs->rect_text.x * scaleX, defs->rect_text.y * scaleY, bpp);
		}
		_txtInfo->setScale(scaleX, scaleY);
		add(_txtInfo);

		_txtInfo->setColor(Palette::blockOffset(15)-1);
		_txtInfo->setSecondaryColor(Palette::blockOffset(15) + 4);
		_txtInfo->setWordWrap(true);
		_txtInfo->setText(tr(defs->getTextForPage(_state->current_page)));

		centerAllSurfaces();
	}

	ArticleStateTextImage::~ArticleStateTextImage()
	{}

}
