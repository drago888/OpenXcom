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

#include "Ufopaedia.h"
#include "ArticleState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Surface.h"
#include "../Engine/LocalizedText.h"
#include "../Interface/TextButton.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/RuleItem.h"
#include "../Mod/Mod.h"
#include "../Savegame/SavedGame.h"
#include "../Engine/Screen.h"
#include "../Mod/ExtraSprites.h"
#include "../Engine/Palette.h"
#include <exception>

namespace OpenXcom
{
	Cursor* ArticleState::_bigCursor = nullptr;
	Cursor* ArticleState::_smallCursor = nullptr;
	bool ArticleState::inPediaArticle = false;
	/**
	 * Change index position to next article.
	 */
	void ArticleCommonState::nextArticle()
	{
		if (current_index >= articleList.size() - 1)
		{
			// goto first
			current_index = 0;
		}
		else
		{
			current_index++;
		}
	}

	/**
	 * Change page to next in article or move to next index position.
	 * Each article can have multiple pages, if we are on last page we will move to first page of next article.
	 */
	void ArticleCommonState::nextArticlePage()
	{
		if (!hasNextArticlePage())
		{
			// goto to first page of next article
			nextArticle();
			current_page = 0;
		}
		else
		{
			current_page++;
		}
	}

	bool ArticleCommonState::hasNextArticlePage()
	{
		return !(current_page >= getCurrentArticle()->getNumberOfPages() - 1);
	}

	/**
	 * Change index position to previous article.
	 */
	void ArticleCommonState::prevArticle()
	{
		if (current_index == 0 || current_index > articleList.size() - 1)
		{
			// goto last
			current_index = articleList.size() - 1;
		}
		else
		{
			current_index--;
		}
	}

	/**
	 * Change page to previous in article or move to previous index position.
	 * Each article can have multiple pages, if we are on first page we will move to last page of previous article.
	 */
	void ArticleCommonState::prevArticlePage()
	{
		if (!hasPrevArticlePage())
		{
			// goto last page of previous article
			prevArticle();
			current_page = getCurrentArticle()->getNumberOfPages() - 1;
		}
		else
		{
			current_page--;
		}
	}

	bool ArticleCommonState::hasPrevArticlePage()
	{
		return !(current_page == 0 || current_page > getCurrentArticle()->getNumberOfPages() - 1);
	}

	/**
	 * Constructor
	 * @param game Pointer to current game.
	 * @param article_id The article id of this article state instance.
	 */
	ArticleState::ArticleState(const std::string &article_id, std::shared_ptr<ArticleCommonState> state) : _id(article_id)
	{
		int bpp = Options::pediaBgResolutionX == Screen::ORIGINAL_WIDTH ? 8 : 32;
		_resX = Options::pediaBgResolutionX, _resY = Options::pediaBgResolutionY;
		_bpp = bpp;

		int scaleX = Options::pediaBgResolutionX / Screen::ORIGINAL_WIDTH;
		int scaleY = Options::pediaBgResolutionY / Screen::ORIGINAL_HEIGHT;
		if (!ArticleState::inPediaArticle)
		{
			_game->changeCursor(_bigCursor);
			_game->mouseScaleXMul = scaleX;
			_game->mouseScaleYMul = scaleY;
			ArticleState::inPediaArticle = true;
			resetScreen = true;
		}

		// init background and navigation elements
		_bg = new Surface(Options::pediaBgResolutionX, Options::pediaBgResolutionY, 0 * scaleX, 0 * scaleY, bpp);
		_btnOk = new TextButton(30 * scaleX, 14 * scaleY, 5 * scaleX, 5 * scaleY, bpp, scaleX, scaleY);
		_btnOk->setScale(scaleX, scaleY);
		_btnPrev = new TextButton(30 * scaleX, 14 * scaleY, 40 * scaleX, 5 * scaleY, bpp, scaleX, scaleY);
		_btnPrev->setScale(scaleX, scaleY);
		_btnNext = new TextButton(30 * scaleX, 14 * scaleY, 75 * scaleX, 5 * scaleY, bpp, scaleX, scaleY);
		_btnNext->setScale(scaleX, scaleY);
		_btnInfo = new TextButton(40 * scaleX, 14 * scaleY, 110 * scaleX, 5 * scaleY, bpp, scaleX, scaleY);
		_btnInfo->setScale(scaleX, scaleY);


		_state = std::move(state);

		// remember this article as seen/normal
		_game->getSavedGame()->setUfopediaRuleStatus(_id, ArticleDefinition::PEDIA_STATUS_NORMAL);
	}

	/**
	 * Destructor
	 */
	ArticleState::~ArticleState()
	{
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
	Surface* ArticleState::get32Surf(std::string id32, std::string id8, Surface* newSurf, std::string palName, bool usePal)
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
			newSurf->convertTo32Bits(newSurf, pal.getColors());
			surf = newSurf;
		}

