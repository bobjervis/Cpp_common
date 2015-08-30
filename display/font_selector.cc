#include "../common/platform.h"
#include "font_selector.h"

#include "../common/locale.h"
#include "device.h"
#include "grid.h"
#include "label.h"
#include "root_canvas.h"

namespace display {

static vector<string> fontList;

static int CALLBACK enumProc(const LOGFONT* logfont, const TEXTMETRIC* metrics, DWORD fontType, LPARAM lParam) {
	fontList.push_back(logfont->lfFaceName);
	return 1;
}

FontSelector::FontSelector(Font *f) {
	_font = f;
	_firstField = null;
	if (fontList.size() == 0)
		fontList.push_back("serif");
	Grid* g = new Grid();
		g->cell(new Label("Family: "));
		_familyList = new DropDown(-1, fontList);
		_familyList->valueChanged.addHandler(this, &FontSelector::onFamilyChanged);
		Spacer* s = new Spacer(2, _familyList);
		s->setBackground(&editableBackground);
		g->cell(new Border(1, s));
		g->cell(new Spacer(5, 0, 0, 0, new Label("Size: ")));
		_size = new Label(string(f->size.value()), 2);
		_size->setBackground(&editableBackground);
		_size->valueCommitted.addHandler(this, &FontSelector::onSizeChanged);
		g->cell(new Spacer(1, new Bevel(2, true, _size)));
		_bold = new Toggle(&f->bold);
		_boldHandler = new ToggleHandler(_bold);
		g->cell(new Spacer(5, 0, 0, 0, new Label("Bold: ")));
		g->cell(_bold);
		_italic = new Toggle(&f->italic);
		_italicHandler = new ToggleHandler(_italic);
		g->cell(new Spacer(5, 0, 0, 0, new Label("Italic: ")));
		g->cell(_italic);
		_underlined = new Toggle(&f->underlined);
		_underlinedHandler = new ToggleHandler(_underlined);
		g->cell(new Spacer(5, 0, 0, 0, new Label("Underlined: ")));
		g->cell(_underlined);
		g->cell();
		f->bold.changed.addHandler(this, &FontSelector::onToggleChanged);
		f->italic.changed.addHandler(this, &FontSelector::onToggleChanged);
		f->underlined.changed.addHandler(this, &FontSelector::onToggleChanged);
	g->complete();
	Grid* g2 = new Grid();
		g2->cell(new Filler(g));
		_sample = new Label("Sample text 1234", f);
		g2->cell(new Border(1, _sample));
		g2->cell();
	g2->complete();
	append(g2);
}

FontSelector::~FontSelector() {
	delete _boldHandler;
	delete _italicHandler;
	delete _underlinedHandler;
}

dimension FontSelector::measure() {
	if (fontList.size() == 1) {
		Device* d = device();
		if (d) {
			LOGFONT logfont;

			fontList.push_back("sansSerif");
			fontList.push_back("monospace");
			memset(&logfont, 0, sizeof logfont);
			logfont.lfCharSet = ANSI_CHARSET;
			logfont.lfFaceName[0] = 0;
			logfont.lfPitchAndFamily = 0;
			EnumFontFamiliesEx(d->hDC(), &logfont, enumProc, null, 0);
			vector<string*> slist;

			for (int i = 0; i < fontList.size(); i++)
				slist.push_back(new string(fontList[i]));
			slist.sort();
			fontList.clear();
			for (int i = 0; i < slist.size(); i++)
				fontList.push_back(*slist[i]);
			slist.deleteAll();
		}
	}
	if (_familyList->value() == -1) {
		int index;
		for (index = 0; index < fontList.size(); index++) {
			if (fontList[index].size() == _font->family().size() &&
				locale::startsWithIgnoreCase(fontList[index], _font->family()))
				break;
		}
		if (index < fontList.size())
			_familyList->set_value(index);
	}
	return child->preferredSize();
}

Field* FontSelector::firstField() {
	if (_firstField == null) {
		RootCanvas* root = rootCanvas();
		_firstField = root->field(_familyList);
		root->field(_size);
	}
	return _firstField;
}

void FontSelector::onFamilyChanged() {
	_font->set_family(fontList[_familyList->value()]);
	_sample->unbind();
	_sample->invalidate();
	changed.fire();
}

void FontSelector::onSizeChanged() {
	_font->size.set_value(_size->value().toInt());
	_font->touch();
	_sample->unbind();
	_sample->invalidate();
	changed.fire();
}

void FontSelector::onToggleChanged() {
	_font->touch();
	_sample->unbind();
	_sample->invalidate();
	changed.fire();
}

}  // namespace display
