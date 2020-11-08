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
#include "ArticleStateBaseFacility.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/Mod.h"
#include "../Mod/RuleBaseFacility.h"
#include "../Engine/Game.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Unicode.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

namespace OpenXcom
{

	ArticleStateBaseFacility::ArticleStateBaseFacility(ArticleDefinitionBaseFacility *defs, std::shared_ptr<ArticleCommonState> state) : ArticleState(defs->id, std::move(state))
	{
		RuleBaseFacility *facility = _game->getMod()->getBaseFacility(defs->id, true);

		int bpp = Options::pediaBgResolutionX == Screen::ORIGINAL_WIDTH ? 8 : 32;
		int scaleX = Options::pediaBgResolutionX / Screen::ORIGINAL_WIDTH;
		int scaleY = Options::pediaBgResolutionY / Screen::ORIGINAL_HEIGHT;
		SDL_Color* buttonTextPalette = _game->getMod()->getPalettes().find("PAL_BATTLEPEDIA")->second->getColors();

		// Set palette
		if (bpp == 8)
		{
			setStandardPalette("PAL_BASESCAPE");
		}
		else
		{
			genPediaPal();
			_cursorColor = Mod::UFOPAEDIA_CURSOR;
		}

		int textColor = Palette::blockOffset(14) + 15;
		int textColor2 = Palette::blockOffset(15) + 4;

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
			_btnOk->setColor(Palette::blockOffset(4));
			_btnPrev->setColor(Palette::blockOffset(4));
			_btnNext->setColor(Palette::blockOffset(4));
			_btnInfo->setColor(Palette::blockOffset(4));
		}
		else
		{
			_btnOk->setColor(Palette::blockOffset(15) - 1);
			_btnPrev->setColor(Palette::blockOffset(15) - 1);
			_btnNext->setColor(Palette::blockOffset(15) - 1);
			_btnInfo->setColor(Palette::blockOffset(15) - 1);
		}

		_btnInfo->setVisible(_game->getMod()->getShowPediaInfoButton());

		// add screen elements
		_txtTitle = new Text(200 * scaleX, 17 * scaleY, 10 * scaleX, 24 * scaleY, bpp);
		_txtTitle->setScale(scaleX, scaleY);
		add(_txtTitle);
		_txtTitle->setColor(Palette::blockOffset(13) + 10);
		_txtTitle->setBig();
		_txtTitle->setText(tr(defs->getTitleForPage(_state->current_page)));

		// Set up objects
		if (bpp == 8)
		{
			_game->getMod()->getSurface("BACK09.SCR")->blitNShade(_bg, 0, 0);
		}
		else
		{
			_game->getMod()->getSurface(getTypeId("BACK09.SCR",bpp), true, Options::pediaBgResolutionX, Options::pediaBgResolutionY)->blitNShade32(_bg, 0, 0);
		}



		// build preview image
		int tile_size = 32;
		_image = new Surface(tile_size * 2 * scaleX, tile_size * 2 * scaleY, 232 * scaleX, 16 * scaleY, bpp);
		add(_image);

		SurfaceSet *graphic = _game->getMod()->getSurfaceSet("BASEBITS.PCK");
		Surface *frame;
		int x_offset, y_offset;
		int x_pos, y_pos;
		int num;

		if (facility->getSize()==1)
		{
			x_offset = tile_size/2 * scaleX;
			y_offset = tile_size / 2 * scaleY;
		}
		else
		{
			x_offset = y_offset = 0;
		}

		num = 0;
		y_pos = y_offset;
		for (int y = 0; y < facility->getSize(); ++y)
		{
			x_pos = x_offset;
			for (int x = 0; x < facility->getSize(); ++x)
			{
				frame = graphic->getFrame(facility->getSpriteShape() + num);
				if (bpp == 8)
				{
					frame->blitNShade(_image, x_pos, y_pos);
				}
				else
				{
					// no 32 bits images thus scale and convert 32 bits
					Surface newSurf = Surface(*frame);
					newSurf.setScale(scaleX, scaleY);
					newSurf.doScale();
					newSurf.convertTo32Bits(&newSurf, _game->getMod()->getPalette("PAL_BASESCAPE")->getColors());
					newSurf.blitNShade32(_image, x_pos, y_pos);
				}


				if (facility->getSize()==1)
				{
					frame = graphic->getFrame(facility->getSpriteFacility() + num);

					if (bpp == 8)
					{
						frame->blitNShade(_image, x_pos, y_pos);
					}
					else
					{
						// no 32 bits images thus scale and convert 32 bits
						Surface newSurf = Surface(*frame);
						newSurf.setScale(scaleX, scaleY);
						newSurf.doScale();
						newSurf.convertTo32Bits(&newSurf, _game->getMod()->getPalette("PAL_BASESCAPE")->getColors());
						newSurf.blitNShade32(_image, x_pos, y_pos);
					}
				}

				x_pos += tile_size;
				num++;
			}
			y_pos += tile_size;
		}

		_txtInfo = new Text(300 * scaleX, 90 * scaleY, 10 * scaleX, 104 * scaleY, bpp);
		_txtInfo->setScale(scaleX, scaleY);
		add(_txtInfo);
		_txtInfo->setColor(Palette::blockOffset(13)+10);
		_txtInfo->setSecondaryColor(Palette::blockOffset(13));
		_txtInfo->setWordWrap(true);
		_txtInfo->setText(tr(defs->getTextForPage(_state->current_page)));

		_lstInfo = new TextList(200 * scaleX, 42 * scaleY, 10 * scaleX, 42 * scaleY, bpp);
		_lstInfo->setScale(scaleX, scaleY);
		_lstInfo->statePalette = _palette;
		_lstInfo->textPalette = _palette;
		_lstInfo->textColor = textColor;
		_lstInfo->textColor2 = textColor2;
		add(_lstInfo);

		_lstInfo->setColor(Palette::blockOffset(13)+10);
		_lstInfo->setColumns(2, 140 * scaleX, 60 * scaleY);
		_lstInfo->setDot(true);

		_lstInfo->addRow(2, tr("STR_CONSTRUCTION_TIME").c_str(), tr("STR_DAY", facility->getBuildTime()).c_str());
		_lstInfo->setCellColor(0, 1, Palette::blockOffset(13)+0);

		std::ostringstream ss;
		ss << Unicode::formatFunding(facility->getBuildCost());
		_lstInfo->addRow(2, tr("STR_CONSTRUCTION_COST").c_str(), ss.str().c_str());
		_lstInfo->setCellColor(1, 1, Palette::blockOffset(13)+0);

		ss.str("");ss.clear();
		ss << Unicode::formatFunding(facility->getMonthlyCost());
		_lstInfo->addRow(2, tr("STR_MAINTENANCE_COST").c_str(), ss.str().c_str());
		_lstInfo->setCellColor(2, 1, Palette::blockOffset(13)+0);

		if (facility->getDefenseValue() > 0)
		{
			ss.str("");ss.clear();
			ss << facility->getDefenseValue();
			_lstInfo->addRow(2, tr("STR_DEFENSE_VALUE").c_str(), ss.str().c_str());
			_lstInfo->setCellColor(3, 1, Palette::blockOffset(13)+0);

			ss.str("");ss.clear();
			ss << Unicode::formatPercentage(facility->getHitRatio());
			_lstInfo->addRow(2, tr("STR_HIT_RATIO").c_str(), ss.str().c_str());
			_lstInfo->setCellColor(4, 1, Palette::blockOffset(13)+0);
		}
		centerAllSurfaces();
	}

	ArticleStateBaseFacility::~ArticleStateBaseFacility()
	{}

}
