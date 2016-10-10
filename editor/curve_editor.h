/*
 * Copyright (C) 2016 Floris Creyf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef CURVE_EDITOR_H
#define CURVE_EDITOR_H

#include "render_system.h"
#include "vector.h"
#include <QtGui/QOpenGLFunctions>
#include <QOpenGLWidget>

class CurveEditor : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	CurveEditor(QWidget *parent = 0);
	QSize sizeHint() const;

public slots:
	void setCurve(vector<tm_vec3> controls, QString name);
	void setEnabled(bool enabled);

signals:
	void curveChanged(vector<tm_vec3> controls, QString name);

protected slots:
	void onFloat(bool);

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void mousePressEvent(QMouseEvent *);
	void mouseDoubleClickEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void keyPressEvent(QKeyEvent *);
	void keyReleaseEvent(QKeyEvent *);
	void mouseMoveEvent(QMouseEvent *);

private:
	RenderSystem rs;
	Entity ui;
	bool enabled;
	int height;
	int width;
	vector<tm_vec3> controls;
	tm_vec3 curve[4];
	int insertIndex;
	int point;
	QString curveName;

	void createInterface();
	int createBackground(int);
	int createControlLines(int);
	int createCurve(int);
	void updateCurve();
	void drawCurve();
	void insertCurve(int, float, float);
	bool reinsertCurve(float);
	bool omitCurve(float);
	void placeOuterControl(float, float);
	void placeInnerControl(float, float);
	void placeTerminalControl(bool, float);
};

#endif /* CURVE_EDITOR_H */
