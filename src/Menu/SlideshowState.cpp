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
#include "SlideshowState.h"
#include "CutsceneState.h"
#include "../Engine/FileMap.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Engine/Timer.h"
#include "../Interface/Text.h"
#include "../Mod/Mod.h"
#include "../Engine/Options.h"
#include "../Interface/Cursor.h"
#include "../Engine/Palette.h"

namespace OpenXcom
{

SlideshowState::SlideshowState(const SlideshowHeader &slideshowHeader, const std::vector<SlideshowSlide> *slideshowSlides)
		: _slideshowHeader(slideshowHeader), _slideshowSlides(slideshowSlides), _curScreen(-1)
{
	resetScreen = true; // resetDisplay at firstBlit
	genCutPal(); // generate cutscene palette
	_wasLetterboxed = CutsceneState::initDisplay();
	_resX = Options::cutsceneResolutionX, _resY = Options::cutsceneResolutionY;
	_bpp = Options::cutsceneResolutionX == Screen::ORIGINAL_WIDTH ? 8 : 32;

	int scaleX = Options::cutsceneResolutionX/Screen::ORIGINAL_WIDTH;
	int scaleY = Options::cutsceneResolutionY/Screen::ORIGINAL_HEIGHT;

	// pre-render and queue up all the frames
	for (std::vector<SlideshowSlide>::const_iterator it = _slideshowSlides->begin(); it != _slideshowSlides->end(); ++it)
	{
		/*InteractiveSurface *slide =
			new InteractiveSurface(Screen::ORIGINAL_WIDTH, Screen::ORIGINAL_HEIGHT, 0, 0);*/
		InteractiveSurface* slide =
			new InteractiveSurface(Options::cutsceneResolutionX, Options::cutsceneResolutionY, 0, 0);
		add(slide);
		if (_bpp == 8)
		{
			slide->loadImage(it->imagePath);
		}
		else if (OpenXcom::in32BitsFolder(it->imagePath))
		{
			slide->loadImage(it->imagePath.substr(0, it->imagePath.find_last_of(".")) + "32"
				+ it->imagePath.substr(it->imagePath.find_last_of("."), it->imagePath.npos));
		}
		else // 32 bits but no 32 bits image
		{
			slide->loadImage(it->imagePath);
			slide->setScale(scaleX, scaleY);
			slide->doScale();
			slide->convertTo32Bits(slide, _game->getMod()->getPalettes().find("PAL_BATTLESCAPE")->second->getColors());
		}
		slide->onMouseClick((ActionHandler)&SlideshowState::screenClick);
		slide->onKeyboardPress((ActionHandler)&SlideshowState::screenClick, Options::keyOk);
		slide->onKeyboardPress((ActionHandler)&SlideshowState::screenSkip, Options::keyCancel);
		slide->setVisible(false);
		_slides.push_back(slide);
		setStatePalette(slide->getPalette());


		// initialize with default rect; may get overridden by
		// category/id definition
		Text *caption = new Text(it->w*scaleX, it->h*scaleY, it->x*scaleX, it->y*scaleY, slide->getSurface()->format->BitsPerPixel);
		caption->setScale(scaleX, scaleY);
		add(caption);
		caption->setColor(it->color);
		caption->setText(tr(it->caption));
		caption->setAlign(it->align);
		caption->setWordWrap(true);
		caption->setVisible(false);
		_captions.push_back(caption);
	}

	centerAllSurfaces();

	int transitionSeconds = _slideshowHeader.transitionSeconds;
	if (_slideshowSlides->front().transitionSeconds > 0)
		transitionSeconds = _slideshowSlides->front().transitionSeconds;
	_transitionTimer = new Timer(transitionSeconds * 1000);
	_transitionTimer->onTimer((StateHandler)&SlideshowState::screenTimer);

	_game->getMod()->playMusic(_slideshowHeader.musicId);
	_game->getCursor()->setVisible(false);
	screenClick(0);
}

SlideshowState::~SlideshowState()
{
	delete _transitionTimer;
}

/**
 * Shows the next screen on a timed basis.
 */
void SlideshowState::screenTimer()
{
	screenClick(0);
}

/**
 * Handle timers.
 */
void SlideshowState::think()
{
	_transitionTimer->think(this, 0);
}

/**
 * Shows the next screen in the slideshow; pops the state when there are no more slides
 */
void SlideshowState::screenClick(Action *action)
{
	if (_curScreen >= 0)
	{
		_slides[_curScreen]->setVisible(false);
		_captions[_curScreen]->setVisible(false);
	}

	++_curScreen;

	// next screen
	if (_curScreen < (int)_slideshowSlides->size())
	{
		int transitionSeconds = _slideshowHeader.transitionSeconds;
		if (_slideshowSlides->at(_curScreen).transitionSeconds > 0)
			transitionSeconds = _slideshowSlides->at(_curScreen).transitionSeconds;
		_transitionTimer->setInterval(transitionSeconds * 1000);
		_transitionTimer->start();
		setStatePalette(_slides[_curScreen]->getPalette());
		_slides[_curScreen]->setVisible(true);
		_captions[_curScreen]->setVisible(true);
		init();
	}
	else
	{
		screenSkip(action);
	}
}

/**
 * Skips the slideshow
 */
void SlideshowState::screenSkip(Action *)
{
	// reset display back to 8 bits ORIGINAL SCREEN
	_game->getScreen()->resetDisplay(true, false, Screen::ORIGINAL_WIDTH, Screen::ORIGINAL_HEIGHT, 8);
	// slideshow is over.  restore the screen scale and pop the state
	_game->getCursor()->setVisible(true);
	CutsceneState::resetDisplay(_wasLetterboxed);
	_game->popState();
}

}