		return surf;
	}

	/*
    * Get the 32 bit sprites type id
    * @param id the 8 bit sprite type id
    * @return the typeid to use
    */
	std::string ArticleState::getTypeId(std::string id, int bpp)
	{
		if (bpp != 8 && _game->getMod()->getExtraSprites().find(id)->second.size() > 0)
		{
			ExtraSprites* sprite = _game->getMod()->getExtraSprites().find(id)->second[0];

			std::string str_chk = "Resources/Backgrounds/";
			std::string str_chk2 = "Resources/";
			std::string str_chk3 = "Resources/Weapons/";
			std::string str_chk4 = "Resources/Pedia/";
			std::string first_sprite = sprite->getSprites() && sprite->getSprites()->size() > 0 ? sprite->getSprites()->begin()->second : "";


			if ((first_sprite.size() > str_chk.size() && first_sprite.substr(0, str_chk.size()) == str_chk) ||
				(first_sprite.size() > str_chk2.size() && first_sprite.substr(0, str_chk2.size()) == str_chk2
					&& first_sprite.substr(str_chk2.size(), std::string::npos).find("/") == std::string::npos) ||
				(first_sprite.size() > str_chk3.size() && first_sprite.substr(0, str_chk3.size()) == str_chk3) ||
				(first_sprite.size() > str_chk4.size() && first_sprite.substr(0, str_chk4.size()) == str_chk4))
			{
				return "32_" + id;
			}
		}

		return id;
	}

	std::string ArticleState::getDamageTypeText(ItemDamageType dt) const
	{
		std::string type;
		switch (dt)
		{
		case DT_NONE:
			type = "STR_DAMAGE_NONE";
			break;
		case DT_AP:
			type = "STR_DAMAGE_ARMOR_PIERCING";
			break;
		case DT_IN:
			type = "STR_DAMAGE_INCENDIARY";
			break;
		case DT_HE:
			type = "STR_DAMAGE_HIGH_EXPLOSIVE";
			break;
		case DT_LASER:
			type = "STR_DAMAGE_LASER_BEAM";
			break;
		case DT_PLASMA:
			type = "STR_DAMAGE_PLASMA_BEAM";
			break;
		case DT_STUN:
			type = "STR_DAMAGE_STUN";
			break;
		case DT_MELEE:
			type = "STR_DAMAGE_MELEE";
			break;
		case DT_ACID:
			type = "STR_DAMAGE_ACID";
			break;
		case DT_SMOKE:
			type = "STR_DAMAGE_SMOKE";
			break;
		case DT_10:
			type = "STR_DAMAGE_10";
			break;
		case DT_11:
			type = "STR_DAMAGE_11";
			break;
		case DT_12:
			type = "STR_DAMAGE_12";
			break;
		case DT_13:
			type = "STR_DAMAGE_13";
			break;
		case DT_14:
			type = "STR_DAMAGE_14";
			break;
		case DT_15:
			type = "STR_DAMAGE_15";
			break;
		case DT_16:
			type = "STR_DAMAGE_16";
			break;
		case DT_17:
			type = "STR_DAMAGE_17";
			break;
		case DT_18:
			type = "STR_DAMAGE_18";
			break;
		case DT_19:
			type = "STR_DAMAGE_19";
			break;
		default:
			type = "STR_UNKNOWN";
			break;
		}
		return type;
	}

	/**
	 * Set captions and click handlers for the common control elements.
	 */
	void ArticleState::initLayout()
	{
		add(_bg);
		add(_btnOk);
		add(_btnPrev);
		add(_btnNext);
		add(_btnInfo);

		_btnOk->setText(tr("STR_OK"));
		_btnOk->onMouseClick((ActionHandler)&ArticleState::btnOkClick);
		_btnOk->onKeyboardPress((ActionHandler)&ArticleState::btnOkClick,Options::keyOk);
		_btnOk->onKeyboardPress((ActionHandler)&ArticleState::btnOkClick,Options::keyCancel);
		_btnOk->onKeyboardPress((ActionHandler)&ArticleState::btnResetMusicClick, Options::keySelectMusicTrack);
		_btnPrev->setText("<<");
		_btnPrev->onMouseClick((ActionHandler)&ArticleState::btnPrevClick);
		_btnPrev->onKeyboardPress((ActionHandler)&ArticleState::btnPrevClick, Options::keyGeoLeft);
		_btnNext->setText(">>");
		_btnNext->onMouseClick((ActionHandler)&ArticleState::btnNextClick);
		_btnNext->onKeyboardPress((ActionHandler)&ArticleState::btnNextClick, Options::keyGeoRight);
		_btnInfo->setText(tr("STR_INFO_UFOPEDIA"));
		_btnInfo->onMouseClick((ActionHandler)&ArticleState::btnInfoClick);
		_btnInfo->onKeyboardPress((ActionHandler)&ArticleState::btnInfoClick, Options::keyGeoUfopedia);
		_btnInfo->setVisible(false);
	}

	/**
	 * Returns to the previous screen.
	 * @param action Pointer to an action.
	 */
	void ArticleState::btnOkClick(Action *)
	{
		_game->getScreen()->resetDisplay(true, false, Screen::ORIGINAL_WIDTH, Screen::ORIGINAL_HEIGHT, 8);
		_game->changeCursor(_smallCursor);
		_game->mouseScaleXMul = 1;
		_game->mouseScaleYMul = 1;
		ArticleState::inPediaArticle = false;
		_game->popState();
	}

	/**
	 * Resets the music to a random geoscape music.
	 * @param action Pointer to an action.
	 */
	void ArticleState::btnResetMusicClick(Action *)
	{
		// reset that pesky interception music!
		_game->getMod()->playMusic("GMGEO");
	}

	/**
	 * Shows the previous available article.
	 * @param action Pointer to an action.
	 */
	void ArticleState::btnPrevClick(Action *)
	{
		Ufopaedia::prev(_game, _state);
	}

	/**
	 * Shows the next available article. Loops to the first.
	 * @param action Pointer to an action.
	 */
	void ArticleState::btnNextClick(Action *)
	{
		Ufopaedia::next(_game, _state);
	}

	/**
	 * Shows the detailed (raw) information about the current topic.
	 * @param action Pointer to an action.
	 */
	void ArticleState::btnInfoClick(Action *)
	{
		Ufopaedia::openArticleDetail(_game, _id);
	}

}
