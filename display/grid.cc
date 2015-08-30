#include "../common/platform.h"
#include "grid.h"

#include "../common/machine.h"
#include "device.h"

namespace display {

Grid::Grid() {
	_constructing = new ConstructionParams();
	_rowSpecs.resize(1);
	_columnSpecs.resize(1);
	_resizeColumn = -1;
	_resizeRow = -1;
	_cells = null;
}

Grid::~Grid() {
	delete _constructing;
	while (_cells) {
		Cell* next = _cells->next;
		delete _cells;
		_cells = next;
	}
}

void Grid::row(bool resize) {
	if (_constructing) {
		_constructing->row++;
		_constructing->column = 0;
		if (resize)
			_resizeRow = _constructing->row;
	}
}

Cell* Grid::cell(bool resize) {
	if (_constructing) {
		if (_constructing->row < 0)
			_constructing->row = 0;
		int i;
		for (i = _constructing->column; i < _constructing->empty.size(); i++)
			if (_constructing->empty[i] <= _constructing->row)
				break;
		Cell* cell = new Cell(i, _constructing->row, i + 1, _constructing->row + 1, null, 0, 0, 0, 0, this);
		attachCell(cell, resize);
		return cell;
	}
	return null;
}

Cell* Grid::cell(Canvas* c, bool resize) {
	if (_constructing) {
		if (_constructing->row < 0)
			_constructing->row = 0;
		int i;
		for (i = _constructing->column; i < _constructing->empty.size(); i++)
			if (_constructing->empty[i] <= _constructing->row)
				break;
		Cell* cell = new Cell(i, _constructing->row, i + 1, _constructing->row + 1, c, 0, 0, 0, 0, this);
		attachCell(cell, resize);
		return cell;
	}
	return null;
}

Cell* Grid::cell(dimension sz, Canvas* c, bool resize) {
	if (_constructing) {
		if (_constructing->row < 0)
			_constructing->row = 0;
		int i;
		for (i = _constructing->column; i < _constructing->empty.size(); i++) {
			int j;
			for (j = i; j < i + sz.width; j++) {
				if (j >= _constructing->empty.size()) {
					j = sz.width;
					break;
				}
				if (_constructing->empty[j] > _constructing->row)
					break;
			}
			if (j >= sz.width)
				break;
		}
		Cell* cell = new Cell(i, _constructing->row, i + sz.width, _constructing->row + sz.height, c, 0, 0, 0, 0, this);
		attachCell(cell, resize);
		return cell;
	}
	return null;
}

void Grid::attachCell(Cell* c, bool resize) {
	if (c->_canvas)
		append(c->_canvas);
	if (c->bottom >= _rowSpecs.size())
		_rowSpecs.resize(c->bottom + 1);
	if (c->right >= _columnSpecs.size())
		_columnSpecs.resize(c->right + 1);
	for (int i = c->left; i < c->right; i++) {
		if (i >= _constructing->empty.size())
			_constructing->empty.push_back(c->bottom);
		else
			_constructing->empty[i] = c->bottom;
	}
	_constructing->column = c->right;
	if (resize)
		_resizeColumn = c->left;
	c->next = _cells;
	_cells = c;
}

void Grid::cell(int left, int top, int right, int bottom, Canvas* c) {
	cell(left, top, right, bottom, c, 0, 0, 0, 0);
}

void Grid::cell(int col, int row, Canvas* c, int leftOffset, int topOffset, int rightOffset, int bottomOffset) {
	cell(col, row, col + 1, row + 1, c, leftOffset, topOffset, rightOffset, bottomOffset);
}

void Grid::cell(int left, int top, int right, int bottom, Canvas* c, int leftOffset, int topOffset, int rightOffset, int bottomOffset) {
	if (_constructing) {
		Cell* cell = new Cell(left, top, right, bottom, c, leftOffset, topOffset, rightOffset, bottomOffset, this);
		attachCell(cell, false);
	}
}

void Grid::columnWidth(int column, coord value) {
	if (column >= 0 && column < _columnSpecs.size() - 1)
		_columnSpecs[column].constraint = value;
}

void Grid::rowHeight(int row, coord value) {
	if (row >= 0 && row < _rowSpecs.size() - 1)
		_rowSpecs[row].constraint = value;
}

void Grid::resizeRow(int row) {
	_resizeRow = row;
}

void Grid::resizeColumn(int column) {
	_resizeColumn = column;
}

void Grid::reopen(int resizeRow, int resizeColumn) {
	_resizeRow = resizeRow;
	_resizeColumn = resizeColumn;
	if (_constructing)
		return;
	_constructing = new ConstructionParams();
	_constructing->row = _rowSpecs.size() - 1;
	for (int i = 0; i < _columnSpecs.size() - 1; i++)
		_constructing->empty.push_back(_constructing->row);
}

void Grid::complete() {
	if (_constructing) {
		if (_resizeRow < 0)
			_resizeRow = _rowSpecs.size() - 2;
		if (_resizeColumn < 0)
			_resizeColumn = _columnSpecs.size() - 2;
		delete _constructing;
		_constructing = null;
	}
}

dimension Grid::measure() {
	collectChildPreferences();
	dimension m;
	for (int i = 0; i < _rowSpecs.size() - 1; i++)
		if (_rowSpecs[i].constraint >= 0)
			m.height += _rowSpecs[i].constraint;
		else
			m.height += _rowSpecs[i].preferred;
	for (int i = 0; i < _columnSpecs.size() - 1; i++)
		if (_columnSpecs[i].constraint >= 0)
			m.width += _columnSpecs[i].constraint;
		else
			m.width += _columnSpecs[i].preferred;
	return m;
}

void Grid::layout(dimension size) {
	if (_constructing)
		fatalMessage("Still constructing grid");
	collectChildPreferences();
	_columnSpecs[_columnSpecs.size() - 1].actual = size.width;
	_rowSpecs[_rowSpecs.size() - 1].actual = size.height;
	if (_resizeRow == -1){			// equal spacing
		for (int i = 0; i < _rowSpecs.size() - 1; i++)
			_rowSpecs[i].actual = coord((size.height * i) / (_rowSpecs.size() - 1));
	} else {
		coord h = 0;
		coord hrr;
		for (int i = 0; i < _rowSpecs.size() - 1; i++){
			if (i == _resizeRow)
				continue;
			int r = _rowSpecs[i].constraint;
			if (r != NO_CONSTRAINT){
				if (r < 0)
					h += -r * size.height / 100;
				else
					h += r;
			} else
				h += _rowSpecs[i].preferred;
		}
		if (h > size.height)
			hrr = 0;
		else
			hrr = coord(size.height - h);
		h = 0;
		for (int i = 0; i < _rowSpecs.size() - 1; i++){
			_rowSpecs[i].actual = h;
			if (i == _resizeRow)
				h += hrr;
			else {
				int r = _rowSpecs[i].constraint;
				if (r != NO_CONSTRAINT){
					if (r < 0)
						h += -r * size.height / 100;
					else
						h += r;
				} else
					h += _rowSpecs[i].preferred;
			}
		}
	}
	if (_resizeColumn == -1){
		for (int i = 0; i < _columnSpecs.size() - 1; i++)
			_columnSpecs[i].actual = coord((size.width * i) / (_columnSpecs.size() - 1));
	} else {
		coord w = 0;
		coord wrc;
		for (int i = 0; i < _columnSpecs.size() - 1; i++){
			if (i == _resizeColumn)
				continue;
			int c = _columnSpecs[i].constraint;
			if (c != NO_CONSTRAINT) {
				if (c < 0)
					w += -c * size.width / 100;
				else
					w += c;
			} else
				w += _columnSpecs[i].preferred;
		}
		if (w > size.width)
			wrc = 0;
		else
			wrc = coord(size.width - w);
		w = 0;
		for (int i = 0; i < _columnSpecs.size() - 1; i++){
			_columnSpecs[i].actual = w;
			if (i == _resizeColumn)
				w += wrc;
			else {
				int c = _columnSpecs[i].constraint;
				if (c != NO_CONSTRAINT){
					if (c < 0)
						w += -c * size.width / 100;
					else
						w += c;
				} else if (i != _resizeColumn)
					w += _columnSpecs[i].preferred;
			}
		}
	}
	for (Cell* cell = _cells; cell != null; cell = cell->next) {
		if (cell->_canvas == null)
			continue;
		point p;

		p.x = coord(bounds.topLeft.x + _columnSpecs[cell->left].actual + cell->leftOffset);
		p.y = coord(bounds.topLeft.y + _rowSpecs[cell->_top].actual + cell->topOffset);
		cell->_canvas->at(p);
		dimension d;
		d.width = coord(_columnSpecs[cell->right].actual + cell->rightOffset - _columnSpecs[cell->left].actual);
		d.height = coord(_rowSpecs[cell->bottom].actual + cell->bottomOffset - _rowSpecs[cell->_top].actual);
//			windows.debugPrint("size=[" + d.width + ":" + d.height + "]\n")
		if (_columnSpecs[cell->left].actual + cell->leftOffset + d.width > size.width)
			d.width = coord(size.width - _columnSpecs[cell->left].actual - cell->leftOffset);
		if (_rowSpecs[cell->_top].actual + cell->topOffset + d.height > size.height)
			d.height = coord(size.height - _rowSpecs[cell->_top].actual - cell->topOffset);
		cell->_canvas->resize(d);
	}
}

Canvas* Grid::cell(point p) {
	for (Cell* cell = _cells; cell != null; cell = cell->next)
		if (cell->_top <= p.y && p.y < cell->bottom &&
			cell->left <= p.x && p.x < cell->right)
			return cell->_canvas;
	return null;
}

Cell* Grid::hit(point p) {
	// Make p relative to the Grid
	p.x -= bounds.topLeft.x;
	p.y -= bounds.topLeft.y;
	for (Cell* cell = _cells; cell != null; cell = cell->next)
		if (_rowSpecs[cell->_top].actual <= p.y && p.y < _rowSpecs[cell->bottom].actual &&
			_columnSpecs[cell->left].actual <= p.x && p.x < _columnSpecs[cell->right].actual)
			return cell;
	return null;
}

void Grid::collectChildPreferences() {
	for (int i = 0; i < _rowSpecs.size() - 1; i++)
		_rowSpecs[i].preferred = 0;
	for (int i = 0; i < _columnSpecs.size() - 1; i++)
		_columnSpecs[i].preferred = 0;
	for (Cell* cell = _cells; cell != null; cell = cell->next) {
		if (cell->_canvas == null)
			continue;
		dimension sz = cell->_canvas->preferredSize();
		if (_rowSpecs[cell->_top].preferred < sz.height)
			_rowSpecs[cell->_top].preferred = sz.height;
		if (_columnSpecs[cell->left].preferred < sz.width)
			_columnSpecs[cell->left].preferred = sz.width;
	}
}

Cell::Cell(int left, int top, int right, int bottom, Canvas* canvas, int leftOffset, int topOffset, int rightOffset, int bottomOffset, Grid* grid) {
	this->left = left;
	_top = top;
	this->right = right;
	this->bottom = bottom;
	this->leftOffset = leftOffset;
	this->topOffset = topOffset;
	this->rightOffset = rightOffset;
	this->bottomOffset = bottomOffset;
	this->next = null;
	_canvas = canvas;
	_grid = grid;
}

void Cell::setCanvas(Canvas *c) {
	if (_canvas == c)
		return;
	if (_canvas)
		_canvas->prune();
	_canvas = c;
	if (_canvas)
		_grid->append(_canvas);
	_grid->jumble();
}

static const int SLIDER_EDGE = 2;

Bevel* slider() {
	return new Bevel(SLIDER_EDGE);
}

VerticalSliderHandler::VerticalSliderHandler(Bevel *slider, Grid* grid, int column) {
	_slider = slider;
	_slider->mouseCursor = standardCursor(SIZEWE);
	_grid = grid;
	_column = column;
	_slider->startDrag.addHandler(this, &VerticalSliderHandler::onStartDrag);
	_slider->drag.addHandler(this, &VerticalSliderHandler::onDrag);
}

void VerticalSliderHandler::onStartDrag(MouseKeys mKeys, point p, Canvas* target) {
	_startingX = p.x;
	_startingLeftSize = _grid->x(_column) - _grid->x(_column - 1);
	_startingRightSize = _grid->x(_column + 2) - _grid->x(_column + 1);
}

void VerticalSliderHandler::onDrag(MouseKeys mKeys, point p, Canvas* target) {
	int deltaX = p.x - _startingX;
	if (deltaX > _startingRightSize)
		deltaX = _startingRightSize;
	else if (deltaX < -_startingLeftSize)
		deltaX = -_startingLeftSize;
	_grid->columnWidth(_column - 1, _startingLeftSize + deltaX);
	_grid->columnWidth(_column + 1, _startingRightSize - deltaX);
	_grid->jumbleContents();
}


}  // namespace display
