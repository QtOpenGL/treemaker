/*
 * Copyright (C) 2016 Floris Creyf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include "view_editor.h"
#include "file_exporter.h"
#include "primitives.h"
#include "collision.h"
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtOpenGL/QGLFormat>

ViewEditor::ViewEditor(QWidget *parent) : QOpenGLWidget(parent)
{
	ctrl = shift = midButton = false;
	camera.action = Camera::NONE;
	rs = NULL;
	setFocus();
}

ViewEditor::~ViewEditor()
{
	bt_delete_tree(tree);
}

void ViewEditor::exportObject(const char *filename)
{
	FileExporter f;
	Entity *e = scene.getEntity(0);
	f.setVertices(&e->geometry.vertices[0], bt_get_vbo_size(tree));
	f.setTriangles(&e->geometry.triangles[0], bt_get_ebo_size(tree));
	f.exportObj(filename);
}

void ViewEditor::setRenderSystem(RenderSystem *rs)
{
	this->rs = rs;
}

void ViewEditor::initializeGL()
{
	initializeOpenGLFunctions();
	rs->init();

	ShaderInfo shaders[] = {
		{GL_VERTEX_SHADER, "shaders/flat.vert"},
		{GL_FRAGMENT_SHADER, "shaders/flat.frag"},
		{GL_VERTEX_SHADER, "shaders/basic.vert"},
		{GL_FRAGMENT_SHADER, "shaders/basic.frag"},
		{GL_VERTEX_SHADER, "shaders/solid.vert"},
		{GL_FRAGMENT_SHADER, "shaders/flat.frag"}
	};

	rs->loadShaders(&shaders[0], 2);
	rs->loadShaders(&shaders[2], 2);
	rs->loadShaders(&shaders[4], 2);

	initializeTree();
	initializeGrid();
}

void ViewEditor::resizeGL(int width, int height)
{
	float aspectRatio = (float)width / (float)height;
	camera.setWindowSize(width, height);
	camera.setPerspective(45.0f, 0.1f, 100.0f, aspectRatio);
	glViewport(0, 0, width, height);
	paintGL();
}

void ViewEditor::initializeGrid()
{
	GeometryComponent g = {};
	RenderComponent r = {};
	bt_vec3 color = {0.46, 0.46, 0.46};
	bt_vec3 sectionColor = {0.41, 0.41, 0.41};
	createGrid(g, 5, color, sectionColor);
	r.program = 0;
	r.type = RenderComponent::LINES;
	r.vertexRange[1] = g.vertices.size() / 6;
	rs->registerComponent(rs->load(g), r);
}

void ViewEditor::initializeTree()
{
	const int es = 8000;
	const int vs = 8000;

	Entity e = {};
	RenderComponent *r;
	GeometryComponent *g;

	e.renderComponent.resize(1);
	r = &e.renderComponent[0];
	g = &e.geometry;

	g->attribs = 2;
	g->stride = sizeof(float) * 6;
	g->vertices.resize(vs);
	g->triangles.resize(es);
	r->program = 1;
	r->type = RenderComponent::TRIANGLES;

	tree = bt_new_tree();
	bt_set_radius(tree, 0, 0.2f);
	bt_set_resolution(tree, 0, 8);
	bt_set_max_branch_depth(tree, 1);
	bt_generate_structure(tree);
	bt_generate_mesh(tree, &g->vertices[0], vs, &g->triangles[0], es);

	r->triangleRange[1] = bt_get_ebo_size(tree);
	r->vertexRange[1] = bt_get_vbo_size(tree);

	rs->registerComponent(rs->load(*g), *r);
	scene.add(e);
}

void ViewEditor::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Control:
		ctrl = true;
		break;
	case Qt::Key_Shift:
		shift = true;
		break;
	}
}

void ViewEditor::keyReleaseEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Control:
		ctrl = false;
		break;
	case Qt::Key_Shift:
		shift = false;
		break;
	}
}

void ViewEditor::selectBranch(int x, int y)
{
	Entity *selection;
	int branch;

	selection = scene.getSelected();
	if (selection != NULL)
		rs->setWireframe(selection->geometry.buffer, 0, 0, 0);

	selection = scene.setSelected(camera, x, y);
	branch = scene.setSelectedBranch(camera, x, y, tree);

	if (selection != NULL && branch >= 0) {
		int s = bt_get_ebo_start(tree, branch);
		int e = bt_get_ebo_end(tree, branch);
		rs->setWireframe(selection->geometry.buffer, 0, s, e);
	}

	emit selectionChanged(tree, branch);
}

void ViewEditor::mousePressEvent(QMouseEvent *event)
{
	QPoint p = event->pos();
	midButton = false;

	if (event->button() == Qt::MidButton) {
		camera.setStartCoordinates(p.x(), p.y());
		midButton = true;

		if (ctrl && !shift)
			camera.action = Camera::ZOOM;
		else if (shift && !ctrl)
			camera.action = Camera::PAN;
		else
			camera.action = Camera::ROTATE;
	} else if (event->button() == Qt::RightButton) {
		selectBranch(p.x(), p.y());
		update();
	}

	setFocus();
}

void ViewEditor::mouseReleaseEvent(QMouseEvent *event)
{
	camera.action = Camera::NONE;
}

void ViewEditor::mouseMoveEvent(QMouseEvent *event)
{
	QPoint point = event->pos();

	switch (camera.action) {
	case Camera::ZOOM:
		camera.zoom(point.x(), point.y());
		break;
	case Camera::ROTATE:
		camera.setCoordinates(point.x(), point.y());
		break;
	case Camera::PAN:
		camera.setPan(point.x(), point.y());
		break;
	}

	update();
}

void ViewEditor::paintGL()
{
	GlobalUniforms gu;
	gu.vp = camera.getVP();
	gu.cameraPosition = camera.getPosition();
	rs->render(gu);
}

void ViewEditor::updateWireframe()
{
	Entity *selection = scene.getSelected();
	if (selection >= 0) {
		int branch = scene.getSelectedBranch();
		if (branch >= 0) {
			int s = bt_get_ebo_start(tree, branch);
			int e = bt_get_ebo_end(tree, branch);
			rs->setWireframe(selection->geometry.buffer, 0, s, e);
		}
	}
}

void ViewEditor::expandBuffer(GeometryComponent &g, RenderComponent &r)
{
	int status = 0;

	while (status == 0) {
		g.vertices.resize(g.vertices.size() + 1000);
		g.triangles.resize(g.triangles.size() + 1000);
		int v = g.vertices.size();
		int e = g.triangles.size();
		status = bt_generate_mesh(tree, &g.vertices[0], v,
				&g.triangles[0], e);
	}

	r.triangleRange[1] = bt_get_ebo_size(tree);
	r.vertexRange[1] = bt_get_vbo_size(tree);
	rs->registerComponent(rs->load(g, 0), r);
}

void ViewEditor::change()
{
	Entity *e = scene.getEntity(0);
	GeometryComponent *g = &e->geometry;
	RenderComponent *r = &e->renderComponent[0];
	int vs = g->vertices.size();
	int es = g->triangles.size();
	int status = bt_generate_mesh(tree, &g->vertices[0], vs,
			&g->triangles[0], es);

	if (status == 0)
		expandBuffer(*g, *r);
	else  {
		int v = bt_get_vbo_size(tree) * 6;
		int i = bt_get_ebo_size(tree);

		if (vs > 8000 && es > 8000 && vs - 2000 > v && es - 2000 > i) {
			g->vertices.resize(v + 1000);
			g->triangles.resize(i + 1000);
			r->triangleRange[1] = v;
			r->vertexRange[1] = i;
			rs->registerComponent(rs->load(*g, 0), *r);
		} else {
			rs->updateVertices(0, &g->vertices[0], 0, v);
			rs->updateTriangles(0, &g->triangles[0], 0, i);
			rs->setVertexRange(0, 0, 0, v/6);
			rs->setTriangleRange(0, 0, 0, i);
		}
	}

	updateWireframe();
	update();
}

void ViewEditor::changeResolution(int i)
{
	bt_set_resolution(tree, scene.getSelectedBranch(), i);
	change();
}

void ViewEditor::changeSections(int i)
{
	bt_set_cross_sections(tree, scene.getSelectedBranch(), i);
	change();
}

void ViewEditor::changeRadius(double d)
{
	bt_set_radius(tree, scene.getSelectedBranch(), d);
	change();
}

void ViewEditor::changeRadiusCurve(std::vector<bt_vec3> c)
{
	bt_set_radius_curve(tree, scene.getSelectedBranch(), &c[0], c.size());
	change();
}
