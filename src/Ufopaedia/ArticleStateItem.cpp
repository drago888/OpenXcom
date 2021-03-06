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
#include <algorithm>
#include "Ufopaedia.h"
#include "ArticleStateItem.h"
#include "../Mod/Mod.h"
#include "../Mod/ArticleDefinition.h"
#include "../Mod/RuleItem.h"
#include "../Engine/Game.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Unicode.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Mod/RuleInterface.h"
#include "../fmath.h"
#include "StatsForNerdsState.h"

namespace OpenXcom
{

	ArticleStateItem::ArticleStateItem(ArticleDefinitionItem *defs, std::shared_ptr<ArticleCommonState> state) : ArticleState(defs->id, std::move(state)), _lstInfo(nullptr)
	{
		RuleItem *item = _game->getMod()->getItem(defs->id, true);

		int bpp = Options::pediaBgResolutionX == Screen::ORIGINAL_WIDTH ? 8 : 32;
		int scaleX = Options::pediaBgResolutionX / Screen::ORIGINAL_WIDTH;
		int scaleY = Options::pediaBgResolutionY / Screen::ORIGINAL_HEIGHT;
		SDL_Color* buttonTextPalette = _game->getMod()->getPalettes().find("PAL_BATTLEPEDIA")->second->getColors();

		int bottomOffset = 20;
		std::string accuracyModifier;
		bool isAccurracyModded = false;
		if (item->getBattleType() == BT_MELEE)
		{
			accuracyModifier = addRuleStatBonus(*item->getMeleeMultiplierRaw());
			isAccurracyModded = item->getMeleeMultiplierRaw()->isModded();
		}
		else
		{
			accuracyModifier = addRuleStatBonus(*item->getAccuracyMultiplierRaw());
			isAccurracyModded = item->getAccuracyMultiplierRaw()->isModded();
		}
		std::string powerBonus = addRuleStatBonus(*item->getDamageBonusRaw());

		if (!isAccurracyModded)
		{
			// don't show default accuracy multiplier
			bottomOffset = 9;
		}

		if (!_game->getMod()->getExtraNerdyPediaInfo())
		{
			// feature turned off
			bottomOffset = 0;
		}
		else if (item->getBattleType() == BT_AMMO)
		{
			// don't show accuracy multiplier... even if someone mods it by mistake, it still makes no sense
			bottomOffset = 9;
		}
		else if (item->getBattleType() == BT_FIREARM || item->getBattleType() == BT_MELEE)
		{
			if (item->getClipSize() != 0)
			{
				// correct info loaded already... weapon has built-in ammo
			}
			else
			{
				// need to check power bonus on all compatible ammo
				bool first = true;
				bool allSame = true;
				for (int slot = 0; slot < RuleItem::AmmoSlotMax; ++slot)
				{
					for (auto ammoItemRule : *item->getCompatibleAmmoForSlot(slot))
					{
						if (first)
						{
							powerBonus = addRuleStatBonus(*ammoItemRule->getDamageBonusRaw());
							first = false;
						}
						else
						{
							std::string otherPowerBonus = addRuleStatBonus(*ammoItemRule->getDamageBonusRaw());
							if (powerBonus != otherPowerBonus)
							{
								allSame = false;
								powerBonus = tr("STR_MULTIPLE_DIFFERENT_BONUSES");
							}
						}
						if (!allSame) break;
					}
					if (!allSame) break;
				}
			}
		}
		else if (item->getBattleType() == BT_PSIAMP)
		{
			// correct info loaded already
		}
		else
		{
			// nothing to show for other item types
			bottomOffset = 0;
		}

		if (powerBonus.empty())
		{
			if (!isAccurracyModded)
			{
				// both power bonus and accuracy multiplier are vanilla, hide info completely
				bottomOffset = 0;
			}
			else
			{
				// display zero instead of empty string
				powerBonus = "0";
			}
		}

		// Set palette
		if (bpp == 8)
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
			_buttonColor = _game->getMod()->getInterface("articleItem")->getElement("button")->color;
			_arrowColor = _buttonColor;
			if (_game->getMod()->getInterface("articleItem")->getElement("arrow"))
			{
				_arrowColor = _game->getMod()->getInterface("articleItem")->getElement("arrow")->color;
			}
			_textColor = _game->getMod()->getInterface("articleItem")->getElement("text")->color;
			_textColor2 = _game->getMod()->getInterface("articleItem")->getElement("text")->color2;
			_listColor1 = _game->getMod()->getInterface("articleItem")->getElement("list")->color;
			_listColor2 = _game->getMod()->getInterface("articleItem")->getElement("list")->color2;
			_ammoColor = _game->getMod()->getInterface("articleItem")->getElement("ammoColor")->color;
		}
		else
		{
			_buttonColor = Palette::blockOffset(15) - 1;
			_arrowColor = _buttonColor;
			if (_game->getMod()->getInterface("articleItem")->getElement("arrow"))
			{
				_arrowColor = _game->getMod()->getInterface("articleItem")->getElement("arrow")->color;
			}
			_textColor = Palette::blockOffset(14) + 15;
			_textColor2 = Palette::blockOffset(15) + 4;
			_listColor1 = Palette::blockOffset(14) + 15;
			_listColor2 = Palette::blockOffset(15) + 4;
			_ammoColor = _game->getMod()->getInterface("articleItem")->getElement("ammoColor")->color;
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
		_btnOk->setColor(_buttonColor);
		_btnPrev->setColor(_buttonColor);
		_btnNext->setColor(_buttonColor);
		_btnInfo->setColor(_buttonColor);
		_btnInfo->setVisible(_game->getMod()->getShowPediaInfoButton());

		// add screen elements

		// Set up objects
		if (bpp == 8)
		{
			_game->getMod()->getSurface("BACK08.SCR")->blitNShade(_bg, 0, 0);
		}
		else
		{
			Surface surf;
			get32Surf("32_BACK08.SCR", "BACK08.SCR", &surf, "PAL_BATTLEPEDIA")->blitNShade32(_bg, 0, 0, -30); //brighten the background
		}

		_txtTitle = new Text(148 * scaleX, 32 * scaleY, 5 * scaleX, 24 * scaleY, bpp);
		_txtTitle->setScale(scaleX, scaleY);
		add(_txtTitle);
		_txtTitle->setColor(_textColor);
		_txtTitle->setBig();
		_txtTitle->setWordWrap(true);
		_txtTitle->setText(tr(defs->getTitleForPage(_state->current_page)));

		_txtWeight = new Text(88 * scaleX, 8 * scaleY, 104 * scaleX, 55 * scaleY, bpp);
		_txtWeight->setScale(scaleX, scaleY);
		add(_txtWeight);
		_txtWeight->setColor(_textColor);
		_txtWeight->setAlign(ALIGN_RIGHT);

		// IMAGE
		_image = new Surface(32 * scaleX, 48 * scaleY, 157 * scaleX, 5 * scaleY, bpp);
		//add(_image);

		if (bpp == 8)
		{
			item->drawHandSprite(_game->getMod()->getSurfaceSet("BIGOBS.PCK"), _image);
		}
		else
		{
			Surface surf;
			item->drawHandSprite(_game->getMod()->getSurfaceSet32(getTypeId("BIGOBS.PCK", bpp)), _image, 0, 0, bpp);
		}

		add(_image);


		auto ammoSlot = defs->getAmmoSlotForPage(_state->current_page);
		auto ammoSlotPrevUsage = defs->getAmmoSlotPrevUsageForPage(_state->current_page);
		const std::vector<const RuleItem*> dummy;
		const std::vector<const RuleItem*> *ammo_data = ammoSlot != RuleItem::AmmoSlotSelfUse ? item->getCompatibleAmmoForSlot(ammoSlot) : &dummy;

		int weight = item->getWeight();
		std::string weightLabel = tr("STR_WEIGHT_PEDIA1").arg(weight);
		if (!ammo_data->empty())
		{
			// Note: weight including primary ammo only!
			const RuleItem *ammo_rule = (*ammo_data)[0];
			weightLabel = tr("STR_WEIGHT_PEDIA2").arg(weight).arg(weight + ammo_rule->getWeight());
		}
		_txtWeight->setText(weight > 0 ? weightLabel : "");

		// SHOT STATS TABLE (for firearms and melee only)
		if (item->getBattleType() == BT_FIREARM || item->getBattleType() == BT_MELEE)
		{
			_txtShotType = new Text(100 * scaleX, 17 * scaleY, 8 * scaleX, 66 * scaleY, bpp);
			_txtShotType->setScale(scaleX, scaleY);
			add(_txtShotType);
			_txtShotType->setColor(_textColor);
			_txtShotType->setWordWrap(true);
			_txtShotType->setText(tr("STR_SHOT_TYPE"));

			_txtAccuracy = new Text(50 * scaleX, 17 * scaleY, 104 * scaleX, 66 * scaleY, bpp);
			_txtAccuracy->setScale(scaleX, scaleY);
			add(_txtAccuracy);
			_txtAccuracy->setColor(_textColor);
			_txtAccuracy->setWordWrap(true);
			_txtAccuracy->setText(tr("STR_ACCURACY_UC"));

			_txtTuCost = new Text(60 * scaleX, 17 * scaleY, 158 * scaleX, 66 * scaleY, bpp);
			_txtTuCost->setScale(scaleX, scaleY);
			add(_txtTuCost);
			_txtTuCost->setColor(_textColor);
			_txtTuCost->setWordWrap(true);
			_txtTuCost->setText(tr("STR_TIME_UNIT_COST"));

			_lstInfo = new TextList(204 * scaleX, 55 * scaleY, 8 * scaleX, 82 * scaleY, bpp);
			_lstInfo->setScale(scaleX, scaleY);
			add(_lstInfo);

			_lstInfo->setColor(_listColor2); // color for % data!
			_lstInfo->statePalette = _palette;
			_lstInfo->textPalette = _palette;
			_lstInfo->textColor = _textColor;
			_lstInfo->textColor2 = _textColor2;
			_lstInfo->setColumns(3, 100 * scaleX, 52 * scaleY, 52 * scaleX);
			_lstInfo->setBig();
		}

		auto addAttack = [&](int& row, const std::string& name, const RuleItemUseCost& cost, const RuleItemUseCost& flat, const RuleItemAction *config)
		{
			if (row < 3 && cost.Time > 0 && config->ammoSlot == ammoSlot)
			{
				std::string tu = Unicode::formatPercentage(cost.Time);
				if (flat.Time)
				{
					tu.erase(tu.end() - 1);
				}
				_lstInfo->addRow(3,
					tr(name).arg(config->shots).c_str(),
					Unicode::formatPercentage(config->accuracy).c_str(),
					tu.c_str());
				_lstInfo->setCellColor(row, 0, _listColor1);
				row++;
			}
		};

		if (item->getBattleType() == BT_FIREARM)
		{
			int current_row = 0;

			addAttack(current_row, "STR_SHOT_TYPE_AUTO", item->getCostAuto(), item->getFlatAuto(), item->getConfigAuto());

			addAttack(current_row, "STR_SHOT_TYPE_SNAP", item->getCostSnap(), item->getFlatSnap(), item->getConfigSnap());

			addAttack(current_row, "STR_SHOT_TYPE_AIMED", item->getCostAimed(), item->getFlatAimed(), item->getConfigAimed());

			//optional melee
			addAttack(current_row, "STR_SHOT_TYPE_MELEE", item->getCostMelee(), item->getFlatMelee(), item->getConfigMelee());

			// text_info is BELOW the info table (table can have 0-3 rows)
			int shift = (3 - current_row) * 16;
			if (ammo_data->size() == 2 && current_row <= 1)
			{
				shift -= (2 - current_row) * 16;
			}
			_txtInfo = new Text((ammo_data->size()<3 ? 300 : 180) * scaleX, (56 + shift - bottomOffset) * scaleY, 8 * scaleX, (138 - shift) * scaleY, bpp);
			_txtInfo->setScale(scaleX, scaleY);
			add(_txtInfo);
		}
		else if (item->getBattleType() == BT_MELEE)
		{
			int current_row = 0;

			addAttack(current_row, "STR_SHOT_TYPE_MELEE", item->getCostMelee(), item->getFlatMelee(), item->getConfigMelee());

			// text_info is BELOW the info table (with 1 row only)
			_txtInfo = new Text(300 * scaleX, (88 - bottomOffset) * scaleY, 8 * scaleX, 106 * scaleY, bpp);
			_txtInfo->setScale(scaleX, scaleY);
			add(_txtInfo);
		}
		else
		{
			// text_info is larger and starts on top
			_txtInfo = new Text(300 * scaleX, (125 - bottomOffset) * scaleY, 8 * scaleX, 67 * scaleY, bpp);
			_txtInfo->setScale(scaleX, scaleY);
			add(_txtInfo);
		}

		_txtInfo->setColor(_textColor);
		_txtInfo->setSecondaryColor(_textColor2);
		_txtInfo->setWordWrap(true);
		_txtInfo->setText(tr(defs->getTextForPage(_state->current_page)));


		// STATS FOR NERDS extract
		_txtAccuracyModifier = new Text(300 * scaleX, 9 * scaleY, 8 * scaleX, 174 * scaleY, bpp);
		_txtAccuracyModifier->setScale(scaleX, scaleY);
		_txtPowerBonus = new Text(300 * scaleX, 17 * scaleY, 8 * scaleX, 183 * scaleY, bpp);
		_txtPowerBonus->setScale(scaleX, scaleY);

		if (bottomOffset > 0)
		{
			if (bottomOffset >= 20)
			{
				add(_txtAccuracyModifier);
			}
			add(_txtPowerBonus);
		}

		_txtAccuracyModifier->setColor(_textColor);
		_txtAccuracyModifier->setSecondaryColor(_listColor2);
		_txtAccuracyModifier->setWordWrap(false);
		_txtAccuracyModifier->setText(tr("STR_ACCURACY_MODIFIER").arg(accuracyModifier));

		_txtPowerBonus->setColor(_textColor);
		_txtPowerBonus->setSecondaryColor(_listColor2);
		_txtPowerBonus->setWordWrap(true);
		_txtPowerBonus->setText(tr("STR_POWER_BONUS").arg(powerBonus));


		// AMMO column
		std::ostringstream ss;

		for (int i = 0; i<3; ++i)
		{
			_txtAmmoType[i] = new Text(82 * scaleX, 16 * scaleY, 194 * scaleX, (20 + i*49) * scaleY, bpp);
			_txtAmmoType[i]->setScale(scaleX, scaleY);
			add(_txtAmmoType[i]);
			_txtAmmoType[i]->setColor(_textColor);
			_txtAmmoType[i]->setAlign(ALIGN_CENTER);
			_txtAmmoType[i]->setVerticalAlign(ALIGN_MIDDLE);
			_txtAmmoType[i]->setWordWrap(true);

			_txtAmmoDamage[i] = new Text(82 * scaleX, 17 * scaleY, 194 * scaleX, (40 + i*49) * scaleY, bpp);
			_txtAmmoDamage[i]->setScale(scaleX, scaleY);
			add(_txtAmmoDamage[i]);
			_txtAmmoDamage[i]->setColor(_ammoColor);
			_txtAmmoDamage[i]->setAlign(ALIGN_CENTER);
			_txtAmmoDamage[i]->setBig();

			_imageAmmo[i] = new Surface(32 * scaleX, 48 * scaleY, 280 * scaleX, (16 + i*49) * scaleY, bpp);
			add(_imageAmmo[i]);
		}

		auto addAmmoDamagePower = [&](int pos, const RuleItem *rule)
		{
			_txtAmmoType[pos]->setText(tr(getDamageTypeText(rule->getDamageType()->ResistType)));

			ss.str("");ss.clear();
			ss << rule->getPower();
			if (rule->getShotgunPellets())
			{
				ss << "x" << rule->getShotgunPellets();
			}
			_txtAmmoDamage[pos]->setText(ss.str());
			_txtAmmoDamage[pos]->setColor(getDamageTypeTextColor(rule->getDamageType()->ResistType));

		};

		switch (item->getBattleType())
		{
			case BT_FIREARM:
				if (item->getHidePower()) break;
				_txtDamage = new Text(82 * scaleX, 10 * scaleY, 194 * scaleX, 7 * scaleY, bpp);
				_txtDamage->setScale(scaleX, scaleY);
				add(_txtDamage);
				_txtDamage->setColor(_textColor);
				_txtDamage->setAlign(ALIGN_CENTER);
				_txtDamage->setText(tr("STR_DAMAGE_UC"));

				_txtAmmo = new Text(50 * scaleX, 10 * scaleY, 268 * scaleX, 7 * scaleY, bpp);
				_txtAmmo->setScale(scaleX, scaleY);
				add(_txtAmmo);
				_txtAmmo->setColor(_textColor);
				_txtAmmo->setAlign(ALIGN_CENTER);
				_txtAmmo->setText(tr("STR_AMMO"));

				if (ammo_data->empty())
				{
					addAmmoDamagePower(0, item);
				}
				else
				{
					int maxShow = 3;
					int skipShow = maxShow * ammoSlotPrevUsage;
					int currShow = 0;
					for (auto& type : *ammo_data)
					{
						ArticleDefinition *ammo_article = _game->getMod()->getUfopaediaArticle(type->getType(), true);
						if (Ufopaedia::isArticleAvailable(_game->getSavedGame(), ammo_article))
						{
							if (skipShow > 0)
							{
								--skipShow;
								continue;
							}

							addAmmoDamagePower(currShow, type);

							if (bpp == 8)
							{
								type->drawHandSprite(_game->getMod()->getSurfaceSet(getTypeId("BIGOBS.PCK", bpp)), _imageAmmo[currShow]);
							}
							else
							{
								type->drawHandSprite(_game->getMod()->getSurfaceSet(getTypeId("BIGOBS.PCK", bpp)), _imageAmmo[currShow], 0, 0, bpp);
							}


							++currShow;
							if (currShow == maxShow)
							{
								break;
							}
						}
					}
				}
				break;
			case BT_AMMO:
			case BT_GRENADE:
			case BT_PROXIMITYGRENADE:
			case BT_MELEE:
				if (item->getHidePower()) break;
				_txtDamage = new Text(82 * scaleX, 10 * scaleY, 194 * scaleX, 7 * scaleY, bpp);
				_txtDamage->setScale(scaleX, scaleY);
				add(_txtDamage);
				_txtDamage->setColor(_textColor);
				_txtDamage->setAlign(ALIGN_CENTER);
				_txtDamage->setText(tr("STR_DAMAGE_UC"));

				addAmmoDamagePower(0, item);
				break;
			default: break;
		}

		// multi-page indicator
		_txtArrows = new Text(32 * scaleX, 9 * scaleY, 280 * scaleX, 183 *scaleY, bpp);
		_txtArrows->setScale(scaleX, scaleY);
		add(_txtArrows);
		_txtArrows->setColor(_arrowColor);
		_txtArrows->setAlign(ALIGN_RIGHT);
		std::ostringstream ss2;
		if (_state->hasPrevArticlePage()) ss2 << "<<";
		if (_state->hasNextArticlePage()) ss2 << " >>";
		_txtArrows->setText(ss2.str());

		centerAllSurfaces();
	}

