/* 
 * The MIT License (MIT)
 * 
 * Copyright 2016, Adrien Destugues
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "NeonLights.h"

#include <math.h>
#include <stdlib.h>

#include <Alignment.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <Slider.h>
#include <String.h>
#include <StringView.h>
#include <Window.h>

#define ARRAY_SIZE(a) \
	(sizeof(a) / sizeof(a[0]))

static const BString kName = "Neon Lights";
static const BString kAuthor = "Adrien Destugues";

static const int32 kSpotCount = 'spot';
static const int32 kSpotSize = 'size';
static const int32 kTrail = 'tril';
static const int32 kStability = 'stab';
static const int32 kDefaults = 'dflt';

extern "C" BScreenSaver*
instantiate_screen_saver(BMessage* msg, image_id id)
{
	return new NeonLights(msg, id);
}


NeonLights::NeonLights(BMessage* archive, image_id id)
	:
	BScreenSaver(archive, id)
{
	fSpots = 22;
	fParticles = 3000;
	fSpotSize = .5;

	fSpots = archive->GetInt32("spots", fSpots);
	fParticles = archive->GetInt32("trails", fParticles);
	fSpotSize = archive->GetFloat("size", fSpotSize);
}


NeonLights::~NeonLights()
{
}


void NeonLights::StartConfig(BView* view)
{
	BSlider* s1;
	BSlider* s2;
	BSlider* s3;

	// Needed for live-application of changes
	BWindow* win = view->Window();
	if (win)
		win->AddHandler(this);

	BStringView* v1 = new BStringView("name", "Neon Lights");
	v1->SetFont(be_bold_font);
	BStringView* v2 = new BStringView("author", "by Adrien Destugues");

	BStringView* v3 = new BStringView("spots", "Spots");
	v3->SetFont(be_bold_font);
	BStringView* v4 = new BStringView("particles", "Trails");
	v4->SetFont(be_bold_font);

	s1 = new BSlider("spots", "", new BMessage(kSpotCount), 5, 64, B_HORIZONTAL);
	s1->SetValue(fSpots);
	s1->SetTarget(this);
	s1->SetLimitLabels("few", "many");

	s2 = new BSlider("size", "", new BMessage(kSpotSize), 10, 500, B_HORIZONTAL);
	s2->SetValue(fSpotSize * 100);
	s2->SetTarget(this);
	s2->SetLimitLabels("thin", "thick");

	s3 = new BSlider("trails", "", new BMessage(kTrail), 200, 5000, B_HORIZONTAL);
	s3->SetValue(fParticles);
	s3->SetTarget(this);
	s3->SetLimitLabels("dark", "colorful");

	BButton* b = new BButton("defaults", "Defaults", new BMessage(kDefaults));
	b->SetTarget(this);

	BLayoutBuilder::Group<>(view, B_VERTICAL, B_USE_ITEM_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_TOP))
		.AddGroup(B_HORIZONTAL)
			.Add(v1)
			.Add(v2)
			.AddGlue()
		.End()
		.Add(v3)
		.Add(s1)
		.Add(s2)
		.AddGlue()
		.Add(v4)
		.Add(s3)
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.Add(b)
			.AddGlue()
		.End();
}


status_t NeonLights::StartSaver(BView* view, bool prev)
{
	srandom(time(NULL));

	BRect rect = view->Bounds();
	fWidth = (int) rect.Width() + 1;
	fHeight = (int) rect.Height() + 1;

	view->SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);
	view->SetFlags(view->Flags() | B_SUBPIXEL_PRECISE);

	view->SetLowColor(make_color(0, 0, 0, 255));
	view->SetViewColor(make_color(0, 0, 0, 255));

	_Restart(view);

	return B_OK;
}


rgb_color somecolor() {
  // pick some random good color

	static const rgb_color goodcolor[] = {
		{ 0, 0, 255, 32 },
		{ 0, 255, 255, 32 },
		{ 255, 0, 255, 32 },
		{ 0, 255, 0, 32 },
		{ 255, 0, 0, 32 },
		{ 255, 255, 0, 32 },
	};

  return goodcolor[random() % ARRAY_SIZE(goodcolor)];
}


// OBJECTS ------------------------------------------------------------
class City {
public:
  float x, y;
  BPoint old;
  int other;
  float vx, vy;
  rgb_color myc;

  void Draw(BView* view) {
    view->AddLine(old, BPoint(x,y), make_color(255, 255, 255, 255));
	old = BPoint(x,y);
  }
};

static const int kMaxCities = 64;
City cities[kMaxCities];

void NeonLights::_Move(City* city, BView* view) {
    city->vx += (cities[city->other].x-city->x)/view->Bounds().Width();
    city->vy += (cities[city->other].y-city->y)/view->Bounds().Height();

	city->vx *= 0.986;
	city->vy *= 0.979;

	if (random() < RAND_MAX / 100)
		city->other = random() % fSpots;

    city->x+=city->vx;
    city->y+=city->vy;

    city->Draw(view);
}


void NeonLights::_Restart(BView* view)
{
	fNeedsRestart = false;

	view->SetDrawingMode(B_OP_COPY);
	view->FillRect(view->Bounds(), B_SOLID_LOW);
	view->SetDrawingMode(B_OP_ALPHA);

	float tinc = 2 * M_PI / fSpots;

	for (int t = 0; t < fSpots; t++)
	{
		cities[t].x = fWidth / 2;
		cities[t].y = fHeight / 2;
		cities[t].old = BPoint(cities[t].x, cities[t].y);
		cities[t].vx = (1+random() % 11)*sin(tinc*t);
		cities[t].vy = (1+random() % 11)*cos(tinc*t);
		cities[t].myc = somecolor();
		do {
			cities[t].other = random() % fSpots;
		} while(cities[t].other == t);
	}
}


float citydistance(int a, int b) {
  if (a!=b) {
    // calculate and return distance between cities
    float dx = cities[b].x-cities[a].x;
    float dy = cities[b].y-cities[a].y;
    float d = sqrt(dx*dx+dy*dy);
    return d;
  } else {
    return 0.0;
  }
}


void NeonLights::Draw(BView* view, int32 frame)
{
	if (fNeedsRestart || (frame & 0x3FF) == 0)
		_Restart(view);


	view->BeginLineArray(fParticles);
	for (int n = 0; n < fParticles; n++)
	{
		int a = random() % fSpots; // int rand
		int b = 0;
		int tr = 0;
		do
		{
			b = random() % fSpots; // int rand
			tr ++;
		}
		while (tr < 100 && citydistance(a,b) < (fWidth * fHeight) / (10 * (fWidth + fHeight)));

		if (tr >= 100)
			continue;

		int r = random();
		float t = r * M_PI / RAND_MAX; // float rand
		float dx = sin(t)*(cities[b].x-cities[a].x)+cities[a].x;
		float dy = sin(t)*(cities[b].y-cities[a].y)+cities[a].y;

			// noise
			dx += random() * 3.0 / RAND_MAX - 1.5;
			dy += random() * 3.0 / RAND_MAX - 1.5;

		rgb_color c = mix_color(cities[b].myc, cities[a].myc, 128);
		//c.alpha = 20;
		view->AddLine(BPoint(dx, dy), BPoint(dx, dy), c);
	}
	view->EndLineArray();

	view->SetPenSize(fSpotSize);
	view->BeginLineArray(fSpots);
	// move cities
	for (int c = 0; c < fSpots; c++)
		_Move(&cities[c], view);
	view->EndLineArray();
}


void NeonLights::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case kSpotCount:
		fSpots = msg->GetInt32("be:value", fSpots);
		fNeedsRestart = true;
		break;
	case kSpotSize:
		fSpotSize = msg->GetInt32("be:value", fSpotSize * 100) / 100;
		fNeedsRestart = true;
		break;
	case kTrail:
		fParticles = msg->GetInt32("be:value", fParticles);
		fNeedsRestart = true;
		break;
	case kDefaults:
		fSpots = 22;
		fSpotSize = .5;
		fParticles = 3000;
		fNeedsRestart = true;
		break;
	default:
		BHandler::MessageReceived(msg);
	}
}


status_t NeonLights::SaveState(BMessage* into) const
{
	into->AddInt32("spots", fSpots);
	into->AddInt32("trails", fParticles);
	into->AddFloat("size", fSpotSize);
	return B_OK;
}

