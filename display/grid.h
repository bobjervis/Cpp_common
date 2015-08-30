#pragma once
#include "../common/string.h"
#include "../common/vector.h"
#include "canvas.h"

namespace display {

const coord NO_CONSTRAINT = COORD_MIN;

const bool RESIZE = true;

class Cell;

class Grid : public Canvas {
	typedef Canvas super;

public:
	Grid();

	~Grid();

	void row(bool resize = false);

	Cell* cell(bool resize = false);

	Cell* cell(Canvas* c, bool resize = false);

	Cell* cell(dimension size, Canvas* c, bool resize = false);

	void xcell(int left, int top, Canvas* c);

	void cell(int left, int top, int right, int bottom, Canvas* c);

	void cell(int col, int row, Canvas* c,
			  int leftOffset, int topOffset, int rightOffset, int bottomOffset);

	void cell(int left, int top, int right, int bottom, Canvas* c,
			  int leftOffset, int topOffset, int rightOffset, int bottomOffset);

	void columnWidth(int column, coord value);

	void rowHeight(int row, coord value);

	void resizeRow(int row);

	void resizeColumn(int column);

	void reopen(int resizeRow, int resizeColumn);

	void complete();

	virtual dimension measure();

	virtual void layout(dimension size);

	Canvas* cell(point p);
	/*
	 *	hit
	 *
	 *	This method returns the Cell under the given point p (if there is one).
	 */
	Cell* hit(point p);

/*
	splitRow: (row: int, splitY: int)
	{
			// Expand prefHeight

		ph := new [rows + 1] coord
		for (i := 0; i < row; i++)
			ph[i] = prefHeight[i]
		for (i = row + 1; i < rows; i++)
			ph[i + 1] = prefHeight[i]
		prefHeight = ph

			// Expand heights

		h := new [rows + 1] coord
		for (i = 0; i < row; i++)
			h[i] = heights[i]
		if (heights[row] == NO_CONSTRAINT ||
			heights[row] >= 0){
			h[row] = coord(splitY - y[row])
			h[row + 1] = coord(y[row + 1] - splitY)
		} else {
			h[row] = coord(-(splitY - y[row]) / (y[row + 1] - y[row]))
			h[row + 1] = coord(heights[row] - h[row])
		}
		for (i = row + 1; i < rows; i++)
			h[i + 1] = heights[i]
		heights = h

			// Expand y
		ny := new [rows + 2] coord
		for (i = 0; i <= row; i++)
			ny[i] = y[i]
		ny[row + 1] = coord(splitY)
		for (i = row + 1; i < rows + 1; i++)
			ny[i + 1] = y[i]
		y = ny

		if (resizeRow >= row)
			resizeRow++
		rows++

		for (c := cells; c != null; c = c.next){
			if (c.top > row)
				c.top++
			if (c.bottom > row)
				c.bottom++
		}
		invalidateSize()
		computeEmpty()
	}

	splitColumn: (col: int, splitX: int)
	{
			// Expand prefWidth

		ph := new [columns + 1] coord
		for (i := 0; i < col; i++)
			ph[i] = prefWidth[i]
		for (i = col + 1; i < columns; i++)
			ph[i + 1] = prefWidth[i]
		prefWidth = ph

			// Expand widths

		w := new [columns + 1] coord
		for (i = 0; i < col; i++)
			w[i] = widths[i]
		if (widths[col] == NO_CONSTRAINT ||
			widths[col] >= 0){
			w[col] = coord(splitX - x[col])
			w[col + 1] = coord(x[col + 1] - splitX)
		} else {
			w[col] = coord(-(splitX - x[col]) / (x[col + 1] - x[col]))
			w[col + 1] = coord(widths[col] - w[col])
		}
		for (i = col + 1; i < columns; i++)
			w[i + 1] = widths[i]
		widths = w

			// Expand x
		nx := new [columns + 2] coord
		for (i = 0; i <= col; i++)
			nx[i] = x[i]
		nx[col + 1] = coord(splitX)
		for (i = col + 1; i < columns + 1; i++)
			nx[i + 1] = x[i]
		x = nx

		if (_resizeColumn >= col)
			_resizeColumn++;
		columns++

		for (c := cells; c != null; c = c.next){
			if (c.left > col)
				c.left++
			if (c.right > col)
				c.right++
		}
		invalidateSize()
		computeEmpty()
	}
 */
/*
	cellAt: (p: point) point		// Note: output point is grid cell numbers, not coords
	{
		c: point

		if (p.x < bounds.topLeft.x || p.y < bounds.topLeft.y ||
			p.x >= bounds.opposite.x || p.y >= bounds.opposite.y){
			c.x = -1
			c.y = -1
			return c
		}
		p.x -= bounds.topLeft.x
		p.y -= bounds.topLeft.y
		for (c.x = 0; c.x < columns - 1; c.x++){
			if (p.x < x[c.x + 1])
				break
		}
		for (c.y = 0; c.y < rows - 1; c.y++){
			if (p.y < y[c.y + 1])
				break
		}
		return c
	}
	*/
	coord x(int column) const { return _columnSpecs[column].actual; }
	coord y(int row) const { return _rowSpecs[row].actual; }

	int rows() const { return _rowSpecs.size(); }
	int columns() const { return _columnSpecs.size(); }

private:
	class ConstructionParams {
	public:
		int				row;
		int				column;
		vector<int>		empty;					// each entry corresponds to the first empty row for that column

		ConstructionParams() {
			row = -1;
			column = 0;
		}
	};

	class RowColumnSpec {
	public:
		coord					constraint;		// constrained width or height
												//		< 0				Indicates % of total grid
												//		>= 0			Indicates absolute size in pixels
												//		NO_CONSTRAINT	Indicates no constraint: use
												//						preferred dimension
		coord					preferred;		// preferred width or height
		coord					actual;			// actual, computed width or height

		RowColumnSpec() {
			constraint = NO_CONSTRAINT;
			preferred = 0;
			actual = 0;
		}
	};

	void init(int rows, int column);

	void attachCell(Cell* c, bool resize);

	void collectChildPreferences();

	ConstructionParams*		_constructing;
	Cell*					_cells;
	coord					_resizeColumn;
	coord					_resizeRow;
	vector<RowColumnSpec>	_rowSpecs;			// contains 1 more element than rows (to hold bottom margin)
	vector<RowColumnSpec>	_columnSpecs;		// contains 1 more element than columns (to hold right margin)
};

class Cell {
	friend Grid;
public:
	Cell(int left, int top, int right, int bottom, Canvas* c, int leftOffset, int topOffset, int rightOffset, int bottomOffset, Grid* grid);

	void setCanvas(Canvas* c);

	Canvas* canvas() const { return _canvas; }

	int top() const { return _top; }

private:
	Cell*			next;
	int				_top;
	int				topOffset;
	int				left;
	int				leftOffset;
	int				bottom;
	int				bottomOffset;
	int				right;
	int				rightOffset;
	Canvas*			_canvas;
	Grid*			_grid;
};

Bevel* slider();

class VerticalSliderHandler {
public:
	VerticalSliderHandler(Bevel* v, Grid* g, int column);

private:
	void onStartDrag(MouseKeys mKeys, point p, Canvas* target);

	void onDrag(MouseKeys mKeys, point p, Canvas* target);

	Bevel*		_slider;
	Grid*		_grid;
	int			_column;
	int			_startingX;
	int			_startingLeftSize;
	int			_startingRightSize;
};

} // namespace display