	ArticleStateItem::~ArticleStateItem()
	{}

	std::string ArticleStateItem::addRuleStatBonus(const RuleStatBonus &value)
	{
		std::ostringstream ss;
		bool isFirst = true;
		for (RuleStatBonusDataOrig item : *value.getBonusRaw())
		{
			int power = 0;
			for (float number : item.second)
			{
				++power;
				if (!AreSame(number, 0.0f))
				{
					float numberAbs = number;
					if (!isFirst)
					{
						if (number > 0.0f)
						{
							ss << " + ";
						}
						else
						{
							ss << " - ";
							numberAbs = std::abs(number);
						}
					}
					if (item.first == "flatOne")
					{
						ss << numberAbs * 1;
					}
					if (item.first == "flatHundred")
					{
						ss << numberAbs * pow(100, power);
					}
					else
					{
						if (!AreSame(numberAbs, 1.0f))
						{
							ss << numberAbs << "*";
						}

						ss << tr(StatsForNerdsState::translationMap.at(item.first));
						if (power > 1)
						{
							ss << "^" << power;
						}
					}
					isFirst = false;
				}
			}
		}
		return ss.str();
	}

	int ArticleStateItem::getDamageTypeTextColor(ItemDamageType dt)
	{
		Element *interfaceElement = 0;
		int color = _ammoColor;

		switch (dt)
		{
			case DT_NONE:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDTNone");
				break;

			case DT_AP:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDTAP");
				break;

			case DT_IN:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDTIN");
				break;

			case DT_HE:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDTHE");
				break;

			case DT_LASER:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDTLaser");
				break;

			case DT_PLASMA:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDTPlasma");
				break;

			case DT_STUN:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDTStun");
				break;

			case DT_MELEE:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDTMelee");
				break;

			case DT_ACID:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDTAcid");
				break;

			case DT_SMOKE:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDTSmoke");
				break;

			case DT_10:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDT10");
				break;

			case DT_11:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDT11");
				break;

			case DT_12:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDT12");
				break;

			case DT_13:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDT13");
				break;

			case DT_14:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDT14");
				break;

			case DT_15:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDT15");
				break;

			case DT_16:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDT16");
				break;

			case DT_17:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDT17");
				break;

			case DT_18:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDT18");
				break;

			case DT_19:
				interfaceElement = _game->getMod()->getInterface("articleItem")->getElement("ammoColorDT19");
				break;

			default :
				break;
		}

		if (interfaceElement)
		{
			color = interfaceElement->color;
		}

		return color;
	}
}
